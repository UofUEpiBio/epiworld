#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Measles model (quarantine)", "[ModelMeaslesQuarantineOn]") {
    
    // Queuing doesn't matter and get results that are meaningful
    int n_seeds = 5;
    epimodels::ModelMeaslesQuarantine<> model_0(
        1000,    // Number of agents
        n_seeds, // Number of initial cases
        2.0,     // Contact rate
        0.2,     // Transmission rate
        0.9,     // Vaccination efficacy
        0.3,     // Vaccination reduction recovery rate
        7.0,     // Incubation period
        4.0,     // Prodromal period
        5.0,     // Rash period
        3.0,     // Days undetected
        0.2,     // Hospitalization rate
        7.0,     // Hospitalization duration
        0.1,     // Proportion vaccinated
        21u,     // Quarantine period
        .8,      // Quarantine willingness
        4u       // Isolation period
    );

    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(v, m);
        return;
    });

    size_t nsims = 1000;
    std::vector< std::vector<epiworld_double> > transitions(nsims);
    std::vector< epiworld_double > R0s(nsims * n_seeds, -1.0);
    auto saver = [&transitions, &R0s, n_seeds](size_t n, Model<>* m) -> void{

        // Saving the transition probabilities
        transitions[n] = m->get_db().transition_probability(false, false);

        // Recording the R0 from the index case
        auto rts = m->get_db().reproductive_number();      
        for (int i = 0; i < n_seeds; ++i)
            R0s[n_seeds * n + i] = static_cast<epiworld_double>(rts[{0, i, 0}]);

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

    // Avarage R0
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
    // Transition to prodromal
    REQUIRE_FALSE(moreless(
        mat(1, 2) + mat(1, 9), 1.0/model_0("Incubation period"), 0.05)
    );

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

    // Transition from Quarantine Exposed to Quarantined Prodromal
    REQUIRE_FALSE(
        moreless(mat(7, 9), 1.0/model_0("Incubation period"), 0.05)
    );

    // From quarantine prodromal to isolated (rash cases are dected immediately)
    REQUIRE_FALSE(
        moreless(mat(9, 4), 1.0/model_0("Prodromal period"), 0.05)
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
    std::cout << "Effective Rt (lower): "
              << R0_observed << " (R0 ~" << R0_theo << ")" << std::endl;

    // Transition to prodromal
    std::cout << "Transition to prodromal: "
              << mat(1, 2) + mat(1, 9) << " (expected ~" << 1.0/model_0("Incubation period") << ")" << std::endl;

    // Transition to rash
    std::cout << "Transition to rash: "
              << mat(2, 3) + mat(2, 4) << " (expected ~" << 1.0/model_0("Prodromal period") << ")" << std::endl;

    // Transition to isolated
    std::cout << "Transition to isolated: "
              << mat(3, 4) + mat(3, 5) + mat(3, 6) << " (expected ~" << 1.0/model_0("Days undetected") << ")" << std::endl;

    // Transition from Quarantine Exposed to Quarantined Prodromal
    std::cout << "Transition from Q. Exposed to Q. Prodromal: "
              << mat(7, 9) << " (expected ~" << 1.0/model_0("Incubation period") << ")" << std::endl;

    // From quarantine prodromal to isolated (rash cases are dected immediately)
    std::cout << "Transition from Q. Prodromal to isolated: "
              << mat(9, 4) << " (expected ~" << 1.0/model_0("Prodromal period") << ")" << std::endl;

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
    #undef mat
    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
    
}