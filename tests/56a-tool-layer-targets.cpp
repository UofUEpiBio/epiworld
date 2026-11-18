#include "tests.hpp"

using namespace epiworld;

// A tool that targets a layer (Phase 4b): everyone wears a perfect mask at
// work (susceptibility reduction 1, targeting the "work" layer), so no one is
// infected at work and the home layer carries the epidemic, pulled or pushed.
// The same mask without layer targets stops every transmission. The effect
// is 1 through the work layer and 0 through home or outside any layer (an
// infection given by code sees only tools without layer targets). A layer
// the model does not have is an error when the run starts.

namespace {

constexpr size_t N = 1000u;

std::unique_ptr< epimodels::ModelSIR > build(bool at_work_only, const char * mode)
{
    auto model = std::make_unique< epimodels::ModelSIR >(
        "Virus", N, 0.02, 0.5, 0.2
    );
    model->verbose_off();
    model->add_layer(layer::network("home", graph::smallworld(4, 0.1)));
    model->add_layer(layer::network("work", graph::smallworld(6, 0.2)));

    Tool mask("Mask");
    mask.set_susceptibility_reduction(1.0);
    if (at_work_only)
        mask.add_layer_target("work");
    model->add_tool(mask, assign::share(1.0));

    model->set_transmission_mode(mode);
    return model;
}

// Transmissions (not the seeds) through each layer
std::array< size_t, 2 > by_layer(Model & model)
{
    std::vector< int > date, source, target, virus, expo, layer;
    model.get_db().get_transmissions(date, source, target, virus, expo, layer);

    std::array< size_t, 2 > n = {0u, 0u};
    for (size_t k = 0u; k < source.size(); ++k)
        if (source[k] >= 0)
            ++n[static_cast< size_t >(layer[k])];
    return n;
}

}

EPIWORLD_TEST_CASE("Tools that target layers", "[layer-participation]") {

    for (const char * mode : {"pull", "push"})
    {

        // At work only: home transmits, work does not
        auto model = build(true, mode);
        REQUIRE(model->get_tool(0).get_layer_targets() == std::vector< std::string >{"work"});
        model->run(30, 99);
        const auto n = by_layer(*model);
        REQUIRE(n[0] > 50u);
        REQUIRE(n[1] == 0u);

        // Everywhere: nothing
        auto everywhere = build(false, mode);
        everywhere->run(30, 99);
        const auto none = by_layer(*everywhere);
        REQUIRE(none[0] + none[1] == 0u);

    }

    // The effect by layer: 1 at work, 0 at home and outside any layer
    {
        auto model = build(true, "pull");
        model->run(5, 1);
        bool found = false;
        for (auto & a : model->get_agents())
        {
            if (a.get_virus() == nullptr)
                continue;

            found = true;
            auto & v = a.get_virus();
            REQUIRE(a.get_susceptibility_reduction(v, *model, 1) == 1.0);
            REQUIRE(a.get_susceptibility_reduction(v, *model, 0) == 0.0);
            REQUIRE(a.get_susceptibility_reduction(v, *model) == 0.0);
            break;
        }
        REQUIRE(found);
    }

    // A layer the model does not have
    {
        auto model = build(true, "pull");
        Tool other("Gloves");
        other.set_transmission_reduction(0.5);
        other.add_layer_target("school");
        model->add_tool(other, assign::share(0.5));
        REQUIRE_THROWS_WITH(
            model->run(5, 1),
            Catch::Matchers::Contains("targets the layer \"school\"")
        );
    }

    // Setters and getters
    Tool t("T");
    t.set_layer_targets({"a", "b", "a"});
    REQUIRE(t.get_layer_targets() == std::vector< std::string >{"a", "b"});
    t.clear_layer_targets();
    REQUIRE(t.get_layer_targets().empty());
    REQUIRE(t.acts_in(-1));
    REQUIRE(t.acts_in(3));

}
