#include "tests.hpp"

using namespace epiworld;

// A tool that targets one disease does not act on another
EPIWORLD_TEST_CASE("Tool targeting a virus", "[tool][target]") {

    // No recovery: infected agents stay infected
    epimodels::ModelSIR model("A", 1000, 0.01, 0.5, 0.0);
    add_random_mixing(model, 4.0);
    model.verbose_off();

    // A virus built from scratch is a disease of its own
    Virus virus_b = sir_virus("B");
    model.add_virus(virus_b, assign::share(0.01));
    REQUIRE(model.get_diseases() == std::vector< std::string >{"A", "B"});

    Tool tool("Vaccine against A");
    tool.set_susceptibility_reduction(1.0);
    tool.add_target("A");
    tool.add_target("A");
    REQUIRE(tool.get_targets() == std::vector< std::string >{"A"});
    model.add_tool(tool, assign::share(1.0));

    // A global event records, each day, the date and the agents infected
    model.set_user_data({"Infected", "Day"});
    model.add_globalevent([](Model * m) -> void {
        m->add_user_data(0u, m->get_db().get_today_total_stage("Infected", 0));
        m->add_user_data(1u, static_cast< epiworld_double >(m->today()));
    }, "Record infected");

    model.run(30, 1231);

    std::vector< int > date, id, counts;
    std::vector< std::string > status, state;
    model.get_db().get_hist_virus(date, id, status, state, counts);

    // Everyone holds the tool on every day
    std::vector< int > tool_date, tool_id, tool_counts;
    std::vector< std::string > tool_state, tool_status;
    model.get_db().get_hist_tool(tool_date, tool_id, tool_status, tool_state, tool_counts, 0);

    std::vector< int > holders(31u, 0);
    for (size_t i = 0u; i < tool_date.size(); ++i)
    {
        REQUIRE(tool_id[i] == 0);
        holders[tool_date[i]] += tool_counts[i];
    }
    REQUIRE(holders == std::vector< int >(31u, 1000));

    // One user-data row per day, matching the daily totals
    std::vector< int > total_date, total_counts;
    std::vector< std::string > total_state;
    model.get_db().get_hist_total(&total_date, nullptr, &total_state, &total_counts, 0);

    auto & user_data = model.get_user_data();
    REQUIRE(user_data.ncol() == 2u);
    REQUIRE(user_data.nrow() == 30u);
    for (size_t i = 0u; i < total_date.size(); ++i)
    {
        if (total_state[i] != "Infected" || total_date[i] == 0)
            continue;

        size_t row = static_cast< size_t >(total_date[i] - 1);
        REQUIRE(user_data.get_dates()[row] == total_date[i]);
        REQUIRE(user_data(row, 0u) == total_counts[i]);
        REQUIRE(user_data(row, "Day") == total_date[i]);
    }

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

    // The model's tool acts on A only; without targets, a model with two
    // diseases is an error
    REQUIRE(model.get_tool(0u).targets(model.get_virus(0)));
    REQUIRE_FALSE(model.get_tool(0u).targets(model.get_virus(1)));
    model.get_tool(0u).clear_targets();
    REQUIRE(model.get_tool(0u).get_targets().empty());
    REQUIRE_THROWS_AS(model.run(1, 1), std::logic_error);

}
