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

// Benchmarks model parameter lookups: how long a `par()` call takes, and how
// long `run()` takes on models whose hot paths look parameters up by name.
// The file uses only the string-based parameter API, so it compiles against
// older releases and can time two versions side by side.
//
// Usage: main [--sizes 100000] [--reps 5] [--days 60]
//             [--scenarios P,A,M] [--calls 10000000]
//
// P: micro benchmark. Nanoseconds per `par()`, `get_param()`, and
//    `operator()` call on a model with 20 parameters, looking up a name that
//    fits in the short-string buffer ("Recovery rate") and one that does not
//    ("Transmission rate", 17 characters). On versions with `ParamId` and
//    `ParamRef`, it also times reading by position.
// A: SEIRH on a Watts-Strogatz network (mean degree 10, R0 near 4). The
//    virus reads "Transmission rate" by name for every susceptible-infected
//    contact, and `new_state_update_transition` reads its rates by name for
//    every exposed and infected agent, every day.
// M: ModelMeaslesMixing (one group, 15 contacts a day, quarantine and
//    contact tracing on). Its update functions call `par()` about 20 times
//    per affected agent per day.
//
// For A and M it prints the median (Q1, Q3) CPU milliseconds per run, the
// median final size, and a checksum of every replicate's daily counts and
// transmissions -- equal checksums mean bit-identical runs.

struct Options {
    std::vector< size_t > sizes = {100000};
    int reps = 5;
    int days = 60;
    std::vector< std::string > scenarios = {"P", "A", "M"};
    long calls = 10000000;
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
            // Scenario M starts with 100 cases, so it needs 100 agents
            for (auto & s : split(val))
                o.sizes.push_back(static_cast< size_t >(parse_long(key, s, 100)));
        }
        else if (key == "--reps")
            o.reps = static_cast< int >(parse_long(key, val, 1, INT_MAX));
        else if (key == "--days")
            o.days = static_cast< int >(parse_long(key, val, 1, INT_MAX));
        else if (key == "--scenarios")
            o.scenarios = split(val);
        else if (key == "--calls")
            o.calls = parse_long(key, val, 1);
        else
            fail("Unknown option " + key);
    }

    if (o.sizes.empty())
        fail("Option --sizes needs at least one size");
    if (o.scenarios.empty())
        fail("Option --scenarios needs at least one scenario");
    for (auto & scen : o.scenarios)
        if (scen != "P" && scen != "A" && scen != "M")
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

// Scenario P ------------------------------------------------------------------

template< typename TFun >
static double ns_per_call(long calls, TFun fun)
{
    volatile double sink = 0.0;
    std::clock_t t0 = std::clock();
    for (long i = 0; i < calls; ++i)
        sink = sink + fun();
    std::clock_t t1 = std::clock();
    (void) sink;
    return 1e6 * cpu_ms(t0, t1) / static_cast< double >(calls);
}

static void run_micro(const Options & o)
{
    Model<> model;
    for (int i = 0; i < 18; ++i)
        model.add_param(0.01 * i, "Filler parameter " + std::to_string(i));
    model.add_param(0.1, "Transmission rate");
    model.add_param(0.2, "Recovery rate");

    std::printf("%-26s %-19s %8s\n", "call", "name", "ns/call");
    for (const char * name : {"Recovery rate", "Transmission rate"})
    {
        double ns;

        ns = ns_per_call(o.calls, [&]() { return model.par(name); });
        std::printf("%-26s %-19s %8.2f\n", "par(\"...\")", name, ns);

        ns = ns_per_call(o.calls, [&]() { return model.get_param(name); });
        std::printf("%-26s %-19s %8.2f\n", "get_param(\"...\")", name, ns);

        ns = ns_per_call(o.calls, [&]() { return model(name); });
        std::printf("%-26s %-19s %8.2f\n", "operator()(\"...\")", name, ns);

        std::string sname(name);
        ns = ns_per_call(o.calls, [&]() { return model.par(sname); });
        std::printf("%-26s %-19s %8.2f\n", "par(std::string)", name, ns);

        #ifdef EPI_PAR
        ParamId id = model.get_param_id(name);
        ns = ns_per_call(o.calls, [&]() { return model.par_at(id); });
        std::printf("%-26s %-19s %8.2f\n", "par_at(ParamId)", name, ns);

        ParamRef ref(name);
        ns = ns_per_call(o.calls, [&]() { return ref(model); });
        std::printf("%-26s %-19s %8.2f\n", "ParamRef", name, ns);
        #endif
    }
    std::fflush(stdout);
}

// Scenario A ------------------------------------------------------------------

static void build_seirh(Model<> & model, size_t n)
{
    model.add_param(0.1, "Transmission rate");
    model.add_param(1.0 / 4.0, "Incubation rate");
    model.add_param(0.01, "Hospitalization rate");
    model.add_param(1.0 / 7.0, "Recovery rate");
    model.add_param(1.0 / 7.0, "Hospital recovery rate");

    model.add_state("Susceptible", sampler::make_update_susceptible<>({2u}));
    model.add_state("Exposed", new_state_update_transition<>({"Incubation rate"}, {2u}));
    model.add_state("Infected", new_state_update_transition<>(
        {"Hospitalization rate", "Recovery rate"}, {3u, 4u}
    ));
    model.add_state("Hospitalized", new_state_update_transition<>(
        {"Hospital recovery rate"}, {4u}
    ));
    model.add_state("Recovered");

    Virus<> pathogen("Benchmark pathogen", 100.0, false);
    pathogen.set_state(1, 4, 4);
    pathogen.set_prob_infecting("Transmission rate");
    model.add_virus(pathogen);

    model.agents_smallworld(n, 10, false, 0.05);
}

// Scenario M ------------------------------------------------------------------

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

// Timing ----------------------------------------------------------------------

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

int main(int argc, char ** argv)
{

    Options o = parse(argc, argv);

    std::printf(
        "epiworld %d.%d.%d | days=%d reps=%d\n",
        EPIWORLD_VERSION_MAJOR, EPIWORLD_VERSION_MINOR, EPIWORLD_VERSION_PATCH,
        o.days, o.reps
    );

    bool header = false;
    for (auto & scen : o.scenarios)
    {

        if (scen == "P")
        {
            run_micro(o);
            continue;
        }

        if (!header)
        {
            std::printf(
                "%-9s %8s %10s %10s %10s %11s %18s\n",
                "scenario", "n", "median_ms", "q1_ms", "q3_ms",
                "final_size", "checksum"
            );
            header = true;
        }

        for (auto n : o.sizes)
        {
            Result res;
            if (scen == "A")
            {
                Model<> model;
                model.seed(20260928);
                build_seirh(model, n);
                res = time_model(model, o);
            }
            else if (scen == "M")
            {
                ModelMeasles model = build_measles(n);
                model.seed(20260928);
                res = time_model(model, o);
            }
            else
            {
                std::fprintf(stderr, "Unknown scenario %s\n", scen.c_str());
                return 1;
            }

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
