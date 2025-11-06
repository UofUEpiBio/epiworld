#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIS and SISD", "[SISD-multiple]") {

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIS<> model_0("SIS", 0.1, 0.3, 0.3);

    model_0.agents_smallworld(2000, 5, false, 0.01);
    
    model_0.verbose_off();

    model_0.run_multiple(100, 10, 1231, nullptr, true, true, 1);



}