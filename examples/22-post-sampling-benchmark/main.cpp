#include "../../include/epiworld/epiworld.hpp"
#include "../../include/measles/measles.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

using namespace epiworld;

// Benchmarks the contact-sampling step of the network and mixing models, with
// no post-sampling callback installed. It is the "before" and "after" of the
// post-sampling hook work (issue #276): a change that only adds an unused
// hook must not slow these models down. The file uses only the long-standing
// public API, so it compiles against any version and two builds can be timed
// side by side (see compare.sh).
//
// Usage: main [--sizes 20000] [--reps 5] [--days 40]
//             [--scenarios L,U,S,E,Q,M]
//
// L: ModelSEIR on a Watts-Strogatz network (mean degree 10), pulling on every
//    day.
// U: the same model, pushing on every day.
// S: ModelSIRMixing, 10 equal groups (8 contacts a day within a group, 0.5
//    with each other group).
// E: ModelSEIRMixing, same groups.
// Q: ModelSEIRMixingQuarantine, same groups. It records contacts for tracing
//    itself, so this is today's inline recording cost.
// M: ModelMeaslesMixing, one group, 15 contacts a day, quarantine and contact
//    tracing on (specialized one- and two-pool samplers).
//
// For each cell it prints the median (Q1, Q3) CPU milliseconds per run, the
// median final size, and a checksum of every replicate's daily counts and
// transmissions -- equal checksums mean bit-identical runs.

struct Options {
    std::vector< size_t > sizes = {20000};
    int reps = 5;
    int days = 40;
    std::vector< std::string > scenarios = {"L", "U", "S", "E", "Q", "M"};
};

static std::vector< std::string > split(const std::string & s)
{
    std::vector< std::string > out;
    size_t start = 0u;
    while (start <= s.size())
    {
        size_t end = s.find(',', start);
        if (end == std::string::npos)
            end = s.size();
        if (end > start)
            out.push_back(s.substr(start, end - start));
        start = end + 1u;
    }
    return out;
}

[[noreturn]] static void fail(const std::string & msg)
{
    std::fprintf(stderr, "%s\n", msg.c_str());
    std::exit(1);
}

// Parses a whole string as an integer in [min, max].
static long parse_long(
    const std::string & key, const std::string & val,
    long min, long max = LONG_MAX
)
{
    size_t used = 0u;
    long x = 0;
    try {
        x = std::stol(val, &used);
    } catch (const std::exception &) {
        used = 0u;
    }
    if (used == 0u || used != val.size())
        fail("Option " + key + " expects an integer, got '" + val + "'");
    if (x < min || x > max)
        fail(
            "Option " + key + " must be between " + std::to_string(min) +
            " and " + std::to_string(max)
        );
    return x;
}

static const size_t N_GROUPS = 10u;

static Options parse(int argc, char ** argv)
{
    Options o;
    for (int i = 1; i < argc; i += 2)
    {
        std::string key = argv[i];
        if (i + 1 >= argc)
            fail("Option " + key + " has no value");
        std::string val = argv[i + 1];
        if (key == "--sizes")
        {
            o.sizes.clear();
            // 100 initial cases, and every mixing group needs agents
            for (auto & s : split(val))
                o.sizes.push_back(static_cast< size_t >(parse_long(key, s, 1000)));
        }
        else if (key == "--reps")
            o.reps = static_cast< int >(parse_long(key, val, 1, INT_MAX));
        else if (key == "--days")
            o.days = static_cast< int >(parse_long(key, val, 1, INT_MAX));
        else if (key == "--scenarios")
            o.scenarios = split(val);
        else
            fail("Unknown option " + key);
    }

    if (o.sizes.empty())
        fail("Option --sizes needs at least one size");
    if (o.scenarios.empty())
        fail("Option --scenarios needs at least one scenario");
    for (auto & scen : o.scenarios)
        if (
            scen != "L" && scen != "U" && scen != "S" && scen != "E" &&
            scen != "Q" && scen != "M"
        )
            fail("Unknown scenario " + scen);

    return o;
}

// FNV-1a over a stream of integers.
static void hash_ints(uint64_t & h, const std::vector< int > & x)
{
    for (int v : x)
    {
        uint32_t u = static_cast< uint32_t >(v);
        for (int b = 0; b < 4; ++b)
        {
            h ^= (u >> (8 * b)) & 0xffu;
            h *= 1099511628211ull;
        }
    }
}

static uint64_t run_checksum(Model<> & m)
{
    uint64_t h = 1469598103934665603ull;
    std::vector< int > counts;
    m.get_db().get_hist_total(nullptr, nullptr, &counts);
    hash_ints(h, counts);
    std::vector< int > date, source, target, virus, sexp;
    m.get_db().get_transmissions(date, source, target, virus, sexp);
    hash_ints(h, date);
    hash_ints(h, source);
    hash_ints(h, target);
    hash_ints(h, virus);
    hash_ints(h, sexp);
    return h;
}

static double quantile(std::vector< double > x, double q)
{
    std::sort(x.begin(), x.end());
    double pos = q * static_cast< double >(x.size() - 1u);
    size_t lo = static_cast< size_t >(std::floor(pos));
    size_t hi = static_cast< size_t >(std::ceil(pos));
    return x[lo] + (x[hi] - x[lo]) * (pos - static_cast< double >(lo));
}

static double cpu_ms(std::clock_t t0, std::clock_t t1)
{
    return 1000.0 * static_cast< double >(t1 - t0) / CLOCKS_PER_SEC;
}

// Scenarios -------------------------------------------------------------------

