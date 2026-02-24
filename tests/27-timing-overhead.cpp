#ifndef CATCH_CONFIG_MAIN
    #define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Timing overhead measurement", "[timing]") {

    epimodels::ModelSIR<> model(
        "a virus",
        0.01,  // prevalence
        0.9,   // transmission rate
        0.3    // recovery rate
    );

    model.agents_smallworld(10000, 5, false, 0.02);
    model.verbose_off();
    model.run(100, 123);

    // Retrieve setup timing
    epiworld_double last_setup, total_setup;
    std::string abbr_setup;
    model.get_elapsed_setup("microseconds", &last_setup, &total_setup, &abbr_setup, false);

    // Retrieve run (simulation loop) timing
    epiworld_double last_run, total_run;
    std::string abbr_run;
    model.get_elapsed("microseconds", &last_run, &total_run, &abbr_run, false);

    printf("Single run:\n");
    printf("  Setup time : %.2f%s\n", last_setup, abbr_setup.c_str());
    printf("  Run time   : %.2f%s\n", last_run, abbr_run.c_str());

    #ifdef CATCH_CONFIG_MAIN
    // Setup time must be non-negative
    REQUIRE(last_setup >= 0.0);
    REQUIRE(total_setup >= 0.0);
    REQUIRE(last_run > 0.0);

    // After a single run, last == total
    REQUIRE(last_setup == total_setup);
    REQUIRE(last_run == total_run);
    #endif

    // Run multiple simulations and verify totals accumulate
    size_t nexperiments = 4;
    model.run_multiple(100, nexperiments, 456);

    model.get_elapsed_setup("microseconds", &last_setup, &total_setup, &abbr_setup, false);
    model.get_elapsed("microseconds", &last_run, &total_run, &abbr_run, false);

    printf("After %zu additional runs:\n", nexperiments);
    printf("  Total setup time : %.2f%s\n", total_setup, abbr_setup.c_str());
    printf("  Total run time   : %.2f%s\n", total_run, abbr_run.c_str());

    #ifdef CATCH_CONFIG_MAIN
    // Total must be >= last (accumulated)
    REQUIRE(total_setup >= last_setup);
    REQUIRE(total_run >= last_run);

    // Total must be positive
    REQUIRE(total_setup > 0.0);
    REQUIRE(total_run > 0.0);
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
