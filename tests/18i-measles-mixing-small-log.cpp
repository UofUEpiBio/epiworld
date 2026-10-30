#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

// A log the user sized below EPI_MAX_TRACKING bounds what the quarantine
// reads (it wraps, so only its capacity is there), and counts to distribute
// to groups are whole numbers, capped at the group's size.
EPIWORLD_TEST_CASE(
    "Measles mixing models with a one-slot layer log",
    "[ModelMeaslesMixingSmallLog]"
) {

    const size_t n = 100u;

    measles::ModelMeaslesMixing<> mixing(
        n, 0.1, 0.3, 1.0, 0.3, 7.0, 4.0, 5.0, 0.2, 7.0, 2.0, 21,
        .8, .8, 4, 0.0, 1.0, 4u
    );
    add_group_mixing(mixing, {50.0}, blocks({0u, n}));
    mixing.get_layer("mixing").enable_tracing(1u);

    measles::ModelMeaslesMixingRiskQuarantine<> risk(
        n, 0.1, 0.3, 0.9, 7.0, 4.0, 5.0, 0.2, 7.0, 3.0, 21, 14, 7,
        .8, .8, 4, 0.0, 0.5, 1.0, 4u
    );
    add_group_mixing(risk, {50.0}, blocks({0u, n}));
    risk.get_layer("mixing").enable_tracing(1u);

    for (Model<> * m : std::vector< Model<> * >{&mixing, &risk})
    {
        m->verbose_off();
        REQUIRE_NOTHROW(m->run(30, 42));
        REQUIRE(m->get_layer("mixing").get_tracing().get_max_contacts() == 1u);
    }

    // A count beyond the group (or beyond size_t) takes the whole group
    mixing.get_virus(0).set_distribution(
        distribute_virus_to_groups<>("mixing", {1e20}, false)
    );
    mixing.run(0, 1);
    REQUIRE(mixing.get_db().get_today_total("Susceptible") == 0);

    // Not a whole number of agents
    for (double bad : {std::numeric_limits< double >::infinity(), 1.5})
        REQUIRE_THROWS(distribute_virus_to_groups<>("mixing", {bad}, false));
    REQUIRE_THROWS(
        distribute_tool_to_groups<>("mixing", {std::nan("")}, true)
    );

}
