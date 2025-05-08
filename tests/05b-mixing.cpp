#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SEIRMixing", "[SEIR-mixing]") {

    std::vector< double > contact_matrix = {
        0.8, 0.1, 0.05,
        0.15, 0.8, 0.25,
        0.05, 0.1, .7
    };

    std::fill(contact_matrix.begin(), contact_matrix.end(), 1.0/3.0);

    // std::fill(contact_matrix.begin(), contact_matrix.end(), 1.0/3.0);

    // Testing reproductive number in plain scenario
    int n_infected = 10;
    #ifdef EPI_DEBUG
    int n_agents = 1000;
    #else
    int n_agents = 10000;
    #endif
    size_t nsims = 400;
    epimodels::ModelSEIRMixing<> model_1(
        "Flu", // std::string vname,
        n_agents, // epiworld_fast_uint n,
        static_cast<double>(n_infected)/n_agents,  // epiworld_double prevalence,
        8.0,  // epiworld_double contact_rate,
        0.05,   // epiworld_double transmission_rate,
        7.0,   // epiworld_double avg_incubation_days,
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
    std::vector< epiworld_double > R0s;
    #ifdef EPI_DEBUG
    int n_groups = 3;
    std::vector< double > group_sampling(n_groups * n_groups, 0.0);
    #endif
    auto saver = [
        &transitions, &R0s, n_infected
        #ifdef EPI_DEBUG
        , &group_sampling, n_groups
        #endif
    ](size_t n, Model<>* m) -> void{

        // Saving the transition probabilities
        transitions[n] = m->get_db().transition_probability(false, false);

        // Recording the R0 from the index case
        auto rts = m->get_db().reproductive_number();      
        for (const auto & i: rts)
        {

            // Only on the first day
            if ((i.first[2] != 0) || (i.first[1] < 0))
                continue;

            R0s.push_back(static_cast<epiworld_double>(i.second));
        }

        #ifdef EPI_DEBUG
        // Saving the group sampling
        auto sampling = dynamic_cast< epimodels::ModelSEIRMixing<>* >(m)->
            get_sampled_times_groups();

        // Saving the transition probabilities
        for (int i = 0; i < n_groups; ++i)
            for (int j = 0; j < n_groups; ++j)
                group_sampling[i * n_groups + j] += sampling[i * n_groups + j];

        #endif

    };

    model_1.run_multiple(100, nsims, 1231, saver, true, true, 2);

    #ifdef EPI_DEBUG
    // Processing group sampling
    for (int i = 0; i < n_groups; ++i)
    {
        double rowsums = 0.0;
        for (int j = 0; j < n_groups; ++j)
        {
            rowsums += group_sampling[j * n_groups + i];
        }

        for (int j = 0; j < n_groups; ++j)
        {
            group_sampling[j * n_groups + i] /= rowsums;
        }
    }
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
    R0_observed /= static_cast<epiworld_double>(nsims * n_infected);
    
    #ifdef EPI_DEBUG
    model_1.print();
    #endif

    double R0_expected =
        model_1("Contact rate") * model_1("Prob. Transmission") /
        model_1("Prob. Recovery");

    // Computing the 95% CI
    std::sort(R0s.begin(), R0s.end());

    std::cout << "R0 in ModelSEIRMixing" << std::endl ;
    std::cout << "observed: " << R0_observed <<
        " vs expected: " << R0_expected << std::endl;

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(moreless(R0_observed, R0_expected, 0.1));
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
