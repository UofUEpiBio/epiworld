#include "tests.hpp"

#define calc_r0(ans_observed, ans_expected, r0, model) \
    double (ans_observed) = std::accumulate(r0.begin(), r0.end(), 0.0); \
    (ans_observed) /= static_cast<epiworld_double>(r0.size()); \
    std::fill(r0.begin(), r0.end(), -1.0); \
    double (ans_expected) = (model)("Contact rate") * \
        (model)("Prob. Transmission") / \
        (model)("Prob. Recovery");

using namespace epiworld;

EPIWORLD_TEST_CASE("SIRMixing R0", "[SIR-mixing R0]") {

    std::vector< double > contact_matrix = {
        0.8, 0.1, 0.05,
        0.15, 0.8, 0.25,
        0.05, 0.1, .7
    };

    // std::fill(contact_matrix.begin(), contact_matrix.end(), 1.0/3.0);

    // Testing reproductive number in plain scenario
    int n_infected = 1;
    int n_agents = 2000;
    size_t nsims = 400;
   
    epimodels::ModelSIRMixing<> model_1(
        "Flu", // std::string vname,
        n_agents, // epiworld_fast_uint n,
        static_cast<double>(n_infected)/n_agents,  // epiworld_double prevalence,
        1.0,  // epiworld_double contact_rate,
        0.1,   // epiworld_double transmission_rate,
        1.0/7.0,// epiworld_double recovery_rate,
        contact_matrix
    );

    // Creating three groups
    Entity<> e1_1("Entity 1", dist_factory<>(0, n_agents/3));
    Entity<> e2_1("Entity 2", dist_factory<>(n_agents/3, n_agents/3*2));
    Entity<> e3_1("Entity 3", dist_factory<>(n_agents/3*2, n_agents));

    model_1.add_entity(e1_1);
    model_1.add_entity(e2_1);
    model_1.add_entity(e3_1);

    model_1.get_virus(0).set_distribution(
        distribute_virus_randomly(n_infected, false)
        // dist_virus
    );

    std::vector< std::vector<epiworld_double> > transitions(nsims);
    std::vector< epiworld_double > R0s(nsims, -1.0);
    auto saver = [&transitions, &R0s](size_t n, Model<>* m) -> void{

        // Saving the transition probabilities
        transitions[n] = m->get_db().get_transition_probability(false, false);

        // Recording the R0 from the index case
        auto rts = m->get_db().get_reproductive_number();      

        for (auto & rt: rts)
        {
            if (rt.first[1] != -1 && rt.first[2] == 0)
            {
                R0s[n] = static_cast<epiworld_double>(
                    rt.second
                );
            }
        }            

    };

    model_1.run_multiple(100, nsims, 1231, saver, true, true, 2);

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
    auto states = model_1.get_states();
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

    // Different runs
    model_1.run_multiple(100, nsims, 1231, saver, true, true, 2);
    calc_r0(R0_obs1, R0_exp1, R0s, model_1);

    model_1("Contact rate") = model_1("Contact rate") * 1.5;
    model_1.run_multiple(100, nsims, 1231, saver, true, true, 2);
    calc_r0(R0_obs2, R0_exp2, R0s, model_1);

    model_1("Contact rate") = model_1("Contact rate")  * 1.5;
    model_1.run_multiple(100, nsims, 1231, saver, true, true, 2);
    calc_r0(R0_obs3, R0_exp3, R0s, model_1);

    model_1("Contact rate") = model_1("Contact rate")  * 1.5;
    model_1.run_multiple(100, nsims, 1231, saver, true, true, 2);
    calc_r0(R0_obs4, R0_exp4, R0s, model_1);
    
    std::cout << "Testing R0 in SIRMixing" << std::endl;
    std::cout << "R0 obs " << R0_obs1 << " vs exp " << R0_exp1 << std::endl;
    std::cout << "R0 obs " << R0_obs2 << " vs exp " << R0_exp2 << std::endl;
    std::cout << "R0 obs " << R0_obs3 << " vs exp " << R0_exp3 << std::endl;
    std::cout << "R0 obs " << R0_obs4 << " vs exp " << R0_exp4 << std::endl;

    REQUIRE_FALSE(moreless(R0_obs1, R0_exp1, 0.2));
    REQUIRE_FALSE(moreless(R0_obs2, R0_exp2, 0.2));
    REQUIRE_FALSE(moreless(R0_obs3, R0_exp3, 0.2));
    REQUIRE_FALSE(moreless(R0_obs4, R0_exp4, 0.2));



}

#undef calc_r0