// Column-major, N_GROUPS x N_GROUPS
static std::vector< double > contact_matrix()
{
    std::vector< double > cm(N_GROUPS * N_GROUPS, 0.5);
    for (size_t g = 0u; g < N_GROUPS; ++g)
        cm[g * N_GROUPS + g] = 8.0;
    return cm;
}

// Equal groups of agents
static void add_groups(Model<> & model, size_t n)
{
    size_t size = n / N_GROUPS;
    for (size_t g = 0u; g < N_GROUPS; ++g)
    {
        size_t from = g * size;
        size_t to = (g == N_GROUPS - 1u) ? n : from + size;
        model.add_entity(Entity<>(
            "Group " + std::to_string(g),
            distribute_entity_to_range<>(
                static_cast< int >(from), static_cast< int >(to)
            )
        ));
    }
}

typedef measles::ModelMeaslesMixing<> ModelMeasles;

static ModelMeasles build_measles(size_t n)
{
    ModelMeasles model(
        static_cast< epiworld_fast_uint >(n),
        100.0 / static_cast< double >(n), // Initial prevalence
        0.1,  // Transmission rate
        0.9,  // Vaccination efficacy
        0.3,  // Vaccination reduction recovery rate
        7.0,  // Incubation period
        4.0,  // Prodromal period
        5.0,  // Rash period
        {15.0}, // Contact matrix (one group)
        0.2,  // Hospitalization rate
        7.0,  // Hospitalization duration
        3.0,  // Days undetected
        21,   // Quarantine period
        0.8,  // Quarantine willingness
        0.8,  // Isolation willingness
        4,    // Isolation period
        0.0,  // Proportion vaccinated
        1.0,  // Contact tracing success rate
        4u    // Contact tracing days window
    );
    model.add_entity(Entity<>("Population", distribute_entity_to_range<>(0, static_cast< int >(n))));
    return model;
}

struct Result {
    std::vector< double > ms;
    std::vector< double > final_size;
    uint64_t checksum = 1469598103934665603ull;
};

static Result time_model(Model<> & model, const Options & o)
{
    Result res;
    model.verbose_off();

    // Warm-up
    model.run(static_cast< epiworld_fast_uint >(o.days), 999);

    for (int r = 0; r < o.reps; ++r)
    {
        std::clock_t t0 = std::clock();
        model.run(static_cast< epiworld_fast_uint >(o.days), 1000 + r);
        std::clock_t t1 = std::clock();
        res.ms.push_back(cpu_ms(t0, t1));

        std::vector< int > today;
        model.get_db().get_today_total(nullptr, &today);
        res.final_size.push_back(
            static_cast< double >(model.size()) - static_cast< double >(today[0u])
        );

        uint64_t h = run_checksum(model);
        res.checksum ^= h + 0x9e3779b97f4a7c15ull + (res.checksum << 6) + (res.checksum >> 2);
    }

    return res;
}

static Result run_scenario(const std::string & scen, size_t n, const Options & o)
{
    const double prevalence = 100.0 / static_cast< double >(n);

    if (scen == "L" || scen == "U")
    {
        epimodels::ModelSEIR<> model("Benchmark pathogen", prevalence, 0.05, 4.0, 0.2);
        model.seed(20260930);
        model.agents_smallworld(n, 10, false, 0.05);
        model.set_transmission_mode(scen == "L" ? "pull" : "push");
        return time_model(model, o);
    }

    if (scen == "S")
    {
        epimodels::ModelSIRMixing<> model(
            "Benchmark pathogen", static_cast< epiworld_fast_uint >(n),
            prevalence, 0.05, 0.2, contact_matrix()
        );
        model.seed(20260930);
        add_groups(model, n);
        return time_model(model, o);
    }

    if (scen == "E")
    {
        epimodels::ModelSEIRMixing<> model(
            "Benchmark pathogen", static_cast< epiworld_fast_uint >(n),
            prevalence, 0.05, 4.0, 0.2, contact_matrix()
        );
        model.seed(20260930);
        add_groups(model, n);
        return time_model(model, o);
    }

    if (scen == "Q")
    {
        epimodels::ModelSEIRMixingQuarantine<> model(
            "Benchmark pathogen", static_cast< epiworld_fast_uint >(n),
            prevalence, 0.05, 4.0, 0.2, contact_matrix(),
            0.05, // Hospitalization rate
            5.0,  // Hospitalization period
            2.0,  // Days undetected
            7,    // Quarantine period
            0.8,  // Quarantine willingness
            0.8,  // Isolation willingness
            5     // Isolation period
        );
        model.seed(20260930);
        add_groups(model, n);
        return time_model(model, o);
    }

    ModelMeasles model = build_measles(n);
    model.seed(20260930);
    return time_model(model, o);
}

int main(int argc, char ** argv)
{

    Options o = parse(argc, argv);

    std::printf(
        "epiworld %d.%d.%d | days=%d reps=%d\n",
        EPIWORLD_VERSION_MAJOR, EPIWORLD_VERSION_MINOR, EPIWORLD_VERSION_PATCH,
        o.days, o.reps
    );
    std::printf(
        "%-9s %8s %10s %10s %10s %11s %18s\n",
        "scenario", "n", "median_ms", "q1_ms", "q3_ms", "final_size", "checksum"
    );

    for (auto & scen : o.scenarios)
    {
        for (auto n : o.sizes)
        {
            Result res = run_scenario(scen, n, o);

            std::printf(
                "%-9s %8zu %10.3f %10.3f %10.3f %11.0f %018llx\n",
                scen.c_str(), n,
                quantile(res.ms, 0.5), quantile(res.ms, 0.25), quantile(res.ms, 0.75),
                quantile(res.final_size, 0.5),
                static_cast< unsigned long long >(res.checksum)
            );
            std::fflush(stdout);
        }
    }

    return 0;

}
