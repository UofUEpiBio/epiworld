#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles Entity Quarantine model basic compilation",
    "[ModelMeaslesEntityQuarantine]"
) {
    
    int n_seeds = 5;

    // Simple contact matrix (single group, all mixing)
    std::vector<double> contact_matrix = {1.0};
    
    // Vaccination prevalence thresholds and quarantine durations
    std::vector<double> vax_thresholds = {0.0, 0.5, 0.8};
    std::vector<size_t> quarantine_durations = {21, 14, 7};
    
    epimodels::ModelMeaslesEntityQuarantine<> model_0(
        1000,        // Number of agents
        n_seeds / 1000.0,  // Initial prevalence
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
        .8,          // Quarantine willingness
        .8,          // Isolation willingness
        4,           // Isolation period
        0.3,         // Proportion vaccinated
        vax_thresholds,      // Vaccination prevalence thresholds
        quarantine_durations // Quarantine durations
    );

    // Adding a single entity (population group)
    model_0.add_entity(Entity<>("Population", dist_factory<>(0, 1000)));
    
    // Initialize entity data structures after adding entities
    model_0.initialize_entity_data();

    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(v, m);
        return;
    });

    size_t nsims = 400; // Reduced for faster testing
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * n_seeds, -1.0);
        
    auto saver = tests_create_saver(transitions, R0s, n_seeds);

    model_0.run_multiple(60, nsims, 1231, saver, true, true, 4);
    
    #ifndef CATCH_CONFIG_MAIN
    model_0.print(false);
    #endif

    // Basic checks
    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(model_0.size() == 1000);
    REQUIRE(model_0.get_n_entities() == 1);
    #endif

}