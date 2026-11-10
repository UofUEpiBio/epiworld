#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Diagrams", "[ModelDiagram]") {

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIR model_0(
        "a virus", 1000, 0.01, .5, .3
        );

    model_0.seed(1231);

    model_0.add_layer(layer::network(
        "network", rgraph_smallworld(1000, 5, 0.01, false, model_0)
    ));

    model_0.
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
        100, 10, 1231, make_save_run(
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

    // Drawn from the declarations (no run): the stages, with their classes,
    // and one unlabeled arrow per pair of nodes some transition joins
    {
        epimodels::ModelSEIR seir("flu", 100, 0.01, 0.5, 4.0, 0.2);
        const std::string d = seir.draw_natural_history();
        auto has = [&d](const std::string & x) { return d.find(x) != std::string::npos; };
        REQUIRE(has("flowchart LR\n"));
        REQUIRE(has("s1[\"Exposed\"]:::carrier"));
        REQUIRE(has("s2[\"Infected\"]:::infectious"));
        REQUIRE(has("s0 --> s1\n"));
        REQUIRE(has("s1 --> s2\n"));
        REQUIRE(has("s2 --> s3\n"));
        REQUIRE_FALSE(has("|"));
        REQUIRE_FALSE(has("subgraph statuses"));

        // Statuses: their own subgraph; two transitions between the same
        // statuses draw one arrow
        Statuses st;
        st.add_status("Free");
        st.add_status("Isolated");
        st.rate(0, 1, 0.5).name("Detected").when({2});
        st.external(0, 1, "Isolated");
        st.after(1, 0, Duration::fixed(7).since(2)).name("Released");
        seir.set_statuses(st);
        const std::string q = seir.draw_natural_history();
        REQUIRE(q.find("subgraph statuses[\"Statuses\"]") != std::string::npos);
        const size_t arrow = q.find("q0 --> q1\n");
        REQUIRE(arrow != std::string::npos);
        REQUIRE(q.find("q0 --> q1\n", arrow + 1u) == std::string::npos);
        REQUIRE(q.find("q1 --> q0\n") != std::string::npos);

        REQUIRE_THROWS(Model(10).draw_natural_history());
    }

}
