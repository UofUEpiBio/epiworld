#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SBM edge count variance", "[sbm-variance]") {

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
