#include "tests.hpp"

using namespace epiworld;

// A placement restricted to a subset of agents: the subset is a group of a
// mixing layer (agents 900 to 999 are group 1 of "set"), and the draws stay
// inside it, whether a count or a share of the group. A count above what is
// eligible fails.
EPIWORLD_TEST_CASE(
    "Assign within the groups of a layer", "[assign][groups]"
) {

    size_t n = 1000u;
    std::vector< size_t > group(n, 0u);
    for (size_t i = 900u; i < n; ++i)
        group[i] = 1u;

    epimodels::ModelSIR model("a virus", n, 0.0, 0.0, 0.0);
    add_random_mixing(model, 0.0);

    auto set = layer::mixing("set");
    set->set_groups(assign::labels(group), 2u);
    set->set_contact_prob(0.0);
    model.add_layer(set);

    model.assign_virus(0, assign::group_counts("set", {0.0, 30.0}));
    model.add_tool(Tool("tool abs"), assign::group_counts("set", {0.0, 40.0}));
    model.add_tool(Tool("tool prop"), assign::group_shares("set", {0.0, 0.5}));

    model.verbose_off();
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

    // More agents than are eligible fails
    model.add_tool(Tool("too many"), assign::count(1001u));
    REQUIRE_THROWS(model.run(0, 123));

}
