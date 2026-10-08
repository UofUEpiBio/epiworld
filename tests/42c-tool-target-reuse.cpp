#include "tests.hpp"

using namespace epiworld;

// A virus added to a second model starts a lineage of that model
EPIWORLD_TEST_CASE("Tool targeting a virus reused across models", "[tool][target]") {

    Virus<> virus_a("A", 0.01, true);
    Virus<> virus_b("B", 0.01, true);
    for (auto * v : {&virus_a, &virus_b})
    {
        v->set_state(1, 2, 2);
        v->set_prob_infecting("Transmission rate");
        v->set_prob_recovery("Recovery rate");
    }

    virus_a.set_sequence(100);
    virus_b.set_sequence(200);

    // No recovery: infected agents stay infected
    epimodels::ModelSIRCONN<> first("C", 1000, 0.0, 4.0, 0.5, 0.0);
    epimodels::ModelSIRCONN<> second("C", 1000, 0.0, 4.0, 0.5, 0.0);
    first.verbose_off();
    second.verbose_off();

    // Registered in a different order in each model
    first.add_virus(virus_a);
    REQUIRE(virus_a.get_lineage_id() == 1);

    second.add_virus(virus_b);
    second.add_virus(virus_a);
    REQUIRE(virus_b.get_lineage_id() == 1);
    REQUIRE(virus_a.get_lineage_id() == 2);
    REQUIRE(second.get_virus(2).get_lineage_id() == 2);

    // Adding a virus twice keeps its record and lineage
    first.add_virus(virus_a);
    REQUIRE(virus_a.get_id() == 1);
    REQUIRE(virus_a.get_lineage_id() == 1);
    REQUIRE(first.get_db().get_n_viruses() == 2u);

    Tool<> tool("Vaccine against A", 1.0, true);
    tool.set_susceptibility_reduction(1.0);
    tool.add_target(second.get_virus(2));
    REQUIRE_FALSE(tool.targets(second.get_virus(1)));
    second.add_tool(tool);

    second.run(30, 1231);

    std::vector< int > date, id, counts;
    std::vector< std::string > state;
    second.get_db().get_hist_virus(date, id, state, counts);

    // Infected counts of each virus on the first and the last day
    int a_first = -1, a_last = -1, b_first = -1, b_last = -1;
    for (size_t i = 0u; i < date.size(); ++i)
    {
        if ((state[i] != "Infected") || (id[i] == 0))
            continue;

        int & first_count = (id[i] == 2) ? a_first : b_first;
        int & last_count  = (id[i] == 2) ? a_last  : b_last;
        if (date[i] == 0)
            first_count = counts[i];
        if (date[i] == 30)
            last_count = counts[i];
    }

    REQUIRE(a_first > 0);
    REQUIRE(b_first > 0);

    // Everyone has the tool: A cannot spread, B is unaffected
    REQUIRE(a_last == a_first);
    REQUIRE(b_last > 5 * b_first);

}
