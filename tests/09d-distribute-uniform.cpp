#include "tests.hpp"

using namespace epiworld;

// Random distribution functions must pick every candidate with the same
// probability. They used to clamp runif_index()'s draw after decrementing the
// number of candidates, which moved the last candidate's draws onto the one
// before it: with one recipient, the last candidate could never be picked.
EPIWORLD_TEST_CASE(
    "Random distribution functions pick candidates uniformly",
    "[DistFuns][DistUniform]"
) {

    size_t nexperiments = 3000u;

    // Tools and virus_randomly(): 3 agents, one recipient each
    epimodels::ModelSIRCONN<> model(
        "a virus", 3u, 0.0, 0.0, 0.0, 0.0
    );
    model.get_virus(0).set_distribution(
        distribute_virus_randomly<>(1, false)
    );

    model.add_entity(Entity<>("everyone", distribute_entity_to_range<>(0, 3)));

    Tool<> tool_set("in set");
    tool_set.set_distribution(distribute_tool_randomly<>(1, false, {0u, 2u}));
    model.add_tool(tool_set);

    Tool<> tool_all("anyone");
    tool_all.set_distribution(distribute_tool_randomly<>(1, false));
    model.add_tool(tool_all);

    Tool<> tool_entity("by entity");
    tool_entity.set_distribution(distribute_tool_to_entities<>({1.0}, false));
    model.add_tool(tool_entity);

    std::map< std::string, std::vector< double > > picked;
    for (auto name : {"virus", "in set", "anyone", "by entity"})
        picked[name] = std::vector< double >(3u, 0.0);

    model.run_multiple(
        0, nexperiments, 1231,
        [&picked](size_t, Model<> * m) -> void {
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

    // virus_to_entities() in a model of its own (an agent holds one virus)
    epimodels::ModelSIRCONN<> model_entity(
        "a virus", 3u, 0.0, 0.0, 0.0, 0.0
    );
    model_entity.get_virus(0).set_distribution(
        distribute_virus_to_entities<>({1.0}, false)
    );
    model_entity.add_entity(
        Entity<>("everyone", distribute_entity_to_range<>(0, 3))
    );

    picked["virus by entity"] = std::vector< double >(3u, 0.0);
    model_entity.run_multiple(
        0, nexperiments, 1231,
        [&picked](size_t, Model<> * m) -> void {
            for (auto & agent : m->get_agents())
                if (agent.get_virus() != nullptr)
                    picked["virus by entity"][agent.get_id()] += 1.0;
        },
        true, false
    );

    // Expected shares; the tolerance is about 6 standard errors
    std::map< std::string, std::vector< double > > expected = {
        {"virus",           {1.0/3.0, 1.0/3.0, 1.0/3.0}},
        {"in set",          {0.5,     0.0,     0.5    }},
        {"anyone",          {1.0/3.0, 1.0/3.0, 1.0/3.0}},
        {"by entity",       {1.0/3.0, 1.0/3.0, 1.0/3.0}},
        {"virus by entity", {1.0/3.0, 1.0/3.0, 1.0/3.0}}
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
