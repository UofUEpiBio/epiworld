#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIS and SISD", "[SISD-multiple]") {

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIS<> model_0("SIS", 2000, 0.1, 0.3, 0.3);

    model_0.add_layer(std::make_unique< layer::Network<> >(
        "network", rgraph_smallworld(2000, 5, 0.01, false, model_0)
    ));
    
    model_0.verbose_off();

    model_0.run_multiple(100, 10, 1231, nullptr, true, true, 1);



}