#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "SEIRMixingQuarantine skip quarantine",
    "[SEIR-mixing-quarantine-skip-quarantine]"
) {

    std::vector< double > contact_matrix(9, 1.0/3.0);

    int n = 200;
    size_t n_seeds = 1;
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
        {true, true, true}, // Entity can quarantine
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
    v1.set_distribution(distribute_virus_randomly<>(n_seeds, false));

    model.add_virus(v1);

    // Adding a vaccine
    Tool<> v2("vax", .1, true);
    model.add_tool(v2);

    // Creating three groups
    Entity<> e1("Entity 1", dist_factory<>(0, n/3));
    Entity<> e2("Entity 2", dist_factory<>(n/3, n/3 * 2));
    Entity<> e3("Entity 3", dist_factory<>(n/3 * 2, n));

    model.add_entity(e1);
    model.add_entity(e2);
    model.add_entity(e3);

    // Global event to capture agents that are quarantined
    std::vector< unsigned int > quarantined_counts(3, 0);
    std::vector< unsigned int > quarantined_states = {
        epimodels::ModelSEIRMixingQuarantine<>::QUARANTINED_SUSCEPTIBLE,
        epimodels::ModelSEIRMixingQuarantine<>::QUARANTINED_EXPOSED
    };

    GlobalFun<> quarantine_accounting = [
        &quarantined_counts, quarantined_states
        ](epiworld::Model<>* m) -> void {

            auto model = dynamic_cast<
                epimodels::ModelSEIRMixingQuarantine<>*
                >(m);

            for (auto & a: m->get_agents())
            {
                if (IN(a.get_state(), quarantined_states))
                    quarantined_counts[a.get_entity(0).get_id()]++;
            }

        };

    model.add_globalevent(quarantine_accounting, "Quarantine accounting");

    // Running and checking the results
    model.run_multiple(60, nsims, 123, nullptr, true, true, 1);
    model.print();
    
    #undef mat
    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
