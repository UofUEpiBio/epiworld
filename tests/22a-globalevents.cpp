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
    epimodels::ModelSIR<> model(
        "virus", 0.01, 0.5, 0.3
    );

    model.seed(123);
    model.agents_smallworld(1000, 5, false, 0.01);
    model.verbose_off();

    // Add a global event that runs every day (day = -1)
    model.add_globalevent(
        [&daily_event_days](Model<>* m) -> void {
            daily_event_days.push_back(m->today());
        },
        "Daily event",
        -1  // Run every day
    );

    // Add a global event that runs on specific days (10, 20, 30)
    for (int day : {10, 20, 30}) {
        model.add_globalevent(
            [&specific_event_days](Model<>* m) -> void {
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
}
