#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

// Detected cases leave isolation once the isolation period is over, the
// initial cases included: with no transmission, every one of them ends up
// Recovered and nobody is left in Isolated Recovered.
EPIWORLD_TEST_CASE(
    "Measles model with mixing: initial cases leave isolation",
    "[ModelMeaslesMixingIsolation]"
) {

    size_t n = 1000;
    measles::ModelMeaslesMixing<> model(
        n,           // Number of agents
        0.05,        // Initial prevalence (50 cases)
        0.0,         // Transmission rate: only the initial cases
        0.99,        // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        0.0,         // Hospitalization rate
        7.0,         // Hospitalization duration
        1.0,         // Days undetected
        21,          // Quarantine period
        1.0,         // Quarantine willingness
        1.0,         // Isolation willingness
        4,           // Isolation period
        0.5          // Proportion vaccinated
    );

    add_group_mixing(model, {1.0}, blocks({0u, n}));
    model.verbose_off();

    for (int seed = 1; seed <= 10; ++seed)
    {
        model.run(150, seed);
        REQUIRE(model.get_db().get_today_total("Isolated Recovered") == 0);
        REQUIRE(model.get_db().get_today_total("Recovered") == 50);
    }

}
