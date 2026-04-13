#include "tests.hpp"

using namespace epiworld;

/**
 * Compute the expected number of unique edges for a block pair with
 * N possible slots and Bernoulli probability p, under uniform
 * sampling with replacement.
 *
 * Formula: E[unique] = N * [1 - (1 - p/N)^N]
 */
static double expected_unique(long long N, double p)
{
    if (N == 0 || p == 0.0)
        return 0.0;

    double Nd = static_cast<double>(N);
    return Nd * (1.0 - std::pow(1.0 - p / Nd, Nd));
}

/**
 * Compute the bias-corrected expected degree for each group, accounting
 * for duplicate edge collisions from uniform sampling with replacement.
 *
 * Within-block:  N = n_g*(n_g-1)/2, p = M(g,g)/n_g
 * Between-block: N = n_g*n_h,       p = M(lo,hi)/n_hi  (lo <= hi)
 */
static std::vector<double> expected_degrees_corrected(
    const std::vector<size_t> & block_sizes,
    const std::vector<double> & mixing_matrix  // row-major
)
{
    size_t K = block_sizes.size();
    std::vector<double> exp_deg(K, 0.0);

    for (size_t g = 0u; g < K; ++g)
    {
        double n_g = static_cast<double>(block_sizes[g]);

        // Within-block contribution: 2 * unique_within / n_g
        double m_gg = mixing_matrix[g * K + g];
        double p_gg = m_gg / n_g;
        long long N_w = static_cast<long long>(block_sizes[g]) *
            static_cast<long long>(block_sizes[g] - 1u) / 2;
        exp_deg[g] += 2.0 * expected_unique(N_w, p_gg) / n_g;

        // Between-block contributions
        for (size_t h = 0u; h < K; ++h)
        {
            if (h == g) continue;

            size_t lo = std::min(g, h);
            size_t hi = std::max(g, h);
            double m_lohi = mixing_matrix[lo * K + hi];

            long long N_b = static_cast<long long>(block_sizes[lo]) *
                static_cast<long long>(block_sizes[hi]);
            double p_bw = m_lohi / static_cast<double>(block_sizes[hi]);

            double unique_bw = expected_unique(N_b, p_bw);
            exp_deg[g] += unique_bw / n_g;
        }
    }

    return exp_deg;
}


EPIWORLD_TEST_CASE("SBM expected degree", "[sbm]") {

    // Setup: 3 blocks with known sizes and a mixing matrix.
    // The mixing matrix is NOT row-stochastic; row sums give the
    // expected degree for agents in that group (before bias correction).
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

    // Compute bias-corrected expected degrees using uniform dedup formula
    auto corrected = expected_degrees_corrected(block_sizes, mixing_matrix);

    // We need multiple runs to get stable average degrees.
    size_t n_reps = 400u;

    // Accumulate degree sums per group across repetitions.
    std::vector< double > degree_sum(n_blocks, 0.0);

    epimodels::ModelSIR<> model(
        "a virus", 0.01, 0.5, 0.3
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
        model.agents_sbm(block_sizes, mixing_matrix, true);

        // Get the edgelist and compute degrees
        std::vector< int > source, target;
        model.write_edgelist(source, target);

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
    std::cout << "SBM expected degree test (bias-corrected, uniform):"
              << std::endl;
    for (size_t g = 0u; g < n_blocks; ++g)
    {
        double row_sum = 0.0;
        for (size_t h = 0u; h < n_blocks; ++h)
            row_sum += mixing_matrix[g * n_blocks + h];

        std::cout << "  Group " << g
                  << ": naive = " << row_sum
                  << ", corrected = " << corrected[g]
                  << ", observed = " << degree_sum[g]
                  << std::endl;
    }

    // Check that observed average degree is close to bias-corrected
    // expected values. With 400 reps and the exact formula, tolerance
    // of 0.15 should be adequate.
    for (size_t g = 0u; g < n_blocks; ++g)
    {
        REQUIRE(std::abs(degree_sum[g] - corrected[g]) < 0.15);
    }

}
