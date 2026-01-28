#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles model (no quarantine)",
    "[ModelMeaslesSchoolOff]"
) {
    
    // Queuing doesn't matter and get results that are meaningful
    int n_seeds = 1;
    epimodels::ModelMeaslesSchool<> model_0(
        500,    // Number of agents
        n_seeds, // Number of initial cases
        2.0,     // Contact rate
        0.2,     // Transmission rate
        0.9,     // Vaccination efficacy
        0.3,     // Vaccination reduction recovery rate
        7.0,     // Incubation period
        4.0,     // Prodromal period
        5.0,     // Rash period
        3.0,     // Days undetected
        0.04411765,     // Hospitalization rate
        7.0,     // Hospitalization duration
        0.0,     // Proportion vaccinated
        21u,     // Quarantine period
        .8,      // Quarantine willingness
        4u       // Isolation period
    );

    // Shutting off the quarantine feature
    // model_0("Quarantine period") = -1.0;
    // model_0("Isolation period") = -1.0;

    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(v, m);
        return;
    });

    size_t nsims = 200;
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * n_seeds, -1.0);
    std::vector< double > outbreak_sizes(nsims, 0.0);
        
    auto saver = tests_create_saver(transitions, R0s, n_seeds, nullptr, &outbreak_sizes);

    model_0.run_multiple(60, nsims, 1231, saver, true, true, 4);
    
    #ifndef CATCH_CONFIG_MAIN
    model_0.print(false);
    #endif

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

    // Checking especific values in the transitions
    #define mat(i, j) avg_transitions[j*n_states + i]
    double p_recovered = 1.0 - (
        1.0/model_0("Rash period") + model_0("Hospitalization rate")
    );
    double R0_theo = model_0("Contact rate") * model_0("Transmission rate") *
        model_0("Prodromal period");
    #ifdef CATCH_CONFIG_MAIN

    // R0
    REQUIRE_FALSE(moreless(R0_observed, R0_theo, 0.1));

    // Transition to prodromal
    REQUIRE_FALSE(moreless(mat(1, 2), 1.0/model_0("Incubation period"), 0.05));

    // Transition to rash
    REQUIRE_FALSE(
        moreless(mat(2, 3) + mat(2, 4), 1.0/model_0("Prodromal period"), 0.05)
    );

    // Transition to isolated
    REQUIRE_FALSE(
        moreless(
            mat(3, 4) + mat(3, 5) + mat(3, 6),
            1.0/model_0("Days undetected"), 0.05
        )
    );

    // Transition to hospitalized
    REQUIRE_FALSE(
        moreless(mat(3, 6) + mat(3, 11), model_0("Hospitalization rate"),
        0.05)
    );
    REQUIRE_FALSE(
        moreless(mat(4, 6) + mat(4, 11), model_0("Hospitalization rate"), 0.05)
    );

    // Transition to recovered
    REQUIRE_FALSE(moreless(mat(3, 5) + mat(3, 12), p_recovered, 0.05));
    REQUIRE_FALSE(moreless(mat(4, 5) + mat(4, 12), p_recovered, 0.05));

    // Transition from hospitalized to recovered
    REQUIRE_FALSE(moreless(mat(11, 12), 1.0/model_0("Hospitalization period"), 0.05));
    #endif
    // Reproductive number
    std::cout << "Reproductive number: "
              << R0_observed << " (expected ~" << R0_theo << ")" << std::endl;

    // Transition to prodromal
    std::cout << "Transition to prodromal: "
              << mat(1, 2) << " (expected ~" << 1.0/model_0("Incubation period") << ")" << std::endl;

    // Transition to rash
    std::cout << "Transition to rash: "
              << mat(2, 3) + mat(2, 4) << " (expected ~" << 1.0/model_0("Prodromal period") << ")" << std::endl;

    // Transition to isolated
    std::cout << "Transition to isolated: "
              << mat(3, 4) + mat(3, 5) + mat(3, 6) << " (expected ~" << 1.0/model_0("Days undetected") << ")" << std::endl;

    // Transition to hospitalized
    std::cout << "Transition to hospitalized (prodromal): "
              << mat(3, 6) + mat(3, 11) << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;
    std::cout << "Transition to hospitalized (rash): "
              << mat(4, 6) + mat(4, 11) << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;

    // Transition to recovered    
    std::cout << "Transition to recovered (prodromal): "
              << mat(3, 5) + mat(3, 12) << " (expected ~" << p_recovered << ")" << std::endl;
    std::cout << "Transition to recovered (rash): "
              << mat(4, 5) + mat(4, 12) << " (expected ~" << p_recovered << ")" << std::endl;

    // Transition from hospitalized to recovered
    std::cout << "Transition from hospitalized to recovered: "
              << mat(11, 12) << " (expected ~" << 1.0/model_0("Hospitalization period") << ")" << std::endl;


    std::cout << "Outbreak size: " <<
        static_cast<double>(
            std::accumulate(outbreak_sizes.begin(), outbreak_sizes.end(), 0.0)
        ) / static_cast<double>(nsims) << std::endl;
    #undef mat
    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
    
}