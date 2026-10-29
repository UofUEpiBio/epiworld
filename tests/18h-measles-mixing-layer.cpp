#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

// The measles mixing model on the mixing layer the user adds: its contacts
// are in the layer's log, it needs that layer and no other, the vaccine can be
// given by group, an agent in no group meets nobody, and the layer's contact
// rate scales the contacts.
EPIWORLD_TEST_CASE(
    "Measles model with mixing on a mixing layer",
    "[ModelMeaslesMixingLayer]"
) {

    const size_t n = 3000u;
    auto build = []() {
        measles::ModelMeaslesMixing<> model(
            n, 30.0 / n, 0.3, 1.0, 0.3, 7.0, 4.0, 5.0, 0.2, 7.0, 2.0, 21,
            .8, .8, 4, 0.5, 1.0, 4u
        );
        model.verbose_off();
        return model;
    };

    // No layer, or another layer next to the mixing one: an error at reset
    auto bare = build();
    REQUIRE_THROWS(bare.run(1, 1));
    auto two = build();
    add_group_mixing(two, {1.0}, blocks({0u, n}));
    add_random_mixing(two, 1.0);
    REQUIRE_THROWS(two.run(1, 1));

    // Three groups of 1,000 but agent 2,999, in no group; the vaccine reaches
    // 20%, 50% and 80% of the groups
    auto model = build();
    add_group_mixing(
        model, {10.0, 2.0, 1.0, 2.0, 10.0, 2.0, 1.0, 2.0, 10.0},
        ranges(n, {{0u, 1000u}, {1000u, 2000u}, {2000u, n - 1u}})
    );
    model.get_tool(0).set_distribution(
        distribute_tool_to_groups<>("mixing", {0.2, 0.5, 0.8})
    );
    auto & mixing = model.get_layer_as< layer::Mixing<> >("mixing");

    model.run(60, 7);

    // The vaccine by group: floor(p x size) agents each
    std::vector< size_t > vaxed(3u, 0u);
    for (const auto & a : model.get_agents())
        if (a.get_n_tools() > 0u)
            ++vaxed[mixing.get_group(a.get_id())];
    REQUIRE(vaxed == std::vector< size_t >{200u, 500u, 799u});

    // Every transmission is a contact in the layer's log, which the model
    // turned on
    REQUIRE(mixing.is_tracing());
    auto & log = mixing.get_tracing();
    std::set< std::pair< size_t, size_t > > logged;
    for (size_t i = 0u; i < n; ++i)
        for (size_t k = 0u; k < log.get_n_contacts(i); ++k)
            logged.insert({i, log.get_contact(i, k).first});

    std::vector< int > date, source, target, virus, exposure, layer;
    model.get_db().get_transmissions(
        date, source, target, virus, exposure, layer
    );
    size_t n_contact = 0u;
    for (size_t i = 0u; i < source.size(); ++i)
    {
        if (source[i] < 0)
            continue;
        ++n_contact;
        REQUIRE(logged.count({
            static_cast< size_t >(source[i]), static_cast< size_t >(target[i])
        }) == 1u);
        REQUIRE(target[i] != static_cast< int >(n - 1u));
    }
    REQUIRE(n_contact > 0u);

    // A contact rate of 0 (a model parameter) silences the layer
    model.add_param(0.0, "Contact rate");
    mixing.set_contact_rate("Contact rate");
    model.run(60, 7);
    model.get_db().get_transmissions(
        date, source, target, virus, exposure, layer
    );
    for (auto s : source)
        REQUIRE(s < 0);

}
