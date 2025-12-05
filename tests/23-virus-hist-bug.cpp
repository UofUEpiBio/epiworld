#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Virus history bug", "[virus_hist]") {

    // Create a simple SIR model
    epimodels::ModelSIR<> model(
        "test virus", 10.0, 0.9, 0.3
    );

    model.seed(1231);
    model.agents_smallworld(1000, 4, false, 0.01);
    model.verbose_off();

    // Run for 10 days, 2 simulations with saver
    int ndays = 10;
    int nsims = 2;
    
    // Create a saver function
    auto saver = make_save_run<>(
        "/home/runner/work/epiworld/epiworld/tests/test-%i",  // fmt with %i for iteration number
        true,   // total_hist
        true,   // virus_info
        true,   // virus_hist
        false,  // tool_info
        false,  // tool_hist
        false,  // transmission
        false,  // transition
        false,  // reproductive
        false   // generation
    );
    
    model.run_multiple(ndays, nsims, 123, saver);

    // Write data to files (this should write the LAST simulation's data)
    model.write_data(
        "", // virus_info
        "test-virus-hist-final.csv", // virus_hist
        "", // tool_info
        "test-tool-hist-final.csv", // tool_hist
        "", // total_hist
        "", // transmission
        "", // transition
        "", // reproductive_number
        ""  // generation_time
    );

    // Get virus history
    std::vector<int> virus_date, virus_id, virus_counts;
    std::vector<std::string> virus_state;
    model.get_db().get_hist_virus(
        virus_date,
        virus_id,
        virus_state,
        virus_counts
    );

    // Get tool history for comparison
    std::vector<int> tool_date, tool_id, tool_counts;
    std::vector<std::string> tool_state;
    model.get_db().get_hist_tool(
        tool_date,
        tool_id,
        tool_state,
        tool_counts
    );

    int nstates = model.get_n_states();

    std::cout << "ndays: " << ndays << ", nsims: " << nsims << ", nstates: " << nstates << std::endl;
    std::cout << "Virus history size: " << virus_date.size() << std::endl;
    std::cout << "Tool history size: " << tool_date.size() << std::endl;

    // Expected: ndays * nsims * nstates entries per virus/tool
    // (assuming 1 virus and 0 tools in this model)
    // But the bug causes only nsims * nstates entries (only day 0)

    // Print some virus history data
    std::cout << "\nVirus history (first 20 entries):" << std::endl;
    for (size_t i = 0; i < std::min(size_t(20), virus_date.size()); ++i) {
        std::cout << "  date=" << virus_date[i] 
                  << " id=" << virus_id[i]
                  << " state=" << virus_state[i]
                  << " count=" << virus_counts[i] << std::endl;
    }

    // Count unique dates in virus history
    std::set<int> unique_virus_dates(virus_date.begin(), virus_date.end());
    std::cout << "\nUnique virus history dates: ";
    for (auto d : unique_virus_dates) {
        std::cout << d << " ";
    }
    std::cout << "(" << unique_virus_dates.size() << " unique dates)" << std::endl;

    // Count unique dates in tool history
    std::set<int> unique_tool_dates(tool_date.begin(), tool_date.end());
    std::cout << "\nUnique tool history dates: ";
    for (auto d : unique_tool_dates) {
        std::cout << d << " ";
    }
    std::cout << "(" << unique_tool_dates.size() << " unique dates)" << std::endl;

    #ifdef CATCH_CONFIG_MAIN
    // Tool history should have data from all days
    REQUIRE(unique_tool_dates.size() > 1);
    
    // Virus history should also have data from all days (this is the bug)
    REQUIRE(unique_virus_dates.size() > 1);
    
    // Both should have similar number of unique dates
    REQUIRE(unique_virus_dates.size() == unique_tool_dates.size());
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
