#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SBM within-group only", "[sbm-within]") {

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

    epimodels::ModelSIR<> model(
        "a virus", 0.01, 0.5, 0.3
    );
    model.verbose_off();
    model.seed(12345);
    model.agents_sbm(block_sizes, mixing_matrix, true);

    // Get the edgelist
    std::vector< int > source, target;
    model.write_edgelist(source, target);

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
