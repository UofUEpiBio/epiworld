#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles Entity Quarantine behavior verification",
    "[ModelMeaslesEntityQuarantine]"
) {
    
    // Simple contact matrix (single group, all mixing)
    std::vector<double> contact_matrix = {1.0};
    
    // Vaccination prevalence thresholds and quarantine durations
    std::vector<double> vax_thresholds = {0.0};
    std::vector<size_t> quarantine_durations = {14};
    
    epimodels::ModelMeaslesEntityQuarantine<> model(
        200,         // Number of agents
        5 / 200.0,   // Initial prevalence
        4.0,         // Contact rate
        0.3,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        contact_matrix, // Contact matrix
        0.1,         // Hospitalization rate
        7.0,         // Hospitalization duration
        2.0,         // Days undetected (more frequent detection)
        1.0,         // Quarantine willingness (everyone willing)
        0.8,         // Isolation willingness
        4,           // Isolation period
        0.1,         // Proportion vaccinated
        vax_thresholds,      // Vaccination prevalence thresholds
        quarantine_durations // Quarantine durations
    );

    // Adding a single entity
    model.add_entity(Entity<>("Population", dist_factory<>(0, 200)));
    
    // Initialize entity data structures after adding entities
    model.initialize_entity_data();

    // Run simulation longer to see quarantine effects
    model.run(40, 456);
    
    // Count quarantined agents at the end
    size_t quarantined_agents = 0;
    for (size_t i = 0; i < model.size(); ++i) {
        int state = model.get_agent(i).get_state();
        if (state >= 7 && state <= 10) { // Quarantined states
            quarantined_agents++;
        }
    }

    #ifndef CATCH_CONFIG_MAIN
    printf("Quarantined agents at end: %zu\n", quarantined_agents);
    model.print(false);
    #endif

    // Basic checks
    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(model.size() == 200);
    REQUIRE(model.get_n_entities() == 1);
    #endif
}