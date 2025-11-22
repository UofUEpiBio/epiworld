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
 * 2. After the intervention, transmission should stop (or be minimal)
 * 
 * Note: We verify this by checking the transmission database, not just
 * the infected counts, since infected individuals remain in that state
 * until recovery.
 */
EPIWORLD_TEST_CASE("GlobalEvents - Parameter modification", "[globalevents][parameters]") {

    // Threshold for validating transmission reduction after intervention
    const double TRANSMISSION_REDUCTION_THRESHOLD = 0.05;

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
    }

    #ifdef CATCH_CONFIG_MAIN
    // We should have transmissions before the intervention
    REQUIRE(total_transmissions_before > 0);
    
    // After setting transmission to 0, there should be NO or very few transmissions
    // We use a threshold to allow for any edge cases
    REQUIRE(total_transmissions_after < static_cast<int>(total_transmissions_before * TRANSMISSION_REDUCTION_THRESHOLD));
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
