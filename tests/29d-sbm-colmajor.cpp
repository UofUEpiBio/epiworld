#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SBM column-major", "[sbm-colmajor]") {

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

        epimodels::ModelSIR<> model(
            "a virus", 0.01, 0.5, 0.3
        );
        model.verbose_off();

        for (size_t rep = 0u; rep < n_reps; ++rep)
        {
            model.seed(rep + 1u);
            model.agents_sbm(block_sizes, matrix, row_major);

            std::vector< int > source, target;
            model.write_edgelist(source, target);

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
