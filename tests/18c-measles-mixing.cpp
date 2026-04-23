#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Quarantine works (size comparison)",
    "[ModelMeaslesMixing-On-size]"
) {

    // Relevant outbreak sizes
    auto outbreak_sizes = {100.0, 500.0, 1'000.0};

    // Queuing doesn't matter and get results that are meaningful
    int n_seeds = 2;

    // Simple contact matrix (single group, all mixing)
    std::vector<double> contact_matrix(9u, 1.5);

    measles::ModelMeaslesMixing<> model_0(
        9'000,        // Number of agents
        n_seeds / 9'000.0, // Initial prevalence
        0.2,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        contact_matrix, // Contact matrix
        0.2,         // Hospitalization rate
        7.0,         // Hospitalization duration
        2.0,         // Days undetected
        21,          // Quarantine period
        1.0,          // Quarantine willingness
        1.0,          // Isolation willingness
        4,           // Isolation period
        0.0,         // Proportion vaccinated
        1.0,         // Contact tracing success rate
        2u           // Contact tracing days window
    );

    // Adding a single entity (population group)
    model_0.add_entity(Entity<>("Population", dist_factory<>(0, 3'000)));
    model_0.add_entity(Entity<>("Population", dist_factory<>(3'000, 6'000)));
    model_0.add_entity(Entity<>("Population", dist_factory<>(6'000, 9'000)));

    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    size_t nsims = 100; // Reduced for faster testing
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * n_seeds, -1.0);
    std::vector<std::vector<int>> final_distribution(nsims);

    auto saver = tests_create_saver(
        transitions, R0s, n_seeds, &final_distribution
    );

    model_0.
        run_multiple(60, nsims, 1231, saver, true, true, 4).
        print(false);

    // Looking at the final outbreak size
    std::vector< size_t > not_infected_states = {
        measles::ModelMeaslesMixing<>::SUSCEPTIBLE,
        measles::ModelMeaslesMixing<>::QUARANTINED_SUSCEPTIBLE
    };

    auto stats_with_quarantine = test_compute_prob_outbreak_gt_k(
        final_distribution,
        not_infected_states,
        outbreak_sizes,
        nsims, false
    );

    // Re-running the model without quarantine --------------------------------
    model_0.set_param("Quarantine period", -1.0);
    auto saver_no_quarantine = tests_create_saver(
        transitions, R0s, n_seeds, &final_distribution
    );
    model_0.run_multiple(60, nsims, 1231, saver_no_quarantine, true, true, 4);

    // Looking at the final outbreak size without quarantine
    auto stats_without_quarantine = test_compute_prob_outbreak_gt_k(
        final_distribution,
        not_infected_states,
        outbreak_sizes,
        nsims, false
    );

    // Re-running setting a longer window for contact tracing -----------------
    model_0.set_param("Contact tracing days window", 7u);
    model_0.set_param("Quarantine period", 21.0);
    auto saver_long_contact_tracing = tests_create_saver(
        transitions, R0s, n_seeds, &final_distribution
    );
    model_0.
        run_multiple(
            60, nsims, 1231, saver_long_contact_tracing, true, true, 4
        ).
        print(false);

    // Looking at the final outbreak size with longer contact tracing
    auto stats_long_contact_tracing = test_compute_prob_outbreak_gt_k(
        final_distribution,
        not_infected_states,
        outbreak_sizes,
        nsims, false
    );

    auto sum_vec = [](const std::vector<double> & v) -> double {
        return std::accumulate(v.begin(), v.end(), 0.0);
    };
    auto p_with_quarantine = sum_vec(stats_with_quarantine);
    auto p_without_quarantine = sum_vec(stats_without_quarantine);
    auto p_long_contact_tracing = sum_vec(stats_long_contact_tracing);

    REQUIRE(p_without_quarantine > p_with_quarantine);
    REQUIRE(p_with_quarantine > p_long_contact_tracing);

    // Printing information
    std::cout << "========================================================" <<
        std::endl;
    std::cout << "Quarantine works (size comparison) test results" <<
        std::endl;
    std::cout << "Outbreak size without quarantine                  : "
        << stats_without_quarantine[0] << " ["
        << stats_without_quarantine[1] << ", "
        << stats_without_quarantine[2] << "]\n";
    std::cout << "Outbreak size with quarantine (4 days)            : "
        << stats_with_quarantine[0] << " ["
        << stats_with_quarantine[1] << ", "
        << stats_with_quarantine[2] << "]\n";
    std::cout << "Outbreak size with longer contact tracing (7 days): "
        << stats_long_contact_tracing[0] << " ["
        << stats_long_contact_tracing[1] << ", "
        << stats_long_contact_tracing[2] << "]\n";
    std::cout << "========================================================" <<
        std::endl;

    #undef mat
}
