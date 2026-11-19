#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test virus history recording across multiple days
 * 
 * This test validates that virus history is correctly recorded for all days
 * of a simulation, not just day 0. This addresses issue about virus history
 * only saving the first day.
 */

/**
 * @brief Test CSV output format for all data types
 * 
 * This test verifies that all CSV outputs have proper quoting for string fields,
 * which is necessary for correct parsing by data.table::fread in R.
 * 
 * All string fields (virus names, tool names, statuses and stages) should be
 * quoted.
 */

/**
 * @brief Test virus history recording with run_multiple
 * 
 * This test validates that virus history is correctly recorded when using
 * run_multiple() with a saver function. Each simulation should save complete
 * history (all days) to separate files.
 */

// Virus history: single run, CSV format and multiple simulations
//
// One test case; each former test file is a block below, with the name it had.
EPIWORLD_TEST_CASE("Virus history: single run, CSV format and multiple simulations", "[virus_hist][csv][output][run_multiple]") {

    // ---- 24a-virus-hist-single-run.cpp: Virus history across multiple days
    {

    // Create a simple SIR model
    epimodels::ModelSIR model(
        "test virus", 100, 0.1, 0.3, 0.1
    );

    model.seed(1231);
    model.add_layer(layer::network(
        "network", rgraph_smallworld(100, 4, 0.01, false, model)
    ));
    model.verbose_off();

    // Run for 20 days (single simulation)
    int ndays = 20;
    model.run(ndays, 123);

    // Get virus history
    std::vector<int> virus_date, virus_id, virus_counts;
    std::vector<std::string> virus_state, virus_status;
    model.get_db().get_hist_virus(
        virus_date,
        virus_id,
        virus_status,
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
        nullptr,
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

    // ---- 24b-csv-output-format.cpp: CSV output format validation
    {

    // Create a model with both virus and tools
    epimodels::ModelSEIR model(
        "test virus", 100, 0.1, 0.5, 3.0, 7.0
    );
    add_random_mixing(model, 5.0);
    
    model.verbose_off();
    model.run(10, 123);

    // Write all data types to files
    auto fn = epi_temp_file("24b-csv-output-format", "csv");
    model.write_data(
        fn.directory + "/csv_test_virus_info.csv",
        fn.directory + "/csv_test_virus_hist.csv",
        fn.directory + "/csv_test_tool_info.csv",
        fn.directory + "/csv_test_tool_hist.csv",
        fn.directory + "/csv_test_total_hist.csv",
        fn.directory + "/csv_test_transmission.csv",
        fn.directory + "/csv_test_transition.csv",
        fn.directory + "/csv_test_reproductive.csv",
        fn.directory + "/csv_test_generation.csv",
        fn.directory + "/csv_test_active_cases.csv",
        fn.directory + "/csv_test_outbreak_size.csv",
        ""
    );

    std::cout << "\nCSV Format Validation Test:" << std::endl;
    std::cout << "============================" << std::endl;

    // Test 1: virus_hist - should have quotes around the disease and virus name, status and stage
    {
        std::ifstream file(fn.directory + "/csv_test_virus_hist.csv");
        std::string header, line;
        std::getline(file, header);
        std::getline(file, line);
        
        std::cout << "\n1. virus_hist format:" << std::endl;
        std::cout << "   Header: " << header << std::endl;
        std::cout << "   Sample: " << line << std::endl;
        
        // Count quotes - should have exactly 8 (disease, virus, status and stage)
        int quote_count = std::count(line.begin(), line.end(), '"');
        std::cout << "   Quote count: " << quote_count << " (expected: 8)" << std::endl;
        
        REQUIRE(quote_count == 8);
    }

    // Test 2: virus_info - should have quotes around the disease and virus name
    {
        std::ifstream file(fn.directory + "/csv_test_virus_info.csv");
        std::string header, line;
        std::getline(file, header);
        std::getline(file, line);
        
        std::cout << "\n2. virus_info format:" << std::endl;
        std::cout << "   Header: " << header << std::endl;
        std::cout << "   Sample: " << line << std::endl;
        
        // Quotes around the disease and the virus name (4)
        int quote_count = std::count(line.begin(), line.end(), '"');
        std::cout << "   Quote count: " << quote_count << " (expected: 4)" << std::endl;
        
        REQUIRE(quote_count == 4);
    }

    // Test 3: tool_hist - should have quotes around the disease and status and stage
    {
        std::ifstream file(fn.directory + "/csv_test_tool_hist.csv");
        if (file.good()) {
            std::string header, line;
            std::getline(file, header);
            if (std::getline(file, line)) {
                std::cout << "\n3. tool_hist format:" << std::endl;
                std::cout << "   Header: " << header << std::endl;
                std::cout << "   Sample: " << line << std::endl;
                
                // Should have quotes around status and stage (4 quotes)
                int quote_count = std::count(line.begin(), line.end(), '"');
                std::cout << "   Quote count: " << quote_count << " (expected: 6)" << std::endl;
                
                REQUIRE(quote_count == 6);
            } else {
                std::cout << "\n3. tool_hist: No data (model has no tools)" << std::endl;
            }
        }
    }

    // Test 4: total_hist - should have quotes around the disease and status and stage
    {
        std::ifstream file(fn.directory + "/csv_test_total_hist.csv");
        std::string header, line;
        std::getline(file, header);
        std::getline(file, line);
        
        std::cout << "\n4. total_hist format:" << std::endl;
        std::cout << "   Header: " << header << std::endl;
        std::cout << "   Sample: " << line << std::endl;
        
        // Should have quotes around status and stage (4 quotes)
        int quote_count = std::count(line.begin(), line.end(), '"');
        std::cout << "   Quote count: " << quote_count << " (expected: 6)" << std::endl;
        
        REQUIRE(quote_count == 6);
    }

    // Test 5: transmission - should have quotes around the disease and the virus and layer names
    {
        std::ifstream file(fn.directory + "/csv_test_transmission.csv");
        std::string header, line;
        std::getline(file, header);
        if (std::getline(file, line)) {
            std::cout << "\n5. transmission format:" << std::endl;
            std::cout << "   Header: " << header << std::endl;
            std::cout << "   Sample: " << line << std::endl;
            
            // Should have quotes around the virus and layer names (4 quotes)
            int quote_count = std::count(line.begin(), line.end(), '"');
            std::cout << "   Quote count: " << quote_count << " (expected: 6)" << std::endl;
            
            REQUIRE(quote_count == 6);
        } else {
            std::cout << "\n5. transmission: No transmissions recorded" << std::endl;
        }
    }

    // Test 6: transition - should have quotes around the disease and the statuses and stages (from and to)
    {
        std::ifstream file(fn.directory + "/csv_test_transition.csv");
        std::string header, line;
        std::getline(file, header);
        if (std::getline(file, line)) {
            std::cout << "\n6. transition format:" << std::endl;
            std::cout << "   Header: " << header << std::endl;
            std::cout << "   Sample: " << line << std::endl;
            
            // Should have quotes around from and to states (4 quotes)
            int quote_count = std::count(line.begin(), line.end(), '"');
            std::cout << "   Quote count: " << quote_count << " (expected: 10)" << std::endl;
            
            REQUIRE(quote_count == 10);
        } else {
            std::cout << "\n6. transition: No transitions recorded" << std::endl;
        }
    }

    // Test 7: reproductive_number - should have quotes around the disease and virus name
    {
        std::ifstream file(fn.directory + "/csv_test_reproductive.csv");
        std::string header, line;
        std::getline(file, header);
        if (std::getline(file, line)) {
            std::cout << "\n7. reproductive_number format:" << std::endl;
            std::cout << "   Header: " << header << std::endl;
            std::cout << "   Sample: " << line << std::endl;
            
            // Should have quotes around virus name (2 quotes)
            int quote_count = std::count(line.begin(), line.end(), '"');
            std::cout << "   Quote count: " << quote_count << " (expected: 4)" << std::endl;
            
            REQUIRE(quote_count == 4);
        } else {
            std::cout << "\n7. reproductive_number: No data" << std::endl;
        }
    }

    // Test 8: generation_time - the disease is its only string field
    {
        std::ifstream file(fn.directory + "/csv_test_generation.csv");
        std::string header, line;
        std::getline(file, header);
        if (std::getline(file, line)) {
            std::cout << "\n8. generation_time format:" << std::endl;
            std::cout << "   Header: " << header << std::endl;
            std::cout << "   Sample: " << line << std::endl;
            std::cout << "   (Only the disease is a string)" << std::endl;
        } else {
            std::cout << "\n8. generation_time: No data" << std::endl;
        }
    }

    std::cout << "\nAll CSV formats validated!" << std::endl;
    }

    // ---- 24c-virus-hist-multiple-sims.cpp: Virus history with run_multiple
    {

    // Create a simple SIR model
    epimodels::ModelSIR model(
        "test virus", 1000, 0.01, 0.9, 0.3
    );

    model.seed(1231);
    model.add_layer(layer::network(
        "network", rgraph_smallworld(1000, 4, 0.01, false, model)
    ));
    model.verbose_off();

    // Run for 10 days, 2 simulations with saver
    int ndays = 10;
    int nsims = 2;
    
    // Create a saver function that writes virus history
    auto fn = epi_temp_file("24c-virus-hist-multiple-sims", "run_multiple_test-%i");
    const char* test_prefix = fn.full_path.c_str();
    auto saver = make_save_run(
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
        false   // outbreak_size
    );
    
    model.run_multiple(ndays, nsims, 123, saver);

    int nstates = model.get_n_states();
    
    std::cout << "\nrun_multiple test:" << std::endl;
    std::cout << "  ndays=" << ndays << ", nsims=" << nsims << ", nstates=" << nstates << std::endl;

    // Check each simulation's output file
    for (int sim = 0; sim < nsims; ++sim) {
        std::string filename = fn.directory + "/run_multiple_test-" + 
                               std::to_string(sim) + "_virus_hist.csv";
        
        std::ifstream file(filename);
        if (!file.good()) {
            std::cout << "  ERROR: File " << filename << " not found!" << std::endl;
            REQUIRE(false);
            
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
            // The date comes after the disease, which is quoted and may
            // have spaces (and after the thread column in debug mode)
            const size_t close = line.find('"', line.find('"') + 1u);
            std::istringstream iss(line.substr(close + 1u));
            int date;
            iss >> date;
            
            dates_in_file.insert(date);
        }
        file.close();
        
        std::cout << "  Simulation " << sim << ":" << std::endl;
        std::cout << "    Lines in file: " << line_count << " (expected " << ((ndays + 1) * nstates) << ")" << std::endl;
        std::cout << "    Unique dates: " << dates_in_file.size() << " (expected " << (ndays + 1) << ")" << std::endl;
        
        // Each file should have (ndays + 1) * nstates lines
        REQUIRE(line_count == (ndays + 1) * nstates);
        
        // Should have all dates from 0 to ndays
        REQUIRE(dates_in_file.size() == static_cast<size_t>(ndays + 1));
        REQUIRE(*dates_in_file.begin() == 0);
        REQUIRE(*dates_in_file.rbegin() == ndays);
    }

    // Also check that the model's current database only has the LAST simulation's data
    std::vector<int> virus_date, virus_id, virus_counts;
    std::vector<std::string> virus_state, virus_status;
    model.get_db().get_hist_virus(
        virus_date,
        virus_id,
        virus_status,
        virus_state,
        virus_counts
    );
    
    std::cout << "  Final database state:" << std::endl;
    std::cout << "    Virus history size: " << virus_date.size() 
              << " (expected " << ((ndays + 1) * nstates) << " - last simulation only)" << std::endl;

    // The database should only contain the last simulation's data
    REQUIRE(virus_date.size() == static_cast<size_t>((ndays + 1) * nstates));
    }

}
