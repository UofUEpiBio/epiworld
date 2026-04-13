#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SBM large block n_possible", "[sbm-large]") {

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

    epimodels::ModelSIR<> model(
        "a virus", 0.001, 0.3, 0.5
    );
    model.verbose_off();
    model.seed(42);

    // This should not throw or produce 0 edges
    model.agents_sbm(block_sizes, mixing_matrix, true);

    std::vector< int > source, target;
    model.write_edgelist(source, target);

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
