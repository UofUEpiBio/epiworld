#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles Entity Quarantine with multiple entities",
    "[ModelMeaslesEntityQuarantine]"
) {
    
    // Contact matrix for 3 entities
    std::vector<double> contact_matrix = {
        1.0, 0.5, 0.2,  // Entity 0 contacts
        0.5, 1.0, 0.3,  // Entity 1 contacts  
        0.2, 0.3, 1.0   // Entity 2 contacts
    };
    
    // Vaccination prevalence thresholds and quarantine durations
    // Lower vaccination prevalence -> longer quarantine
    std::vector<double> vax_thresholds = {0.0, 0.5, 0.8};
    std::vector<size_t> quarantine_durations = {21, 14, 7};
    
    epimodels::ModelMeaslesEntityQuarantine<> model(
        1500,        // Number of agents
        10 / 1500.0, // Initial prevalence
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
        .9,          // Quarantine willingness
        .8,          // Isolation willingness
        4,           // Isolation period
        0.0,         // Proportion vaccinated (set to 0 initially)
        vax_thresholds,      // Vaccination prevalence thresholds
        quarantine_durations // Quarantine durations
    );

    // Adding three entities with different vaccination rates
    model.add_entity(Entity<>("Low Vax Entity", dist_factory<>(0, 500)));      // 0% vaccinated
    model.add_entity(Entity<>("Medium Vax Entity", dist_factory<>(500, 1000))); // Will set to ~60%
    model.add_entity(Entity<>("High Vax Entity", dist_factory<>(1000, 1500))); // Will set to ~90%
    
    // Initialize entity data structures after adding entities
    model.initialize_entity_data();

    // Test vaccination prevalence calculation with no vaccines first
    double vax_prev_0 = model.calculate_entity_vaccination_prevalence(0);
    double vax_prev_1 = model.calculate_entity_vaccination_prevalence(1);
    double vax_prev_2 = model.calculate_entity_vaccination_prevalence(2);

    #ifndef CATCH_CONFIG_MAIN
    printf("Entity 0 vaccination prevalence (before): %.3f\n", vax_prev_0);
    printf("Entity 1 vaccination prevalence (before): %.3f\n", vax_prev_1);
    printf("Entity 2 vaccination prevalence (before): %.3f\n", vax_prev_2);
    #endif

    // Test quarantine duration calculation
    size_t quarantine_dur_0 = model.get_quarantine_duration_for_entity(0);
    size_t quarantine_dur_1 = model.get_quarantine_duration_for_entity(1);
    size_t quarantine_dur_2 = model.get_quarantine_duration_for_entity(2);

    #ifndef CATCH_CONFIG_MAIN
    printf("Entity 0 quarantine duration: %zu days\n", quarantine_dur_0);
    printf("Entity 1 quarantine duration: %zu days\n", quarantine_dur_1);
    printf("Entity 2 quarantine duration: %zu days\n", quarantine_dur_2);
    #endif

    // Run simulation
    model.run(30, 123);
    
    #ifndef CATCH_CONFIG_MAIN
    model.print(false);
    #endif

    // Basic checks
    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(model.size() == 1500);
    REQUIRE(model.get_n_entities() == 3);
    
    // Check vaccination prevalence calculations (all should be 0 since no manual vaccination)
    REQUIRE(vax_prev_0 == Approx(0.0).margin(0.01));      // 0% vaccinated
    REQUIRE(vax_prev_1 == Approx(0.0).margin(0.01));      // 0% vaccinated
    REQUIRE(vax_prev_2 == Approx(0.0).margin(0.01));      // 0% vaccinated
    
    // Check quarantine durations (all should be 21 due to low vaccination)
    REQUIRE(quarantine_dur_0 == 21);  // Low vaccination -> longer quarantine
    REQUIRE(quarantine_dur_1 == 21);  // Low vaccination -> longer quarantine
    REQUIRE(quarantine_dur_2 == 21);  // Low vaccination -> longer quarantine
    #endif

}