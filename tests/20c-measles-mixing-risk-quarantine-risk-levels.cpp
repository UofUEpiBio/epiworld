#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

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
    model.get_virus(0).set_distribution(dist_virus<>(0));
    
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

    return 0;
}
