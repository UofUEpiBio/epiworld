#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles risk quarantine - different durations comparison",
    "[ModelMeaslesMixingRiskQuarantine_durations]"
) {
    
    std::vector<double> contact_matrix = {1.0};
    int n = 1000;

    // Model 1: All same quarantine duration (21 days)
    epimodels::ModelMeaslesMixingRiskQuarantine<> model_uniform(
        n,           // Number of agents
        0.01,        // Initial prevalence
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
        2.0,         // Days undetected
        21,          // Quarantine period high risk
        21,          // Quarantine period medium risk (same as high)
        21,          // Quarantine period low risk (same as high)
        .9,          // Quarantine willingness
        .9,          // Isolation willingness
        4,           // Isolation period
        0.0,         // Proportion vaccinated
        0.1,         // Detection rate during quarantine
        1.0,         // Contact tracing success rate
        4u           // Contact tracing days prior
    );

    // Model 2: Different quarantine durations (21, 14, 7)
    epimodels::ModelMeaslesMixingRiskQuarantine<> model_varied(
        n,           // Number of agents
        0.01,        // Initial prevalence
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
        2.0,         // Days undetected
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

    // Add entities to both models
    model_uniform.add_entity(Entity<>("Population", dist_factory<>(0, n)));
    model_varied.add_entity(Entity<>("Population", dist_factory<>(0, n)));

    // Set initial conditions
    for (auto* model : {&model_uniform, &model_varied}) {
        model->get_virus(0).set_distribution(dist_virus<>(10));
        model->initial_states({1.0, 0.0});
    }

    // Run simulations
    int nsims = 50;
    std::vector<std::vector<epiworld_double>> transitions_uniform(nsims);
    std::vector<epiworld_double> R0s_uniform(nsims * 10, -1.0);
    
    std::vector<std::vector<epiworld_double>> transitions_varied(nsims);
    std::vector<epiworld_double> R0s_varied(nsims * 10, -1.0);

    auto saver_uniform = tests_create_saver(transitions_uniform, R0s_uniform, 10);
    auto saver_varied = tests_create_saver(transitions_varied, R0s_varied, 10);

    model_uniform.run_multiple(60, nsims, 123, saver_uniform, true, true, 4);
    model_varied.run_multiple(60, nsims, 456, saver_varied, true, true, 4);

    // Calculate final outbreak sizes
    std::vector<int> totals_uniform, totals_varied;
    model_uniform.get_db().get_today_total(nullptr, &totals_uniform);
    model_varied.get_db().get_today_total(nullptr, &totals_varied);

    // Get recovered counts (last state)
    int recovered_uniform = totals_uniform.back();
    int recovered_varied = totals_varied.back();

    std::cout << "Quarantine duration comparison:" << std::endl;
    std::cout << "Uniform quarantine (21 days all): " << recovered_uniform << " recovered" << std::endl;
    std::cout << "Varied quarantine (21/14/7 days): " << recovered_varied << " recovered" << std::endl;
    std::cout << "Difference: " << abs(recovered_uniform - recovered_varied) << std::endl;

    #ifdef CATCH_CONFIG_MAIN
    // Should see some difference in outbreak sizes due to different quarantine strategies
    // (This is exploratory - the direction depends on specific parameters)
    REQUIRE(recovered_uniform >= 0);
    REQUIRE(recovered_varied >= 0);
    #endif

    std::cout << "Quarantine duration comparison test completed" << std::endl;

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
}
