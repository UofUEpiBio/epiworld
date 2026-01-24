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
        "test virus", 0.1, 0.3, 0.1
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

    // Should have data from all days (0 to ndays inclusive)
    REQUIRE(unique_dates.size() == static_cast<size_t>(ndays + 1));
    
    // Each date should have nstates entries (one per state)
    for (auto& p : counts_per_date) {
        REQUIRE(p.second == nstates);
    }
    
    // Total entries should be (ndays + 1) * nstates
    REQUIRE(virus_date.size() == static_cast<size_t>((ndays + 1) * nstates));

    // We now verify that the outbreak size matches the total infected count
    std::vector<int> total_date, total_counts;
    std::vector<std::string> total_state;
    model.get_db().get_hist_total(
        &total_date,
        &total_state,
        &total_counts
    );

    // Collapsing total infected counts per day
    std::vector<int> total_infected_per_day(ndays + 1, 0);
    auto total_active_cases = total_infected_per_day;
    for (size_t i = 0; i < total_date.size(); ++i)
    {
        if (
            (total_state[i] == "Infected") ||
            (total_state[i] == "Recovered")
        )
        {
            total_infected_per_day[total_date[i]] += total_counts[i];
        }

        if (total_state[i] == "Infected")
        {
            total_active_cases[total_date[i]] += total_counts[i];
        }

    }

    std::vector<int> exposed_date, exposed_virus_id, active_cases;
    model.get_db().get_active_cases(
        exposed_date,
        exposed_virus_id,
        active_cases
    );

    std::vector< int > outbreak_date, outbreak_virus_id, outbreak_size;
    model.get_db().get_outbreak_size(
        outbreak_date,
        outbreak_virus_id,
        outbreak_size
    );

    REQUIRE(outbreak_size == total_infected_per_day);
    REQUIRE(active_cases == total_active_cases);
}
