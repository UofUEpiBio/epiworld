#ifndef CATCH_CONFIG_MAIN
    #define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIR", "[SIR]") {
    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIR<> model_0("a virus", 0.01, .9, .3);

    model_0.seed(1231);

    model_0.agents_smallworld(10000, 5, false, 0.01).verbose_off().run(100);

    epimodels::ModelSIR<> model_1("a virus", 0.01, .9, .3);

    model_1.seed(1231);

    model_1.agents_smallworld(10000, 5, false, 0.01)
        .queuing_off()
        .verbose_off()
        .run(100);

    // Now, with parameters
    epimodels::ModelSIR<> model_2("a virus", 0.01, .0, .0);

    model_2.seed(1231);

    model_2.agents_smallworld(10000, 5, false, 0.01)
        .read_params("01-sir_parameters.yaml", true)
        .queuing_off()
        .verbose_off()
        .run(100);

    std::vector<int> h_0, h_1, h_2;
    model_0.get_db().get_hist_total(nullptr, nullptr, &h_0);
    model_1.get_db().get_hist_total(nullptr, nullptr, &h_1);
    model_2.get_db().get_hist_total(nullptr, nullptr, &h_2);

    // Getting transition matrix
    auto tmat_0 = model_0.get_db().get_transition_probability(false);
    int out_of_range_0 = 0;

    for (auto& v : tmat_0)
        if (v < 0.0 | v > 1.0)
            out_of_range_0++;

    auto tmat_1 = model_1.get_db().get_transition_probability(false);
    int out_of_range_1 = 0;

    for (auto& v : tmat_1)
        if (v < 0.0 | v > 1.0)
            out_of_range_1++;

    auto tmat_2 = model_2.get_db().get_transition_probability(false);
    int out_of_range_2 = 0;

    for (auto& v : tmat_2)
        if (v < 0.0 | v > 1.0)
            out_of_range_2++;

    std::vector<epiworld_double> tmat_expected = {
        0.962440431,
        0.0,
        0.0,
        0.0386752182,
        0.704328,
        0.0,
        3.3772063e-05,
        0.298277199,
        1.0
    };

#ifdef CATCH_CONFIG_MAIN
    REQUIRE_THAT(tmat_0, Catch::Approx(tmat_expected).margin(0.025));
    REQUIRE_THAT(tmat_1, Catch::Equals(tmat_0));
    REQUIRE_THAT(tmat_1, Catch::Equals(tmat_2));
    REQUIRE_THAT(h_0, Catch::Equals(h_1));
    REQUIRE_THAT(h_0, Catch::Equals(h_2));
    REQUIRE(out_of_range_0 == 0);
    REQUIRE(out_of_range_1 == 0);
    REQUIRE(out_of_range_2 == 0);
    REQUIRE_THROWS(model_2.read_params("bad_params_test.yaml", true));
#else
    model_0.print(false);
    model_1.print(false);
    model_2.print(false);
#endif

    model_0.draw();

    model_1
        .write_data("", "", "", "", "", "", "01-sir_transitions.txt", "", "");

    std::cout << "Printing transitions from file" << std::endl;

    epiworld::ModelDiagram diagram;
    diagram.draw_from_file("01-sir_transitions.txt");

#ifndef CATCH_CONFIG_MAIN
    return 0;
#endif
}