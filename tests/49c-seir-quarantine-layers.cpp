#include "tests.hpp"

using namespace epiworld;

// ModelSEIRQuarantine traces every layer it is given. A carrier (agent 0) in a
// ring network and in a household of four is detected the day it becomes
// infectious; nobody is infected (transmission probability 0), but its
// contacts that day are in the logs, so its two ring neighbors and its three
// housemates are quarantined, and nobody else.
EPIWORLD_TEST_CASE("SEIR quarantine traces every layer", "[seir-quarantine]") {

    const size_t n = 40u;
    using Model_ = epimodels::ModelSEIRQuarantine<>;

    Model_ model(
        "v", n, 0.0,
        0.0,  // transmission_prob
        1.0,  // latent_period (one day)
        0.0,  // recovery_rate
        0.0,  // hospitalization_rate
        7.0,  // hospitalization_period
        0.0,  // detection_delay (detected at once)
        14,   // quarantine_period
        1.0,  // quarantine_willingness
        1.0,  // isolation_willingness
        14,   // isolation_period
        1.0,  // contact_tracing_prob
        4u    // contact_tracing_window
    );
    model.get_virus(0).set_distribution(distribute_virus_to_set<>({0u}));

    std::vector< int > source, target;
    for (size_t i = 0u; i < n; ++i)
    {
        source.push_back(static_cast< int >(i));
        target.push_back(static_cast< int >((i + 1u) % n));
    }
    model.add_layer(layer::network(
        "ring", AdjList(source, target, static_cast< int >(n), false)
    ));

    std::vector< size_t > households(n);
    for (size_t i = 0u; i < n; ++i)
        households[i] = i / 4u;

    auto hh = layer::mixing<>("households");
    hh->set_groups(households, n / 4u);
    hh->set_contact_prob(1.0);
    model.add_layer(hh);

    model.verbose_off();
    model.run(3, 1);

    // Both layers were traced, the household log by the model itself
    REQUIRE(model.get_layer("ring").is_tracing());
    REQUIRE(model.get_layer("households").is_tracing());

    std::vector< size_t > quarantined;
    for (const auto & a : model.get_agents())
        if (a.get_state() == Model_::QUARANTINED_SUSCEPTIBLE)
            quarantined.push_back(static_cast< size_t >(a.get_id()));

    REQUIRE(model.get_agent(0).get_state() == static_cast< unsigned int >(Model_::ISOLATED));
    REQUIRE_THAT(quarantined, Catch::Equals(std::vector< size_t >{1, 2, 3, 39}));

}
