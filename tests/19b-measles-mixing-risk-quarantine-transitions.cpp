#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles risk quarantine - transition matrix validation",
    "[ModelMeaslesMixingRiskQuarantine_transitions]"
) {
    
    // Contact matrix for 3 groups with equal mixing
    size_t nsims = 400;
    size_t n     = 300;
    std::vector<double> contact_matrix(9u, 2.0/3.0);
    
    epimodels::ModelMeaslesMixingRiskQuarantine<> model(
        n,           // Number of agents
        0.1 ,        // Initial prevalence
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
    std::vector< double > outbreak_sizes(nsims, 0.0);
    std::vector< double > hospitalizations(nsims, 0.0);

    auto saver = tests_create_saver(transitions, R0s, 10, nullptr, &outbreak_sizes, &hospitalizations);
    model.run_multiple(60, nsims, 123, saver, true, true, 4);

    // Briefly printing the model
    model.print(false);

    // Calculate average transitions
    auto avg_transitions = tests_calculate_avg_transitions(transitions, model);

    tests_print_avg_transitions(avg_transitions, model);
    #define mat(i, j) avg_transitions[j * model.get_n_states() + i]

    // Average hospitalizations
    double obs_hosp_probability = 0.0;
    for (auto i = 0u; i < hospitalizations.size(); ++i)
    {
        if (hospitalizations[i] >= 0.0)
            obs_hosp_probability += hospitalizations[i]/outbreak_sizes[i];
        else
            throw std::range_error(
                "The number of hospitalizations is negative. This should not happen."
            );
    }
    obs_hosp_probability /= static_cast<epiworld_double>(nsims);

    std::cout << "\n=== TRANSITION MATRIX VALIDATION ===" << std::endl;
    
    // From exposed
    std::cout << "Transition from Exposed to Prodromal: " <<
        mat(1, 2) + mat(1, 8) << " (expected: " << 
        1.0/model("Incubation period") << ")" << std::endl;

    // From prodromal
    std::cout << "Transition from Prodromal to Rash: " <<
        mat(2, 3) + mat(2, 4) << " (expected: " << 
        1.0/model("Prodromal period") << ")" << std::endl;

    // From Rash
    std::cout << "Transition from Rash to Recovered: " <<
        mat(3, 11) + mat(3, 5) << " (expected: " << 
        (1.0 - model("Hospitalization rate") - 1.0/model("Rash period")) << ")" << std::endl;

    std::cout << "Transition from Rash to Hospitalized: " <<
        mat(3, 10) << " (expected: " <<
        model("Hospitalization rate") << ")" << std::endl;

    // Isolated
    std::cout << "Transition from Isolated to hospitalized: " <<
        mat(4, 10) << " (expected: " <<
        model("Hospitalization rate") << ")" << std::endl;
    
    // Quarantined Exposed
    std::cout << "Transition from Quarantined Exposed to Prodromal: " <<
        mat(6, 2) + mat(6, 8) << " (expected: " <<
        1.0/model("Incubation period") << ")" << std::endl;

    // Quarantined Prodromal
    std::cout << "Transition from Quarantined Prodromal to Rash: " <<
        mat(8, 3) + mat(8, 4) << " (expected: " <<
        1.0/model("Prodromal period") << ")" << std::endl;

    // From Hospitalized
    std::cout << "Transition from Hospitalized to Recovered: " <<
        mat(10, 11) << " (expected: " <<
        1.0/model("Hospitalization period") << ")" << std::endl;
        
    // Some transitions should be zero. Building a vector of the transitions
    // we expect not to be zero, so the rest should be zero.
    std::set< std::pair<size_t, size_t> > non_zero_transitions = {
        // From Susceptible
        {0, 0}, {0, 1}, {0, 6}, {0, 7},
        // From Latent
        {1, 1}, {1, 2}, {1, 6}, {1, 8},
        // From Prodromal
        {2, 2}, {2, 3}, {2, 4}, {2, 8},
        // From Rash
        {3, 3}, {3, 4}, {3, 5}, {3, 10}, {3, 11},
        // From Isolated
        {4, 4}, {4, 5}, {4, 10}, {4, 11}, {4, 3},
        // From Isolated Recovered
        {5, 5}, {5, 11},
        // From Quarantined Latent
        {6, 6}, {6, 1}, {6, 2}, {6, 8},
        // From Quarantined Susceptible
        {7, 7}, {7, 0},
        // From Quarantined Prodromal
        {8, 8}, {8, 2}, {8, 3}, {8, 4},
        // From Quarantined Recovered
        {9, 9}, {9, 11},
        // From Hospitalized
        {10, 10}, {10, 11},
        // From Recovered
        {11, 11}
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
        moreless(mat(1, 2) + mat(1, 8), 1.0/model("Incubation period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(2, 3) + mat(2, 4), 1.0/model("Prodromal period"), 0.1)
    );

    double p_recovered = 1.0/model("Rash period");
    REQUIRE_FALSE(
        moreless(mat(3, 11) + mat(3, 5), p_recovered, 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(3, 10), model("Hospitalization rate"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(4, 10), model("Hospitalization rate"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(6, 2) + mat(6, 8), 1.0/model("Incubation period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(8, 3) + mat(8, 4), 1.0/model("Prodromal period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(10, 11), 1.0/model("Hospitalization period"), 0.1)
    );

    // Hospitalization probability
    REQUIRE_FALSE(
        moreless(
            model("Hospitalization rate")/(
                model("Hospitalization rate") + p_recovered
            ),
            obs_hosp_probability,
            0.1
        )
    );

    #undef mat

    // Hospitalization probability
    std::cout << "Hospitalization probability: " <<
        model("Hospitalization rate")/(
            model("Hospitalization rate") + p_recovered
        ) << " (observed ~" << obs_hosp_probability << ")" << std::endl;

    std::cout << "\nTransition matrix validation completed" << std::endl;


}
