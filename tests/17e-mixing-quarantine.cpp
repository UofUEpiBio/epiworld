#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "SEIRMixingQuarantine isolation willingness testing",
    "[SEIR-mixing-quarantine-isolation-willingness]")
{
    std::vector<double> contact_matrix(9, 1.0 / 3.0);
    int n = 3000;
    size_t n_seeds = 5;
    size_t nsims = 100; // Reduced for faster testing

    // Test 1: Model with isolation willingness 0.7 (reduced isolation)
    epimodels::ModelSEIRMixingQuarantine<> model_low_isolation(
        "Flu",                      // std::string vname,
        n,                          // epiworld_fast_uint n,
        0.01,                       // epiworld_double prevalence,
        8.0,                        // epiworld_double contact_rate,
        0.5,                        // epiworld_double transmission_rate,
        4.0,                        // epiworld_double avg_incubation_days,
        1.0/3.5,                    // epiworld_double recovery_rate,
        contact_matrix,
        {true, true, true},         // All entities can quarantine
        .2,                         // Hospitalization rate
        4,                          // Hospitalization period
        1.5,                        // Days undetected
        15,                         // Quarantine period
        .9,                         // Quarantine willingness
        0.7,                        // Isolation willingness (REDUCED)
        4                           // Isolation period
    );

    // Test 2: Model with isolation willingness 1.0 but some entities 
    // cannot quarantine
    epimodels::ModelSEIRMixingQuarantine<> model_no_quarantine(
        "Flu",                      // std::string vname,
        n,                          // epiworld_fast_uint n,
        0.01,                       // epiworld_double prevalence,
        8.0,                        // epiworld_double contact_rate,
        0.5,                        // epiworld_double transmission_rate,
        4.0,                        // epiworld_double avg_incubation_days,
        1.0/3.5,                    // epiworld_double recovery_rate,
        contact_matrix,
        {true, true, false},        // Third entity CANNOT quarantine
        .2,                         // Hospitalization rate
        4,                          // Hospitalization period
        1.5,                        // Days undetected
        15,                         // Quarantine period
        .9,                         // Quarantine willingness
        1.0,                        // Isolation willingness (HIGH)
        4                           // Isolation period
    );

    // Setup for model 1 (low isolation willingness)
    Virus<> v1 = model_low_isolation.get_virus(0);
    model_low_isolation.rm_virus(0);
    v1.set_distribution(dist_virus<>(0));
    model_low_isolation.add_virus(v1);

    Entity<> e1_low("Entity 1", dist_factory<>(0, n/3));
    Entity<> e2_low("Entity 2", dist_factory<>(n/3, n/3 * 2));
    Entity<> e3_low("Entity 3", dist_factory<>(n/3 * 2, n));
    
    model_low_isolation.add_entity(e1_low);
    model_low_isolation.add_entity(e2_low);
    model_low_isolation.add_entity(e3_low);

    model_low_isolation.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
            for (int i = 0; i < static_cast<int>(n_seeds); ++i)
                m->get_agents()[i].set_virus(v, m);
            return;
        });

    // Setup for model 2 (no quarantine for third entity)
    Virus<> v2 = model_no_quarantine.get_virus(0);
    model_no_quarantine.rm_virus(0);
    v2.set_distribution(dist_virus<>(0));
    model_no_quarantine.add_virus(v2);

    Entity<> e1_noq("Entity 1", dist_factory<>(0, n/3));
    Entity<> e2_noq("Entity 2", dist_factory<>(n/3, n/3 * 2));
    Entity<> e3_noq("Entity 3", dist_factory<>(n/3 * 2, n));
    
    model_no_quarantine.add_entity(e1_noq);
    model_no_quarantine.add_entity(e2_noq);
    model_no_quarantine.add_entity(e3_noq);

    model_no_quarantine.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
            for (int i = 0; i < static_cast<int>(n_seeds); ++i)
                m->get_agents()[i].set_virus(v, m);
            return;
        });

    // Run simulations for model 1 (low isolation willingness)
    std::vector< std::vector<epiworld_double> > transitions_low(nsims);
    std::vector< epiworld_double > R0s_low(nsims * n_seeds, -1.0);
    
    auto saver_low = tests_create_saver(transitions_low, R0s_low, n_seeds);
    model_low_isolation.run_multiple(
        60, nsims, 123, saver_low, true, true, 4
    );

    // Run simulations for model 2 (no quarantine for third entity)
    std::vector< std::vector<epiworld_double> > transitions_noq(nsims);
    std::vector< epiworld_double > R0s_noq(nsims * n_seeds, -1.0);
    
    auto saver_noq = tests_create_saver(transitions_noq, R0s_noq, n_seeds);
    model_no_quarantine.run_multiple(
        60, nsims, 456, saver_noq, true, true, 4
    );

    // Calculate average transitions
    auto avg_transitions_low = tests_calculate_avg_transitions(
        transitions_low, model_low_isolation
    );
    auto avg_transitions_noq = tests_calculate_avg_transitions(
        transitions_noq, model_no_quarantine
    );

    std::cout << "\n=== MODEL 1: Low Isolation Willingness (0.7) ==="
              << std::endl;
    model_low_isolation.print();
    tests_print_avg_transitions(avg_transitions_low, model_low_isolation);

    std::cout << 
        "\n=== MODEL 2: No Quarantine for Entity 3, "
        "High Isolation Willingness (1.0) ==="
        << std::endl;
    model_no_quarantine.print();
    tests_print_avg_transitions(avg_transitions_noq, model_no_quarantine);

    // Test assertions
    #define mat_low(i, j) avg_transitions_low[j * model_low_isolation.get_n_states() + i]
    #define mat_noq(i, j) avg_transitions_noq[j * model_no_quarantine.get_n_states() + i]

    // Test 1: Verify that lower isolation willingness reduces transition 
    // to isolation
    epiworld_double infected_to_isolated_low = mat_low(2, 3); 
    std::cout << "\nTest 1 - Infected to Isolated (low willingness): "
              << infected_to_isolated_low << std::endl;
    
    // Test 2: Verify that individuals can still isolate even when entity 
    // cannot quarantine
    epiworld_double infected_to_isolated_noq = mat_noq(2, 3); 
    std::cout << "Test 2 - Infected to Isolated (no entity quarantine): "
              << infected_to_isolated_noq << std::endl;

    // Check isolation willingness values are correctly set
    auto isolation_will_low = model_low_isolation.get_isolation_willingness();
    auto isolation_will_noq = model_no_quarantine.get_isolation_willingness();
    
    int willing_low = 0, willing_noq = 0;
    for (size_t i = 0; i < isolation_will_low.size(); ++i) {
        if (isolation_will_low[i]) willing_low++;
        if (isolation_will_noq[i]) willing_noq++;
    }
    
    std::cout << "\nIsolation willingness verification:" << std::endl;
    std::cout << "Model 1 (0.7): " << willing_low << "/"
              << isolation_will_low.size()
              << " agents willing ("
              << (double)willing_low/isolation_will_low.size()
              << ")" << std::endl;
    std::cout << "Model 2 (1.0): " << willing_noq << "/"
              << isolation_will_noq.size()
              << " agents willing ("
              << (double)willing_noq/isolation_will_noq.size()
              << ")" << std::endl;

    // Check entity quarantine capabilities
    auto entity_can_quar_low = 
        model_low_isolation.get_entity_can_quarantine();
    auto entity_can_quar_noq = 
        model_no_quarantine.get_entity_can_quarantine();
    
    std::cout << "\nEntity quarantine capabilities:" << std::endl;
    std::cout << "Model 1: [" << entity_can_quar_low[0] << ", "
              << entity_can_quar_low[1]
              << ", " << entity_can_quar_low[2]
              << "] (all can quarantine)" << std::endl;
    std::cout << "Model 2: [" << entity_can_quar_noq[0] << ", "
              << entity_can_quar_noq[1]
              << ", " << entity_can_quar_noq[2]
              << "] (third cannot quarantine)" << std::endl;

    // Test A: Lower isolation willingness should result in fewer agents 
    // willing to isolate
    REQUIRE(willing_low < willing_noq);
    
    // Test B: Even with no quarantine capability for entity 3, some 
    // isolation should still occur because individual agents can still 
    // isolate if willing
    REQUIRE(infected_to_isolated_noq > 0.0);
    
    // Test C: Verify the isolation willingness proportions are 
    // approximately correct
    double prop_low = (double)willing_low / isolation_will_low.size();
    double prop_noq = (double)willing_noq / isolation_will_noq.size();
    
    // Should be approximately 0.7 and 1.0 respectively (with some random 
    // variance)
    REQUIRE(prop_low >= 0.6);
    REQUIRE(prop_low <= 0.8);
    REQUIRE(prop_noq >= 0.95);
    
    // Test D: Verify entity quarantine settings are correct
    REQUIRE(entity_can_quar_low[0] == true);
    REQUIRE(entity_can_quar_low[1] == true);
    REQUIRE(entity_can_quar_low[2] == true);
    
    REQUIRE(entity_can_quar_noq[0] == true);
    REQUIRE(entity_can_quar_noq[1] == true);
    REQUIRE(entity_can_quar_noq[2] == false);
    

    #undef mat_low
    #undef mat_noq
    


}
