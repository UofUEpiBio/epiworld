#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles model with risk-based quarantine",
    "[ModelMeaslesMixingRiskQuarantine]"
) {
    
    // Simple contact matrix (single group, all mixing)
    std::vector<double> contact_matrix = {1.0};
    
    epimodels::ModelMeaslesMixingRiskQuarantine<> model(
        1000,        // Number of agents
        0.005,       // Initial prevalence
        2.0,         // Contact rate
        0.2,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        contact_matrix, // Contact matrix
        0.2,         // Hospitalization rate
        7.0,         // Hospitalization duration
        3.0,         // Days undetected
        21,          // Quarantine period high risk
        14,          // Quarantine period medium risk
        7,           // Quarantine period low risk
        .8,          // Quarantine willingness
        .8,          // Isolation willingness
        4,           // Isolation period
        0.0,         // Proportion vaccinated
        0.1,         // Detection rate during quarantine
        1.0,         // Contact tracing success rate
        4u           // Contact tracing days prior
    );

    // Adding a single entity (population group)
    model.add_entity(Entity<>("Population", dist_factory<>(0, 1000)));

    // Setting initial states
    model.initial_states({1.0, 0.0});

    // Run simulation
    model.run(50, 123);
    model.print();

    // Check that the model ran successfully
    auto db = model.get_db();
    std::vector<int> totals;
    db.get_today_total(nullptr, &totals);

    #ifdef CATCH_CONFIG_MAIN
    // Basic sanity check - make sure we have agents in the system
    int total_agents = 0;
    for (size_t i = 0; i < totals.size(); ++i) {
        total_agents += totals[i];
    }
    REQUIRE(total_agents == 1000);
    #endif

    std::cout << "Basic test passed - model runs successfully" << std::endl;

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
}
