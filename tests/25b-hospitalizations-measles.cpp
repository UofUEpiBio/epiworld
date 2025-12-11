#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test hospitalization tracking for Measles models
 * 
 * This test validates that the get_hospitalizations function correctly extracts
 * hospitalization events for Measles models which have both "Hospitalized" and
 * "Detected Hospitalized" states.
 */
EPIWORLD_TEST_CASE("Hospitalization tracking - Measles", "[hospitalizations-measles]") {

    // Create a Measles model with mixing and hospitalization
    std::vector<double> contact_matrix = {
        0.8, 0.2,  // Group 1 contacts: 80% within group, 20% with group 2
        0.2, 0.8   // Group 2 contacts: 20% with group 1, 80% within group
    };

    epimodels::ModelMeaslesMixing<> model(
        300,             // Total population
        0.01,            // Initial prevalence
        15.0,            // Contact rate
        0.9,             // Transmission rate (highly contagious)
        0.9,             // Vaccination efficacy
        0.3,             // Vaccination reduction recovery rate
        7.0,             // Incubation period
        4.0,             // Prodromal period
        5.0,             // Rash period
        contact_matrix,  // Contact matrix (2x2, so 2 groups)
        0.15,            // Hospitalization rate (15% of cases)
        7.0,             // Hospitalization period
        3.0,             // Days undetected
        14,              // Quarantine period
        0.8,             // Quarantine willingness
        0.8,             // Isolation willingness
        7,               // Isolation period
        0.0,             // Proportion vaccinated
        0.9,             // Contact tracing success rate
        3u               // Contact tracing days prior
    );

    // Creating two groups/entities
    Entity<> e1("Group 1", dist_factory<>(0, 150));
    Entity<> e2("Group 2", dist_factory<>(150, 300));
    
    model.add_entity(e1);
    model.add_entity(e2);

    model.seed(54321);
    model.verbose_off();

    // Run the simulation
    int ndays = 40;
    model.run(ndays, 456);

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

    // Count transitions to hospitalized states (both "Hospitalized" and "Detected Hospitalized")
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
    std::cout << "Measles hospitalization tracking test passed:" << std::endl;
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
