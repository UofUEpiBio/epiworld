#ifndef CATCH_CONFIG_MAIN
// #define EPI_DEBUG
#endif


#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles risk quarantine - transition matrix validation",
    "[ModelMeaslesMixingRiskQuarantine_transitions]"
) {
    
    // Contact matrix for 3 groups with equal mixing
    size_t nsims = 400;
    size_t n     = 300;
    std::vector<double> contact_matrix(9u, 1.0/3.0);
    
    epimodels::ModelMeaslesMixingRiskQuarantine<> model(
        n,           // Number of agents
        0.1 ,        // Initial prevalence
        2.0,         // Contact rate
        0.2,         // Transmission rate
        0.9,         // Vaccination efficacy
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
    model.add_entity(Entity<>("Population", dist_factory<>(0, n/3 - 1)));
    model.add_entity(Entity<>("Population", dist_factory<>(n/3, 2*n/3 - 1)));
    model.add_entity(Entity<>("Population", dist_factory<>(2*n/3, n - 1)));

    // Moving the virus to the first agent
    model.get_virus(0).set_distribution(
        distribute_virus_randomly<>(10.0, false)
    );

    // Run multiple simulations to get transition matrix
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * 10, -1.0);

    auto saver = tests_create_saver(transitions, R0s, 10);
    model.run_multiple(60, nsims, 123, saver, true, true, 4);

    // Briefly printing the model
    model.print(false);

    // Calculate average transitions
    auto avg_transitions = tests_calculate_avg_transitions(transitions, model);

    tests_print_avg_transitions(avg_transitions, model);
    #define mat(i, j) avg_transitions[j * model.get_n_states() + i]

    std::cout << "\n=== TRANSITION MATRIX VALIDATION ===" << std::endl;
    
    // From exposed
    std::cout << "Transition from Exposed to Prodromal: " <<
        mat(1, 2) + mat(1, 9) << " (expected: " << 
        1.0/model("Incubation period") << ")" << std::endl;

    // From prodromal
    std::cout << "Transition from Prodromal to Rash: " <<
        mat(2, 3) + mat(2, 4) << " (expected: " << 
        1.0/model("Prodromal period") << ")" << std::endl;

    // From Rash
    std::cout << "Transition from Rash to Recovered: " <<
        mat(3, 12) + mat(3, 5) << " (expected: " << 
        (1.0 - model("Hospitalization rate") - 1.0/model("Rash period")) << ")" << std::endl;

    std::cout << "Transition from Rash to Hospitalized: " <<
        mat(3, 6) + mat(3, 11) << " (expected: " <<
        model("Hospitalization rate") << ")" << std::endl;

    // Isolated
    std::cout << "Transition from Isolated to hospitalized: " <<
        mat(4, 6) + mat(4, 11) << " (expected: " <<
        model("Hospitalization rate") << ")" << std::endl;
    
    // Quarantined Exposed
    std::cout << "Transition from Quarantined Exposed to Prodromal: " <<
        mat(7, 2) + mat(7, 9) << " (expected: " <<
        1.0/model("Incubation period") << ")" << std::endl;

    // Quarantined Prodromal
    std::cout << "Transition from Quarantined Prodromal to Rash: " <<
        mat(9, 3) + mat(9, 4) << " (expected: " <<
        1.0/model("Prodromal period") << ")" << std::endl;

    // From Hospitalized
    std::cout << "Transition from Hospitalized to Recovered: " <<
        mat(11, 12) << " (expected: " <<
        1.0/model("Hospitalization period") << ")" << std::endl;
        
    // Some transitions should be zero. Building a vector of the transitions
    // we expect not to be zero, so the rest should be zero.
    std::set< std::pair<size_t, size_t> > non_zero_transitions = {
        // From exposed
        {0, 0}, {0, 1}, {0, 7}, {0, 8},
        // From Exposed
        {1, 1}, {1, 2}, {1, 7}, {1, 9},
        // From Prodromal
        {2, 2}, {2, 3}, {2, 4}, {2, 9},
        // From Rash
        {3, 3}, {3, 4}, {3, 5}, {3, 6}, {3, 11}, {3, 12},
        // From isolated
        {4, 4}, {4, 5}, {4, 6}, {4, 11}, {4, 12}, {4, 3},
        // From isolated recovered
        {5, 5}, {5, 12},
        // From detected hospitalized
        {6, 6}, {6, 11}, {6, 12},
        // From quarantined exposed
        {7, 7}, {7, 1}, {7, 2}, {7, 9}, 
        // From quarantined susceptible
        {8, 8}, {8, 0},
        // From quarantined prodromal
        {9, 9}, {9, 2}, {9, 3}, {9, 4},
        // From quarantined recovered
        {10, 10}, {10, 12},
        // From hospitalized
        {11, 11}, {11, 12},
        // From recovered
        {12, 12}
    };

    auto nstates = model.get_n_states();
    size_t n_nonzero_that_should_be_zero = 0u;
    std::set< std::pair<size_t, size_t> > found_nonzero_that_should_be_zero;
    for (size_t i = 0; i < nstates; ++i)
    {
        for (size_t j = 0; j < nstates; ++j)
        {
            // Finding transition
            if (non_zero_transitions.find({i, j}) == non_zero_transitions.end())
            {
                if (mat(i, j) > 1e-10)
                {
                    n_nonzero_that_should_be_zero++;
                    found_nonzero_that_should_be_zero.insert({i, j});
                }
            }
        }
    }

    // Validating zero transitions  
    REQUIRE(n_nonzero_that_should_be_zero == 0u);

    // Validate core disease progression  
    REQUIRE_FALSE(
        moreless(mat(1, 2) + mat(1, 9), 1.0/model("Incubation period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(2, 3) + mat(2, 4), 1.0/model("Prodromal period"), 0.1)
    );

    double p_recovered = 1.0 - (
        1.0/model("Rash period") + model("Hospitalization rate")
    );
    REQUIRE_FALSE(
        moreless(mat(3, 12) + mat(3, 5), p_recovered, 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(3, 6) + mat(3, 11), model("Hospitalization rate"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(4, 6) + mat(4, 11), model("Hospitalization rate"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(7, 2) + mat(7, 9), 1.0/model("Incubation period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(9, 3) + mat(9, 4), 1.0/model("Prodromal period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(11, 12), 1.0/model("Hospitalization period"), 0.1)
    );

    #undef mat

    std::cout << "\nTransition matrix validation completed" << std::endl;


}
