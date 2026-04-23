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

    
    auto fn_0 = epi_temp_file("12-diagrams", "transitions.txt");
    auto fn_1 = epi_temp_file("12-diagrams-saves", "%li");

    model_0.draw(epiworld::DiagramType::Mermaid);
    std::cout << "Printing transitions from file" << std::endl;

    model_0.write_data(
        "", "", "", "", "", "", fn_0.full_path.c_str(), "", "", "", "", ""
    );
    epiworld::ModelDiagram diagram;
    diagram.draw_from_file(epiworld::DiagramType::Mermaid, fn_0.full_path.c_str());

    model_0.run_multiple(
        100, 10, 1231, make_save_run<>(
            fn_1.full_path.c_str(),
            false, false, false, false, false, false, true, false, false, false, false
        )
    );

    // Listing files in the directory
    std::vector< std::string > files;
    for (size_t i = 0u; i < 10; ++i)
        files.push_back(
            fn_1.directory + "/" + std::to_string(i) + "_transition.csv"
            );

    std::cout << "Printing transitions from multiple files" << std::endl;
    diagram.draw_from_files(epiworld::DiagramType::Mermaid, files);

    REQUIRE_THROWS(diagram.draw_from_file(epiworld::DiagramType::Mermaid, "non_existant_file.txt"));

}
