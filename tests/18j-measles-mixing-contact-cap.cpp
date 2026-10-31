#include "tests.hpp"
#include "measles-stages.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

// When the contacts per day outnumber the agents available for contact, the
// probability of contacting each infectious agent is capped at 1 (it was
// contacts / available agents, above 1, which aborts under
// _GLIBCXX_ASSERTIONS and is undefined behavior otherwise). Here 20 agents
// make 50 contacts a day: every susceptible agent meets the one prodromal
// case and, with transmission 1, all 19 are infected on day 2.
EPIWORLD_TEST_CASE(
    "Measles mixing: contact probability capped at 1",
    "[MeaslesMixingContactCap]"
) {

    size_t n = 20u;

    measles::ModelMeaslesMixing<> model(
        n,           // Number of agents
        1.0 / 20.0,  // Initial prevalence: one case
        1.0,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        1.0,         // Incubation period
        3.0,         // Prodromal period
        2.0,         // Rash period
        0.0,         // Hospitalization rate
        7.0,         // Hospitalization duration
        2.0,         // Days undetected
        -1,          // Quarantine period (off)
        1.0,         // Quarantine willingness
        1.0,         // Isolation willingness
        -1,          // Isolation period (off)
        0.0,         // Proportion vaccinated
        1.0,         // Contact tracing success rate
        4u,          // Contact tracing days window
        1.0,         // Rash reduction contact rate
        0.0,         // Incubation period sd
        0.0,         // Prodromal period sd
        0.0          // Rash period sd
    );

    add_group_mixing(model, {50.0}, blocks({0u, n}));
    model.verbose_off();
    model.run(5, 123);

    // The case is prodromal from day 1, so everyone else is infected on day 2
    auto infected = transitions_by_day(model, "Susceptible", "Latent");
    REQUIRE(infected[2] == static_cast<int>(n) - 1);

}
