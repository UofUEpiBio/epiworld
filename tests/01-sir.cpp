#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIR", "[SIR]") {

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIR<> model_0(
        "a virus", 0.01, .9, .3
        );

    model_0.agents_smallworld(100000, 5, false, 0.01);
    
    model_0.verbose_off();

    model_0.init(100, 1231);
    model_0.run();

    epimodels::ModelSIR<> model_1(
        "a virus", 0.01, .9, .3
        );

    model_1.agents_smallworld(100000, 5, false, 0.01);

    model_1.queuing_off();
    model_1.verbose_off();
    
    model_1.init(100, 1231);
    model_1.run();

    std::vector< int > h_0, h_1;
    model_0.get_db().get_hist_total(nullptr, nullptr, &h_0);
    model_1.get_db().get_hist_total(nullptr, nullptr, &h_1);

    // Getting transition matrix
    auto tmat_0 = model_0.get_db().transition_probability(false);
    int out_of_range_0 = 0;

    for (auto & v: tmat_0)
        if (v < 0.0 | v > 1.0)
            out_of_range_0++;
    
    auto tmat_1 = model_1.get_db().transition_probability(false);
    int out_of_range_1 = 0;

    for (auto & v: tmat_1)
        if (v < 0.0 | v > 1.0)
            out_of_range_1++;

    std::vector< epiworld_double > tmat_expected = {0.8683, 0.0, 0.0, 0.131610, 0.704328, 0.0, 0.00003972, 0.295671, 1.0};

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_THAT(tmat_0, Catch::Approx(tmat_expected).margin(0.001));
    REQUIRE_THAT(tmat_1, Catch::Approx(tmat_expected).margin(0.001));
    REQUIRE_THAT(h_0, Catch::Equals(h_1));
    REQUIRE(out_of_range_0 == 0);
    REQUIRE(out_of_range_1 == 0);
    #else
    model_0.print();
    model_1.print();
    #endif 

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}