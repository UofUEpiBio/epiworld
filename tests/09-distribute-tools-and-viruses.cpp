#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Distribution funs", "[DistFuns]") {

    epimodels::ModelSIRCONN<> model_0(
        "a virus", 10000u, 0.01, 4.0, 1.0, 1.0/10000.0
    );
    
    Tool<> tool("vax");
    tool.set_susceptibility_reduction(0.5);
    tool.set_distribution(
        distribute_tool_randomly(0.1, true)
    );

    model_0.add_tool(tool);

    model_0.run(0, 131);

    // Listing agents with viruses
    std::vector< size_t > got_it;
    for (size_t i = 0; i < model_0.size(); i++)
    {
        if (model_0.get_agent(i).get_virus() != nullptr)
            got_it.push_back(i);
    }

    // Listing agents with tools
    std::vector< size_t > got_tool;
    for (size_t i = 0; i < model_0.size(); i++)
    {
        if (model_0.get_agent(i).get_n_tools() != 0)
            got_tool.push_back(i);
    }

    epimodels::ModelSIRCONN<> model_1(
        "a virus", 10000u, 0.01, 4.0, 1.0, 1.0/10000.0
    );

    model_1.get_virus(0).set_distribution(
        distribute_virus_to_set<>(got_it)
    );

    model_1.add_tool(tool);
    model_1.get_tool(0).set_distribution(
        distribute_tool_to_set<>(got_tool)
    );
    
    model_1.run(0, 131);

    // Listing agents with viruses
    std::vector< size_t > got_it1;
    for (size_t i = 0; i < model_1.size(); i++)
    {
        if (model_1.get_agent(i).get_virus() != nullptr)
            got_it1.push_back(i);
    }

    // Listing agents with tools
    std::vector< size_t > got_tool1;
    for (size_t i = 0; i < model_1.size(); i++)
    {
        if (model_1.get_agent(i).get_n_tools() != 0)
            got_tool1.push_back(i);
    }
    
    // Sorting got_it asc
    std::sort(got_it.begin(), got_it.end());
    std::sort(got_it1.begin(), got_it1.end());

    std::sort(got_tool.begin(), got_tool.end());
    std::sort(got_tool1.begin(), got_tool1.end());

    // Comparing both sets: Finding the non-matching elements
    std::vector< size_t > diff;
    std::set_difference(
        got_it.begin(), got_it.end(),
        got_it1.begin(), got_it1.end(),
        std::inserter(diff, diff.begin())
    );

    std::vector< size_t > diff_tool;
    std::set_difference(
        got_tool.begin(), got_tool.end(),
        got_tool1.begin(), got_tool1.end(),
        std::inserter(diff_tool, diff_tool.begin())
    );

    REQUIRE(got_it.size() > 0);
    REQUIRE(got_it == got_it1);

    REQUIRE(got_tool.size() > 0);
    REQUIRE(got_tool == got_tool1);



}