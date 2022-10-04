#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIRCON", "[SIR connected]") {

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIRCONN<> model_0(
        "a virus", 10000u, 0.01, 2.0, .9, .3
        );
    
    model_0.verbose_off();

    model_0.init(100, 1231);
    model_0.run();

    epimodels::ModelSIRCONN<> model_1(
        "a virus", 10000u, 0.01, 2.0, .9, .3
        );

    model_1.queuing_off();
    model_1.verbose_off();
    
    model_1.init(100, 1231);
    model_1.run();

    std::vector< int > h_0, h_1;
    model_0.get_db().get_hist_total(nullptr, nullptr, &h_0);
    model_1.get_db().get_hist_total(nullptr, nullptr, &h_1);

    // Getting transition matrix
    auto tmat = model_0.get_db().transition_probability(false);
    int out_of_range = 0;
    for (auto & v: tmat)
        if (v < 0.0 | v > 1.0)
            out_of_range++;

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_THAT(h_0, Catch::Equals(h_1));
    REQUIRE(out_of_range == 0);
    #endif 

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}