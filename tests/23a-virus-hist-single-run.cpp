#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Virus history over multiple days", "[virus_hist]") {

    // Create a simple SIR model
    epimodels::ModelSIR<> model(
        "test virus", 0.5, 0.9, 0.1
    );

    model.seed(1231);
    model.agents_smallworld(100, 4, false, 0.01);
    model.verbose_off();

    // Run for 20 days (single simulation)
    model.run(20, 123);

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

    std::cout << "ndays: 20, nstates: " << nstates << std::endl;
    std::cout << "Virus history size: " << virus_date.size() << std::endl;

    // Count unique dates
    std::set<int> unique_dates(virus_date.begin(), virus_date.end());
    std::cout << "Unique dates: ";
    for (auto d : unique_dates) {
        std::cout << d << " ";
    }
    std::cout << "(" << unique_dates.size() << " unique)" << std::endl;

    // Check counts per date
    std::map<int, int> counts_per_date;
    for (auto d : virus_date) {
        counts_per_date[d]++;
    }
    
    std::cout << "\nEntries per date:" << std::endl;
    for (auto& p : counts_per_date) {
        std::cout << "  Date " << p.first << ": " << p.second << " entries" << std::endl;
    }

    #ifdef CATCH_CONFIG_MAIN
    // Should have data from multiple days (0 to 20)
    REQUIRE(unique_dates.size() > 1);
    REQUIRE(unique_dates.size() == 21);  // Days 0-20
    
    // Each date should have nstates entries (one per state)
    for (auto& p : counts_per_date) {
        REQUIRE(p.second == nstates);
    }
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
