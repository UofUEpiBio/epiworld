#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SBM between-block edge count", "[sbm-er]") {

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
