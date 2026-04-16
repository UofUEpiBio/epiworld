#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SEIRNetworkQuarantine", "[SEIR-network-quarantine]") {

    // Two disconnected blocks: only block 0 should see infection
    std::vector< size_t > block_sizes = {500, 500};
    std::vector< double > mixing_matrix = {
        20.0,  0.0,
         0.0, 20.0
    };

    epimodels::ModelSEIRNetworkQuarantine<> model(
        "Flu",      // vname
        0.01,       // prevalence
        1.0,        // transmission_rate
        2.0,        // avg_incubation_days
        1.0/2.0,    // recovery_rate
        0.1,        // hospitalization_rate
        5.0,        // hospitalization_period
        2.0,        // days_undetected
        4,          // quarantine_period
        0.9,        // quarantine_willingness
        1.0,        // isolation_willingness
        10,         // isolation_period
        1.0,        // contact_tracing_success_rate
        4           // contact_tracing_days_prior
    );

    // Replace virus to infect only agent 0 (in block 0)
    Virus<> v1 = model.get_virus(0);
    model.rm_virus(0);
    v1.set_distribution(dist_virus<>(0));
    model.add_virus(v1);

    // Set up the SBM network (disconnected blocks)
    model.agents_sbm(block_sizes, mixing_matrix, true);

    model.verbose_off();
    model.run(50, 123);
    model.print();

    // Check that no agent in block 1 (agents 500-999) got infected or quarantined
    int n_wrong = 0;
    for (size_t i = 500; i < 1000; ++i)
    {
        const auto & a = model.get_agent(i);
        if (a.get_state() != epimodels::ModelSEIRNetworkQuarantine<>::SUSCEPTIBLE)
        {
            n_wrong++;
        }
    }

    // All agents in block 1 should remain susceptible
    REQUIRE(n_wrong == 0);

    // Check that some agents in block 0 did get infected/recovered
    int n_affected_block0 = 0;
    for (size_t i = 0; i < 500; ++i)
    {
        const auto & a = model.get_agent(i);
        if (a.get_state() != epimodels::ModelSEIRNetworkQuarantine<>::SUSCEPTIBLE)
        {
            n_affected_block0++;
        }
    }

    // With high transmission, some agents in block 0 should be affected
    REQUIRE(n_affected_block0 > 0);

    // Now run a model with full mixing between blocks
    // First recreate the model with connected blocks
    std::vector< double > full_mixing = {
        10.0, 10.0,
        10.0, 10.0
    };

    epimodels::ModelSEIRNetworkQuarantine<> model2(
        "Flu",      // vname
        0.01,       // prevalence
        1.0,        // transmission_rate
        2.0,        // avg_incubation_days
        1.0/2.0,    // recovery_rate
        0.1,        // hospitalization_rate
        5.0,        // hospitalization_period
        -1.0,       // days_undetected (no detection - run to completion)
        4,          // quarantine_period
        0.9,        // quarantine_willingness
        1.0,        // isolation_willingness
        10,         // isolation_period
        1.0,        // contact_tracing_success_rate
        4           // contact_tracing_days_prior
    );

    // Replace virus to infect only agent 0
    Virus<> v2 = model2.get_virus(0);
    model2.rm_virus(0);
    v2.set_distribution(dist_virus<>(0));
    model2.add_virus(v2);

    model2.agents_sbm(block_sizes, full_mixing, true);

    model2.verbose_off();
    model2.run(50, 456);
    model2.print();

    // With full mixing and high transmission, all (or nearly all) agents should
    // end up recovered (no quarantine/isolation since detection is off)
    std::vector< int > totals;
    model2.get_db().get_today_total(nullptr, &totals);

    // Hospitalized agents also recover eventually, so combine
    // hospitalized + recovered for the final check
    int total_final = totals[epimodels::ModelSEIRNetworkQuarantine<>::RECOVERED] +
                      totals[epimodels::ModelSEIRNetworkQuarantine<>::HOSPITALIZED];

    // With high transmission and connected graph, we expect most agents to be
    // recovered
    REQUIRE(total_final > 500);

}
