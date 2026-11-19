#include "tests.hpp"

using namespace epiworld;

// A placement restricted to a subset of agents: the subset is a group of a
// mixing layer (agents 900 to 999 are group 1 of "set"), and the draws stay
// inside it, whether a count or a share of the group. A count above what is
// eligible fails.

// Placing a virus and a tool on a large given set of agents
// (assign::agents()) costs about what a random placement of the same number
// does: the ids are shared, not copied per agent.

// Random placements must pick every candidate with the same probability.
// (The distribution functions of epiworld 0.x used to clamp runif_index()'s
// draw after decrementing the number of candidates, which moved the last
// candidate's draws onto the one before it.)

// Assignment: in groups, scaling and uniform
//
// One test case; each former test file is a block below, with the name it had.
EPIWORLD_TEST_CASE("Assignment: in groups, scaling and uniform", "[assign][groups][scaling][uniform]") {

    // ---- 09b-assign-in-groups.cpp: Assign within the groups of a layer
    {

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

    // ---- 09c-assign-scaling.cpp: Assigning to a large set is linear
    {

    size_t n = 100000u;
    size_t k = n / 2;

    std::vector< size_t > set;
    for (size_t i = 0u; i < k; ++i)
        set.push_back(2u * i);

    auto time_it = [&](Assign who) -> double {

        epimodels::ModelSIR model("a virus", n, 0.0, 0.0, 0.0);
        add_random_mixing(model, 0.0);
        model.assign_virus(0, who);
        model.add_tool(Tool("tool"), who);
        model.verbose_off();

        auto start = std::chrono::steady_clock::now();
        model.run(0, 1);
        std::chrono::duration< double > elapsed =
            std::chrono::steady_clock::now() - start;

        size_t n_tools = 0u, n_virus = 0u;
        for (auto & agent : model.get_agents())
        {
            n_tools += agent.get_n_tools();
            n_virus += (agent.get_virus() != nullptr) ? 1u : 0u;
        }

        REQUIRE(n_tools == k);
        REQUIRE(n_virus == k);

        return elapsed.count();

    };

    double t_random = time_it(assign::count(k));
    double t_set = time_it(assign::agents(set));

    #ifdef EPI_DEBUG
    std::cout << "Assigning " << k << " agents: randomly "
        << t_random << "s, to a set " << t_set << "s" << std::endl;
    #endif

    REQUIRE(t_set < 10.0 * t_random + 0.1);
    }

    // ---- 09d-assign-uniform.cpp: Random placements pick candidates uniformly
    {

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

}
