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
        1.0, // Isolation willingness
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
    std::vector< double > quarantined_counts(3, 0);
    std::vector< int > initial_agent(nsims);
    std::vector< unsigned int > quarantined_states = {
        epimodels::ModelSEIRMixingQuarantine<>::QUARANTINED_SUSCEPTIBLE,
        epimodels::ModelSEIRMixingQuarantine<>::QUARANTINED_EXPOSED
    };

    GlobalFun<> quarantine_accounting = [
        &quarantined_counts,
        quarantined_states
        ](epiworld::Model<>* m) -> void {

            for (auto & a: m->get_agents())
            {
                if (IN(a.get_state(), quarantined_states))
                    quarantined_counts[a.get_entity(0).get_id()] += 1.0;
            }

        };

    // Making a saver function
    std::function<void(size_t,Model<int>*)> saver = [&initial_agent](size_t sim, Model<int>* m) {
        auto model = dynamic_cast<epimodels::ModelSEIRMixingQuarantine<>*>(m);
        if (!model) return;

        // Saving the initial agent counts
        auto rt = model->get_db().get_reproductive_number();

        // Finding the seed case
        for (auto & r : rt) {
            // Identifying the seed case
            if ((r.first[0] == 0) && (r.first[1] != -1) && (r.first[2] == 0)) {
                initial_agent[sim] = r.first[1];
                break;
            }
        }
    };

    model.add_globalevent(quarantine_accounting, "Quarantine accounting");

    // Running and checking the results
    model.run_multiple(60, nsims, 123, saver, true, true, 1);
    model.print();

    // Computing the probability that the initial agent is
    // from group 0, 1, or 2
    std::vector< double > prob_seed(3, 0.0);
    for (size_t i = 0; i < nsims; i++) {
        if (initial_agent[i] < n/3) {
            prob_seed[0]++;
        } else if (initial_agent[i] < n/3 * 2) {
            prob_seed[1]++;
        } else {
            prob_seed[2]++;
        }
    }

    for (size_t i = 0; i < 3; i++) {
        prob_seed[i] /= nsims;
    }

    // Normalizing the vector of `quarantined_counts`
    double total_quarantined = std::accumulate(
        quarantined_counts.begin(),
        quarantined_counts.end(),
        0.0
    );
    for (size_t i = 0; i < quarantined_counts.size(); i++)
    {
        quarantined_counts[i] /= total_quarantined;
    }

    #ifdef CATCH_CONFIG_MAIN
    auto quarantined_counts_expected = std::vector< double >(3, 1.0/3.0);
    auto prob_seed_expected = std::vector< double >(3, 1.0/3.0);
    REQUIRE_THAT(
        quarantined_counts,
        Catch::Approx(quarantined_counts_expected).margin(0.05)
    );
    REQUIRE_THAT(
        prob_seed,
        Catch::Approx(prob_seed_expected).margin(0.05)
    );
    // Checking the results
    #endif

    // Changing whether the entity 0 can be quarantined
    model.set_entity_can_quarantine(
        {false, true, true}
    );

    // Restarting the quarantine counts
    std::fill(quarantined_counts.begin(), quarantined_counts.end(), 0.0);

    // Running the model again
    model.run_multiple(100, nsims, 123, saver, true, true, 1);

    model.print();

    // Normalizing the vector of `quarantined_counts`
    total_quarantined = std::accumulate(
        quarantined_counts.begin(),
        quarantined_counts.end(),
        0.0
    );

    for (size_t i = 0; i < quarantined_counts.size(); i++)
    {
        quarantined_counts[i] /= total_quarantined;
    }

    // Checking the results
    #ifdef CATCH_CONFIG_MAIN
    quarantined_counts_expected = {0.0, 0.5, 0.5};
    REQUIRE_THAT(
        quarantined_counts,
        Catch::Approx(quarantined_counts_expected).margin(0.05)
    );
    #endif

    
    #undef mat
    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
