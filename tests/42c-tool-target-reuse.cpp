#include "tests.hpp"

using namespace epiworld;

// A virus added to a second model starts a lineage of that model
EPIWORLD_TEST_CASE("Tool targeting a virus reused across models", "[tool][target]") {

    Virus<> virus_a = sir_virus("A");
    Virus<> virus_b = sir_virus("B");

    // No recovery: infected agents stay infected
    epimodels::ModelSIR<> first("C", 1000, 0.0, 0.5, 0.0);
    add_random_mixing(first, 4.0);
    epimodels::ModelSIR<> second("C", 1000, 0.0, 0.5, 0.0);
    add_random_mixing(second, 4.0);
    first.verbose_off();
    second.verbose_off();

    // Registered in a different order in each model
    // (ids and lineages are recorded on each model's own copy)
    first.add_virus(virus_a, assign::share(0.01));
    REQUIRE(first.get_virus(1).get_lineage_id() == 1);

    second.add_virus(virus_b, assign::share(0.01));
    second.add_virus(virus_a, assign::share(0.01));
    REQUIRE(second.get_virus(1).get_lineage_id() == 1);
    REQUIRE(second.get_virus(2).get_lineage_id() == 2);

    // Adding a virus twice records two lineages
    first.add_virus(virus_a);
    REQUIRE(first.get_virus(2).get_id() == 2);
    REQUIRE(first.get_virus(2).get_lineage_id() == 2);
    REQUIRE(first.get_db().get_n_viruses() == 3u);

    Tool<> tool("Vaccine against A");
    tool.set_susceptibility_reduction(1.0);
    tool.add_target(second.get_virus(2));
    REQUIRE_FALSE(tool.targets(second.get_virus(1)));
    second.add_tool(tool, assign::share(1.0));

    second.run(30, 1231);

    std::vector< int > date, id, counts;
    std::vector< std::string > status, state;
    second.get_db().get_hist_virus(date, id, status, state, counts);

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

    // A removed virus keeps its record: the others keep their ids, and a
    // virus added later gets a new one
    {
        epimodels::ModelSIR<> model("C", 500, 0.0, 0.5, 0.0);
        add_random_mixing(model, 4.0);
        model.verbose_off();
        model.add_virus(virus_a, assign::share(0.02));
        model.add_virus(virus_b, assign::share(0.02));
        model.rm_virus(1u);                    // A; B moves to position 1
        REQUIRE(model.get_virus(1).get_id() == 2);
        REQUIRE_NOTHROW(model.run(5, 1));

        model.add_virus(sir_virus("D"), assign::share(0.02));
        REQUIRE(model.get_virus(2).get_id() == 3);
        REQUIRE_NOTHROW(model.run(5, 1));
        REQUIRE(model.get_db().get_n_viruses() == 4u);

        // B and D infect agents under their own ids
        std::vector< int > date, id, counts;
        std::vector< std::string > status, state;
        model.get_db().get_hist_virus(date, id, status, state, counts);
        int b0 = 0, d0 = 0, a0 = 0;
        for (size_t k = 0u; k < date.size(); ++k)
            if ((date[k] == 0) && (state[k] == "Infected"))
            {
                if (id[k] == 1) a0 += counts[k];
                if (id[k] == 2) b0 += counts[k];
                if (id[k] == 3) d0 += counts[k];
            }
        REQUIRE(a0 == 0);
        REQUIRE(b0 > 0);
        REQUIRE(d0 > 0);
    }

}
