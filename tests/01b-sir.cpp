#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIR multiple", "[SIR-multiple]") {

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIR<> model_0(
        "a virus", 0.01, .9, .3
        );

    model_0.agents_smallworld(10000, 5, false, 0.01);
    
    model_0.verbose_off();

    model_0.run_multiple(100, 10, 1231, nullptr, true, true, 2);



}
