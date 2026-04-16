#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test heterogeneous contact rates in ModelMeaslesMixing
 *
 * This test verifies that agents with higher contact rates have higher
 * reproductive numbers. Setup:
 * - ~1000 agents in 5 equal groups (homogeneous mixing)
 * - Two initially infected agents (agents 0 and 1)
 * - Agent 0 is in group 0 with contact rate 10
 * - Agent 1 is in group 1 with contact rate 5
 * - Groups 2, 3, and 4 also have contact rate 5
 * - Both agents start in the same group conceptually (both infected at t=0)
 * - Contact rates are uniformly distributed across groups
 *
 * We verify that the agent with the highest contact rate (agent 0)
 * also has the highest reproductive number on average.
 */
EPIWORLD_TEST_CASE(
    "MeaslesMixing heterogeneous contact rates",
    "[Measles-mixing-hetero]"
) {

    // 5 groups, homogeneous mixing
    // Group 0 has contact rate 10, groups 1-4 have contact rate 5
    // Uniformly distributed across groups means each entry in a row = row_sum / 5
    size_t ngroups = 5;
    double high_rate = 10.0;
    double low_rate = 5.0;

    // Contact matrix in column-major order: MM(i,j,n) = j*n + i
    // Row i, col j = contact_matrix[j * ngroups + i]
    std::vector<double> contact_matrix(ngroups * ngroups, 0.0);
    for (size_t i = 0; i < ngroups; ++i)
    {
        double rate = (i == 0) ? high_rate : low_rate;
        for (size_t j = 0; j < ngroups; ++j)
            contact_matrix[j * ngroups + i] = rate / static_cast<double>(ngroups);
    }

    int n_agents = 801;
    size_t nsims = 200;

    epimodels::ModelMeaslesMixing<> model(
        n_agents,
        2.0 / n_agents,    // prevalence: 2 infected agents
        0.01,              // transmission_rate (low to keep outbreaks small)
        0.0,               // vax_efficacy (no vaccine effect)
        0.0,               // vax_reduction_recovery_rate
        7.0,               // incubation_period
        4.0,               // prodromal_period
        5.0,               // rash_period
        contact_matrix,    // contact_matrix
        0.1,               // hospitalization_rate
        7.0,               // hospitalization_period
        3.0,               // days_undetected
        -1,                // quarantine_period (disabled)
        0.0,               // quarantine_willingness
        0.0,               // isolation_willingness
        -1,                // isolation_period (disabled)
        0.0                // prop_vaccinated
    );

    // Create groups
    int from = 0;
    for (size_t g = 0; g < ngroups; ++g)
    {
        int size = (g == 0) ? 1 : 200;
        int to = from + size;
        model.add_entity(
            Entity<>("Group " + std::to_string(g), dist_factory<>(from, to))
        );
        from = to;
    }

    // Override the virus distribution to place exactly 2 infected agents:
    // Agent 0 in group 0 (high contact rate) and agent 1 in group 1 (low rate)
    model.get_virus(0).set_distribution(
        [](Virus<> & v, Model<> * m) -> void {
            m->get_agents()[0].set_virus(*m, v);             // group 0
            m->get_agents()[1].set_virus(*m, v);             // group 1
        }
    );

    // Track reproductive numbers for agent 0 (group 0) and agent 1 (group 1)
    std::vector<double> R0_high(nsims, 0.0);
    std::vector<double> R0_low(nsims, 0.0);

    auto saver = [&R0_high, &R0_low](size_t n, Model<> * m) -> void {

        auto rts = m->get_db().get_reproductive_number();

        // Agent 0 (high contact rate group) infected at day 0
        auto it0 = rts.find({0, 0, 0});
        if (it0 != rts.end())
            R0_high[n] = static_cast<double>(it0->second);

        // Agent 1 (low contact rate group) infected at day 0
        auto it1 = rts.find({0, 1, 0});
        if (it1 != rts.end())
            R0_low[n] = static_cast<double>(it1->second);

    };

    model.run_multiple(60, nsims, 1231, saver, true, true, 2);

    // Compute average R0 for each agent
    double avg_R0_high = std::accumulate(
        R0_high.begin(), R0_high.end(), 0.0
    ) / static_cast<double>(nsims);

    double avg_R0_low = std::accumulate(
        R0_low.begin(), R0_low.end(), 0.0
    ) / static_cast<double>(nsims);

    // Print results
    std::cout << "========================================================" << std::endl;
    std::cout << "Heterogeneous contact rates test (ModelMeaslesMixing)" << std::endl;
    std::cout << "Agent 0 (group 0, contact rate " << high_rate
              << "): avg R0 = " << avg_R0_high << std::endl;
    std::cout << "Agent " << 1 << " (group 1, contact rate " << low_rate
              << "): avg R0 = " << avg_R0_low << std::endl;
    std::cout << "========================================================" << std::endl;

    // The agent with the higher contact rate should have a higher R0
    REQUIRE(avg_R0_high > avg_R0_low);

}
