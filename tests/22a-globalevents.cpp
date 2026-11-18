#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test that GlobalEvent is called at the correct times
 * 
 * This test verifies that:
 * 1. An event with a specific day is called only on that day
 * 2. An event with day < 0 is called every day
 * 
 * Note: Due to how the model initialize, global events start being called
 * from day 1 (not day 0). Day 0 is the initial state before any updates.
 */
EPIWORLD_TEST_CASE("GlobalEvents - Event timing", "[globalevents][timing]") {

    // Create vectors to store the days when events are called
    std::vector<int> daily_event_days;
    std::vector<int> specific_event_days;

    // Create a simple SIR model
    epimodels::ModelSIR model(
        "virus", 1000, 0.01, 0.5, 0.3
    );

    model.seed(123);
    model.add_layer(layer::network(
        "network", rgraph_smallworld(1000, 5, 0.01, false, model)
    ));
    model.verbose_off();

    // Add a global event that runs every day (day = -1)
    model.add_globalevent(
        [&daily_event_days](Model* m) -> void {
            daily_event_days.push_back(m->today());
        },
        "Daily event",
        -1  // Run every day
    );

    // Add a global event that runs on specific days (10, 20, 30)
    for (int day : {10, 20, 30}) {
        model.add_globalevent(
            [&specific_event_days](Model* m) -> void {
                specific_event_days.push_back(m->today());
            },
            "Specific day event " + std::to_string(day),
            day  // Run on specific day
        );
    }

    // Run the model for 50 days (deterministic with fixed seed)
    int ndays = 50;
    model.run(ndays);

    // Verify daily event was called for all days (1 to ndays, inclusive)
    REQUIRE(static_cast<int>(daily_event_days.size()) == ndays);
    
    // Verify daily event was called for each day (starting from 1, not 0)
    for (int i = 0; i < ndays; ++i) {
        int expected_day = i + 1;  // Days start from 1
        REQUIRE(daily_event_days[i] == expected_day);
    }

    // Verify specific events were called exactly 3 times
    REQUIRE(specific_event_days.size() == 3);
    
    // Verify specific events were called on correct days
    std::vector<int> expected_specific = {10, 20, 30};
    REQUIRE_THAT(specific_event_days, Catch::Equals(expected_specific));

    // A global event gives agents the model's copy of a tool; the caller's
    // copy is not in the model (only the model's has an id there), so giving
    // it is an error, not a crash
    epimodels::ModelSIR vax_model("virus", 1000, 0.01, 0.5, 0.3);
    vax_model.add_layer(layer::network("network", graph::smallworld(5, 0.01)));
    vax_model.verbose_off();

    Tool vax("Vaccine");
    vax.set_susceptibility_reduction(0.9);
    vax_model.add_tool(vax);
    REQUIRE(vax.get_id() < 0);
    REQUIRE(vax_model.get_tool(0).get_id() == 0);

    GlobalEvent model_copy(
        epimodels::globalevent_tool(vax_model.get_tool(0), 0.5), "Vaccinate", 5
    );
    vax_model.add_globalevent(model_copy);
    vax_model.run(10, 123);

    std::vector< int > tdate, tid, tcounts;
    std::vector< std::string > tstatus, tstage;
    vax_model.get_db().get_hist_tool(tdate, tid, tstatus, tstage, tcounts);
    int holders = 0;
    for (size_t i = 0u; i < tdate.size(); ++i)
        if (tdate[i] == 10)
            holders += tcounts[i];
    REQUIRE(holders > 300);

    vax_model.rm_globalevent("Vaccinate");
    GlobalEvent caller_copy(epimodels::globalevent_tool(vax, 0.5), "Vaccinate", 5);
    vax_model.add_globalevent(caller_copy);
    REQUIRE_THROWS_WITH(
        vax_model.run(10, 123),
        Catch::Matchers::Contains("give agents the model's copy")
    );

    // The same for a virus
    vax_model.rm_globalevent("Vaccinate");
    Virus outside("Outside");
    vax_model.add_globalevent(
        [&outside](Model * m) { m->get_agent(0).set_virus(*m, outside); },
        "Import",
        5
    );
    REQUIRE_THROWS_WITH(
        vax_model.run(10, 123),
        Catch::Matchers::Contains("is not in the model")
    );
}
