#include "tests.hpp"

using namespace epiworld;

/**
 * Compute the exact expected degree for each group using the
 * mixing matrix and block sizes.
 *
 * With the Batagelj-Brandes algorithm, each edge is generated
 * independently with exactly the correct probability — no
 * duplicate collisions. The expected degree is:
 *
 * Within-block:  E[deg_within] = M(g,g) * (n_g - 1) / n_g
 * Between-block: E[deg_between from h] = M(lo,hi) * n_lo / n_g
 *   where lo = min(g,h), hi = max(g,h), and the edge probability
 *   is M(lo,hi) / n_hi. Each agent in g gets n_h such trials
 *   (or n_g for the other block), contributing unique_edges / n_g.
 */
static std::vector<double> expected_degrees(
    const std::vector<size_t> & block_sizes,
    const std::vector<double> & mixing_matrix  // row-major
)
{
    size_t K = block_sizes.size();
    std::vector<double> exp_deg(K, 0.0);

    for (size_t g = 0u; g < K; ++g)
    {
        double n_g = static_cast<double>(block_sizes[g]);

        // Within-block contribution:
        // n_g*(n_g-1)/2 pairs, each with prob p_gg = M(g,g)/n_g.
        // Expected edges = n_g*(n_g-1)/2 * p_gg
        // Each edge adds 2 to total degree, so contribution per agent:
        // 2 * [n_g*(n_g-1)/2 * p_gg] / n_g = (n_g-1) * p_gg
        //   = M(g,g) * (n_g-1) / n_g
        double m_gg = mixing_matrix[g * K + g];
        exp_deg[g] += m_gg * (n_g - 1.0) / n_g;

        // Between-block contributions
        for (size_t h = 0u; h < K; ++h)
        {
            if (h == g) continue;

            size_t lo = std::min(g, h);
            size_t hi = std::max(g, h);
            double m_lohi = mixing_matrix[lo * K + hi];

            // Degree contribution per agent in g = expected_edges / n_g
            double expected_edges = static_cast<double>(block_sizes[lo]) *
                m_lohi;
            exp_deg[g] += expected_edges / n_g;
        }
    }

    return exp_deg;
}

