#include "tests.hpp"

using namespace epiworld;

// A mixing layer groups agents by its own table: here an agent's group is its
// id modulo 3, and some agents are left out of the layer. With
// contacts only within groups and a fully transmissible virus, the infection
// that starts in agent 0 must fill group 0 and nothing else. A group edit made
// during a step is in force for the next step's contacts.
EPIWORLD_TEST_CASE("Mixing layer groups agents on its own", "[layer-mixing-groups]") {

    const size_t n = 300u;
    const size_t no_group = layer::Mixing<>::NO_GROUP;

    // Within-group contacts only (column-major, 3 x 3)
    std::vector< double > cm = {
        20.0, 0.0, 0.0,
        0.0, 20.0, 0.0,
        0.0, 0.0, 20.0
    };

    epimodels::ModelSIR<> model("Flu", n, 0.0, 1.0, 0.01);

    Virus<> v = model.get_virus(0);
    model.rm_virus(0);
    v.set_distribution(dist_virus<>(0));
    model.add_virus(v);

    // Agents 270-299 take no part in the layer (one of them is in group 0's
    // residue class, 273)
    std::vector< size_t > groups(n);
    for (size_t i = 0u; i < n; ++i)
        groups[i] = (i >= 270u) ? no_group : (i % 3u);

    add_group_mixing(model, cm, groups);
    auto & mixing = model.get_layer_as< layer::Mixing<> >("mixing");

    model.verbose_off();
    model.run(20, 1912);

    size_t in_group[3] = {0u, 0u, 0u};
    size_t outside = 0u;
    for (size_t i = 0u; i < n; ++i)
    {
        const bool reached =
            model.get_agent(i).get_state() != epimodels::ModelSIR<>::SUSCEPTIBLE;

        if (!reached)
            continue;

        if (groups[i] == no_group)
            ++outside;
        else
            ++in_group[groups[i]];
    }

    // 90 agents per group; group 0 is fully infected, nobody else is
    REQUIRE(in_group[0] == 90u);
    REQUIRE(in_group[1] == 0u);
    REQUIRE(in_group[2] == 0u);
    REQUIRE(outside == 0u);

    // The layer reports the grouping it was given
    REQUIRE(mixing.get_n_groups() == 3u);
    REQUIRE(mixing.get_group(4u) == 1u);
    REQUIRE(mixing.in_layer(4u));
    REQUIRE_FALSE(mixing.in_layer(273u));

    // The layers of the model are reachable by name and by id
    REQUIRE(model.get_n_layers() == 1u);
    REQUIRE(model.get_layer_id("mixing") == 0u);
    REQUIRE(model.get_layer("mixing").get_name() == "mixing");

    // The only infectious agent leaves the layer on day 1, when the matrix
    // also opens: from day 2 on there is no one to be infected by
    epimodels::ModelSIR<> solo("Flu", 100u, 0.0, 1.0, 0.0);
    add_group_mixing(solo, {0.0}, std::vector< size_t >(100u, 0u));
    solo.rm_virus(0);
    solo.add_virus(v);
    solo.add_globalevent(
        [no_group](Model<> * m) -> void {
            auto & layer = m->get_layer_as< layer::Mixing<> >("mixing");
            layer.set_group(0u, no_group);
            layer.set_contact_matrix({50.0});
        },
        "leave", 1
    );
    solo.verbose_off();
    solo.run(5, 77);

    size_t reached_solo = 0u;
    for (auto & a : solo.get_agents())
        if (a.get_state() != epimodels::ModelSIR<>::SUSCEPTIBLE)
            ++reached_solo;
    REQUIRE(reached_solo == 1u);

    // A grouping drawn at every reset: groups of 100, 100 and 50, the rest in
    // none; each seed its own draw, the same seed the same draw
    epimodels::ModelSIR<> drawn("Flu", n, 0.0, 1.0, 0.01);
    drawn.verbose_off();
    add_group_mixing(drawn, cm, grouping::random<>({100.0, 100.0, 50.0}));
    auto & dl = drawn.get_layer_as< layer::Mixing<> >("mixing");
    auto drawn_groups = [&]() -> std::vector< size_t > {
        std::vector< size_t > g(n);
        for (size_t i = 0u; i < n; ++i)
            g[i] = dl.get_group(i);
        return g;
    };

    drawn.run(1, 1);
    const auto g1 = drawn_groups();
    drawn.run(1, 2);
    const auto g2 = drawn_groups();
    drawn.run(1, 1);
    REQUIRE(g1 == drawn_groups());
    REQUIRE(g1 != g2);
    REQUIRE(std::count(g1.begin(), g1.end(), 0u) == 100);
    REQUIRE(std::count(g1.begin(), g1.end(), 2u) == 50);
    REQUIRE(std::count(g1.begin(), g1.end(), no_group) == 50);

    // A move between runs would be lost at the next draw: an error
    REQUIRE_THROWS(dl.set_group(0u, 1u));

    // A mixing layer needs its groups
    epimodels::ModelSIR<> ungrouped("Flu", n, 0.0, 1.0, 0.01);
    ungrouped.verbose_off();
    auto bare = layer::mixing<>("mixing");
    bare->set_contact_matrix({1.0});
    ungrouped.add_layer(bare);
    REQUIRE_THROWS(ungrouped.run(1, 1));

}
