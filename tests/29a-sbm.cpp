#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SBM expected degree", "[sbm]") {

    // Setup: 3 blocks with known sizes and a mixing matrix.
    // The mixing matrix is NOT row-stochastic; row sums give the
    // expected degree for agents in that group.
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

    // Compute expected row sums
    std::vector< double > expected_degree(n_blocks, 0.0);
    for (size_t g = 0u; g < n_blocks; ++g)
        for (size_t h = 0u; h < n_blocks; ++h)
            expected_degree[g] += mixing_matrix[g * n_blocks + h];

    // Print results
    std::cout << "SBM expected degree test:" << std::endl;
    for (size_t g = 0u; g < n_blocks; ++g)
    {
        std::cout << "  Group " << g
                  << ": expected = " << expected_degree[g]
                  << ", observed = " << degree_sum[g]
                  << std::endl;
    }

    // Check that observed average degree is close to expected.
    // With 400 repetitions and these block sizes, tolerance of 0.5
    // should be adequate.
    for (size_t g = 0u; g < n_blocks; ++g)
    {
        REQUIRE(std::abs(degree_sum[g] - expected_degree[g]) < 0.5);
    }

}
