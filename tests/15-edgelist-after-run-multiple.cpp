#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Edgelist is preserved in run multiple", "[Edgelist]") {

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIR<> model_0(
        "a virus", 500, 0.01, .9, .3
        );

    model_0.seed(1231);
    model_0.add_layer(std::make_unique< layer::Network<> >(
        "network", rgraph_smallworld(500, 5, 0.01, false, model_0)
    ));
    model_0.verbose_off();

    std::vector< int > source0a, target0a;
    std::vector< int > source0b, target0b;
    tests_net(model_0).get_edgelist(source0a, target0a);
    model_0.run_multiple(100, 100, 1231, nullptr, true, true, 1);
    tests_net(model_0).get_edgelist(source0b, target0b);

    REQUIRE( source0a == source0b );
    REQUIRE( target0a == target0b );



}