#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "SEIRMixingQuarantine transitions",
    "[SEIR-mixing-quarantine-transitions]"
) {

    std::vector< double > contact_matrix(9, 1.0/3.0);

    int n = 600;
    epimodels::ModelSEIRMixingQuarantine<> model(
        "Flu", // std::string vname,
        n, // epiworld_fast_uint n,
        0.01,  // epiworld_double prevalence,
        10.0,  // epiworld_double contact_rate,
        1.0,   // epiworld_double transmission_rate,
        2.0,   // epiworld_double avg_incubation_days,
        1.0/2.0,// epiworld_double recovery_rate,
        contact_matrix,
        2, // Days undetected
        4, // Quarantine period
        .9, // Quarantine willingness
        10 // Isolation period
    );

    // Copy the original virus
    Virus<> v1 = model.get_virus(0);
    model.rm_virus(0);
    v1.set_distribution(dist_virus<>(0));

    model.add_virus(v1);

    // Creating three groups
    Entity<> e1("Entity 1", dist_factory<>(0, n/3));
    Entity<> e2("Entity 2", dist_factory<>(n/3, n/3 * 2));
    Entity<> e3("Entity 3", dist_factory<>(n/3 * 2, n));

    model.add_entity(e1);
    model.add_entity(e2);
    model.add_entity(e3);

    // Setting the distribution function of the initial cases
    size_t n_seeds = 5;
    model.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(v, m);
        return;
    });

    size_t nsims = 500;
    std::vector< std::vector<epiworld_double> > transitions(nsims);
    std::vector< epiworld_double > R0s(nsims * n_seeds, -1.0);
    
    // Generating the saver function
    auto saver = tests_create_saver(transitions, R0s, n_seeds);

    // Running and checking the results
    model.run_multiple(60, nsims, 123, saver, true, true, 4);
    model.print();

    // Calculate average transitions
    auto avg_transitions = tests_calculate_avg_transitions(
        transitions, model
    );

    // Checking the average transitions
    tests_print_avg_transitions(avg_transitions, model);

    
    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
