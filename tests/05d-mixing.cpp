#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test heterogeneous contact rates in ModelSEIRMixing
 *
 * This test verifies that agents with higher contact rates have higher
 * reproductive numbers. Setup:
 * - ~1000 agents in 4 equal groups (homogeneous mixing)
 * - Two initially infected agents: agent 0 in group 0 and agent group_size
 *   in group 1
 * - Group 0 has contact rate 10, groups 1-3 have contact rate 5
 * - Contact rates are uniformly distributed across groups
 *
 * We verify that the agent with the highest contact rate (agent 0 in group 0)
 * also has the highest reproductive number on average.
 */
EPIWORLD_TEST_CASE(
    "SEIRMixing heterogeneous contact rates",
    "[SEIR-mixing-hetero]"
) {

    // 4 groups, homogeneous mixing
    // Group 0 has contact rate 10, groups 1-3 have contact rate 5
    // Uniformly distributed across groups means each entry in a row = row_sum / 4
    size_t ngroups = 4;
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

    int n_agents = 1000;
    size_t nsims = 200;
    int group_size = n_agents / static_cast<int>(ngroups);

    epimodels::ModelSEIRMixing<> model(
        "Flu",
        n_agents,
        2.0 / n_agents,    // prevalence: 2 infected agents
        0.05,              // transmission_rate (low to keep outbreaks small)
        7.0,               // avg_incubation_days
        1.0 / 7.0,        // recovery_rate
        contact_matrix
    );

    // Create 4 equal groups
    for (size_t g = 0; g < ngroups; ++g)
    {
        int from = static_cast<int>(g) * group_size;
        int to = from + group_size;
        Entity<> e(
            "Group " + std::to_string(g),
            dist_factory<>(from, to)
        );
        model.add_entity(e);
    }

    // Override the virus distribution to place exactly 2 infected agents:
    // Agent 0 in group 0 (high contact rate) and agent group_size in group 1
    // (low contact rate)
    model.get_virus(0).set_distribution(
        [&group_size](Virus<> & v, Model<> * m) -> void {
            m->get_agents()[0].set_virus(*m, v);             // group 0
            m->get_agents()[group_size].set_virus(*m, v);    // group 1
        }
    );

    // Track reproductive numbers for agent 0 (group 0) and agent group_size
    // (group 1) across simulations
    std::vector<double> R0_high(nsims, 0.0);
    std::vector<double> R0_low(nsims, 0.0);

    auto saver = [&R0_high, &R0_low, &group_size](
        size_t n, Model<> * m
    ) -> void {

        auto rts = m->get_db().get_reproductive_number();

        // Agent 0 (high contact rate group) infected at day 0
        auto it0 = rts.find({0, 0, 0});
        if (it0 != rts.end())
            R0_high[n] = static_cast<double>(it0->second);

        // Agent group_size (low contact rate group) infected at day 0
        auto it1 = rts.find({0, group_size, 0});
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
    std::cout << "Heterogeneous contact rates test (ModelSEIRMixing)" << std::endl;
    std::cout << "Agent 0 (group 0, contact rate " << high_rate
              << "): avg R0 = " << avg_R0_high << std::endl;
    std::cout << "Agent " << group_size << " (group 1, contact rate " << low_rate
              << "): avg R0 = " << avg_R0_low << std::endl;
    std::cout << "========================================================" << std::endl;

    // The agent with the higher contact rate should have a higher R0
    REQUIRE(avg_R0_high > avg_R0_low);

}
