#include "tests.hpp"
#include "measles-stages.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

// With fixed stage durations (standard deviations of zero) and no
// transmission, every initial case of the risk-quarantine model goes through
// its stages on the same days: latent for 5 days, prodromal for 3 and rash
// for 2. With drawn durations, a run depends only on its seed. (The measles
// mixing model, which has stages of a natural history, is tested in
// 51b-measles-mixing-stages.cpp.)
EPIWORLD_TEST_CASE(
    "Measles models: fixed stage durations",
    "[MeaslesStages]"
) {

    size_t n = 200u;

    measles::ModelMeaslesMixingRiskQuarantine<> model_risk(
        n,           // Number of agents
        1.0,         // Initial prevalence: everyone
        0.0,         // Transmission rate
        0.9,         // Vaccination efficacy
        5.0,         // Incubation period
        3.0,         // Prodromal period
        2.0,         // Rash period
        0.0,         // Hospitalization rate
        7.0,         // Hospitalization duration
        2.0,         // Days undetected
        21,          // Quarantine period high risk
        14,          // Quarantine period medium risk
        7,           // Quarantine period low risk
        1.0,         // Quarantine willingness
        1.0,         // Isolation willingness
        -1,          // Isolation period (no detection)
        0.0,         // Proportion vaccinated
        0.0,         // Detection rate during quarantine
        1.0,         // Contact tracing success rate
        4u,          // Contact tracing days window
        0.0,         // Incubation period sd
        0.0,         // Prodromal period sd
        0.0          // Rash period sd
    );

    add_group_mixing(model_risk, {1.0}, blocks({0u, n}));
    model_risk.verbose_off();
    model_risk.run(20, 123);

    auto risk_on = [&](const std::string & from, const std::string & to, int day) {
        auto counts = stage_transitions_by_day(model_risk, from, to);
        return counts[day] == static_cast<int>(n);
    };

    REQUIRE(risk_on("Latent", "Prodromal", 5));
    REQUIRE(risk_on("Prodromal", "Rash", 8));
    REQUIRE(risk_on("Rash", "Recovered", 10));

    // With drawn (lognormal) durations, a run depends only on its seed: the
    // same seed gives the same history after a run with another seed
    model_risk.set_param("Incubation period sd", 2.5);
    model_risk.set_param("Prodromal period sd", 1.7);
    model_risk.set_param("Rash period sd", 1.0);

    auto history = [&](int seed) {
        model_risk.run(30, seed);
        std::vector< std::string > from, to;
        std::vector< int > date, counts;
        model_risk.get_db().get_hist_transition_matrix(from, to, date, counts, true);
        return std::make_pair(date, counts);
    };

    auto first = history(7);
    history(99);
    REQUIRE(history(7) == first);

}
