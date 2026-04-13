#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SBM balance violation", "[sbm-balance]") {

    // When the balance condition M(g,h)*n_g = M(h,g)*n_h is violated,
    // the implementation still generates a valid network — it just uses
    // M(g,h)/n_h for the block pair (g,h) with g <= h. With the
    // Batagelj-Brandes algorithm, the expected edge count is exactly
    // N * p (no dedup bias).
    //
    // This test uses an unbalanced matrix and verifies:
    // 1. The network is generated without error
    // 2. Observed edge counts per block pair match E = n_g * n_h * p

    size_t n_0 = 100u;
    size_t n_1 = 300u;

    // Unbalanced: M(0,1)*n_0 = 8*100 = 800 ≠ M(1,0)*n_1 = 2*300 = 600
    std::vector< size_t > block_sizes = {n_0, n_1};
    std::vector< double > mixing_matrix = {
        0.0, 8.0,   // group 0: M(0,1) = 8
        2.0, 0.0    // group 1: M(1,0) = 2 (not used since 1 > 0)
    };

    // The implementation uses block pair (0, 1) with g=0, h=1:
    //   p = M(0,1)/n_1 = 8/300
    //   N = n_0 * n_1 = 30000
    //   E[edges] = N * p = 30000 * 8/300 = 800
    double p_01 = 8.0 / static_cast<double>(n_1);
    double N_01 = static_cast<double>(n_0) * static_cast<double>(n_1);
    double exp_edges = N_01 * p_01;

    size_t n_reps = 200u;
    double edge_sum = 0.0;
    double deg0_sum = 0.0;
    double deg1_sum = 0.0;

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

        edge_sum += static_cast<double>(source.size());

        // Compute average degree per group
        size_t n = n_0 + n_1;
        std::vector<int> degree(n, 0);
        for (size_t e = 0u; e < source.size(); ++e)
        {
            degree[source[e]]++;
            degree[target[e]]++;
        }

        double d0 = 0.0, d1 = 0.0;
        for (size_t i = 0u; i < n_0; ++i)
            d0 += degree[i];
        for (size_t i = n_0; i < n; ++i)
            d1 += degree[i];

        deg0_sum += d0 / static_cast<double>(n_0);
        deg1_sum += d1 / static_cast<double>(n_1);
    }

    double avg_edges = edge_sum / static_cast<double>(n_reps);
    double avg_deg0 = deg0_sum / static_cast<double>(n_reps);
    double avg_deg1 = deg1_sum / static_cast<double>(n_reps);

    // Expected degrees from the specific probability used:
    // Group 0: exp_edges/n_0 = 800/100 = 8
    // Group 1: exp_edges/n_1 = 800/300 ≈ 2.667
    double exp_deg0 = exp_edges / static_cast<double>(n_0);
    double exp_deg1 = exp_edges / static_cast<double>(n_1);

    std::cout << "SBM balance violation test:" << std::endl;
    std::cout << "  Unbalanced: M(0,1)*n_0 = "
              << 8.0 * n_0 << " != M(1,0)*n_1 = " << 2.0 * n_1
              << std::endl;
    std::cout << "  Expected edges: " << exp_edges << std::endl;
    std::cout << "  Observed avg edges:    " << avg_edges << std::endl;
    std::cout << "  Group 0: exp_deg = " << exp_deg0
              << ", observed = " << avg_deg0 << std::endl;
    std::cout << "  Group 1: exp_deg = " << exp_deg1
              << ", observed = " << avg_deg1 << std::endl;

    // Edge count should match expected edges
    REQUIRE(std::abs(avg_edges - exp_edges) < 10.0);

    // Group degrees should match the specific probability used
    REQUIRE(std::abs(avg_deg0 - exp_deg0) < 0.3);
    REQUIRE(std::abs(avg_deg1 - exp_deg1) < 0.15);

    // Key insight: with the unbalanced matrix, row sums do NOT
    // match expected degrees:
    //   Row sum for group 0 = 0 + 8 = 8 → observed ≈ 8 (matches!)
    //   Row sum for group 1 = 2 + 0 = 2, but observed ≈ 2.667
    // The actual expected degree for group 1 is exp_edges/300 ≈ 2.667,
    // which differs from M(1,0) = 2 because the implementation uses
    // M(0,1)/n_1 for the (0,1) pair.
    std::cout << "  Row sum group 0 = 8 vs observed ≈ " << avg_deg0
              << std::endl;
    std::cout << "  Row sum group 1 = 2 vs observed ≈ " << avg_deg1
              << " (differs because balance violated)" << std::endl;

}
