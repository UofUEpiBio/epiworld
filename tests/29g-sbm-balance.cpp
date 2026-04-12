#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SBM balance violation", "[sbm-balance]") {

    // When the balance condition M(g,h)*n_g = M(h,g)*n_h is violated,
    // the implementation still generates a valid network — it just uses
    // M(g,h)/n_h for the block pair (g,h) with g <= h. The expected
    // unique edge count per block pair can still be predicted using the
    // same deduplication formula.
    //
    // This test uses an unbalanced matrix and verifies:
    // 1. The network is generated without error
    // 2. Observed edge counts per block pair match the formula
    //    E[unique] = n_g * n_h * (1 - (1 - p/(n_g*n_h))^(n_g*n_h))
    //    for between-block pairs (with p = M(lo,hi)/n_hi)

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
    //   q = p/N = 8/(300*30000) = 8/9000000
    //   E[unique] = 30000 * (1 - (1-q)^30000)

    double p_01 = 8.0 / static_cast<double>(n_1);
    double N_01 = static_cast<double>(n_0) * static_cast<double>(n_1);
    double q_01 = p_01 / N_01;
    double exp_unique = N_01 * (1.0 - std::pow(1.0 - q_01, N_01));

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
    // Group 0: E[unique]/n_0 = exp_unique/100
    // Group 1: E[unique]/n_1 = exp_unique/300
    double exp_deg0 = exp_unique / static_cast<double>(n_0);
    double exp_deg1 = exp_unique / static_cast<double>(n_1);

    std::cout << "SBM balance violation test:" << std::endl;
    std::cout << "  Unbalanced: M(0,1)*n_0 = "
              << 8.0 * n_0 << " != M(1,0)*n_1 = " << 2.0 * n_1
              << std::endl;
    std::cout << "  Expected unique edges: " << exp_unique << std::endl;
    std::cout << "  Observed avg edges:    " << avg_edges << std::endl;
    std::cout << "  Group 0: exp_deg = " << exp_deg0
              << ", observed = " << avg_deg0 << std::endl;
    std::cout << "  Group 1: exp_deg = " << exp_deg1
              << ", observed = " << avg_deg1 << std::endl;

    // Edge count should match expected unique edges
    REQUIRE(std::abs(avg_edges - exp_unique) < 10.0);

    // Group degrees should match the specific probability used
    REQUIRE(std::abs(avg_deg0 - exp_deg0) < 0.3);
    REQUIRE(std::abs(avg_deg1 - exp_deg1) < 0.15);

    // Key insight: with the unbalanced matrix, row sums do NOT
    // match expected degrees:
    //   Row sum for group 0 = 0 + 8 = 8, but observed ≈ exp_deg0
    //   Row sum for group 1 = 2 + 0 = 2, but observed ≈ exp_deg1
    // The actual expected degree for group 1 is exp_unique/300, which
    // is much larger than M(1,0) = 2 because the implementation uses
    // M(0,1)/n_1 for the (0,1) pair.
    std::cout << "  Row sum group 0 = 8 vs observed ≈ " << avg_deg0
              << std::endl;
    std::cout << "  Row sum group 1 = 2 vs observed ≈ " << avg_deg1
              << " (differs because balance violated)" << std::endl;

}
