#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "SEIRMixingQuarantine transitions 2",
    "[SEIR-mixing-quarantine-transitions-2]"
) {

    std::vector< double > contact_matrix(9, 1.0/3.0);

    int n = 2000;
    size_t n_seeds = 10;
    size_t nsims = 400;

    epimodels::ModelSEIRMixingQuarantine<> model(
        "Flu", // std::string vname,
        n, // epiworld_fast_uint n,
        0.01,  // epiworld_double prevalence,
        8.0,  // epiworld_double contact_rate,
        0.5,   // epiworld_double transmission_rate,
        4.0,   // epiworld_double avg_incubation_days,
        1.0/3.5,// epiworld_double recovery_rate,
        contact_matrix,
        .2, // Hospitalization rate
        4, // Hospitalization period
        1.5, // Days undetected (negative means no quarantine)
        15, // Quarantine period
        .9, // Quarantine willingness
        4 // Isolation period
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
    model.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (size_t i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(v, m);
        return;
    });

    std::vector< std::vector<epiworld_double> > transitions(nsims);
    std::vector< epiworld_double > R0s(nsims * n_seeds, -1.0);
    
    // Generating the saver function
    auto saver = tests_create_saver(transitions, R0s, n_seeds);

    // Running and checking the results
    model.run_multiple(60, nsims, 123, saver, true, true, 1);
    model.print();

    // Calculate average transitions
    auto avg_transitions = tests_calculate_avg_transitions(
        transitions, model
    );

    // Checking the average transitions
    tests_print_avg_transitions(avg_transitions, model);

    // Checking the expected and effective R0
    epiworld_double R0_expected =
        model("Contact rate") * model("Prob. Transmission") / 
        (model("Prob. Recovery") + model("Hospitalization rate"));

    epiworld_double R0 = 0.0;
    for (size_t i = 0; i < R0s.size(); ++i)
    {
        R0 += R0s[i];
    }
    R0 /= static_cast<epiworld_double>(R0s.size());
    printf_epiworld(
        "R0 observed (lower): %.2f (expected %.2f)\n", R0, R0_expected
    );

    // Transition to Infected
    #define mat(i, j) avg_transitions[j * model.get_n_states() + i]

    std::cout << "Transition to infected (exposed): " <<
        mat(1, 2) + mat(1, 3) << " (expected: " << 
        1.0/model("Avg. Incubation days") << ")" << std::endl;

    // Transition to recovered
    std::cout << "Transition to recovered (infected): " <<
        mat(2, 7) + mat(2, 9) << " (expected: " << 
        model("Prob. Recovery") << ")" << std::endl;

    // Transition to isolated
    std::cout << "Transition to isolated (infected; larger b/c of quarantine): " <<
        mat(2, 3) + mat(2, 4) + mat(2, 7) << " (expected: " << 
        1.0/model("Days undetected") << ")" << std::endl;

    // Transition to infected (from quarantined exposed)
    std::cout << "Transition to q. infected (q. exposed): " <<
        mat(6, 2) + mat(6, 3) << " (expected: " << 
        1.0/model("Avg. Incubation days") << ")" << std::endl;

    #ifdef CATCH_CONFIG_MAIN
    // Transition to infected
    REQUIRE_FALSE(
        moreless(mat(1, 2) + mat(1, 3), 1.0/model("Avg. Incubation days"), 0.05)
    );

    // Transition to recovered
    REQUIRE_FALSE(
        moreless(
            mat(2, 7) + mat(2, 9),
            model("Prob. Recovery"), 0.05
        )
    );

    // Transition to isolated
    REQUIRE_FALSE(
        moreless(
            mat(2, 3) + mat(2, 4) + mat(2, 7),
            1.0/model("Days undetected") +
            // The quarantine process moves agents to isolation
            // so it increases the transition to isolated
            .04,
            0.05
        )
    );

    // Transition to infected (from quarantined exposed)
    REQUIRE_FALSE(
        moreless(
            mat(6, 2) + mat(6, 3),
            1.0/model("Avg. Incubation days"), 0.05
        )
    );
    #endif
    
    #undef mat
    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
