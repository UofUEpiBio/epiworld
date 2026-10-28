#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SEIRMixingQuarantine", "[SEIR-mixing-quarantine]") {

    std::vector< double > contact_matrix = {
        40.0, 0.0, 0.0,
        0.0, 40.0, 0.0,
        0.0, 0.0, 40.0
    };

    epimodels::ModelSEIRQuarantine<> model(
        "Flu", // std::string vname,
        10000, // epiworld_fast_uint n,
        0.01,  // epiworld_double prevalence,
        1.0,   // epiworld_double transmission_prob,
        2.0,   // epiworld_double latent_period,
        1.0/2.0, // epiworld_double recovery_rate,
        .1,     // epiworld_double hospitalization_rate,
        5,      // epiworld_double hospitalization_period,
        2,      // epiworld_double detection_delay,
        4,      // epiworld_fast_int quarantine_period,
        .9,     // epiworld_double quarantine_willingness,
        1.0,     // epiworld_double isolation_willingness,
        10,      // epiworld_fast_int isolation_period
        1.0,    // epiworld_double contact_tracing_prob,
        4       // epiworld_fast_uint contact_tracing_window
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
    int n_wrong = 0;

    for (const Agent<> & a : model.get_agents())
    {
        if (a.get_state() != epimodels::ModelSEIRQuarantine<>::SUSCEPTIBLE)
        {
            if (mixing.get_group(a.get_id()) == 0)
            {
                continue;
            }

            n_wrong++;
            
        }
            
    }

    REQUIRE_FALSE((n_wrong != 0));

    // Reruning the model where individuals from group 0 transmit all to group 1
    // Contact matrix is column-major: index = j * n + i
    // at(i, j) -> contact_matrix[j * n + i]
    size_t n_groups = static_cast<size_t>(
        std::round(std::sqrt(contact_matrix.size()))
    );
    auto at = [&contact_matrix, n_groups](size_t i, size_t j) -> double & {
        return contact_matrix[j * n_groups + i];
    };
    at(0, 0) = 0.0;   // group 0 -> group 0: 0 (no self-contacts for group 0)
    at(0, 2) = 40.0;  // group 0 -> group 2: 40 (group 0 contacts group 2)
    at(1, 1) = 20.0;  // group 1 -> group 1: 20
    at(1, 0) = 20.0;  // group 1 -> group 0: 20 (group 1 also contacts group 0)
    mixing.set_contact_matrix(contact_matrix);

    // Running and checking the results
    model.run(50, 123);
    model.print();

    // Getting all agents
    n_wrong = 0;

    for (const Agent<> & a : model.get_agents())
    {

        if (a.get_id() == 0)
        {
            continue;
        }
        else if (a.get_state() != epimodels::ModelSEIRQuarantine<>::SUSCEPTIBLE)
        {
            if (mixing.get_group(a.get_id()) == 1)
            {
                continue;
            }

            n_wrong++;
            
        }
            
    }

    REQUIRE_FALSE((n_wrong != 0));

    // Rerunning with plain mixing
    std::fill(contact_matrix.begin(), contact_matrix.end(), 40.0/3.0);
    mixing.set_contact_matrix(contact_matrix);

    // Running and checking the results
    model.set_param("Detection delay", -1.0); // No detection
    model.run(50, 123); 
    model.print();

    std::vector< int > totals;
    model.get_db().get_today_total(nullptr, &totals);

    std::vector< int > expected_totals(model.get_n_states(), 0);
    expected_totals[expected_totals.size() - 1] = static_cast<int>(model.size());

    // In the totals, hospitalized agents also count as recovered
    // So we set the last element to be the sum of the last two
    // and the second to last to be zero.
    totals[totals.size() - 1] += totals[totals.size() - 2];
    totals[totals.size() - 2] = 0;

    REQUIRE_THAT(totals, Catch::Equals(expected_totals));

    // Random groups of 2000, drawn at every reset: 6000 agents in a group,
    // 2000 in each, and the rest in none
    mixing.set_groups(grouping::random<>({2000.0, 2000.0, 2000.0}));
    model.run(50, 123);

    std::vector< size_t > sizes(4u, 0u);
    for (size_t i = 0u; i < model.size(); ++i)
    {
        const size_t g = mixing.get_group(i);
        ++sizes[(g == layer::Mixing<>::NO_GROUP) ? 3u : g];
    }

    REQUIRE(sizes == std::vector< size_t >({2000u, 2000u, 2000u, 4000u}));



}
