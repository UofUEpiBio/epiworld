#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test virus history recording across multiple days
 * 
 * This test validates that virus history is correctly recorded for all days
 * of a simulation, not just day 0. This addresses issue about virus history
 * only saving the first day.
 */
EPIWORLD_TEST_CASE("Virus history across multiple days", "[virus_hist]") {

    // Create a simple SIR model
    epimodels::ModelSIR<> model(
        "test virus", 0.5, 0.9, 0.1
    );

    model.seed(1231);
    model.agents_smallworld(100, 4, false, 0.01);
    model.verbose_off();

    // Run for 20 days (single simulation)
    int ndays = 20;
    model.run(ndays, 123);

    // Get virus history
    std::vector<int> virus_date, virus_id, virus_counts;
    std::vector<std::string> virus_state;
    model.get_db().get_hist_virus(
        virus_date,
        virus_id,
        virus_state,
        virus_counts
    );

    int nstates = model.get_n_states();

    // Count unique dates
    std::set<int> unique_dates(virus_date.begin(), virus_date.end());
    
    // Count entries per date
    std::map<int, int> counts_per_date;
    for (auto d : virus_date) {
        counts_per_date[d]++;
    }

    std::cout << "Single run test:" << std::endl;
    std::cout << "  ndays=" << ndays << ", nstates=" << nstates << std::endl;
    std::cout << "  Virus history size: " << virus_date.size() << std::endl;
    std::cout << "  Unique dates: " << unique_dates.size() << " (expected " << (ndays + 1) << ")" << std::endl;

    #ifdef CATCH_CONFIG_MAIN
    // Should have data from all days (0 to ndays inclusive)
    REQUIRE(unique_dates.size() == static_cast<size_t>(ndays + 1));
    
    // Each date should have nstates entries (one per state)
    for (auto& p : counts_per_date) {
        REQUIRE(p.second == nstates);
    }
    
    // Total entries should be (ndays + 1) * nstates
    REQUIRE(virus_date.size() == static_cast<size_t>((ndays + 1) * nstates));
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
