#include "tests.hpp"

using namespace epiworld;



EPIWORLD_TEST_CASE("SIRMixing", "[SIR-mixing]") {

    std::vector< double > contact_matrix = {
        40.0, 0.0, 0.0,
        0.0, 40.0, 0.0,
        0.0, 0.0, 40.0
    };

    epimodels::ModelSIR<> model(
        "Flu", // std::string vname,
        10000, // epiworld_fast_uint n,
        0.01,// epiworld_double prevalence,
        1.0,// epiworld_double transmission_prob,
        1.0/2.0 // epiworld_double recovery_rate,
    );
    // Three groups
    add_group_mixing(model, contact_matrix, blocks({0u, 3000u, 6000u, 10000u}));
    auto & mixing = model.get_layer_as< layer::Mixing<> >("mixing");

    // Copy the original virus
    Virus<> v1 = model.get_virus(0);
    model.rm_virus(0);
    v1.set_distribution(dist_virus<>(0));

    model.add_virus(v1);


    // Running and checking the results
    model.run(50, 123);
    model.print();

    // Getting all agents
    int n_right = 0;
    int n_wrong = 0;

    for (const auto & a : model.get_agents())
    {
        if (static_cast<int>(a.get_state()) != model.state_of("Susceptible"))
        {
            if (mixing.get_group(a.get_id()) == 0)
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
    contact_matrix[6] = 40.0;
    contact_matrix[4] = 20.0;
    contact_matrix[1] = 20.0;
    mixing.set_contact_matrix(contact_matrix);

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
        else if (static_cast<int>(a.get_state()) != model.state_of("Susceptible"))
        {
            if (mixing.get_group(a.get_id()) == 1)
            {
                n_right++;
                continue;
            }

            n_wrong++;

        }

    }

    REQUIRE_FALSE((n_wrong != 0 | n_right != 3001));

    // Rerunning with plain mixing
    std::fill(contact_matrix.begin(), contact_matrix.end(), 40.0/3.0);
    mixing.set_contact_matrix(contact_matrix);

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
