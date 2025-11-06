#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles model with mixing (quarantine transitions)",
    "[ModelMeaslesMixingOn]"
) {
    
    // Queuing doesn't matter and get results that are meaningful
    int n_seeds = 5;
    
    // Simple contact matrix (single group, all mixing)
    std::vector<double> contact_matrix(9u, 1.0/3.0);
    
    epimodels::ModelMeaslesMixing<> model_0(
        900,        // Number of agents
        n_seeds / 900.0, // Initial prevalence
        2.0,         // Contact rate
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
        4u           // Contact tracing days prior
    );

    // Adding a single entity (population group)
    model_0.add_entity(Entity<>("Population", dist_factory<>(0, 300)));
    model_0.add_entity(Entity<>("Population", dist_factory<>(300, 600)));
    model_0.add_entity(Entity<>("Population", dist_factory<>(600, 900)));


    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(v, m);
        return;
    });

    size_t nsims = 100; // Reduced for faster testing
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * n_seeds, -1.0);
    std::vector<std::vector<int>> final_distribution(nsims);

    auto saver = tests_create_saver(
        transitions, R0s, n_seeds, &final_distribution
    );

    model_0.run_multiple(60, nsims, 1231, saver, true, true, 4);
    
    // Calculate average transitions
    auto avg_transitions = tests_calculate_avg_transitions(
        transitions, model_0
    );

    size_t n_states = model_0.get_n_states();

    tests_print_avg_transitions(avg_transitions, model_0);

    // Average R0
    double R0_observed = 0.0;
    for (auto & i: R0s)
    {
        if (i >= 0.0)
            R0_observed += i;
        else
            throw std::range_error(
                "The R0 value is negative. This should not happen."
            );
    }
    R0_observed /= static_cast<epiworld_double>(nsims * n_seeds);

    // Checking specific values in the transitions
    #define mat(i, j) avg_transitions[j*n_states + i]
    double p_recovered = 1.0 - (
        1.0/model_0("Rash period") + model_0("Hospitalization rate")
    );
    double R0_theo = model_0("Contact rate") * model_0("Transmission rate") *
        model_0("Prodromal period");

    // R0 - should be approximately equal to theoretical value
    REQUIRE_FALSE(moreless(R0_observed, R0_theo, 0.3));

    // Transition from exposed to prodromal (including quarantined)
    REQUIRE_FALSE(moreless(
        mat(1, 2) + mat(1, 9), 1.0/model_0("Incubation period"), 0.1)
    );

    // Transition from prodromal to rash
    REQUIRE_FALSE(
        moreless(mat(2, 3) + mat(2, 4), 1.0/model_0("Prodromal period"), 0.1)
    );

    // Transition from rash to isolated/recovered (detection)
    REQUIRE_FALSE(
        moreless(
            mat(3, 4) + mat(3, 5) + mat(3, 6),
            1.0/model_0("Days undetected"), 0.1
        )
    );

    // Transition from Quarantined Exposed to Quarantined Prodromal
    REQUIRE_FALSE(
        moreless(mat(7, 9), 1.0/model_0("Incubation period"), 0.1)
    );

    // Transition from Quarantined Prodromal to Isolated (rash cases are detected immediately)
    REQUIRE_FALSE(
        moreless(mat(9, 4), 1.0/model_0("Prodromal period"), 0.1)
    );

    // Transition to hospitalized from rash
    REQUIRE_FALSE(
        moreless(mat(3, 6) + mat(3, 11), model_0("Hospitalization rate"),
        0.1)
    );
    REQUIRE_FALSE(
        moreless(mat(4, 6) + mat(4, 11), model_0("Hospitalization rate"), 0.1)
    );

    // Transition to recovered from rash
    REQUIRE_FALSE(moreless(mat(3, 5) + mat(3, 12), p_recovered, 0.1));
    REQUIRE_FALSE(moreless(mat(4, 5) + mat(4, 12), p_recovered, 0.1));

    // Transition from hospitalized to recovered
    REQUIRE_FALSE(moreless(mat(11, 12), 1.0/model_0("Hospitalization period"), 0.1));

    // Transition from detected hospitalized to recovered
    REQUIRE_FALSE(moreless(mat(6, 12), 1.0/model_0("Hospitalization period"), 0.1));
    
    // Reproductive number
    std::cout <<
        "====================\n" <<
        "In the case of R0, we don't expect to have a big difference\n" <<
        "between quarantine and no quarantine. The biggest difference\n" <<
        "is in the final size of the outbreak.\n" <<
        "====================" <<
        std::endl;

    std::cout << "Reproductive number: "
              << R0_observed << " (expected ~" << R0_theo << ")" << std::endl;

    // Transition from exposed to prodromal (including quarantined)
    std::cout << "Transition to prodromal: "
              << mat(1, 2) + mat(1, 9) << " (expected ~" << 1.0/model_0("Incubation period") << ")" << std::endl;

    // Transition from prodromal to rash
    std::cout << "Transition to rash: "
              << mat(2, 3) + mat(2, 4) << " (expected ~" << 1.0/model_0("Prodromal period") << ")" << std::endl;

    // Transition from rash to isolated (detection)
    std::cout << "Transition to isolated: "
              << mat(3, 4) + mat(3, 5) + mat(3, 6) << " (expected ~" << 1.0/model_0("Days undetected") << ")" << std::endl;

    // Transition from Quarantined Exposed to Quarantined Prodromal
    std::cout << "Transition from Q. Exposed to Q. Prodromal: "
              << mat(7, 9) << " (expected ~" << 1.0/model_0("Incubation period") << ")" << std::endl;

    // Transition from Quarantined Prodromal to Isolated (rash cases are detected immediately)
    std::cout << "Transition from Q. Prodromal to isolated: "
              << mat(9, 4) << " (expected ~" << 1.0/model_0("Prodromal period") << ")" << std::endl;

    // Transition to hospitalized from rash
    std::cout << "Transition to hospitalized (rash): "
              << mat(3, 6) + mat(3, 11) << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;
    std::cout << "Transition to hospitalized (isolated): "
              << mat(4, 6) + mat(4, 11) << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;

    // Transition to recovered from rash    
    std::cout << "Transition to recovered (rash): "
              << mat(3, 5) + mat(3, 12) << " (expected ~" << p_recovered << ")" << std::endl;
    std::cout << "Transition to recovered (isolated): "
              << mat(4, 5) + mat(4, 12) << " (expected ~" << p_recovered << ")" << std::endl;

    // Transition from hospitalized to recovered
    std::cout << "Transition from hospitalized to recovered: "
              << mat(11, 12) << " (expected ~" << 1.0/model_0("Hospitalization period") << ")" << std::endl;

    // Transition from detected hospitalized to recovered
    std::cout << "Transition from detected hospitalized to recovered: "
              << mat(6, 12) << " (expected ~" << 1.0/model_0("Hospitalization period") << ")" << std::endl;

    // Transition from isolated recovered to recovered (deterministic based on rash onset)
    std::cout << "Transition from isolated recovered to recovered: "
              << mat(5, 12) << " (expected to be faster than " << 1.0/model_0("Isolation period") << ")" << std::endl;

    // Looking at the final outbreak size
    std::vector< size_t > not_infected_states = {0u, 8u};
    (void) test_compute_final_sizes(
        final_distribution,
        not_infected_states,
        nsims, true
    );

    #undef mat

    
}

