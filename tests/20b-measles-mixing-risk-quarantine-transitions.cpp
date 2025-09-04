#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles risk quarantine - transition matrix validation",
    "[ModelMeaslesMixingRiskQuarantine_transitions]"
) {
    
    std::vector<double> contact_matrix = {1.0};
    
    epimodels::ModelMeaslesMixingRiskQuarantine<> model(
        1000,        // Number of agents
        0.05,        // Initial prevalence (higher for testing)
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
        .9,          // Quarantine willingness
        .9,          // Isolation willingness
        4,           // Isolation period
        0.0,         // Proportion vaccinated
        0.1,         // Detection rate during quarantine
        1.0,         // Contact tracing success rate
        4u           // Contact tracing days prior
    );

    // Adding a single entity
    model.add_entity(Entity<>("Population", dist_factory<>(0, 1000)));

    // Setting the distribution function of initial cases
    model.get_virus(0).set_distribution(dist_virus<>(50));
    
    model.initial_states({1.0, 0.0});

    // Run multiple simulations to get transition matrix
    int nsims = 100;
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * 50, -1.0);
    
    auto saver = tests_create_saver(transitions, R0s, 50);
    model.run_multiple(60, nsims, 123, saver, true, true, 4);

    // Calculate average transitions
    auto avg_transitions = tests_calculate_avg_transitions(transitions, model);
    #define mat(i, j) avg_transitions[j * model.get_n_states() + i]

    std::cout << "\n=== TRANSITION MATRIX VALIDATION ===" << std::endl;
    
    // Test basic disease progression transitions
    std::cout << "Transition from Exposed to Prodromal: " <<
        mat(1, 2) << " (expected: " << 
        1.0/model("Incubation period") << ")" << std::endl;

    std::cout << "Transition from Prodromal to Rash: " <<
        mat(2, 3) << " (expected: " << 
        1.0/model("Prodromal period") << ")" << std::endl;

    std::cout << "Transition from Rash to Recovered: " <<
        mat(3, 12) << " (expected: " << 
        1.0/model("Rash period") << ")" << std::endl;

    // Test enhanced detection during quarantine
    std::cout << "\n--- Enhanced Detection During Quarantine ---" << std::endl;
    std::cout << "Transition from Prodromal to Quarantined Prodromal: " <<
        mat(2, 9) << " (should be > 0 due to enhanced detection)" << std::endl;

    // Test quarantine transitions  
    std::cout << "\n--- Quarantine State Transitions ---" << std::endl;
    std::cout << "Quarantined Susceptible to Susceptible: " << mat(8, 0) << std::endl;
    std::cout << "Quarantined Exposed to Exposed: " << mat(7, 1) << std::endl;
    std::cout << "Quarantined Exposed to Prodromal: " << mat(7, 2) << std::endl;
    std::cout << "Quarantined Exposed to Quarantined Prodromal: " << mat(7, 9) << std::endl;

    // Test isolation transitions
    std::cout << "\n--- Isolation State Transitions ---" << std::endl;
    std::cout << "Isolated to Isolated Recovered: " << mat(4, 5) << std::endl;
    std::cout << "Isolated Recovered to Recovered: " << mat(5, 12) << std::endl;

    #ifdef CATCH_CONFIG_MAIN
    // Validate core disease progression  
    REQUIRE_FALSE(
        moreless(mat(1, 2), 1.0/model("Incubation period"), 0.1)
    );
    
    REQUIRE_FALSE(
        moreless(mat(2, 3), 1.0/model("Prodromal period"), 0.1)
    );
    
    // Should have enhanced detection during quarantine
    REQUIRE(mat(2, 9) > 0.0);
    
    // Should have quarantine transitions
    REQUIRE(mat(8, 0) > 0.0); // Quarantined susceptible to susceptible
    REQUIRE(mat(7, 1) > 0.0); // Quarantined exposed to exposed
    
    // Should have isolation recovery  
    REQUIRE(mat(5, 12) > 0.0); // Isolated recovered to recovered
    #endif

    #undef mat

    std::cout << "\nTransition matrix validation completed" << std::endl;

    return 0;
}
