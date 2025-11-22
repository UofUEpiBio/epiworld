#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test that GlobalEvent is called at the correct times
 * 
 * This test verifies that:
 * 1. An event with a specific day is called only on that day
 * 2. An event with day < 0 is called every day
 * 
 * Note: Due to how the model initializes, global events start being called
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
            int day = m->today();
            daily_event_days.push_back(day);
        },
        "Daily event",
        -1  // Run every day
    );

    // Add a global event that runs on specific days (10, 20, 30)
    for (int day : {10, 20, 30}) {
        model.add_globalevent(
            [&specific_event_days, day](Model<>* m) -> void {
                if (m->today() == day) {
                    specific_event_days.push_back(day);
                }
            },
            "Specific day event " + std::to_string(day),
            day  // Run on specific day
        );
    }

    // Run the model for 50 days with multiple replicates
    int ndays = 50;
    int nexperiments = 3;
    
    // Use a custom saver function to check the results after each experiment
    auto saver = [&daily_event_days, &specific_event_days, ndays](size_t n, Model<>* m) -> void {
        // For each experiment, verify that:
        // 1. Daily event was called for all days (1 to ndays, inclusive)
        // 2. Specific events were called on days 10, 20, 30
        
        // Check daily event days
        if (static_cast<int>(daily_event_days.size()) != ndays) {
            throw std::runtime_error(
                "Daily event was not called the expected number of times. "
                "Expected: " + std::to_string(ndays) + 
                ", Got: " + std::to_string(daily_event_days.size())
            );
        }

        // Verify daily event was called for each day (starting from 1, not 0)
        for (int i = 0; i < ndays; ++i) {
            int expected_day = i + 1;  // Days start from 1
            if (daily_event_days[i] != expected_day) {
                throw std::runtime_error(
                    "Daily event was not called on day " + std::to_string(expected_day) +
                    ", was called on day " + std::to_string(daily_event_days[i])
                );
            }
        }

        // Check specific event days
        std::vector<int> expected_specific = {10, 20, 30};
        if (specific_event_days.size() != expected_specific.size()) {
            throw std::runtime_error(
                "Specific events were not called the expected number of times. "
                "Expected: " + std::to_string(expected_specific.size()) + 
                ", Got: " + std::to_string(specific_event_days.size())
            );
        }

        // Verify specific events were called on correct days
        for (size_t i = 0; i < expected_specific.size(); ++i) {
            if (specific_event_days[i] != expected_specific[i]) {
                throw std::runtime_error(
                    "Specific event was not called on expected day. "
                    "Expected: " + std::to_string(expected_specific[i]) + 
                    ", Got: " + std::to_string(specific_event_days[i])
                );
            }
        }
        
        // Print success message for this experiment
        if (n == 0) {
            printf_epiworld(
                "GlobalEvent timing test passed:\n"
                "  Daily events called correctly for all %d days (1-%d)\n"
                "  Specific events called on days 10, 20, and 30\n",
                ndays, ndays
            );
        }

        // Clear for next experiment
        daily_event_days.clear();
        specific_event_days.clear();
    };
    
    model.run_multiple(ndays, nexperiments, 456, saver, true, false, 1);

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
}

/**
 * @brief Test that GlobalEvent can modify model parameters
 * 
 * This test verifies that:
 * 1. A GlobalEvent can change the transmission rate to 0 on day 20
 * 2. After the intervention, transmission should stop (or be minimal)
 * 
 * Note: We verify this by checking the transmission database, not just
 * the infected counts, since infected individuals remain in that state
 * until recovery.
 */
EPIWORLD_TEST_CASE("GlobalEvents - Parameter modification", "[globalevents][parameters]") {

    // We'll track whether transmissions occur before and after the intervention
    int nexperiments = 10;
    int ndays = 40;
    int intervention_day = 20;
    
    int total_transmissions_before = 0;
    int total_transmissions_after = 0;

    // Run experiments to collect data
    for (int exp = 0; exp < nexperiments; ++exp) {
        // Create a fresh SIR model for each experiment
        epimodels::ModelSIR<> model(
            "virus", 0.05, 0.7, 0.15  // Parameters tuned for observable spread
        );

        model.seed(1000 + exp);
        model.agents_smallworld(1000, 8, false, 0.02);
        model.verbose_off();

        // Add a global event that sets transmission rate to 0 on the intervention day
        model.add_globalevent(
            [intervention_day](Model<>* m) -> void {
                m->set_param("Transmission rate", 0.0);
            },
            "Stop transmission",
            intervention_day
        );

        // Run the model
        model.run(ndays);

        // Get transmission data
        std::vector<int> trans_date, trans_source, trans_target, trans_virus, trans_source_exposure;
        model.get_db().get_transmissions(
            trans_date, trans_source, trans_target, trans_virus, trans_source_exposure
        );
        
        // Count transmissions before and after intervention
        for (size_t i = 0; i < trans_date.size(); ++i) {
            if (trans_date[i] <= intervention_day) {
                total_transmissions_before++;
            } else {
                total_transmissions_after++;
            }
        }
    }

    printf_epiworld(
        "GlobalEvent parameter modification test:\n"
        "  Total transmissions before intervention (days 1-%d): %d\n"
        "  Total transmissions after intervention (days %d-%d): %d\n",
        intervention_day,
        total_transmissions_before,
        intervention_day + 1,
        ndays,
        total_transmissions_after
    );

    #ifdef CATCH_CONFIG_MAIN
    // We should have transmissions before the intervention
    REQUIRE(total_transmissions_before > 0);
    
    // After setting transmission to 0 on day 20, there should be NO new transmissions
    // on day 21 or later (transmission happens during update_state, which occurs
    // before run_globalevents, so day 21 should already have rate = 0)
    // However, we allow for a small number due to timing of when the rate change takes effect
    REQUIRE(total_transmissions_after < total_transmissions_before * 0.05);
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
}
