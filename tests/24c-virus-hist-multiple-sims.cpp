#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test virus history recording with run_multiple
 * 
 * This test validates that virus history is correctly recorded when using
 * run_multiple() with a saver function. Each simulation should save complete
 * history (all days) to separate files.
 */
EPIWORLD_TEST_CASE("Virus history with run_multiple", "[virus_hist][run_multiple]") {

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
    
    // Create a saver function that writes virus history
    const char* test_prefix = "24c-virus-hist-multiple-sims-saves/run_multiple_test-%i";
    auto saver = make_save_run<>(
        test_prefix,
        false,  // total_hist
        false,  // virus_info
        true,   // virus_hist
        false,  // tool_info
        false,  // tool_hist
        false,  // transmission
        false,  // transition
        false,  // reproductive
        false,  // generation
        false,  // active_cases
        false,  // outbreak_size
        false   // hospitalizations
    );
    
    model.run_multiple(ndays, nsims, 123, saver);

    int nstates = model.get_n_states();
    
    std::cout << "\nrun_multiple test:" << std::endl;
    std::cout << "  ndays=" << ndays << ", nsims=" << nsims << ", nstates=" << nstates << std::endl;

    // Check each simulation's output file
    for (int sim = 0; sim < nsims; ++sim) {
        std::string filename = std::string("24c-virus-hist-multiple-sims-saves/run_multiple_test-") + 
                               std::to_string(sim) + "_virus_hist.csv";
        
        std::ifstream file(filename);
        if (!file.good()) {
            std::cout << "  ERROR: File " << filename << " not found!" << std::endl;
            #ifdef CATCH_CONFIG_MAIN
            REQUIRE(false);
            #endif
            continue;
        }
        
        // Count lines in file (excluding header)
        int line_count = 0;  // Counter for data lines (header already read separately)
        std::string line;
        std::set<int> dates_in_file;
        
        // Read header
        std::getline(file, line);
        
        // Read data lines
        while (std::getline(file, line)) {
            line_count++;
            // Extract date (first column after thread column in debug mode)
            std::istringstream iss(line);
            #ifdef EPI_DEBUG
            int thread_id, date;
            iss >> thread_id >> date;
            #else
            int date;
            iss >> date;
            #endif
            dates_in_file.insert(date);
        }
        file.close();
        
        std::cout << "  Simulation " << sim << ":" << std::endl;
        std::cout << "    Lines in file: " << line_count << " (expected " << ((ndays + 1) * nstates) << ")" << std::endl;
        std::cout << "    Unique dates: " << dates_in_file.size() << " (expected " << (ndays + 1) << ")" << std::endl;
        
        #ifdef CATCH_CONFIG_MAIN
        // Each file should have (ndays + 1) * nstates lines
        REQUIRE(line_count == (ndays + 1) * nstates);
        
        // Should have all dates from 0 to ndays
        REQUIRE(dates_in_file.size() == static_cast<size_t>(ndays + 1));
        REQUIRE(*dates_in_file.begin() == 0);
        REQUIRE(*dates_in_file.rbegin() == ndays);
        #endif
    }

    // Also check that the model's current database only has the LAST simulation's data
    std::vector<int> virus_date, virus_id, virus_counts;
    std::vector<std::string> virus_state;
    model.get_db().get_hist_virus(
        virus_date,
        virus_id,
        virus_state,
        virus_counts
    );
    
    std::cout << "  Final database state:" << std::endl;
    std::cout << "    Virus history size: " << virus_date.size() 
              << " (expected " << ((ndays + 1) * nstates) << " - last simulation only)" << std::endl;

    #ifdef CATCH_CONFIG_MAIN
    // The database should only contain the last simulation's data
    REQUIRE(virus_date.size() == static_cast<size_t>((ndays + 1) * nstates));
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
