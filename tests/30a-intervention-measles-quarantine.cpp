#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("InterventionMeaslesQuarantine standalone", "[intervention-measles-quarantine]") {

    // ---------------------------------------------------------------
    // Build a minimal SEIR-like model from scratch that uses
    // InterventionMeaslesQuarantine as a global event.
    // ---------------------------------------------------------------

    // State codes
    constexpr int SUSCEPTIBLE  = 0;
    constexpr int LATENT       = 1;
    constexpr int INFECTIOUS   = 2;
    constexpr int QUARANTINED_SUSCEPTIBLE = 3;
    constexpr int QUARANTINED_LATENT      = 4;
    constexpr int ISOLATED                = 5;
    constexpr int RECOVERED              = 6;

    Model<> model;
    model.add_state("Susceptible");
    model.add_state("Latent");
    model.add_state("Infectious");
    model.add_state("Quarantined Susceptible");
    model.add_state("Quarantined Latent");
    model.add_state("Isolated");
    model.add_state("Recovered");

    // Parameters
    model.add_param(5.0, "Contact rate");
    model.add_param(0.3, "Transmission rate");
    model.add_param(5.0, "Incubation period");
    model.add_param(5.0, "Infectious period");
    model.add_param(7.0, "Quarantine period");
    model.add_param(1.0, "Quarantine willingness");
    model.add_param(0.0, "Isolation willingness");
    model.add_param(7.0, "Isolation period");

    // Virus
    Virus<> virus("TestVirus", 0.01, true);
    virus.set_state(LATENT, RECOVERED, RECOVERED);
    virus.set_prob_infecting("Transmission rate");
    virus.set_prob_recovery("Infectious period");
    virus.set_incubation("Incubation period");
    model.add_virus(virus);

    model.queuing_off();
    model.agents_empty_graph(500);

    // Create the quarantine intervention
    epimodels::InterventionMeaslesQuarantine<> quarantine(
        "Test Quarantine",
        1.0,  // quarantine willingness
        0.0,  // isolation willingness
        {SUSCEPTIBLE, LATENT},                        // quarantinable states
        {QUARANTINED_SUSCEPTIBLE, QUARANTINED_LATENT}, // targets
        INFECTIOUS,     // isolatable state
        ISOLATED,       // isolation state
        INFECTIOUS      // max base state
    );
    model.add_globalevent(quarantine);

    // ---------------------------------------------------------------
    // Test 1: With quarantine disabled (period = -1), no quarantine
    // ---------------------------------------------------------------
    model.set_param("Quarantine period", -1);
    model.run(30, 42);

    int q_susceptible = model.get_db().get_today_total("Quarantined Susceptible");
    int q_latent      = model.get_db().get_today_total("Quarantined Latent");
    int isolated      = model.get_db().get_today_total("Isolated");

    REQUIRE(q_susceptible == 0);
    REQUIRE(q_latent == 0);
    REQUIRE(isolated == 0);

    // ---------------------------------------------------------------
    // Test 2: With quarantine enabled and triggered, agents get
    //         quarantined.
    // ---------------------------------------------------------------

    // We need to trigger system quarantine. Let's add a global event
    // that triggers it on day 3 (unconditionally, since this minimal
    // model has no network for transmission).
    GlobalEvent<> trigger_event(
        [](Model<>* m) {
            if (m->today() != 3)
                return;
            auto & ge = m->get_globalevent("Test Quarantine");
            auto * q = dynamic_cast<epimodels::InterventionMeaslesQuarantine<>*>(&ge);
            q->trigger_system_quarantine();
        },
        "Trigger quarantine on day 3"
    );
    model.add_globalevent(trigger_event);

    model.set_param("Quarantine period", 7);
    model.run(30, 42);

    int q_susceptible2 = model.get_db().get_today_total("Quarantined Susceptible");
    int q_latent2      = model.get_db().get_today_total("Quarantined Latent");

    // With quarantine enabled and triggered, we should see quarantined agents
    REQUIRE(q_susceptible2 + q_latent2 > 0);

    // ---------------------------------------------------------------
    // Test 3: get_exposure_date / set_exposure_date work
    // ---------------------------------------------------------------
    auto & ge = model.get_globalevent("Test Quarantine");
    auto * q = dynamic_cast<epimodels::InterventionMeaslesQuarantine<>*>(&ge);

    q->set_exposure_date(0, 5);
    REQUIRE(q->get_exposure_date(0) == 5);

    q->set_exposure_date(10, 12);
    REQUIRE(q->get_exposure_date(10) == 12);

    // ---------------------------------------------------------------
    // Test 4: get_day_flagged / set_day_flagged work
    // ---------------------------------------------------------------
    q->set_day_flagged(0, 3);
    REQUIRE(q->get_day_flagged(0) == 3);

}
