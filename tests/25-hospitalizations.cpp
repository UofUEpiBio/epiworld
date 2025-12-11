#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test hospitalization tracking for models with hospitalization states
 * 
 * This test validates that the get_hospitalizations function correctly extracts
 * hospitalization events from the transition matrix. The test:
 * 1. Runs a simulation with a model containing hospitalization states
 * 2. Extracts both the transition matrix and hospitalizations using get_hospitalizations
 * 3. Computes hospitalizations manually from the transition matrix
 * 4. Verifies that the two methods produce the same results
 */
EPIWORLD_TEST_CASE("Hospitalization tracking", "[hospitalizations]") {

    // Create a SEIR model with mixing, quarantine and hospitalization
    // Using ModelSEIRMixingQuarantine which has "Hospitalized" states
    // Contact matrix (rows must sum to 1)
    std::vector<double> contact_matrix = {
        0.7, 0.3,  // Group 1 contacts: 70% within group, 30% with group 2
        0.3, 0.7   // Group 2 contacts: 30% with group 1, 70% within group
    };

    epimodels::ModelSEIRMixingQuarantine<> model(
        "test virus",     // Virus name
        200,              // Total population
        0.01,             // Initial prevalence
        10.0,             // Contact rate
        0.2,              // Transmission rate
        7.0,              // Average incubation days
        0.3,              // Recovery rate
        contact_matrix,   // Contact matrix (2x2, so 2 groups)
        0.1,              // Hospitalization rate
        7.0,              // Hospitalization period
        3.0,              // Days undetected
        14,               // Quarantine period
        0.8,              // Quarantine willingness
        0.8,              // Isolation willingness
        7,                // Isolation period
        0.9,              // Contact tracing success rate
        3u                // Contact tracing days prior
    );

    // Creating two groups/entities
    Entity<> e1("Group 1", dist_factory<>(0, 100));
    Entity<> e2("Group 2", dist_factory<>(100, 200));
    
    model.add_entity(e1);
    model.add_entity(e2);

    model.seed(12345);
    model.verbose_off();

    // Run the simulation
    int ndays = 30;
    model.run(ndays, 123);

    // Get hospitalizations using the new function
    std::vector<int> hosp_date, hosp_virus_id, hosp_counts;
    model.get_db().get_hospitalizations(
        hosp_date,
        hosp_virus_id,
        hosp_counts
    );

    // Manually compute hospitalizations from transition matrix
    std::vector<std::string> state_from, state_to;
    std::vector<int> trans_date, trans_counts;
    model.get_db().get_hist_transition_matrix(
        state_from, state_to, trans_date, trans_counts, true
    );

    // Count transitions to hospitalized states
    size_t n_days = model.get_ndays() + 1u;
    size_t n_viruses = model.get_n_viruses();
    std::vector<int> expected_counts(n_days * n_viruses, 0);
    
    for (size_t i = 0u; i < state_to.size(); ++i)
    {
        // Check if the destination state contains "ospitalized"
        // Using "ospitalized" to match both "Hospitalized" and "Detected Hospitalized"
        if (state_to[i].find("ospitalized") != std::string::npos)
        {
            int v_id = 0; // Single virus model
            int d = trans_date[i];
            auto location = d + v_id * n_days;
            expected_counts[location] += trans_counts[i];
        }
    }

    #ifdef CATCH_CONFIG_MAIN
    // Verify that the two methods match
    REQUIRE(hosp_counts.size() == expected_counts.size());
    
    int total_hospitalizations_new = 0;
    int total_hospitalizations_expected = 0;
    
    for (size_t i = 0; i < hosp_counts.size(); ++i)
    {
        total_hospitalizations_new += hosp_counts[i];
        total_hospitalizations_expected += expected_counts[i];
        REQUIRE(hosp_counts[i] == expected_counts[i]);
    }
    
    // Verify we actually had some hospitalizations
    REQUIRE(total_hospitalizations_new > 0);
    REQUIRE(total_hospitalizations_expected > 0);
    REQUIRE(total_hospitalizations_new == total_hospitalizations_expected);
    
    // Print summary for debugging
    std::cout << "Hospitalization tracking test passed:" << std::endl;
    std::cout << "  Total hospitalizations: " << total_hospitalizations_new << std::endl;
    std::cout << "  Days simulated: " << ndays << std::endl;
    #endif

    #ifndef CATCH_CONFIG_MAIN
    // Print some information for manual verification
    int total = 0;
    for (size_t i = 0; i < hosp_counts.size(); ++i)
    {
        if (hosp_counts[i] > 0)
        {
            std::cout << "Day " << hosp_date[i] 
                      << ": " << hosp_counts[i] 
                      << " hospitalizations" << std::endl;
            total += hosp_counts[i];
        }
    }
    std::cout << "Total hospitalizations: " << total << std::endl;
    return 0;
    #endif

}
