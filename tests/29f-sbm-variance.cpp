#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SBM edge count variance", "[sbm-variance]") {

    // Verify that the variance of total edge counts is consistent with
    // the sampling-with-replacement mechanism. For each block pair, the
    // number of binomial draws is m ~ Binom(N, p), then deduplication
    // reduces the count. We check the empirical standard deviation of
    // total edges across reps is within a plausible range.
    //
    // For a block pair with N possible slots and edge probability p:
    //   - Binomial draws: E[m] = N*p, Var[m] = N*p*(1-p)
    //   - Unique edges ≈ N * (1 - exp(-p)) for uniform placement
    //   - The variance of unique edges is bounded by Var[m] (dedup
    //     reduces variance), so std(unique) <= sqrt(N*p*(1-p)).
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

    // Theoretical upper bound on std: sqrt(N * p * (1-p))
    // With 2 symmetric between-block pairs (g=0,h=1 and g=1,h=0 but
    // only g<=h is sampled, so just one pair), N = n_g * n_h, p = m_gh/n_h.
    // Since write_edgelist counts edges from both block pairs (g,h) and
    // (h,g), but only one is sampled per invocation:
    double N_pair = static_cast<double>(n_g * n_h);
    double p_pair = m_gh / static_cast<double>(n_h);
    double binom_std = std::sqrt(N_pair * p_pair * (1.0 - p_pair));

    // Expected unique edges (uniform placement approximation)
    double exp_unique = N_pair *
        (1.0 - std::pow(1.0 - p_pair / N_pair, N_pair));

    std::cout << "SBM edge count variance test:" << std::endl;
    std::cout << "  Mean edges: " << mean
              << " (expected ≈ " << exp_unique << ")" << std::endl;
    std::cout << "  Empirical std: " << emp_std
              << " (binomial std = " << binom_std << ")" << std::endl;

    // The empirical mean should match expected unique edges
    // (tolerance: 3 * std / sqrt(n_reps))
    double tol_mean = 3.0 * emp_std / std::sqrt(static_cast<double>(n_reps));
    REQUIRE(std::abs(mean - exp_unique) < tol_mean);

    // The empirical std should be positive (edges are stochastic)
    REQUIRE(emp_std > 0.0);

    // The empirical std should be bounded by the binomial std
    // (deduplication can only reduce variance). Allow 20% margin
    // for sampling variability.
    REQUIRE(emp_std < binom_std * 1.2);

}
