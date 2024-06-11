#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Distribution funs", "[DistFuns]") {

    epimodels::ModelSIRCONN<> model_0(
        "a virus", 10000u, 0.01, 4.0, 1.0, 1.0/10000.0
    );
    
    model_0.run(0, 131);

    // Listing agents with viruses
    std::vector< size_t > got_it;
    for (size_t i = 0; i < model_0.size(); i++)
    {
        if (model_0.get_agent(i).get_virus() != nullptr)
            got_it.push_back(i);
    }

    epimodels::ModelSIRCONN<> model_1(
        "a virus", 10000u, 0.01, 4.0, 1.0, 1.0/10000.0
    );

    model_1.get_virus(0).set_dist_fun(
        distribute_virus_to_set<>(got_it)
    );
    
    model_1.run(0, 131);

    // Listing agents with viruses
    std::vector< size_t > got_it1;
    for (size_t i = 0; i < model_1.size(); i++)
    {
        if (model_1.get_agent(i).get_virus() != nullptr)
            got_it1.push_back(i);
    }
    
    // Sorting got_it asc
    std::sort(got_it.begin(), got_it.end());
    std::sort(got_it1.begin(), got_it1.end());

    // Comparing both sets: Finding the non-matching elements
    std::vector< size_t > diff;
    std::set_difference(
        got_it.begin(), got_it.end(),
        got_it1.begin(), got_it1.end(),
        std::inserter(diff, diff.begin())
    );

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(got_it.size() > 0);
    REQUIRE(got_it == got_it1);
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}