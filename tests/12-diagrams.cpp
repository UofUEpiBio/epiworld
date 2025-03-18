#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Diagrams", "[ModelDiagram]") {

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIR<> model_0(
        "a virus", 0.01, .5, .3
        );

    model_0.seed(1231);

    model_0.agents_smallworld(1000, 5, false, 0.01).
        verbose_off().
        run(100);

    
    model_0.draw();
    std::cout << "Printing transitions from file" << std::endl;

    model_0.write_data(
        "", "", "", "", "", "", "01-sir_transitions.txt", "", ""
    );
    epiworld::ModelDiagram diagram;
    diagram.draw_from_file("01-sir_transitions.txt");

    model_0.run_multiple(
        100, 10, 1231, make_save_run<>(
            "12-diagrams-saves/%i",
            false, false, false, false, false, false, true, false, false
        )
    );

    // Listing files in the directory
    std::vector< std::string > files;
    for (size_t i = 0u; i < 10; ++i)
        files.push_back(
            "12-diagrams-saves/" + std::to_string(i) + "_transition.csv"
            );

    std::cout << "Printing transitions from multiple files" << std::endl;
    diagram.draw_from_files(files);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_THROWS(diagram.draw_from_file("non_existant_file.txt"));
    #endif 

    
    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}