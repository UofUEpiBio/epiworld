#include "tests.hpp"

using namespace epiworld;

// distribute_tool_randomly() and distribute_virus_randomly() must sample from
// the agents in `agents_ids` when a set is given (not from agents 0..k-1).
EPIWORLD_TEST_CASE(
    "Random distribution restricted to a set", "[DistFuns][DistToSet]"
) {

    size_t n = 1000u;
    std::vector< size_t > set;
    for (size_t i = 900u; i < n; ++i)
        set.push_back(i);

    epimodels::ModelSIRCONN<> model(
        "a virus", n, 0.0, 0.0, 0.0, 0.0
    );
    model.get_virus(0).set_distribution(
        distribute_virus_randomly<>(30, false, set)
    );

    // Absolute number and proportion (of the set) for tools
    Tool<> tool_abs("tool abs");
    tool_abs.set_distribution(distribute_tool_randomly<>(40, false, set));

    Tool<> tool_prop("tool prop");
    tool_prop.set_distribution(distribute_tool_randomly<>(0.5, true, set));

    model.add_tool(tool_abs);
    model.add_tool(tool_prop);

    model.run(0, 123);

    size_t n_abs = 0u, n_prop = 0u, n_virus = 0u;
    for (auto & agent : model.get_agents())
    {
        bool in_set = agent.get_id() >= 900;

        for (size_t t = 0u; t < agent.get_n_tools(); ++t)
        {
            REQUIRE(in_set);
            if (agent.get_tool(static_cast< int >(t))->get_name() == "tool abs")
                ++n_abs;
            else
                ++n_prop;
        }

        if (agent.get_virus() != nullptr)
        {
            REQUIRE(in_set);
            ++n_virus;
        }
    }

    REQUIRE(n_abs == 40u);
    REQUIRE(n_prop == 50u);
    REQUIRE(n_virus == 30u);

    // Asking for more agents than the set has must fail
    Tool<> tool_too_many("too many");
    tool_too_many.set_distribution(
        distribute_tool_randomly<>(101, false, set)
    );
    model.add_tool(tool_too_many);
    REQUIRE_THROWS(model.run(0, 123));

}
