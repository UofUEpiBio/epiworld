#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Measles model", "[ModelMeaslesQuarantine]") {
    
    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelMeaslesQuarantine<> model_0(
        500, // Number of agents
        1,   // Number of initial cases
        2.0, // Contact rate
        0.5, // Transmission rate
        0.9, //  Vaccination efficacy
        0.3, // Vaccination reduction recovery rate
        7.0, // Incubation period
        5.0, // Prodromal period
        5.0, // Rash period
        2.0, // Days undetected
        0.2, // Hospitalization rate
        7.0, // Hospitalization duration
        0.5, // Proportion vaccinated
        21u, // Quarantine period
        .8,  // Quarantine willingness
        4u   // Isolation period
    );

    // Shutting off the quarantine feature
    model_0("Quarantine willingness") = -1.0;

    size_t nsims = 500;
    std::vector< std::vector<epiworld_double> > transitions(nsims);
    auto saver = [&transitions](size_t n, Model<>* m) -> void{
        transitions[n] = m->get_db().transition_probability(false, false);
    };

    model_0.run_multiple(60, nsims, 1231, saver, true, true, 4);
    
    #ifndef CATCH_CONFIG_MAIN
    model_0.print(false);
    #endif

    // Creating an average across the transitions vectors
    std::vector<epiworld_double> avg_transitions(transitions[0].size(), 0.0);
    for (size_t i = 0; i < transitions.size(); ++i)
    {
        for (size_t j = 0; j < transitions[i].size(); ++j)
        {
            avg_transitions[j] += transitions[i][j];
        }
    }
    // Normalizing the average
    auto states = model_0.get_states();
    size_t n_states = states.size();
    for (size_t i = 0; i < n_states; ++i)
    {
        double rowsums = 0.0;
        for (size_t j = 0; j < n_states; ++j)
        {
            rowsums += avg_transitions[j * n_states + i];
        }

        // Normalizing the rows
        // If the row is empty, we skip it
        if (rowsums == 0.0)
            continue;

        for (size_t j = 0; j < n_states; ++j)
        {
            avg_transitions[j * n_states + i] /= rowsums;
        }

    }

    // Printing the entry as a matrix
    std::cout << "Average transitions: " << std::endl;
    for (size_t i = 0; i < n_states; ++i)
    {
        printf_epiworld("%25s ", states[i].c_str());
        for (size_t j = 0; j < n_states; ++j)
        {
            if (avg_transitions[j*n_states + i] == 0.0)
            {
                printf_epiworld("  -   ");
                continue;
            }

            printf_epiworld("%5.2f ", avg_transitions[j*n_states + i]);
        }
        printf_epiworld("\n");
    }

    // Checking especific values in the transitions
    #define mat(i, j) avg_transitions[j*n_states + i]
    double p_recovered = 1.0 - (
        1.0/model_0("Rash period") + model_0("Hospitalization rate")
    );
    #ifdef CATCH_CONFIG_MAIN

    // Transition to prodromal
    REQUIRE_FALSE(moreless(mat(1, 2), 1.0/model_0("Incubation period"), 0.05));

    // Transition to rash
    REQUIRE_FALSE(
        moreless(mat(2, 3) + mat(2, 4), 1.0/model_0("Prodromal period"), 0.05)
    );

    // Transition to isolated
    REQUIRE_FALSE(
        moreless(mat(3, 4) + mat(3, 5), 1.0/model_0("Days undetected"), 0.05)
    );

    // Transition to hospitalized
    REQUIRE_FALSE(moreless(mat(3, 10), model_0("Hospitalization rate"), 0.05));
    REQUIRE_FALSE(moreless(mat(4, 10), model_0("Hospitalization rate"), 0.05));

    // Transition to recovered
    REQUIRE_FALSE(moreless(mat(3, 5) + mat(3, 11), p_recovered, 0.05));
    REQUIRE_FALSE(moreless(mat(4, 5) + mat(4, 11), p_recovered, 0.05));

    // Transition from hospitalized to recovered
    REQUIRE_FALSE(moreless(mat(10, 11), 1.0/model_0("Hospitalization period"), 0.05));
    #endif
    // Transition to prodromal
    std::cout << "Transition to prodromal: "
              << mat(1, 2) << " (expected ~" << 1.0/model_0("Incubation period") << ")" << std::endl;

    // Transition to rash
    std::cout << "Transition to rash: "
              << mat(2, 3) + mat(2, 4) << " (expected ~" << 1.0/model_0("Prodromal period") << ")" << std::endl;

    // Transition to isolated
    std::cout << "Transition to isolated: "
              << mat(3, 4) + mat(3, 5) << " (expected ~" << 1.0/model_0("Days undetected") << ")" << std::endl;

    // Transition to hospitalized
    std::cout << "Transition to hospitalized (prodromal): "
              << mat(3, 10) << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;
    std::cout << "Transition to hospitalized (rash): "
              << mat(4, 10) << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;

    // Transition to recovered    
    std::cout << "Transition to recovered (prodromal): "
              << mat(3, 5) + mat(3, 11) << " (expected ~" << p_recovered << ")" << std::endl;
    std::cout << "Transition to recovered (rash): "
              << mat(4, 5) + mat(4, 11) << " (expected ~" << p_recovered << ")" << std::endl;

    // Transition from hospitalized to recovered
    std::cout << "Transition from hospitalized to recovered: "
              << mat(10, 11) << " (expected ~" << 1.0/model_0("Hospitalization period") << ")" << std::endl;
    #undef mat
    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
    
}