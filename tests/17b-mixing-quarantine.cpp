#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "SEIRMixingQuarantine transitions",
    "[SEIR-mixing-quarantine-transitions]"
) {

    std::vector< double > contact_matrix = {
        0.8, 0.1, 0.1,
        0.1, 0.8, 0.1,
        0.1, 0.1, 0.8
    };

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

    // Running and checking the results
    model.run(50, 123);
    model.print();

    
    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
