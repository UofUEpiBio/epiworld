#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIRCON", "[SIR connected]") {

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIRCONN<> model_0(
        "a virus", 10000u, 0.01, 4.0, .5, 1.0/7.0
        );
    
    model_0.verbose_off();
    model_0.run(100, 131);

    epimodels::ModelSIRCONN<> model_1(
        "a virus", 10000u, 0.01, 4.0, .5, 1.0/7.0
        );

    model_1.queuing_off();
    model_1.verbose_off();
    model_1.run(100, 131);

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

    std::vector< epiworld_double > tmat_expected = {
        0.556414008,
        0,
        0,
        0.443586022,
        0.859082818,
        0,
        0,
        0.140917242,
        1};

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_THAT(tmat_0, Catch::Approx(tmat_expected).margin(0.05));
    REQUIRE_THAT(tmat_1, Catch::Approx(tmat_expected).margin(0.05));
    REQUIRE_THAT(h_0, Catch::Equals(h_1));
    REQUIRE(out_of_range_0 == 0);
    REQUIRE(out_of_range_1 == 0);
    #endif 

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}