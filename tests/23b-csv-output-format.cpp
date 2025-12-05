#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test CSV output format for all data types
 * 
 * This test verifies that all CSV outputs have proper quoting for string fields,
 * which is necessary for correct parsing by data.table::fread in R.
 * 
 * All string fields (virus names, tool names, state labels) should be quoted.
 */
EPIWORLD_TEST_CASE("CSV output format validation", "[csv][output]") {

    // Create a model with both virus and tools
    epimodels::ModelSEIRCONN<> model(
        "test virus", 100, 0.1, 5.0, 0.5, 3.0, 7.0
    );
    
    model.verbose_off();
    model.run(10, 123);

    // Write all data types to files
    std::string test_dir = "/home/runner/work/epiworld/epiworld/tests/";
    model.write_data(
        test_dir + "csv_test_virus_info.csv",
        test_dir + "csv_test_virus_hist.csv",
        test_dir + "csv_test_tool_info.csv",
        test_dir + "csv_test_tool_hist.csv",
        test_dir + "csv_test_total_hist.csv",
        test_dir + "csv_test_transmission.csv",
        test_dir + "csv_test_transition.csv",
        test_dir + "csv_test_reproductive.csv",
        test_dir + "csv_test_generation.csv"
    );

    std::cout << "\nCSV Format Validation Test:" << std::endl;
    std::cout << "============================" << std::endl;

    // Test 1: virus_hist - should have quotes around virus name AND state
    {
        std::ifstream file(test_dir + "csv_test_virus_hist.csv");
        std::string header, line;
        std::getline(file, header);
        std::getline(file, line);
        
        std::cout << "\n1. virus_hist format:" << std::endl;
        std::cout << "   Header: " << header << std::endl;
        std::cout << "   Sample: " << line << std::endl;
        
        // Count quotes - should have exactly 4 (opening and closing for virus and state)
        int quote_count = std::count(line.begin(), line.end(), '"');
        std::cout << "   Quote count: " << quote_count << " (expected: 4)" << std::endl;
        
        #ifdef CATCH_CONFIG_MAIN
        REQUIRE(quote_count == 4);
        #endif
    }

    // Test 2: virus_info - should have quotes around virus name
    {
        std::ifstream file(test_dir + "csv_test_virus_info.csv");
        std::string header, line;
        std::getline(file, header);
        std::getline(file, line);
        
        std::cout << "\n2. virus_info format:" << std::endl;
        std::cout << "   Header: " << header << std::endl;
        std::cout << "   Sample: " << line << std::endl;
        
        // Should have quotes around virus name (2 quotes)
        int quote_count = std::count(line.begin(), line.end(), '"');
        std::cout << "   Quote count: " << quote_count << " (expected: >= 2)" << std::endl;
        
        #ifdef CATCH_CONFIG_MAIN
        REQUIRE(quote_count >= 2);
        #endif
    }

    // Test 3: tool_hist - should have quotes around state
    {
        std::ifstream file(test_dir + "csv_test_tool_hist.csv");
        if (file.good()) {
            std::string header, line;
            std::getline(file, header);
            if (std::getline(file, line)) {
                std::cout << "\n3. tool_hist format:" << std::endl;
                std::cout << "   Header: " << header << std::endl;
                std::cout << "   Sample: " << line << std::endl;
                
                // Should have quotes around state (2 quotes)
                int quote_count = std::count(line.begin(), line.end(), '"');
                std::cout << "   Quote count: " << quote_count << " (expected: 2)" << std::endl;
                
                #ifdef CATCH_CONFIG_MAIN
                REQUIRE(quote_count == 2);
                #endif
            } else {
                std::cout << "\n3. tool_hist: No data (model has no tools)" << std::endl;
            }
        }
    }

    // Test 4: total_hist - should have quotes around state
    {
        std::ifstream file(test_dir + "csv_test_total_hist.csv");
        std::string header, line;
        std::getline(file, header);
        std::getline(file, line);
        
        std::cout << "\n4. total_hist format:" << std::endl;
        std::cout << "   Header: " << header << std::endl;
        std::cout << "   Sample: " << line << std::endl;
        
        // Should have quotes around state (2 quotes)
        int quote_count = std::count(line.begin(), line.end(), '"');
        std::cout << "   Quote count: " << quote_count << " (expected: 2)" << std::endl;
        
        #ifdef CATCH_CONFIG_MAIN
        REQUIRE(quote_count == 2);
        #endif
    }

    // Test 5: transmission - should have quotes around virus name
    {
        std::ifstream file(test_dir + "csv_test_transmission.csv");
        std::string header, line;
        std::getline(file, header);
        if (std::getline(file, line)) {
            std::cout << "\n5. transmission format:" << std::endl;
            std::cout << "   Header: " << header << std::endl;
            std::cout << "   Sample: " << line << std::endl;
            
            // Should have quotes around virus name (2 quotes)
            int quote_count = std::count(line.begin(), line.end(), '"');
            std::cout << "   Quote count: " << quote_count << " (expected: 2)" << std::endl;
            
            #ifdef CATCH_CONFIG_MAIN
            REQUIRE(quote_count == 2);
            #endif
        } else {
            std::cout << "\n5. transmission: No transmissions recorded" << std::endl;
        }
    }

    // Test 6: transition - should have quotes around both from and to states
    {
        std::ifstream file(test_dir + "csv_test_transition.csv");
        std::string header, line;
        std::getline(file, header);
        if (std::getline(file, line)) {
            std::cout << "\n6. transition format:" << std::endl;
            std::cout << "   Header: " << header << std::endl;
            std::cout << "   Sample: " << line << std::endl;
            
            // Should have quotes around from and to states (4 quotes)
            int quote_count = std::count(line.begin(), line.end(), '"');
            std::cout << "   Quote count: " << quote_count << " (expected: 4)" << std::endl;
            
            #ifdef CATCH_CONFIG_MAIN
            REQUIRE(quote_count == 4);
            #endif
        } else {
            std::cout << "\n6. transition: No transitions recorded" << std::endl;
        }
    }

    // Test 7: reproductive_number - should have quotes around virus name
    {
        std::ifstream file(test_dir + "csv_test_reproductive.csv");
        std::string header, line;
        std::getline(file, header);
        if (std::getline(file, line)) {
            std::cout << "\n7. reproductive_number format:" << std::endl;
            std::cout << "   Header: " << header << std::endl;
            std::cout << "   Sample: " << line << std::endl;
            
            // Should have quotes around virus name (2 quotes)
            int quote_count = std::count(line.begin(), line.end(), '"');
            std::cout << "   Quote count: " << quote_count << " (expected: 2)" << std::endl;
            
            #ifdef CATCH_CONFIG_MAIN
            REQUIRE(quote_count == 2);
            #endif
        } else {
            std::cout << "\n7. reproductive_number: No data" << std::endl;
        }
    }

    // Test 8: generation_time - no string fields, just numeric IDs
    {
        std::ifstream file(test_dir + "csv_test_generation.csv");
        std::string header, line;
        std::getline(file, header);
        if (std::getline(file, line)) {
            std::cout << "\n8. generation_time format:" << std::endl;
            std::cout << "   Header: " << header << std::endl;
            std::cout << "   Sample: " << line << std::endl;
            std::cout << "   (No string fields - numeric IDs only)" << std::endl;
        } else {
            std::cout << "\n8. generation_time: No data" << std::endl;
        }
    }

    std::cout << "\nAll CSV formats validated!" << std::endl;

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
