#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#define N_THREADS 8
#else
#define N_THREADS 4
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

    double R0       = 4.0;
    double c_rate   = 10.0;
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
        .9,             // Quarantine willingness
        .9,             // Isolation willingness
        4,              // Isolation period
        0.5,            // Proportion vaccinated
        0.9,            // Detection rate during quarantine
        1.0,            // Contact tracing success rate
        7u              // Contact tracing days prior
    );

    model.add_entity(Entity<>("Population", dist_factory<>(0, n/3 - 1)));
    model.add_entity(Entity<>("Population", dist_factory<>(n/3, 2*n/3 - 1)));
    model.add_entity(Entity<>("Population", dist_factory<>(2*n/3, n - 1)));

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
    int nsims = 1000;
    size_t n  = 600;
    double n_seeds = 1.0;
    
    // More contact within groups
    // According to Toth and others, 83% of contacts are within the same class
    // (for school-aged children). 17% are with other classes.
    double seventeen = .17 / 2.0;
    std::vector<double> contact_matrix = {
        .83, seventeen, seventeen,
        seventeen, .83, seventeen,
        seventeen, seventeen, .83
    };

    auto model_uniform = test_model_builder_20d(n, contact_matrix, {21, 21, 21});
    auto model_varied  = test_model_builder_20d(n, contact_matrix, {21, 14, 21});

    // Run simulations
    std::vector<std::vector<epiworld_double>> transitions_uniform(nsims);
    std::vector<epiworld_double> R0s_uniform(nsims * n_seeds, -1.0);
    std::vector< std::vector< int > > final_distribution_uniform(nsims);
    
    std::vector<std::vector<epiworld_double>> transitions_varied(nsims);
    std::vector<epiworld_double> R0s_varied(nsims * n_seeds, -1.0);
    std::vector< std::vector< int > > final_distribution_varied(nsims);

    auto saver_uniform = tests_create_saver(
        transitions_uniform, R0s_uniform, static_cast<int>(n_seeds),
        &final_distribution_uniform
    );
    auto saver_varied = tests_create_saver(
        transitions_varied, R0s_varied, static_cast<int>(n_seeds),
        &final_distribution_varied
    );

    model_uniform.run_multiple(60, nsims, 123, saver_uniform, true, true, N_THREADS);
    model_varied.run_multiple(60, nsims, 123, saver_varied, true, true, N_THREADS);

    // Computing the mean R0 and 95% CI
    auto sizes_uniform = test_compute_final_sizes(
        final_distribution_uniform,
        // Not infected states
        {
            epimodels::ModelMeaslesMixingRiskQuarantine<>::SUSCEPTIBLE,
            epimodels::ModelMeaslesMixingRiskQuarantine<>::QUARANTINED_SUSCEPTIBLE
        }, 
        nsims,
        true
    );

    auto sizes_varied = test_compute_final_sizes(
        final_distribution_varied,
        {
            epimodels::ModelMeaslesMixingRiskQuarantine<>::SUSCEPTIBLE,
            epimodels::ModelMeaslesMixingRiskQuarantine<>::QUARANTINED_SUSCEPTIBLE
        },
        nsims,
        true
    );

    // model_uniform.print();
    // model_varied.print();

    #ifdef CATCH_CONFIG_MAIN
    // Should see some difference in outbreak sizes due to different quarantine strategies
    // (This is exploratory - the direction depends on specific parameters)
    REQUIRE(sizes_uniform[0] <= sizes_varied[0]);
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
}

#undef N_THREADS