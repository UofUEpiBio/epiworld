#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SaverMemory does not depend on the number of threads", "[saver-memory]") {

    auto run = [](int nthreads) {

        epimodels::ModelSEIRCONN<> model("a virus", 500, 0.1, 3.0, 0.3, 4.0, 0.2);

        Tool<> tool("a tool");
        tool.set_susceptibility_reduction(.5);
        tool.set_distribution(distribute_tool_randomly<>(0.3, false));
        model.add_tool(tool);
        model.verbose_off();

        // Passed by value: copies must share the results
        SaverMemory saver(run_output_names());
        model.run_multiple(30, 9, 123, saver, true, false, nthreads);

        return saver.results();

    };

    auto res_1 = run(1);
    auto res_4 = run(4);

    REQUIRE(res_1.size() == run_output_names().size());
    REQUIRE(res_1.size() == res_4.size());

    for (const auto & what : run_output_names())
    {
        REQUIRE(res_1.at(what).colnames == res_4.at(what).colnames);
        REQUIRE(res_1.at(what).columns == res_4.at(what).columns);
    }

    // All the simulations are there, in order
    const auto & sim_id = std::get< std::vector< int > >(
        res_1.at("total_hist").columns[0u]
    );
    REQUIRE(sim_id.front() == 0);
    REQUIRE(sim_id.back() == 8);
    REQUIRE(std::is_sorted(sim_id.begin(), sim_id.end()));

}
