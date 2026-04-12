#include "tests.hpp"

using namespace epiworld;

/**
 * Compute the expected number of unique within-block edges for a block
 * of size n_g with mixing matrix diagonal entry m_gg. Accounts for the
 * non-uniform pair selection (b = floor(U * a), with a=0 mapped to a=1)
 * and duplicate collisions from sampling with replacement.
 *
 * Formula: E[unique] = sum_k [1 - (1 - p * q_k)^N]
 * where N = n_g*(n_g-1)/2, p = m_gg/n_g, and q_k is the selection
 * probability for pair k.
 */
static double expected_unique_within(size_t n_g, double m_gg)
{
    if (n_g <= 1u || m_gg == 0.0)
        return 0.0;

    size_t N_w = n_g * (n_g - 1u) / 2u;
    double p_gg = m_gg / static_cast<double>(n_g);

    // Pair (1, 0): selection probability = 2/n_g
    double q10 = 2.0 / static_cast<double>(n_g);
    double result = 1.0 - std::pow(1.0 - p_gg * q10, static_cast<double>(N_w));

    // Pairs (a, b) for a >= 2, 0 <= b < a: selection probability = 1/(n_g*a)
    for (size_t a = 2u; a < n_g; ++a)
    {
        double q_a = 1.0 / (static_cast<double>(n_g) * static_cast<double>(a));
        result += static_cast<double>(a) *
            (1.0 - std::pow(1.0 - p_gg * q_a, static_cast<double>(N_w)));
    }

    return result;
}

/**
 * Compute the expected number of unique between-block edges for blocks
 * of sizes n_g and n_h with mixing matrix entry m_gh. Pair selection is
 * uniform over n_g * n_h slots.
 *
 * Formula: E[unique] = n_g * n_h * [1 - (1 - p/(n_g*n_h))^(n_g*n_h)]
 * where p = m_gh / n_h.
 */
static double expected_unique_between(
    size_t n_g, size_t n_h, double m_gh
)
{
    if (m_gh == 0.0)
        return 0.0;

    double N_b = static_cast<double>(n_g) * static_cast<double>(n_h);
    double p_gh = m_gh / static_cast<double>(n_h);
    double q = p_gh / N_b;

    return N_b * (1.0 - std::pow(1.0 - q, N_b));
}

/**
 * Compute the bias-corrected expected degree for each group, accounting
 * for duplicate edge collisions from sampling with replacement.
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
        exp_deg[g] += 2.0 * expected_unique_within(block_sizes[g], m_gg) / n_g;

        // Between-block contributions
        for (size_t h = 0u; h < K; ++h)
        {
            if (h == g) continue;

            size_t lo = std::min(g, h);
            size_t hi = std::max(g, h);
            double m_lohi = mixing_matrix[lo * K + hi];

            double unique_bw = expected_unique_between(
                block_sizes[lo], block_sizes[hi], m_lohi
            );

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

    // Compute bias-corrected expected degrees
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
    std::cout << "SBM expected degree test (bias-corrected):" << std::endl;
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
