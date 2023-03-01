#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

    // Adding multi-file write
auto sav_0 = epiworld::make_save_run<int>(
    "saves/main_out_%04li", // std::string fmt,
    true,  // bool total_hist,
    false, // bool variant_info,
    false, // bool variant_hist,
    false, // bool tool_info,
    false, // bool tool_hist,
    true , // bool transmission,
    false, // bool transition,
    true   // bool reproductive
);

auto sav_1 = epiworld::make_save_run<int>(
    "saves/main_out_pll_%04li", // std::string fmt,
    true,  // bool total_hist,
    false, // bool variant_info,
    false, // bool variant_hist,
    false, // bool tool_info,
    false, // bool tool_hist,
    true , // bool transmission,
    false, // bool transition,
    true   // bool reproductive
);


EPIWORLD_TEST_CASE("SIR parallel", "[SIR parallel]") {

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIR<> model_0(
        "a virus", 0.01, .9, .3
        );
    
    model_0.seed(112);

    model_0.agents_smallworld(100000, 5, false, 0.01);
    
    model_0.verbose_off();

    model_0.run_multiple(100, 4, 1231, sav_0, true, true, 1);

    epimodels::ModelSIR<> model_1(
        "a virus", 0.01, .9, .3
        );

    model_1.seed(112);

    model_1.agents_smallworld(100000, 5, false, 0.01);

    model_1.run_multiple(100, 4, 1231, sav_1, true, true, 2);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_THAT(tmat_0, Catch::Approx(tmat_expected).margin(0.025));
    REQUIRE_THAT(tmat_1, Catch::Approx(tmat_expected).margin(0.025));
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