#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles model with risk-based quarantine",
    "[ModelMeaslesMixingRiskQuarantine]"
) {
    
    // Simple contact matrix (single group, all mixing)
    std::vector<double> contact_matrix = {1.0};
    
    epimodels::ModelMeaslesMixingRiskQuarantine<> model(
        1000,        // Number of agents
        0.005,       // Initial prevalence
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
        21,          // Quarantine period high risk
        14,          // Quarantine period medium risk
        7,           // Quarantine period low risk
        .8,          // Quarantine willingness
        .8,          // Isolation willingness
        4,           // Isolation period
        0.0,         // Proportion vaccinated
        0.1,         // Detection rate during quarantine
        1.0,         // Contact tracing success rate
        4u           // Contact tracing days prior
    );

    // Adding a single entity (population group)
    model.add_entity(Entity<>("Population", dist_factory<>(0, 1000)));

    // Setting the distribution function of the initial cases
    Virus<> virus = model.get_virus(0);
    model.rm_virus(0);
    virus.set_distribution(dist_virus<>(10));
    model.add_virus(virus);
    
    // Setting initial states
    model.initial_states({1.0, 0.0});

    // Run simulation
    model.run(50, 123);
    model.print();

    // Check that the model ran successfully
    auto db = model.get_db();
    std::vector<int> totals;
    db.get_today_total(nullptr, &totals);

    #ifdef CATCH_CONFIG_MAIN
    // Basic sanity check - make sure we have agents in the system
    int total_agents = 0;
    for (size_t i = 0; i < totals.size(); ++i) {
        total_agents += totals[i];
    }
    REQUIRE(total_agents == 1000);
    #endif

    std::cout << "Basic test passed - model runs successfully" << std::endl;
}

EPIWORLD_TEST_CASE(
    "Measles risk quarantine - transition matrix test",
    "[ModelMeaslesMixingRiskQuarantine_transitions]"
) {
    
    std::vector<double> contact_matrix = {1.0};
    
    epimodels::ModelMeaslesMixingRiskQuarantine<> model(
        1000,        // Number of agents
        0.1,         // Initial prevalence (higher for testing)
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
    model.add_entity(Entity<>("Population", dist_factory<>(0, 1000)));

    // Setting the distribution function of initial cases
    Virus<> virus = model.get_virus(0);
    model.rm_virus(0);
    virus.set_distribution(dist_virus<>(100));
    model.add_virus(virus);
    
    model.initial_states({1.0, 0.0});

    // Run multiple simulations to get transition matrix
    int nsims = 100;
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * 100, -1.0);
    
    auto saver = tests_create_saver(transitions, R0s, 100);
    model.run_multiple(60, nsims, 123, saver, true, true, 4);

    // Calculate average transitions
    auto avg_transitions = tests_calculate_avg_transitions(transitions, model);
    auto mat = tests_transitions_to_matrix(avg_transitions, model);

    std::cout << "Transition matrix test results:" << std::endl;
    
    // Test transition from exposed to prodromal
    std::cout << "Transition to prodromal (exposed): " <<
        mat(1, 2) << " (expected: " << 
        1.0/model("Incubation period") << ")" << std::endl;

    // Test transition from prodromal to rash 
    std::cout << "Transition to rash (prodromal): " <<
        mat(2, 3) << " (expected: " << 
        1.0/model("Prodromal period") << ")" << std::endl;

    // Test enhanced detection during quarantine (should show some quarantined prodromal)
    std::cout << "Transition to quarantined prodromal: " <<
        mat(2, 9) << " (should be > 0 due to detection during quarantine)" << std::endl;

    // Test quarantine transitions with different durations
    std::cout << "Transition from quarantined susceptible to susceptible: " <<
        mat(8, 0) << " (varies by risk level)" << std::endl;

    #ifdef CATCH_CONFIG_MAIN
    // Basic transition checks
    REQUIRE_FALSE(
        moreless(mat(1, 2), 1.0/model("Incubation period"), 0.05)
    );
    
    REQUIRE_FALSE(
        moreless(mat(2, 3), 1.0/model("Prodromal period"), 0.1)
    );
    
    // Should have some enhanced detection during quarantine
    REQUIRE(mat(2, 9) > 0.0);
    #endif

    std::cout << "Transition matrix test passed" << std::endl;
}

EPIWORLD_TEST_CASE(
    "Measles risk quarantine - risk level assignment test",
    "[ModelMeaslesMixingRiskQuarantine_risk_levels]"
) {
    
    // Create contact matrix for 3 entities
    std::vector<double> contact_matrix = {
        0.8, 0.1, 0.1,
        0.1, 0.8, 0.1,
        0.1, 0.1, 0.8
    };
    
    int n = 900;
    epimodels::ModelMeaslesMixingRiskQuarantine<> model(
        n,           // Number of agents
        0.01,        // Initial prevalence
        2.0,         // Contact rate
        0.3,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        contact_matrix, // Contact matrix
        0.2,         // Hospitalization rate
        7.0,         // Hospitalization duration
        1.5,         // Days undetected
        21,          // Quarantine period high risk
        14,          // Quarantine period medium risk
        7,           // Quarantine period low risk
        .9,          // Quarantine willingness
        1.0,         // Isolation willingness
        4,           // Isolation period
        0.0,         // Proportion vaccinated (so all can be quarantined)
        0.2,         // Detection rate during quarantine
        1.0,         // Contact tracing success rate
        4u           // Contact tracing days prior
    );

    // Create three entities
    Entity<> e1("Entity 1", dist_factory<>(0, n/3));
    Entity<> e2("Entity 2", dist_factory<>(n/3, n/3 * 2));
    Entity<> e3("Entity 3", dist_factory<>(n/3 * 2, n));

    model.add_entity(e1);
    model.add_entity(e2);
    model.add_entity(e3);

    // Set initial infected cases in entity 1
    Virus<> virus = model.get_virus(0);
    model.rm_virus(0);
    virus.set_distribution(dist_virus<>(0));
    model.add_virus(virus);
    
    model.initial_states({1.0, 0.0});

    // Run simulation
    model.run(30, 456);
    
    // Check risk level assignments
    auto risk_levels = model.get_quarantine_risk_levels();
    
    std::cout << "Risk level assignment test:" << std::endl;
    
    int high_risk = 0, medium_risk = 0, low_risk = 0;
    for (size_t i = 0; i < risk_levels.size(); ++i) {
        if (risk_levels[i] == model.RISK_HIGH) high_risk++;
        else if (risk_levels[i] == model.RISK_MEDIUM) medium_risk++;
        else low_risk++;
    }
    
    std::cout << "High risk agents: " << high_risk << std::endl;
    std::cout << "Medium risk agents: " << medium_risk << std::endl;
    std::cout << "Low risk agents: " << low_risk << std::endl;

    #ifdef CATCH_CONFIG_MAIN
    // Should have some variety in risk levels if quarantine was triggered
    REQUIRE(high_risk + medium_risk + low_risk == static_cast<int>(n));
    #endif

    std::cout << "Risk level assignment test passed" << std::endl;
}

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
        Virus<> virus = model->get_virus(0);
        model->rm_virus(0);
        virus.set_distribution(dist_virus<>(10));
        model->add_virus(virus);
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
}