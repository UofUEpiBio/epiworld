#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test that GlobalEvent can modify model parameters
 * 
 * This test verifies that:
 * 1. A GlobalEvent can change the transmission rate to 0 on day 20
 * 2. After the intervention, transmission should stop completely
 * 
 * Note: We verify this by checking the transmission database, not just
 * the infected counts, since infected individuals remain in that state
 * until recovery.
 */
EPIWORLD_TEST_CASE("GlobalEvents - Parameter modification", "[globalevents][parameters]") {

    int nexperiments = 10;
    int ndays = 40;
    int intervention_day = 20;
    
    int total_transmissions_before = 0;
    int total_transmissions_after = 0;

    // Create a saver function to collect transmission data from each experiment
    auto saver = [&total_transmissions_before, &total_transmissions_after, intervention_day](
        size_t, Model<>* m) -> void {
        
        // Get transmission data
        std::vector<int> trans_date, trans_source, trans_target, trans_virus, trans_source_exposure;
        m->get_db().get_transmissions(
            trans_date, trans_source, trans_target, trans_virus, trans_source_exposure
        );
        
        // Count transmissions before and after intervention
        // Transmissions on the intervention day are counted as "before" since
        // the global event runs after update_state() on that day
        for (size_t i = 0; i < trans_date.size(); ++i) {
            if (trans_date[i] < intervention_day) {
                total_transmissions_before++;
            } else if (trans_date[i] > intervention_day) {
                total_transmissions_after++;
            } else if (trans_date[i] == intervention_day) {
                // Explicitly exclude transmissions on intervention_day from both counts,
                // as they may occur during the transition period (see comment above).
            }
        }
    };

    // Create a single SIR model
    epimodels::ModelSIR<> model(
        "virus", 0.05, 0.7, 0.15  // Parameters tuned for observable spread
    );

    model.agents_smallworld(1000, 8, false, 0.02);
    model.verbose_off();

    // Add a global event that sets transmission rate to 0 on the intervention day
    model.add_globalevent(
        [](Model<>* m) -> void {
            m->set_param("Transmission rate", 0.0);
        },
        "Stop transmission",
        intervention_day
    );

    // Run multiple experiments with the saver function
    model.run_multiple(ndays, nexperiments, 1000, saver, true, false, 1);

    #ifdef CATCH_CONFIG_MAIN
    // We should have transmissions before the intervention
    REQUIRE(total_transmissions_before > 0);
    
    // After setting transmission to 0, there should be NO transmissions
    REQUIRE(total_transmissions_after == 0);
    #else
    // Print for standalone execution
    printf_epiworld(
        "GlobalEvent parameter modification test:\n"
        "  Total transmissions before intervention (days 1-%d): %d\n"
        "  Total transmissions after intervention (days %d-%d): %d\n",
        intervention_day - 1,
        total_transmissions_before,
        intervention_day + 1,
        ndays,
        total_transmissions_after
    );
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
}
