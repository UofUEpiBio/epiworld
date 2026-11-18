#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;
using Catch::Matchers::Contains;

// What the registry refuses, and says why: in make_model() an unknown model or
// argument (naming what there is), a value outside its limits or not whole, a
// virus name for a model that names its own; in add() a taken id, a repeated
// argument, a default outside its limits, no builder.
EPIWORLD_TEST_CASE("Model registry - errors", "[registry]") {

    ModelRegistry registry;
    epimodels::register_models(registry);
    measles::register_models(registry);

    REQUIRE_THROWS_WITH(registry.make_model("SEIRS"), Contains("SEIR, SISD"));
    REQUIRE_THROWS_WITH(
        registry.make_model("SEIR", {{"transmission_rate", 0.1}}),
        Contains("n, prevalence, transmission_prob, latent_period, recovery_rate")
    );
    REQUIRE_THROWS_WITH(
        registry.make_model("SEIR", {{"transmission_prob", 1.5}}),
        Contains("\"transmission_prob\"") && Contains("[0.000000, 1.000000]")
    );
    REQUIRE_THROWS_WITH(
        registry.make_model("SEIRQuarantine", {{"quarantine_period", 2.5}}),
        Contains("whole number")
    );
    REQUIRE_THROWS_WITH(
        registry.make_model("MeaslesSchool", {}, "flu"),
        Contains("names its own virus")
    );

    // Defaults only: every model builds
    for (const auto & info : registry.list_models())
        REQUIRE_NOTHROW(registry.make_model(info.id));

    auto entry = registry.get_model_info("SIR");
    REQUIRE_THROWS_WITH(registry.add(entry), Contains("already has"));

    entry.id = "SIR2";
    auto repeated = entry;
    repeated.params.push_back(repeated.params[0u]);
    REQUIRE_THROWS_WITH(registry.add(repeated), Contains("twice"));

    auto bad_default = entry;
    bad_default.params[2u].value = 2.0;
    REQUIRE_THROWS_WITH(registry.add(bad_default), Contains("outside its limits"));

    auto no_builder = entry;
    no_builder.build = nullptr;
    REQUIRE_THROWS_WITH(registry.add(no_builder), Contains("no builder"));

    // A model of one's own, added like the others
    registry.add(entry);
    REQUIRE(registry.list_models().back().id == "SIR2");
    REQUIRE(registry.make_model("SIR2", {{"n", 10}})->size() == 10u);

}
