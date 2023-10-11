#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIR dist", "[SIR-dist]") {

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIR<> model_0(
        "a virus", 0.01, .9, .3
        );

    model_0.initial_states(
      {1.0, 0.0, 2.0},
      {}
      );

    model_0.agents_smallworld(10000, 5, false, 0.01);
    
    model_0.verbose_off();

    model_0.run(100, 1231);

    epimodels::ModelSIR<> model_1(
        "a virus", 0.01, .9, .3
        );

    model_1.agents_smallworld(10000, 5, false, 0.01);

    model_1.queuing_off();
    model_1.verbose_off();

    model_1.initial_states(
      {1.0, 0.0, 2.0},
      {}
      );
    
    model_1.run(100, 1231);

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

    std::vector< epiworld_double > tmat_expected = {0.962440431, 0.0, 0.0, 0.0386752182, 0.704328, 0.0, 3.3772063e-05, 0.298277199, 1.0};

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_THAT(tmat_0, Catch::Approx(tmat_expected).margin(0.025));
    REQUIRE_THAT(tmat_1, Catch::Equals(tmat_0));
    REQUIRE_THAT(h_0, Catch::Equals(h_1));
    REQUIRE(out_of_range_0 == 0);
    REQUIRE(out_of_range_1 == 0);
    #else
    model_0.print(false);
    model_1.print(false);
    #endif 

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}