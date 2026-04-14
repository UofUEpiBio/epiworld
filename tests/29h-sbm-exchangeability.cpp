#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SBM within-block exchangeability", "[sbm-exchange]") {

    // Verify that all agents within a block have the same expected
    // degree — i.e., there is no systematic degree gradient by node
    // index. This is a regression test for the old non-uniform sampler
    // which used b = floor(U * a), heavily overweighting low-index pairs.

    size_t n_g = 30u;
    double m_gg = 6.0;

    // Single block with only within-group connections
    std::vector< size_t > block_sizes = {n_g};
    std::vector< double > mixing_matrix = {m_gg};

    size_t n_reps = 3000u;
    std::vector< double > degree_sum(n_g, 0.0);

    epimodels::ModelSIR<> model(
        "a virus", 0.01, 0.5, 0.3
    );
    model.verbose_off();

    for (size_t rep = 0u; rep < n_reps; ++rep)
    {
        model.seed(rep + 1u);
        model.agents_sbm(block_sizes, mixing_matrix, true);

        std::vector< int > source, target;
        model.write_edgelist(source, target);

        std::vector< int > degree(n_g, 0);
        for (size_t e = 0u; e < source.size(); ++e)
        {
            degree[source[e]]++;
            degree[target[e]]++;
        }

        for (size_t i = 0u; i < n_g; ++i)
            degree_sum[i] += static_cast<double>(degree[i]);
    }

    // Compute per-agent average degree
    std::vector< double > avg_deg(n_g, 0.0);
    for (size_t i = 0u; i < n_g; ++i)
        avg_deg[i] = degree_sum[i] / static_cast<double>(n_reps);

    // Compute overall mean degree
    double overall_mean = 0.0;
    for (size_t i = 0u; i < n_g; ++i)
        overall_mean += avg_deg[i];
    overall_mean /= static_cast<double>(n_g);

    std::cout << "SBM within-block exchangeability test:" << std::endl;
    std::cout << "  Block size = " << n_g << ", M(g,g) = " << m_gg
              << ", n_reps = " << n_reps << std::endl;
    std::cout << "  Overall mean degree: " << overall_mean << std::endl;

    // Find min and max average degrees across agents
    double min_deg = avg_deg[0], max_deg = avg_deg[0];
    size_t min_idx = 0u, max_idx = 0u;
    for (size_t i = 1u; i < n_g; ++i)
    {
        if (avg_deg[i] < min_deg) { min_deg = avg_deg[i]; min_idx = i; }
        if (avg_deg[i] > max_deg) { max_deg = avg_deg[i]; max_idx = i; }
    }

    std::cout << "  Min avg degree: agent " << min_idx
              << " = " << min_deg << std::endl;
    std::cout << "  Max avg degree: agent " << max_idx
              << " = " << max_deg << std::endl;
    std::cout << "  Spread (max - min): " << (max_deg - min_deg)
              << std::endl;

    // All agents should have similar average degree (within tolerance).
    // With n_g=30, m_gg=6, 3000 reps: the standard error of each
    // agent's mean degree is roughly sqrt(6)/sqrt(3000) ≈ 0.045.
    // The max spread across 30 agents should be well within 1.0.
    // The old non-uniform sampler would give spreads of 3+ here.
    REQUIRE((max_deg - min_deg) < 1.0);

    // Also check that no individual agent deviates from the overall
    // mean by more than 0.5.
    for (size_t i = 0u; i < n_g; ++i)
    {
        REQUIRE(std::abs(avg_deg[i] - overall_mean) < 0.5);
    }

}
