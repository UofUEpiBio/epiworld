#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;



EPIWORLD_TEST_CASE("SIRMixing", "[SIR-mixing]") {

    std::vector< double > contact_matrix = {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    };

    epimodels::ModelSIRMixing<> model(
        "Flu", // std::string vname,
        10000, // epiworld_fast_uint n,
        0.01,// epiworld_double prevalence,
        40.0,// epiworld_double contact_rate,
        1.0,// epiworld_double transmission_rate,
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
        if (a.get_state() != epimodels::ModelSIRMixing<>::SUSCEPTIBLE)
        {
            if (a.get_entity(0).get_id() == 0)
            {
                n_right++;
                continue;
            }

            n_wrong++;
            
        }
            
    }

    REQUIRE_FALSE((n_wrong != 0 | n_right != 3000));

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
        else if (a.get_state() != epimodels::ModelSIRMixing<>::SUSCEPTIBLE)
        {
            if (a.get_entity(0).get_id() == 1)
            {
                n_right++;
                continue;
            }

            n_wrong++;
            
        }
            
    }

    REQUIRE_FALSE((n_wrong != 0 | n_right != 3001));

    // Rerunning with plain mixing
    std::fill(contact_matrix.begin(), contact_matrix.end(), 1.0/3.0);
    model.set_contact_matrix(contact_matrix);

    // Running and checking the results
    model.run(50, 123);
    model.print();

    std::vector< int > totals;
    model.get_db().get_today_total(nullptr, &totals);

    std::vector< int > expected_totals = {
        0, 0,
        static_cast<int>(model.size())
        };

    REQUIRE_THAT(totals, Catch::Equals(expected_totals));





}
