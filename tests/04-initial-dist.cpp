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

    model_0.initial_states({0.5}).
        agents_smallworld(10000, 5, false, 0.01).
        verbose_off().
        run(100, 1231);

    epimodels::ModelSIR<> model_1(
        "a virus", 0.01, .9, .3
        );

    model_1.initial_states({.5}).
        agents_smallworld(10000, 5, false, 0.01).
        queuing_off().
        verbose_off().
        run(100, 1231);

    std::vector< int > h_0, h_1;
    model_0.get_db().get_hist_total(nullptr, nullptr, &h_0);
    model_1.get_db().get_hist_total(nullptr, nullptr, &h_1);

    // Getting transition matrix
    auto tmat_0 = model_0.get_db().get_transition_probability(false);
    int out_of_range_0 = 0;

    for (auto & v: tmat_0)
        if ((v < 0.0) | (v > 1.0))
            out_of_range_0++;
    
    auto tmat_1 = model_1.get_db().get_transition_probability(false);
    int out_of_range_1 = 0;

    for (auto & v: tmat_1)
        if ((v < 0.0) | (v > 1.0))
            out_of_range_1++;

    // Test when normalize = false
    auto tmat_0_not_norm = model_0.get_db().get_transition_probability(false, false);
    auto tmat_1_not_norm = model_1.get_db().get_transition_probability(false, false);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_THAT(tmat_1, Catch::Equals(tmat_0));
    REQUIRE_THAT(h_0, Catch::Equals(h_1));
    REQUIRE(out_of_range_0 == 0);
    REQUIRE(out_of_range_1 == 0);
    REQUIRE_THAT(tmat_1_not_norm, Catch::Equals(tmat_0_not_norm));
    #else
    model_0.print(false);
    model_1.print(false);
    #endif 

    // Checking that the history matches the expected values
    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(
        ((h_0[0u] != 4950) || (h_0[1u] != 100) || (h_0[2u] != 4950))
        );
    #endif

    // Trying out the SEIR model now -------------------------------------------
    epimodels::ModelSEIR<> model_2(
        "a virus", 0.01, .5, 7.0, .1
        );

    
    model_2.agents_smallworld(10000, 5, false, 0.01).
        queuing_off().
        verbose_off().
        initial_states({.3, .5}, {}).
        run(100, 1231).
        print(false);

    h_0.clear();
    model_2.get_db().get_hist_total(nullptr, nullptr, &h_0);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(
        ((h_0[0u] != 4950) || (h_0[1u] != 70) || (h_0[2u] != 30) || (h_0[3u] != 4950))
        );
    #endif

    // Trying SIRCONN ----------------------------------------------------------
    epimodels::ModelSIRCONN<> model_3(
        "a virus", 10000, 0.01, 2, .5, .3
        );

    model_3.initial_states({.5}).
        verbose_off().
        run(100, 222).
        print(false);

    h_0.clear();
    model_3.get_db().get_hist_total(nullptr, nullptr, &h_0);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(
        ((h_0[0u] != 4950) || (h_0[1u] != 100) || (h_0[2u] != 4950))
        );
    #endif

    // Trying SEIRCONN ---------------------------------------------------------
    epimodels::ModelSEIRCONN<> model_4(
        "a virus", 10000, 0.01, 2, .5, 7, .1
    );

    model_4.initial_states({.3, .5}).
        verbose_off().
        run(100, 222).
        print(false);

    h_0.clear();
    model_4.get_db().get_hist_total(nullptr, nullptr, &h_0);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(
        ((h_0[0u] != 4950) || (h_0[1u] != 70) || (h_0[2u] != 30) || (h_0[3u] != 4950))
        );
    #endif

    // Trying SIRD -------------------------------------------------------------
    epimodels::ModelSIRD<> model_5(
        "a virus", 0.01, .9, .3, .1
    );

    model_5.initial_states({0.5, .05}).
        agents_smallworld(10000, 5, false, 0.01).
        verbose_off().
        run(100, 1231).
        print(false);

    h_0.clear();
    model_5.get_db().get_hist_total(nullptr, nullptr, &h_0);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(
        (
            moreless(h_0[0u], 4700) || 
            moreless(h_0[1u], 100) || 
            moreless(h_0[2u], 4705) || 
            moreless(h_0[3u], 495))
        );
    #endif

    // Trying SEIRD -------------------------------------------------------------
    epimodels::ModelSEIRD<> model_6(
        "a virus", 0.1, .9, 7, .3, .1
    );

    model_6.initial_states({0.5, .1, .1}).
        agents_smallworld(10000, 5, false, 0.01).
        verbose_off().
        run(100, 1231).
        print(false);

    h_0.clear();
    model_6.get_db().get_hist_total(nullptr, nullptr, &h_0);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(
        (
            moreless(h_0[0u], 7200) ||
            moreless(h_0[1u], 500) ||
            moreless(h_0[2u], 500) ||
            moreless(h_0[3u], 900) ||
            moreless(h_0[4u], 900)
            )
        );
    #endif

    // Trying SEIRDCONN --------------------------------------------------------
    epimodels::ModelSEIRDCONN<> model_7(
        "a virus", 10000, 0.1, 4, .9, 7, .3, .1
    );

    model_7.initial_states({0.5, .1, .1}).
        verbose_off().
        run(100, 1231).
        print(false);

    h_0.clear();
    model_7.get_db().get_hist_total(nullptr, nullptr, &h_0);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(
        (
            moreless(h_0[0u], 7200) ||
            moreless(h_0[1u], 500) ||
            moreless(h_0[2u], 500) ||
            moreless(h_0[3u], 900) ||
            moreless(h_0[4u], 900)
            )
        );
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
