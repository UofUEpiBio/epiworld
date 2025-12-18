#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"
#include <fstream>
#include <sstream>

using namespace epiworld;

/**
 * @brief Test hospitalization file output
 * 
 * This test validates that the write_data function correctly writes
 * hospitalization data to a CSV file.
 */
EPIWORLD_TEST_CASE("Hospitalization file output", "[hospitalizations-output]") {

    // Create a SEIR model with mixing, quarantine and hospitalization
    std::vector<double> contact_matrix = {
        0.7, 0.3,
        0.3, 0.7
    };

    epimodels::ModelSEIRMixingQuarantine<> model(
        "test virus",
        200,
        0.01,
        10.0,
        0.2,
        7.0,
        0.3,
        contact_matrix,
        0.1,
        7.0,
        3.0,
        14,
        0.8,
        0.8,
        7,
        0.9,
        3u
    );

    Entity<> e1("Group 1", dist_factory<>(0, 100));
    Entity<> e2("Group 2", dist_factory<>(100, 200));
    
    model.add_entity(e1);
    model.add_entity(e2);

    model.seed(12345);
    model.verbose_off();

    int ndays = 30;
    model.run(ndays, 123);

    // Write data including hospitalizations
    std::string hosp_file = "25c-hospitalizations-output-saves/test_hospitalizations.csv";

    model.write_data(
        "",  // virus_info
        "",  // virus_hist
        "",  // tool_info
        "",  // tool_hist
        "",  // total_hist
        "",  // transmission
        "",  // transition
        "",  // reproductive
        "",  // generation
        "",  // active_cases
        "",  // outbreak_size
        hosp_file  // hospitalizations
    );

    #ifdef CATCH_CONFIG_MAIN
    // Verify the file was created
    std::ifstream file(hosp_file);
    REQUIRE(file.is_open());
    
    // Read the file and verify it has content
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();
    
    // Should have at least a header line
    REQUIRE(lines.size() > 0);
    
    // Check header format
    std::string header = lines[0];
    #ifdef EPI_DEBUG
    REQUIRE(header.find("thread") != std::string::npos);
    #endif
    REQUIRE(header.find("date") != std::string::npos);
    REQUIRE(header.find("virus_id") != std::string::npos);
    REQUIRE(header.find("hospitalizations") != std::string::npos);
    
    // Count data lines (excluding header)
    int data_lines = 0;
    for (size_t i = 1; i < lines.size(); ++i) {
        if (!lines[i].empty()) {
            data_lines++;
        }
    }
    
    // Should have some data lines (at least one hospitalization event expected)
    REQUIRE(data_lines > 0);
    
    std::cout << "Hospitalization file output test passed:" << std::endl;
    std::cout << "  File: " << hosp_file << std::endl;
    std::cout << "  Data lines: " << data_lines << std::endl;
    #endif

    #ifndef CATCH_CONFIG_MAIN
    std::cout << "Hospitalization data written to: " << hosp_file << std::endl;
    return 0;
    #endif

}
