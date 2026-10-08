#include "tests.hpp"

using namespace epiworld;

// A tool that targets one virus does not act on the others
EPIWORLD_TEST_CASE("Tool targeting a virus", "[tool][target]") {

    // No recovery: infected agents stay infected
    epimodels::ModelSIRCONN<> model("A", 1000, 0.01, 4.0, 0.5, 0.0);
    model.verbose_off();

    Virus<> virus_b("B", 0.01, true);
    virus_b.set_state(1, 2, 2);
    virus_b.set_prob_infecting("Transmission rate");
    virus_b.set_prob_recovery("Recovery rate");

    // Targeting requires the virus to be in the model
    Tool<> tool("Vaccine against A", 1.0, true);
    tool.set_susceptibility_reduction(1.0);
    REQUIRE_THROWS_AS(tool.add_target(virus_b), std::logic_error);
    REQUIRE_THROWS_AS(tool.add_target(-1), std::range_error);
    REQUIRE_THROWS_AS(tool.add_target(63), std::range_error);
    REQUIRE(tool.get_targets().empty());

    model.add_virus(virus_b);
    REQUIRE(model.get_virus(0).get_lineage_id() == 0);
    REQUIRE(virus_b.get_lineage_id() == 1);

    tool.add_target(model.get_virus(0));
    REQUIRE(tool.get_targets() == std::vector< int >{0});
    REQUIRE(tool.targets(model.get_virus(0)));
    REQUIRE_FALSE(tool.targets(virus_b));

    // Rejected updates keep the previous targets
    REQUIRE_THROWS_AS(tool.set_targets({63}), std::range_error);
    REQUIRE_THROWS_AS(tool.set_targets({1, -1}), std::range_error);
    REQUIRE(tool.get_targets() == std::vector< int >{0});

    model.add_tool(tool);
    model.run(30, 1231);

    std::vector< int > date, id, counts;
    std::vector< std::string > state;
    model.get_db().get_hist_virus(date, id, state, counts);

    // Infected counts of each virus on the first and the last day
    int a_first = -1, a_last = -1, b_first = -1, b_last = -1;
    for (size_t i = 0u; i < date.size(); ++i)
    {
        if (state[i] != "Infected")
            continue;

        int & first = (id[i] == 0) ? a_first : b_first;
        int & last  = (id[i] == 0) ? a_last  : b_last;
        if (date[i] == 0)
            first = counts[i];
        if (date[i] == 30)
            last = counts[i];
    }

    REQUIRE(a_first > 0);
    REQUIRE(b_first > 0);

    // Everyone has the tool: A cannot spread, B is unaffected
    REQUIRE(a_last == a_first);
    REQUIRE(b_last > 5 * b_first);

    // Clearing the targets makes the tool act on every virus
    tool.clear_targets();
    REQUIRE(tool.get_targets().empty());
    REQUIRE(tool.targets(virus_b));

}
