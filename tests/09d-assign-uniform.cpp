#include "tests.hpp"

using namespace epiworld;

// Random placements must pick every candidate with the same probability.
// (The distribution functions of epiworld 0.x used to clamp runif_index()'s
// draw after decrementing the number of candidates, which moved the last
// candidate's draws onto the one before it.)
EPIWORLD_TEST_CASE(
    "Random placements pick candidates uniformly",
    "[assign][uniform]"
) {

    size_t nexperiments = 3000u;

    // Tools and a virus: 3 agents, one recipient each; the mixing
    // layer puts the three in one group
    epimodels::ModelSIR model(
        "a virus", 3u, 0.0, 0.0, 0.0
    );
    add_random_mixing(model, 0.0);
    model.assign_virus(0, assign::count(1));

    // Agents 0 and 2 are group 0 of "pair"
    auto pair = layer::mixing("pair");
    pair->set_groups(assign::labels(std::vector< int >{0, 1, 0}), 2u);
    pair->set_contact_prob(0.0);
    model.add_layer(pair);

    Tool tool_set("in set");
    model.add_tool(tool_set, assign::group_counts("pair", {1.0, 0.0}));

    Tool tool_all("anyone");
    model.add_tool(tool_all, assign::count(1));

    Tool tool_group("by group");
    model.add_tool(tool_group, assign::group_counts("random mixing", {1.0}));

    std::map< std::string, std::vector< double > > picked;
    for (auto name : {"virus", "in set", "anyone", "by group"})
        picked[name] = std::vector< double >(3u, 0.0);

    model.run_multiple(
        0, nexperiments, 1231,
        [&picked](size_t, Model * m) -> void {
            for (auto & agent : m->get_agents())
            {
                if (agent.get_virus() != nullptr)
                    picked["virus"][agent.get_id()] += 1.0;

                for (size_t t = 0u; t < agent.get_n_tools(); ++t)
                    picked[agent.get_tool(static_cast< int >(t))->get_name()]
                        [agent.get_id()] += 1.0;
            }
        },
        true, false
    );

    // A virus by group, in a model of its own (an agent holds one virus)
    epimodels::ModelSIR model_group(
        "a virus", 3u, 0.0, 0.0, 0.0
    );
    add_random_mixing(model_group, 0.0);
    model_group.assign_virus(0, assign::group_counts("random mixing", {1.0}));

    picked["virus by group"] = std::vector< double >(3u, 0.0);
    model_group.run_multiple(
        0, nexperiments, 1231,
        [&picked](size_t, Model * m) -> void {
            for (auto & agent : m->get_agents())
                if (agent.get_virus() != nullptr)
                    picked["virus by group"][agent.get_id()] += 1.0;
        },
        true, false
    );

    // Expected shares; the tolerance is about 6 standard errors
    std::map< std::string, std::vector< double > > expected = {
        {"virus",           {1.0/3.0, 1.0/3.0, 1.0/3.0}},
        {"in set",          {0.5,     0.0,     0.5    }},
        {"anyone",          {1.0/3.0, 1.0/3.0, 1.0/3.0}},
        {"by group",        {1.0/3.0, 1.0/3.0, 1.0/3.0}},
        {"virus by group",  {1.0/3.0, 1.0/3.0, 1.0/3.0}}
    };

    for (auto & [name, counts] : picked)
    {
        for (size_t i = 0u; i < 3u; ++i)
        {
            double share = counts[i] / static_cast< double >(nexperiments);

            #ifdef EPI_DEBUG
            printf_epiworld(
                "%-16s agent %zu: %.3f (expected %.3f)\n",
                name.c_str(), i, share, expected[name][i]
            );
            #endif

            REQUIRE_THAT(
                share,
                Catch::Matchers::WithinAbs(expected[name][i], 0.05)
            );
        }
    }

}