// SBM generator
//
// One test case; each former test file is a block below, with the name it had.
EPIWORLD_TEST_CASE("SBM generator", "[sbm][sbm-within][sbm-between][sbm-colmajor][sbm-er][sbm-variance][sbm-balance][sbm-exchange][sbm-large]") {

    // ---- 29a-sbm.cpp: SBM expected degree
    {

    // Setup: 3 blocks with known sizes and a mixing matrix.
    // The mixing matrix is NOT row-stochastic; row sums give the
    // expected degree for agents in that group (approximately,
    // with the (n_g-1)/n_g correction for within-block edges).
    //
    // Block sizes: 200, 300, 500
    //
    // For an undirected network, the balance condition must hold:
    //   M(g,h) * n_g = M(h,g) * n_h
    //
    // Mixing matrix (row-major):
    //   group 0 -> [ 4.5, 3.0, 2.5 ]  row sum = 10
    //   group 1 -> [ 2.0, 5.5, 2.5 ]  row sum = 10
    //   group 2 -> [ 1.0, 1.5, 7.5 ]  row sum = 10

    std::vector< size_t > block_sizes = {200, 300, 500};
    std::vector< double > mixing_matrix = {
        4.5, 3.0, 2.5,
        2.0, 5.5, 2.5,
        1.0, 1.5, 7.5
    };

    size_t n_blocks = block_sizes.size();
    size_t n = 0u;
    for (auto bs : block_sizes)
        n += bs;

    // Compute exact expected degrees (Batagelj-Brandes: no bias)
    auto expected = expected_degrees(block_sizes, mixing_matrix);

    // We need multiple runs to get stable average degrees.
    size_t n_reps = 400u;

    // Accumulate degree sums per group across repetitions.
    std::vector< double > degree_sum(n_blocks, 0.0);

    epimodels::ModelSIR model(
        "a virus", std::accumulate(block_sizes.begin(), block_sizes.end(), size_t(0)), 0.01, 0.5, 0.3
    );
    model.verbose_off();

    // Generating different seeds for each replication to get
    // different random graphs.
    std::vector< size_t > seeds(n_reps);
    for (size_t i = 0u; i < n_reps; ++i)
        seeds[i] = model.runif() * std::numeric_limits< size_t >::max();

    std::cout << "Running " << n_reps << " SBM simulations..." << std::endl;
    Progress pb(n_reps, 80);
    for (size_t rep = 0u; rep < n_reps; ++rep)
    {
        model.seed(seeds[rep]);
        // The model only provides the random numbers
        auto net = layer::network("network", rgraph_sbm(block_sizes, mixing_matrix, true, model));

        // Get the edgelist and compute degrees
        std::vector< int > source, target;
        net->get_edgelist(source, target);

        std::vector< int > degree(n, 0);
        for (size_t e = 0u; e < source.size(); ++e)
        {
            degree[source[e]]++;
            degree[target[e]]++;
        }

        // Sum degrees per group
        size_t offset = 0u;
        for (size_t g = 0u; g < n_blocks; ++g)
        {
            double group_degree_sum = 0.0;
            for (size_t i = 0u; i < block_sizes[g]; ++i)
                group_degree_sum += degree[offset + i];

            degree_sum[g] += group_degree_sum / static_cast<double>(
                block_sizes[g]
            );
            offset += block_sizes[g];
        }

        pb.next();
    }


    // Compute average degree per group across replications
    for (size_t g = 0u; g < n_blocks; ++g)
        degree_sum[g] /= static_cast<double>(n_reps);

    // Print results
    std::cout << "SBM expected degree test (Batagelj-Brandes, exact):"
              << std::endl;
    for (size_t g = 0u; g < n_blocks; ++g)
    {
        double row_sum = 0.0;
        for (size_t h = 0u; h < n_blocks; ++h)
            row_sum += mixing_matrix[g * n_blocks + h];

        std::cout << "  Group " << g
                  << ": row_sum = " << row_sum
                  << ", expected = " << expected[g]
                  << ", observed = " << degree_sum[g]
                  << std::endl;
    }

    // Check that observed average degree is close to exact expected
    // values. With BB (no bias) and 400 reps, tolerance of 0.15
    // should be adequate.
    for (size_t g = 0u; g < n_blocks; ++g)
    {
        REQUIRE(std::abs(degree_sum[g] - expected[g]) < 0.15);
    }
    }

    // ---- 29b-sbm-within.cpp: SBM within-group only
    {

    // Diagonal mixing matrix: connections only within groups.
    // Block sizes: 50, 50, 50
    // Mixing matrix:
    //   [ 5.0, 0.0, 0.0 ]
    //   [ 0.0, 5.0, 0.0 ]
    //   [ 0.0, 0.0, 5.0 ]

    std::vector< size_t > block_sizes = {50, 50, 50};
    std::vector< double > mixing_matrix = {
        5.0, 0.0, 0.0,
        0.0, 5.0, 0.0,
        0.0, 0.0, 5.0
    };

    size_t n_blocks = block_sizes.size();

    epimodels::ModelSIR model(
        "a virus", std::accumulate(block_sizes.begin(), block_sizes.end(), size_t(0)), 0.01, 0.5, 0.3
    );
    model.verbose_off();
    model.seed(12345);
    model.add_layer(layer::network(
        "network", rgraph_sbm(block_sizes, mixing_matrix, true, model)
    ));

    // Get the edgelist
    std::vector< int > source, target;
    tests_net(model).get_edgelist(source, target);

    // Compute block membership for each agent
    size_t n = 0u;
    for (auto bs : block_sizes)
        n += bs;

    std::vector< size_t > block_of(n, 0u);
    size_t offset = 0u;
    for (size_t g = 0u; g < n_blocks; ++g)
    {
        for (size_t i = 0u; i < block_sizes[g]; ++i)
            block_of[offset + i] = g;
        offset += block_sizes[g];
    }

    // Verify: all edges must be within the same group
    size_t n_between = 0u;
    for (size_t e = 0u; e < source.size(); ++e)
    {
        if (block_of[source[e]] != block_of[target[e]])
            n_between++;
    }

    std::cout << "SBM within-group test:" << std::endl;
    std::cout << "  Total edges: " << source.size() << std::endl;
    std::cout << "  Between-group edges: " << n_between << std::endl;

    // There should be edges (network is not empty)
    REQUIRE(source.size() > 0u);

    // No between-group edges
    REQUIRE(n_between == 0u);
    }

    // ---- 29c-sbm-between.cpp: SBM between-group only
    {

    // Off-diagonal mixing matrix: connections only between groups.
    // Block sizes: 50, 50, 50
    // Mixing matrix:
    //   [ 0.0, 3.0, 3.0 ]
    //   [ 3.0, 0.0, 3.0 ]
    //   [ 3.0, 3.0, 0.0 ]

    std::vector< size_t > block_sizes = {50, 50, 50};
    std::vector< double > mixing_matrix = {
        0.0, 3.0, 3.0,
        3.0, 0.0, 3.0,
        3.0, 3.0, 0.0
    };

    size_t n_blocks = block_sizes.size();

    epimodels::ModelSIR model(
        "a virus", std::accumulate(block_sizes.begin(), block_sizes.end(), size_t(0)), 0.01, 0.5, 0.3
    );
    model.verbose_off();
    model.seed(54321);
    model.add_layer(layer::network(
        "network", rgraph_sbm(block_sizes, mixing_matrix, true, model)
    ));

    // Get the edgelist
    std::vector< int > source, target;
    tests_net(model).get_edgelist(source, target);

    // Compute block membership for each agent
    size_t n = 0u;
    for (auto bs : block_sizes)
        n += bs;

    std::vector< size_t > block_of(n, 0u);
    size_t offset = 0u;
    for (size_t g = 0u; g < n_blocks; ++g)
    {
        for (size_t i = 0u; i < block_sizes[g]; ++i)
            block_of[offset + i] = g;
        offset += block_sizes[g];
    }

    // Verify: all edges must be between different groups
    size_t n_within = 0u;
    for (size_t e = 0u; e < source.size(); ++e)
    {
        if (block_of[source[e]] == block_of[target[e]])
            n_within++;
    }

    std::cout << "SBM between-group test:" << std::endl;
    std::cout << "  Total edges: " << source.size() << std::endl;
    std::cout << "  Within-group edges: " << n_within << std::endl;

    // There should be edges (network is not empty)
    REQUIRE(source.size() > 0u);

    // No within-group edges
    REQUIRE(n_within == 0u);
    }

    // ---- 29d-sbm-colmajor.cpp: SBM column-major
    {

    // Verify that row_major=false (column-major) produces the same
    // network properties as row_major=true with a transposed matrix.
    //
    // Row-major mixing matrix:
    //   group 0 -> [ 4.5, 3.0, 2.5 ]  row sum = 10
    //   group 1 -> [ 2.0, 5.5, 2.5 ]  row sum = 10
    //   group 2 -> [ 1.0, 1.5, 7.5 ]  row sum = 10
    //
    // Column-major layout stores the SAME logical matrix but column
    // by column: M(g,h) = matrix[h * K + g] instead of matrix[g * K + h].
    // So we transpose the flat vector.

    std::vector< size_t > block_sizes = {200, 300, 500};

    // Row-major layout
    std::vector< double > row_major_matrix = {
        4.5, 3.0, 2.5,
        2.0, 5.5, 2.5,
        1.0, 1.5, 7.5
    };

    // Column-major layout (transpose of above)
    // col 0: M(0,0)=4.5, M(1,0)=2.0, M(2,0)=1.0
    // col 1: M(0,1)=3.0, M(1,1)=5.5, M(2,1)=1.5
    // col 2: M(0,2)=2.5, M(1,2)=2.5, M(2,2)=7.5
    std::vector< double > col_major_matrix = {
        4.5, 2.0, 1.0,
        3.0, 5.5, 1.5,
        2.5, 2.5, 7.5
    };

    size_t n_blocks = block_sizes.size();
    size_t n = 0u;
    for (auto bs : block_sizes)
        n += bs;

    // Run both with same seed and compare average degree per group
    size_t n_reps = 100u;

    auto compute_avg_degrees = [&](
        const std::vector< double > & matrix,
        bool row_major
    ) -> std::vector< double >
    {
        std::vector< double > degree_sum(n_blocks, 0.0);

        epimodels::ModelSIR model(
            "a virus", std::accumulate(block_sizes.begin(), block_sizes.end(), size_t(0)), 0.01, 0.5, 0.3
        );
        model.verbose_off();

        for (size_t rep = 0u; rep < n_reps; ++rep)
        {
            model.seed(rep + 1u);
            // The model only provides the random numbers
            auto net = layer::network("network", rgraph_sbm(block_sizes, matrix, row_major, model));

            std::vector< int > source, target;
            net->get_edgelist(source, target);

            std::vector< int > degree(n, 0);
            for (size_t e = 0u; e < source.size(); ++e)
            {
                degree[source[e]]++;
                degree[target[e]]++;
            }

            size_t offset = 0u;
            for (size_t g = 0u; g < n_blocks; ++g)
            {
                double group_degree_sum = 0.0;
                for (size_t i = 0u; i < block_sizes[g]; ++i)
                    group_degree_sum += degree[offset + i];

                degree_sum[g] += group_degree_sum /
                    static_cast<double>(block_sizes[g]);
                offset += block_sizes[g];
            }
        }

        for (size_t g = 0u; g < n_blocks; ++g)
            degree_sum[g] /= static_cast<double>(n_reps);

        return degree_sum;
    };

    auto deg_row = compute_avg_degrees(row_major_matrix, true);
    auto deg_col = compute_avg_degrees(col_major_matrix, false);

    std::cout << "SBM column-major test:" << std::endl;
    for (size_t g = 0u; g < n_blocks; ++g)
    {
        std::cout << "  Group " << g
                  << ": row_major = " << deg_row[g]
                  << ", col_major = " << deg_col[g]
                  << std::endl;
    }

    // Both should be close to expected degree = 10
    // and close to each other (within tolerance)
    for (size_t g = 0u; g < n_blocks; ++g)
    {
        REQUIRE(std::abs(deg_row[g] - 10.0) < 0.5);
        REQUIRE(std::abs(deg_col[g] - 10.0) < 0.5);
        REQUIRE(std::abs(deg_row[g] - deg_col[g]) < 0.5);
    }
    }

    // ---- 29e-sbm-erdos-renyi.cpp: SBM between-block edge count
    {

    // With Batagelj-Brandes, the expected number of between-block
    // edges is exactly n_g * n_h * p_gh (no dedup bias).
    //
    // This test uses two equal-sized blocks with only between-block
    // connections to validate the sampling mechanism.

    size_t n_g = 300u;
    size_t n_h = 300u;
    double m_gh = 6.0; // expected between-block degree
    double p_gh = m_gh / static_cast<double>(n_h);

    // Expected between-block edges (exact with BB)
    double expected_edges = static_cast<double>(n_g) *
        static_cast<double>(n_h) * p_gh;

    // Off-diagonal only mixing matrix
    std::vector< size_t > block_sizes = {n_g, n_h};
    std::vector< double > mixing_matrix = {
        0.0, m_gh,
        m_gh, 0.0
    };

    size_t n_reps = 200u;
    double edge_sum = 0.0;

    epimodels::ModelSIR model(
        "a virus", std::accumulate(block_sizes.begin(), block_sizes.end(), size_t(0)), 0.01, 0.5, 0.3
    );
    model.verbose_off();

    for (size_t rep = 0u; rep < n_reps; ++rep)
    {
        model.seed(rep + 1u);
        // The model only provides the random numbers
        auto net = layer::network("network", rgraph_sbm(block_sizes, mixing_matrix, true, model));

        std::vector< int > source, target;
        net->get_edgelist(source, target);
        edge_sum += static_cast<double>(source.size());
    }

    double avg_edges = edge_sum / static_cast<double>(n_reps);

    std::cout << "SBM between-block edge count test:" << std::endl;
    std::cout << "  n_g = " << n_g << ", n_h = " << n_h
              << ", M(g,h) = " << m_gh << std::endl;
    std::cout << "  p_gh = " << p_gh << std::endl;
    std::cout << "  Expected edges: " << expected_edges
              << std::endl;
    std::cout << "  Observed avg edges:    " << avg_edges << std::endl;

    // With BB (no bias), the analytical expected value should be
    // accurate. Tolerance of 15 edges.
    REQUIRE(std::abs(avg_edges - expected_edges) < 15.0);
    }

    // ---- 29f-sbm-variance.cpp: SBM edge count variance
    {

    // Verify that the variance of total edge counts is consistent with
    // the Batagelj-Brandes mechanism, which produces independent
    // Bernoulli trials with no duplicates.
    //
    // For a block pair with N possible slots and edge probability p:
    //   - Expected edges = N * p (exact, no dedup bias)
    //   - Var[edges] = N * p * (1 - p) (sum of independent Bernoullis)
    //   - std[edges] = sqrt(N * p * (1 - p))
    //
    // We use 2 blocks with only between-block connections for a clean
    // test: all edges come from a single block pair.

    size_t n_g = 200u;
    size_t n_h = 200u;
    double m_gh = 5.0;

    std::vector< size_t > block_sizes = {n_g, n_h};
    std::vector< double > mixing_matrix = {
        0.0, m_gh,
        m_gh, 0.0
    };

    size_t n_reps = 500u;
    std::vector< double > edge_counts(n_reps, 0.0);

    epimodels::ModelSIR model(
        "a virus", std::accumulate(block_sizes.begin(), block_sizes.end(), size_t(0)), 0.01, 0.5, 0.3
    );
    model.verbose_off();

    for (size_t rep = 0u; rep < n_reps; ++rep)
    {
        model.seed(rep + 1u);
        // The model only provides the random numbers
        auto net = layer::network("network", rgraph_sbm(block_sizes, mixing_matrix, true, model));

        std::vector< int > source, target;
        net->get_edgelist(source, target);
        edge_counts[rep] = static_cast<double>(source.size());
    }

    // Compute empirical mean and std
    double sum = 0.0;
    for (auto c : edge_counts)
        sum += c;
    double mean = sum / static_cast<double>(n_reps);

    double sq_sum = 0.0;
    for (auto c : edge_counts)
        sq_sum += (c - mean) * (c - mean);
    double emp_var = sq_sum / static_cast<double>(n_reps - 1u);
    double emp_std = std::sqrt(emp_var);

    // Theoretical std: sqrt(N * p * (1-p)), exact for BB
    double N_pair = static_cast<double>(n_g * n_h);
    double p_pair = m_gh / static_cast<double>(n_h);
    double binom_std = std::sqrt(N_pair * p_pair * (1.0 - p_pair));

    // Expected edges (exact with BB, no dedup bias)
    double exp_edges = N_pair * p_pair;

    std::cout << "SBM edge count variance test:" << std::endl;
    std::cout << "  Mean edges: " << mean
              << " (expected = " << exp_edges << ")" << std::endl;
    std::cout << "  Empirical std: " << emp_std
              << " (theoretical std = " << binom_std << ")" << std::endl;

    // The empirical mean should match expected edges
    // (tolerance: 3 * std / sqrt(n_reps))
    double tol_mean = 3.0 * emp_std / std::sqrt(static_cast<double>(n_reps));
    REQUIRE(std::abs(mean - exp_edges) < tol_mean);

    // The empirical std should be positive (edges are stochastic)
    REQUIRE(emp_std > 0.0);

    // The empirical std should match the theoretical std.
    // Allow 20% margin for sampling variability.
    REQUIRE(emp_std < binom_std * 1.2);
    }

    // ---- 29g-sbm-balance.cpp: SBM balance violation
    {

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

    epimodels::ModelSIR model(
        "a virus", std::accumulate(block_sizes.begin(), block_sizes.end(), size_t(0)), 0.01, 0.5, 0.3
    );
    model.verbose_off();

    for (size_t rep = 0u; rep < n_reps; ++rep)
    {
        model.seed(rep + 1u);
        // The model only provides the random numbers
        auto net = layer::network("network", rgraph_sbm(block_sizes, mixing_matrix, true, model));

        std::vector< int > source, target;
        net->get_edgelist(source, target);

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

    // ---- 29h-sbm-exchangeability.cpp: SBM within-block exchangeability
    {

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

    epimodels::ModelSIR model(
        "a virus", std::accumulate(block_sizes.begin(), block_sizes.end(), size_t(0)), 0.01, 0.5, 0.3
    );
    model.verbose_off();

    for (size_t rep = 0u; rep < n_reps; ++rep)
    {
        model.seed(rep + 1u);
        // The model only provides the random numbers
        auto net = layer::network("network", rgraph_sbm(block_sizes, mixing_matrix, true, model));

        std::vector< int > source, target;
        net->get_edgelist(source, target);

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

    // ---- 29i-sbm-large-blocks.cpp: SBM large block n_possible
    {

    // Regression test: when n_g * n_h > INT_MAX, ensure the
    // Batagelj-Brandes algorithm handles large pair spaces correctly
    // via long long arithmetic.

    size_t n_g = 47000u;
    size_t n_h = 47000u;
    // n_g * n_h = 2,209,000,000 > INT_MAX = 2,147,483,647

    // Very sparse: expected degree ≈ 0.5 from each block pair
    double m_gh = 0.5;

    std::vector< size_t > block_sizes = {n_g, n_h};
    std::vector< double > mixing_matrix = {
        0.0, m_gh,
        m_gh, 0.0
    };

    epimodels::ModelSIR model(
        "a virus", std::accumulate(block_sizes.begin(), block_sizes.end(), size_t(0)), 0.001, 0.3, 0.5
    );
    model.verbose_off();
    model.seed(42);

    // This should not throw or produce 0 edges
    model.add_layer(layer::network(
        "network", rgraph_sbm(block_sizes, mixing_matrix, true, model)
    ));

    std::vector< int > source, target;
    tests_net(model).get_edgelist(source, target);

    // Expected: p = 0.5 / 47000 ≈ 1.064e-5
    // N = 47000 * 47000 = 2.209e9
    // E[draws] = N * p = 2.209e9 * 1.064e-5 ≈ 23,500
    // E[unique] ≈ E[draws] (since p/N is tiny, almost no collisions)
    double p_gh = m_gh / static_cast<double>(n_h);
    double N_pair = static_cast<double>(n_g) * static_cast<double>(n_h);
    double expected_edges = N_pair * p_gh;

    std::cout << "SBM large block test:" << std::endl;
    std::cout << "  n_g = " << n_g << ", n_h = " << n_h << std::endl;
    std::cout << "  n_g * n_h = " << (static_cast<long long>(n_g) *
        static_cast<long long>(n_h)) << " (> INT_MAX)" << std::endl;
    std::cout << "  Expected edge draws: " << expected_edges << std::endl;
    std::cout << "  Observed edges: " << source.size() << std::endl;

    // The network should have a non-trivial number of edges
    // (the old overflow bug would produce 0 edges)
    REQUIRE(source.size() > 0u);

    // Should be in the right ballpark (within 30% of expected)
    double obs = static_cast<double>(source.size());
    REQUIRE(obs > expected_edges * 0.7);
    REQUIRE(obs < expected_edges * 1.3);
    }

}
