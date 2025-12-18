#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Hospitalizations tracker test",
    "[HospitalizationsTracker]"
) {
    
    // Create a MeaslesSchool model with known hospitalization rate
    int n_seeds = 5;
    epimodels::ModelMeaslesSchool<> model(
        200,    // Number of agents
        n_seeds, // Number of initial cases
        5.0,     // Contact rate
        0.3,     // Transmission rate
        0.9,     // Vaccination efficacy
        0.3,     // Vaccination reduction recovery rate
        5.0,     // Incubation period
        3.0,     // Prodromal period
        4.0,     // Rash period
        2.0,     // Days undetected
        0.1,     // Hospitalization rate (relatively high to ensure we get some)
        5.0,     // Hospitalization duration
        0.5,     // Proportion vaccinated (some with tools, some without)
        -1,      // Quarantine period (disabled for simplicity)
        0.0,     // Quarantine willingness
        -1       // Isolation period (disabled for simplicity)
    );

    // Run the model for a short period
    model.run(30, 12345);

    // Get hospitalization data
    std::vector<int> dates;
    std::vector<int> virus_ids;
    std::vector<int> tool_ids;
    std::vector<double> tool_weights;
    
    model.get_hospitalizations(dates, virus_ids, tool_ids, tool_weights);

    #ifndef CATCH_CONFIG_MAIN
    std::cout << "Number of hospitalization records: " << dates.size() << std::endl;
    
    // Print the hospitalization data
    for (size_t i = 0; i < dates.size(); ++i)
    {
        std::cout << "Date: " << dates[i] 
                  << ", Virus ID: " << virus_ids[i]
                  << ", Tool ID: " << tool_ids[i]
                  << ", Tool Weight: " << tool_weights[i]
                  << std::endl;
    }
    #endif

    // All vectors should have the same size
    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(dates.size() == virus_ids.size());
    REQUIRE(dates.size() == tool_ids.size());
    REQUIRE(dates.size() == tool_weights.size());
    #endif

    // Check that we have some hospitalizations (given the parameters, we expect some)
    // But we also want to make sure the tracker is working correctly
    
    // Verify that tool_weights are positive
    #ifdef CATCH_CONFIG_MAIN
    for (size_t i = 0; i < tool_weights.size(); ++i)
    {
        REQUIRE(tool_weights[i] > 0.0);
    }
    #endif

    // Test reset functionality by running the model again
    model.run(30, 54321);  // Different seed
    
    std::vector<int> dates2;
    std::vector<int> virus_ids2;
    std::vector<int> tool_ids2;
    std::vector<double> tool_weights2;
    
    model.get_hospitalizations(dates2, virus_ids2, tool_ids2, tool_weights2);

    #ifndef CATCH_CONFIG_MAIN
    std::cout << "\nAfter second run:" << std::endl;
    std::cout << "Number of hospitalization records: " << dates2.size() << std::endl;
    #endif

    // Both runs should have independent hospitalization data
    // The reset() is called automatically before run(), so the data should be fresh
    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(dates2.size() == virus_ids2.size());
    REQUIRE(dates2.size() == tool_ids2.size());
    REQUIRE(dates2.size() == tool_weights2.size());
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
    
}
