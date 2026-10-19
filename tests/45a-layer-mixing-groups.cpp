#include "tests.hpp"

using namespace epiworld;

// A mixing layer groups agents by its own table, not by entities. Here the model
// has no entities at all: the layer is given a grouping in which an agent's
// group is its id modulo 3, and some agents are left out of the layer. With
// contacts only within groups and a fully transmissible virus, the infection
// that starts in agent 0 must fill group 0 and nothing else.
EPIWORLD_TEST_CASE("Mixing layer groups agents on its own", "[layer-mixing-groups]") {

    const size_t n = 300u;
    const size_t no_group = layer::Mixing<>::NO_GROUP;

    // Within-group contacts only (column-major, 3 x 3)
    std::vector< double > cm = {
        20.0, 0.0, 0.0,
        0.0, 20.0, 0.0,
        0.0, 0.0, 20.0
    };

    epimodels::ModelSIRMixing<> model("Flu", n, 0.0, 1.0, 0.01, cm);

    Virus<> v = model.get_virus(0);
    model.rm_virus(0);
    v.set_distribution(dist_virus<>(0));
    model.add_virus(v);

    // Agents 270-299 take no part in the layer (one of them is in group 0's
    // residue class, 273)
    std::vector< size_t > groups(n);
    for (size_t i = 0u; i < n; ++i)
        groups[i] = (i >= 270u) ? no_group : (i % 3u);

    model.mixing().set_groups(groups, 3u);

    model.verbose_off();
    model.run(20, 1912);

    size_t in_group[3] = {0u, 0u, 0u};
    size_t outside = 0u;
    for (size_t i = 0u; i < n; ++i)
    {
        const bool reached =
            model.get_agent(i).get_state() != epimodels::ModelSIRMixing<>::SUSCEPTIBLE;

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
    REQUIRE(model.mixing().get_n_groups() == 3u);
    REQUIRE(model.mixing().get_group(4u) == 1u);
    REQUIRE(model.mixing().in_layer(4u));
    REQUIRE_FALSE(model.mixing().in_layer(273u));

    // The layers of the model are reachable by name and by id
    REQUIRE(model.get_n_layers() == 1u);
    REQUIRE(model.get_layer_id("mixing") == 0u);
    REQUIRE(model.get_layer("mixing").get_name() == "mixing");

}
