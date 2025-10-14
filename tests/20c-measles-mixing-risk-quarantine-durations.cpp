#define EPI_DEBUG
#ifndef CATCH_CONFIG_MAIN
#define N_THREADS 9
#define N_SIMS 1000
#else
#define N_THREADS 4
#define N_SIMS 400
#endif

#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Builds a ModelMeaslesMixingRiskQuarantine model for testing
 * @param n Number of agents in the population
 * @param durations Quarantine durations for high, medium, and low risk groups
 * @return Configured ModelMeaslesMixingRiskQuarantine instance
 */
inline auto test_model_builder_20d(
    size_t n,
    std::vector< double > contact_matrix,
    std::vector< int > durations = {21, 14, 7}
)
{

    double R0       = 10.0;
    double c_rate   = 20.0;
    double p_infect = R0 / (c_rate) * (1.0/4.0);

    epimodels::ModelMeaslesMixingRiskQuarantine<> model(
        n,              // Number of agents
        0.1,            // Initial prevalence
        c_rate,         // Contact rate
        p_infect,       // Transmission rate
        0.95,            // Vaccination efficacy
        7.0,            // Incubation period
        4.0,            // Prodromal period
        5.0,            // Rash period
        contact_matrix, // Contact matrix
        0.2,            // Hospitalization rate
        7.0,            // Hospitalization duration
        2.0,            // Days undetected
        durations[0],   // Quarantine period high risk
        durations[1],   // Quarantine period medium risk (same as high)
        durations[2],   // Quarantine period low risk (same as high)
        1.0,             // Quarantine willingness
        1.0,             // Isolation willingness
        4,              // Isolation period
        0.3,            // Proportion vaccinated
        0.0,            // Detection rate during quarantine
        1.0,            // Contact tracing success rate
        7u              // Contact tracing days prior
    );

    model.add_entity(Entity<>("Population", dist_factory<>(0, n/3)));
    model.add_entity(Entity<>("Population", dist_factory<>(n/3, 2*n/3)));
    model.add_entity(Entity<>("Population", dist_factory<>(2*n/3, n)));

    // Moving the virus to the first agent
    model.get_virus(0).set_distribution(
        distribute_virus_to_set<>({0u})
    );

    return model;

}

EPIWORLD_TEST_CASE(
    "Measles risk quarantine - different durations comparison",
    "[ModelMeaslesMixingRiskQuarantine_durations]"
) {

    // Contact matrix for 3 groups with equal mixing
    int nsims = N_SIMS;
    size_t n  = 600;
    double n_seeds = 1.0;
    
    // More contact within groups
    // According to Toth and others, 83% of contacts are within the same class
    // (for school-aged children). 17% are with other classes.
    double rate_self = 0.83;
    double rate_others = (1.0 - rate_self) / 2.0;
    std::vector<double> contact_matrix = {
        rate_self, rate_others, rate_others,
        rate_others, rate_self, rate_others,
        rate_others, rate_others, rate_self
    };

    auto model_uniform = test_model_builder_20d(n, contact_matrix, {21, 21, 21});
    auto model_high    = test_model_builder_20d(n, contact_matrix, { 7, 21, 21});
    auto model_mid     = test_model_builder_20d(n, contact_matrix, {21,  7, 21});
    auto model_low     = test_model_builder_20d(n, contact_matrix, {21, 21,  7});

    // Run simulations
    std::vector<std::vector<epiworld_double>>
        transitions_uniform(nsims),
        transitions_high(nsims),
        transitions_mid(nsims),
        transitions_low(nsims)
        ;

    std::vector<epiworld_double>
        R0s_uniform(nsims * n_seeds, -1.0),
        R0s_high(nsims * n_seeds, -1.0),
        R0s_mid(nsims * n_seeds, -1.0),
        R0s_low(nsims * n_seeds, -1.0)
        ;

    std::vector< std::vector< int > >
        final_distribution_uniform(nsims),
        final_distribution_high(nsims),
        final_distribution_mid(nsims),
        final_distribution_low(nsims)
        ;

    auto saver_uniform = tests_create_saver(
        transitions_uniform, R0s_uniform, static_cast<int>(n_seeds),
        &final_distribution_uniform
    );

    auto saver_high = tests_create_saver(
        transitions_high, R0s_high, static_cast<int>(n_seeds),
        &final_distribution_high
    );

    auto saver_mid = tests_create_saver(
        transitions_mid, R0s_mid, static_cast<int>(n_seeds),
        &final_distribution_mid
    );

    auto saver_low = tests_create_saver(
        transitions_low, R0s_low, static_cast<int>(n_seeds),
        &final_distribution_low
    );


    model_uniform.run_multiple(60, nsims, 123, saver_uniform, true, true, N_THREADS);
    model_high.run_multiple(60, nsims, 123, saver_high, true, true, N_THREADS);
    model_mid.run_multiple(60, nsims, 123, saver_mid, true, true, N_THREADS);
    model_low.run_multiple(60, nsims, 123, saver_low, true, true, N_THREADS);

    // Computing the mean R0 and 95% CI
    std::vector< size_t > non_infected = {
        epimodels::ModelMeaslesMixingRiskQuarantine<>::SUSCEPTIBLE,
        epimodels::ModelMeaslesMixingRiskQuarantine<>::QUARANTINED_SUSCEPTIBLE
    };

    std::cout << "Uniform durations: " <<
        model_uniform("Quarantine period high") << ", " <<
        model_uniform("Quarantine period medium") << ", and " <<
        model_uniform("Quarantine period low") << " days (high, low, medium)" <<
        std::endl;
    auto sizes_uniform = test_compute_final_sizes(
        final_distribution_uniform, non_infected, nsims, true
    );

    std::cout << "High risk " << model_high("Quarantine period high") <<
        " days:" << std::endl;
    auto sizes_high = test_compute_final_sizes(
        final_distribution_high, non_infected, nsims, true
    );

    std::cout << "Medium risk " << model_mid("Quarantine period medium") <<
        " days:" << std::endl;
    auto sizes_mid = test_compute_final_sizes(
        final_distribution_mid, non_infected, nsims, true
    );

    std::cout << "Low risk " << model_low("Quarantine period low") <<
        " days:" << std::endl;
    auto sizes_low = test_compute_final_sizes(
        final_distribution_low, non_infected, nsims, true
    );

    
    // model_uniform.print();
    // model_varied.print();

    #ifdef CATCH_CONFIG_MAIN
    // Should see some difference in outbreak sizes due to different quarantine strategies
    // (This is exploratory - the direction depends on specific parameters)
    REQUIRE(sizes_uniform[0] <= sizes_high[0]);
    REQUIRE(sizes_uniform[0] <= sizes_mid[0]);
    REQUIRE(sizes_uniform[0] <= sizes_low[0]);
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
}

#undef N_THREADS
#undef EPI_DEBUG