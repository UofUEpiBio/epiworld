#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("model-methods", "[model-methods]") {
    // Creating a model
    Model<> model;

    REQUIRE_NOTHROW(model.add_param(0.1, "Prob hospitalization"));
    REQUIRE_THROWS_WITH(model.add_param(0.2, "Prob hospitalization"), Catch::Matchers::Contains("already exists."));
    REQUIRE_NOTHROW(model.add_param(0.2, "Prob hospitalization", true));
}