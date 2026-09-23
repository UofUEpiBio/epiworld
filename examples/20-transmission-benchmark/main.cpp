#include "../../include/epiworld/epiworld.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

using namespace epiworld;

// Benchmarks the network transmission step: the time per `run()` of three
// network models, in each transmission mode. The same file compiles against
// releases that predate transmission modes (they always pull), so it can time
// an older version side by side with this one.
//
// Usage: main [--sizes 10000,100000] [--reps 10] [--days 100]
//             [--scenarios A,B,C] [--modes auto,push,pull] [--queuing on|off]
//             [--kappa 1.0]
//
// A: the SEIRH model of the epiworld-benchmark study (Watts-Strogatz graph,
//    mean degree 10, R0 = 2, 100 initial cases), built exactly as its epiworldR
//    runner builds it.
// B: the built-in ModelSEIR on the same graph.
// C: a dense, high-prevalence SIR (mean degree 50), where pulling can be the
//    cheaper step near the peak.
//
// For each cell it prints the median (Q1, Q3) CPU milliseconds per run (CPU time
// rather than wall time, so that other load on the machine does not count), the
// median
// final size, how many steps pushed and pulled, and a checksum of every
// replicate's daily counts and transmissions -- equal checksums mean
// bit-identical runs.

struct Options {
    std::vector< size_t > sizes = {10000};
    int reps = 5;
    int days = 100;
    std::vector< std::string > scenarios = {"A", "B", "C"};
    std::vector< std::string > modes = {"auto", "push", "pull"};
    bool queuing = true;
    double kappa = -1.0; // < 0: the model's default
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

static Options parse(int argc, char ** argv)
{
    Options o;
    for (int i = 1; i + 1 < argc; i += 2)
    {
        std::string key = argv[i];
        std::string val = argv[i + 1];
        if (key == "--sizes")
        {
            o.sizes.clear();
            for (auto & s : split(val))
                o.sizes.push_back(static_cast< size_t >(std::stoul(s)));
        }
        else if (key == "--reps")
            o.reps = std::stoi(val);
        else if (key == "--days")
            o.days = std::stoi(val);
        else if (key == "--scenarios")
            o.scenarios = split(val);
        else if (key == "--modes")
            o.modes = split(val);
        else if (key == "--queuing")
            o.queuing = (val == "on");
        else if (key == "--kappa")
            o.kappa = std::stod(val);
        else
        {
            std::fprintf(stderr, "Unknown option %s\n", key.c_str());
            std::exit(1);
        }
    }
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

// Scenario A: epiworld-benchmark's SEIRH runner (runners/epiworld.R),
// translated line by line.
static void build_seirh(Model<> & model, size_t n)
{
    const double mean_degree = 10.0, target_r0 = 2.0;
    const double latent_days = 4.0, infectious_days = 7.0;
    const double hosp_prob = 0.05, hosp_days = 7.0;

    double recovery = 1.0 / infectious_days;
    double transmissibility = std::min(0.999, target_r0 / std::max(1.0, mean_degree - 1.0));
    double beta = transmissibility * recovery /
        (1.0 - transmissibility * (1.0 - recovery));
    double hosp_rate = hosp_prob * recovery / (1.0 - hosp_prob * (1.0 - recovery));

    model.add_param(beta, "Transmission rate");
    model.add_param(1.0 / latent_days, "Incubation rate");
    model.add_param(hosp_rate, "Hospitalization rate");
    model.add_param(recovery, "Recovery rate");
    model.add_param(1.0 / hosp_days, "Hospital recovery rate");

    model.add_state("Susceptible", sampler::make_update_susceptible<>({1u, 3u, 4u}));
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

struct Result {
    std::vector< double > ms;
    std::vector< double > final_size;
    uint64_t checksum = 1469598103934665603ull;
    int n_push = 0;
    int n_pull = 0;
};

template< typename TModel >
static Result time_model(
    TModel & model,
    const Options & o,
    const std::string & mode,
    size_t n_susceptible_states
) {

    Result res;

    #ifdef EPIWORLD_HAS_TRANSMISSION_MODE
    model.set_transmission_mode(mode);
    if (o.kappa >= 0.0)
        model.set_transmission_kappa(o.kappa);
    int * n_push = &res.n_push;
    int * n_pull = &res.n_pull;
    model.add_globalevent(
        [n_push, n_pull](Model<> * m) -> void {
            if (m->get_last_transmission_mode() == TransmissionMode::push)
                ++(*n_push);
            else
                ++(*n_pull);
        },
        "count modes"
    );
    #else
    (void) mode;
    #endif

    if (!o.queuing)
        model.queuing_off();

    model.verbose_off();

    // Warm-up
    model.run(static_cast< epiworld_fast_uint >(o.days), 999);
    res.n_push = res.n_pull = 0;

    for (int r = 0; r < o.reps; ++r)
    {
        std::clock_t t0 = std::clock();
        model.run(static_cast< epiworld_fast_uint >(o.days), 1000 + r);
        std::clock_t t1 = std::clock();
        res.ms.push_back(1000.0 * static_cast< double >(t1 - t0) / CLOCKS_PER_SEC);

        std::vector< int > today;
        model.get_db().get_today_total(nullptr, &today);
        double not_infected = 0.0;
        for (size_t s = 0u; s < n_susceptible_states; ++s)
            not_infected += today[s];
        res.final_size.push_back(static_cast< double >(model.size()) - not_infected);

        uint64_t h = run_checksum(model);
        res.checksum ^= h + 0x9e3779b97f4a7c15ull + (res.checksum << 6) + (res.checksum >> 2);
    }

    return res;

}

int main(int argc, char ** argv)
{

    Options o = parse(argc, argv);

    #ifdef EPIWORLD_HAS_TRANSMISSION_MODE
    const bool has_modes = true;
    #else
    const bool has_modes = false;
    #endif

    std::printf(
        "epiworld %d.%d.%d | days=%d reps=%d queuing=%s\n",
        EPIWORLD_VERSION_MAJOR, EPIWORLD_VERSION_MINOR, EPIWORLD_VERSION_PATCH,
        o.days, o.reps, o.queuing ? "on" : "off"
    );
    std::printf(
        "%-9s %8s %-5s %10s %10s %10s %11s %6s %6s %18s\n",
        "scenario", "n", "mode", "median_ms", "q1_ms", "q3_ms",
        "final_size", "push", "pull", "checksum"
    );

    std::vector< std::string > modes = has_modes ?
        o.modes : std::vector< std::string >({"pull"});

    for (auto & scen : o.scenarios)
    {
        for (auto n : o.sizes)
        {
            for (auto & mode : modes)
            {

                Result res;

                if (scen == "A")
                {
                    Model<> model;
                    model.seed(20260907);
                    build_seirh(model, n);
                    res = time_model(model, o, mode, 1u);
                }
                else if (scen == "B")
                {
                    double recovery = 1.0 / 7.0;
                    double t = 2.0 / 9.0;
                    double beta = t * recovery / (1.0 - t * (1.0 - recovery));
                    epimodels::ModelSEIR<> model(
                        "Benchmark pathogen",
                        100.0 / static_cast< double >(n),
                        beta, 4.0, recovery
                    );
                    model.seed(20260907);
                    model.agents_smallworld(n, 10, false, 0.05);
                    res = time_model(model, o, mode, 1u);
                }
                else if (scen == "C")
                {
                    epimodels::ModelSIR<> model("Dense pathogen", 0.01, 0.05, 0.2);
                    model.seed(20260907);
                    model.agents_smallworld(n, 50, false, 0.1);
                    res = time_model(model, o, mode, 1u);
                }
                else
                {
                    std::fprintf(stderr, "Unknown scenario %s\n", scen.c_str());
                    return 1;
                }

                std::printf(
                    "%-9s %8zu %-5s %10.3f %10.3f %10.3f %11.0f %6d %6d %018llx\n",
                    scen.c_str(), n, has_modes ? mode.c_str() : "-",
                    quantile(res.ms, 0.5), quantile(res.ms, 0.25), quantile(res.ms, 0.75),
                    quantile(res.final_size, 0.5), res.n_push, res.n_pull,
                    static_cast< unsigned long long >(res.checksum)
                );
                std::fflush(stdout);

            }
        }
    }

    return 0;

}
