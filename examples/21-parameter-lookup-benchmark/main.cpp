#include "../../include/epiworld/epiworld.hpp"
#include "../../include/measles/measles.hpp"
#include "../../include/cli/cli.hpp"
#include "../../include/bench/bench.hpp"
#include "../../include/stats/stats.hpp"

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

static Options parse(int argc, char ** argv)
{
    Options o;
    // Scenario M starts with 100 cases, so it needs 100 agents
    cli::Parser(
        "Times model parameter lookups, alone and inside models."
    )
        .add_size_list("--sizes", o.sizes, "Population sizes", 100)
        .add_int("--reps", o.reps, "Replicates per cell", 1)
        .add_int("--days", o.days, "Days per run", 1)
        .add_list("--scenarios", o.scenarios, "Scenarios to run", {"P", "A", "M"})
        .add_long("--calls", o.calls, "Calls per row in scenario P", 1)
        .parse(argc, argv);
    return o;
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
    return 1e6 * bench::cpu_ms(t0, t1) / static_cast< double >(calls);
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
            bench::Timing res;
            if (scen == "A")
            {
                Model<> model;
                model.seed(20260928);
                build_seirh(model, n);
                res = bench::time_runs(model, o.days, o.reps);
            }
            else if (scen == "M")
            {
                ModelMeasles model = build_measles(n);
                model.seed(20260928);
                res = bench::time_runs(
                    model, o.days, o.reps,
                    {model.SUSCEPTIBLE, model.QUARANTINED_SUSCEPTIBLE}
                );
            }
            else
            {
                std::fprintf(stderr, "Unknown scenario %s\n", scen.c_str());
                return 1;
            }

            std::printf(
                "%-9s %8zu %10.3f %10.3f %10.3f %11.0f %018llx\n",
                scen.c_str(), n,
                stats::quantile(res.ms, 0.5), stats::quantile(res.ms, 0.25), stats::quantile(res.ms, 0.75),
                stats::quantile(res.final_size, 0.5),
                static_cast< unsigned long long >(res.checksum)
            );
            std::fflush(stdout);
        }
    }

    return 0;

}
