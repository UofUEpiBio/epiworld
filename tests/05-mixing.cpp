#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SEIRMixing", "[SEIR-mixing]") {

    std::vector< double > contact_matrix = {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    };

    epimodels::ModelSEIRMixing<> model(
        "Flu", // std::string vname,
        10000, // epiworld_fast_uint n,
        0.01,  // epiworld_double prevalence,
        40.0,  // epiworld_double contact_rate,
        1.0,   // epiworld_double transmission_rate,
        1.0,   // epiworld_double avg_incubation_days,
        1.0/2.0,// epiworld_double recovery_rate,
        contact_matrix
    );

    // Copy the original virus
    Virus<> v1 = model.get_virus(0);
    model.rm_virus(0);
    v1.set_distribution(dist_virus<>(0));

    model.add_virus(v1);

    // Creating three groups
    Entity<> e1("Entity 1", dist_factory<>(0, 3000));
    Entity<> e2("Entity 2", dist_factory<>(3000, 6000));
    Entity<> e3("Entity 3", dist_factory<>(6000, 10000));

    model.add_entity(e1);
    model.add_entity(e2);
    model.add_entity(e3);

    // Running and checking the results
    model.run(50, 123);
    model.print();

    // Getting all agents
    int n_right = 0;
    int n_wrong = 0;

    for (const auto & a : model.get_agents())
    {
        if (a.get_state() != epimodels::ModelSEIRMixing<>::SUSCEPTIBLE)
        {
            if (a.get_entity(0).get_id() == 0)
            {
                n_right++;
                continue;
            }

            n_wrong++;
            
        }
            
    }

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE((n_wrong != 0 | n_right != 3000));
    #endif

    // Reruning the model where individuals from group 0 transmit all to group 1
    contact_matrix[0] = 0.0;
    contact_matrix[6] = 1.0;
    contact_matrix[4] = 0.5;
    contact_matrix[1] = 0.5;
    model.set_contact_matrix(contact_matrix);

    // Running and checking the results
    model.run(50, 123);
    model.print();

    // Getting all agents
    n_right = 0;
    n_wrong = 0;

    for (const auto & a : model.get_agents())
    {

        if (a.get_id() == 0)
        {
            n_right++;
        } 
        else if (a.get_state() != epimodels::ModelSEIRMixing<>::SUSCEPTIBLE)
        {
            if (a.get_entity(0).get_id() == 1)
            {
                n_right++;
                continue;
            }

            n_wrong++;
            
        }
            
    }

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE((n_wrong != 0 | n_right != 3001));
    #endif

    // Rerunning with plain mixing
    std::fill(contact_matrix.begin(), contact_matrix.end(), 1.0/3.0);
    model.set_contact_matrix(contact_matrix);

    // Running and checking the results
    model.run(50, 123);
    model.print();

    std::vector< int > totals;
    model.get_db().get_today_total(nullptr, &totals);

    std::vector< int > expected_totals = {
        0, 0, 0,
        static_cast<int>(model.size())
        };

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_THAT(totals, Catch::Equals(expected_totals));
    #endif

    // If entities don't have a dist function, then it should be
    // OK
    e1.set_distribution(distribute_entity_randomly<>(2000, false, true));
    e2.set_distribution(distribute_entity_randomly<>(2000, false, true));
    e3.set_distribution(distribute_entity_randomly<>(2000, false, true));

    model.rm_entity(0);
    model.rm_entity(1);
    model.rm_entity(2);

    model.add_entity(e1);
    model.add_entity(e2);
    model.add_entity(e3);

    // Running and checking the results
    model.run(50, 123);

    auto agents1 = model.get_entity(0).get_agents();
    auto agents2 = model.get_entity(1).get_agents();
    auto agents3 = model.get_entity(2).get_agents();

    std::vector< int > counts(model.size(), 0);
    for (const auto & a: agents1)
        counts[a]++;

    for (const auto & a: agents2)
        counts[a]++;

    for (const auto & a: agents3)
        counts[a]++;
    
    double n0 = 0, n1 = 0, n2 = 0, n3 = 0;
    for (const auto & c: counts)
    {
        if (c == 0)
            n0++;
        else if (c == 1)
            n1++;
        else if (c == 2)
            n2++;
        else if (c == 3)
            n3++;
    }

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(!(n0 == 4000 && n1 == 6000 && n2 == 0 && n3 == 0));
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
