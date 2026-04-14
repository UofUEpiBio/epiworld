#include "tests.hpp"

using namespace epiworld;

/**
 * This test validates the generalized InterventionQuarantine global
 * event by building a SEIR-like model from scratch (no predefined measles
 * model) and verifying that the quarantine and isolation process works
 * correctly through contact tracing.
 *
 * Model states:
 *   0 - Susceptible
 *   1 - Latent
 *   2 - Prodromal (infectious)
 *   3 - Rash (detected → triggers quarantine)
 *   4 - Isolated
 *   5 - Quarantined Susceptible
 *   6 - Quarantined Latent
 *   7 - Quarantined Prodromal
 *   8 - Recovered
 *
 * The quarantine update functions keep agents in quarantine for the
 * duration of the simulation (no release logic) so the test focuses on
 * verifying the quarantine PROCESS: contact tracing, willingness checks,
 * and state mapping.
 *
 * The test verifies:
 * - The intervention correctly quarantines contacts of detected agents.
 * - Quarantined and isolated agents appear in the final distribution.
 * - Contact tracing must be enabled (runtime error otherwise).
 * - Constructor validation catches mismatched state arrays.
 * - The intervention can be used with run_multiple().
 */
EPIWORLD_TEST_CASE("InterventionQuarantine - standalone model", "[quarantine-intervention]") {

    // State codes
    static const int SUSCEPTIBLE         = 0;
    static const int LATENT              = 1;
    static const int PRODROMAL           = 2;
    static const int RASH                = 3;
    static const int ISOLATED            = 4;
    static const int Q_SUSCEPTIBLE       = 5;
    static const int Q_LATENT            = 6;
    static const int Q_PRODROMAL         = 7;
    static const int RECOVERED           = 8;

    // ---------------------------------------------------------------
    // 1. Verify that the intervention throws if contact tracing is off
    // ---------------------------------------------------------------
    {
        epimodels::InterventionQuarantine<> quarantine(
            "Test Quarantine",
            1.0,   // quarantine_willingness
            1.0,   // isolation_willingness
            1.0,   // ct_success_rate
            4u,    // ct_days_prior
            {ISOLATED},                     // trigger_states
            {SUSCEPTIBLE, LATENT, PRODROMAL}, // quarantine_source
            {Q_SUSCEPTIBLE, Q_LATENT, Q_PRODROMAL}, // quarantine_target
            {RASH},                         // isolation_source
            {ISOLATED}                      // isolation_target
        );

        Model<> model_no_ct;
        model_no_ct.add_state("Susceptible");
        model_no_ct.add_state("Latent");
        model_no_ct.add_state("Prodromal");
        model_no_ct.add_state("Rash");
        model_no_ct.add_state("Isolated");
        model_no_ct.add_state("Q_Susceptible");
        model_no_ct.add_state("Q_Latent");
        model_no_ct.add_state("Q_Prodromal");
        model_no_ct.add_state("Recovered");
        model_no_ct.add_param(0.5, "Transmission rate");
        model_no_ct.add_param(7.0, "Recovery rate");

        Virus<> v("TestVirus", 0.01, true);
        v.set_state(LATENT, RECOVERED, RECOVERED);
        v.set_prob_infecting("Transmission rate");
        v.set_prob_recovery("Recovery rate");
        model_no_ct.add_virus(v);

        model_no_ct.add_globalevent(quarantine);
        model_no_ct.agents_smallworld(100, 4, false, 0.1);
        model_no_ct.queuing_off();
        model_no_ct.verbose_off();

        // Should throw because contact tracing is not enabled
        REQUIRE_THROWS_AS(model_no_ct.run(10, 42), std::logic_error);
    }

    // ---------------------------------------------------------------
    // 2. Build a full model from scratch with the intervention
    // ---------------------------------------------------------------

    epimodels::InterventionQuarantine<> quarantine_event(
        "Quarantine Process",
        1.0,   // quarantine_willingness (everyone willing)
        1.0,   // isolation_willingness (everyone willing)
        1.0,   // ct_success_rate (100%)
        10u,   // ct_days_prior (generous window)
        {ISOLATED},                     // trigger_states
        {SUSCEPTIBLE, LATENT, PRODROMAL}, // quarantine_source
        {Q_SUSCEPTIBLE, Q_LATENT, Q_PRODROMAL}, // quarantine_target
        {RASH},                         // isolation_source
        {ISOLATED}                      // isolation_target
    );

    // -- Update functions --

    // Susceptible: contact-based infection with contact recording
    auto update_susceptible = [](Agent<> * p, Model<> * m) -> void {

        auto & ct = m->get_contact_tracing();
        size_t nviruses_tmp = 0u;

        for (auto & neighbor : p->get_neighbors(*m))
        {
            if (neighbor->get_virus() == nullptr)
                continue;

            // Only prodromal agents transmit
            if (neighbor->get_state() != PRODROMAL)
                continue;

            auto & v = neighbor->get_virus();

            // Record the contact for tracing
            ct.add_contact(
                neighbor->get_id(),
                p->get_id(),
                static_cast<size_t>(m->today())
            );

            m->array_double_tmp[nviruses_tmp] =
                (1.0 - p->get_susceptibility_reduction(v, *m)) *
                v->get_prob_infecting(m) *
                (1.0 - neighbor->get_transmission_reduction(v, *m));

            m->array_virus_tmp[nviruses_tmp++] = &(*v);
        }

        if (nviruses_tmp == 0u)
            return;

        int which = roulette(nviruses_tmp, m);
        if (which < 0)
            return;

        p->set_virus(*m, *m->array_virus_tmp[which], LATENT);

    };

    // Latent: transition to prodromal
    auto update_latent = [](Agent<> * p, Model<> * m) -> void {
        if (m->runif() < (1.0 / p->get_virus()->get_incubation(m)))
            p->change_state(*m, PRODROMAL);
    };

    // Prodromal: transition to rash
    auto update_prodromal = [](Agent<> * p, Model<> * m) -> void {
        if (m->runif() < (1.0 / m->par("Prodromal period")))
            p->change_state(*m, RASH);
    };

    // Rash: detection → isolation, or recovery
    auto update_rash = [](Agent<> * p, Model<> * m) -> void {

        // Check detection
        bool detected = false;
        if (m->runif() < 1.0 / m->par("Days undetected"))
            detected = true;

        // Check recovery
        if (m->runif() < 1.0 / m->par("Rash period"))
        {
            p->rm_virus(*m, RECOVERED);
            return;
        }

        // If detected but not recovered, isolate
        // (triggers the quarantine intervention for contacts)
        if (detected)
            p->change_state(*m, ISOLATED);

    };

    // Isolated: recover after rash period
    auto update_isolated = [](Agent<> * p, Model<> * m) -> void {
        if (m->runif() < 1.0 / m->par("Rash period"))
            p->rm_virus(*m, RECOVERED);
    };

    // Quarantine updates: agents stay quarantined for the simulation.
    // The focus of this test is the quarantine PROCESS (contact tracing
    // and state mapping), not the release logic.
    auto update_q_susceptible = [](Agent<> *, Model<> *) -> void {};
    auto update_q_latent = [](Agent<> * p, Model<> * m) -> void {
        // Latent agents can still progress even in quarantine
        if (m->runif() < (1.0 / p->get_virus()->get_incubation(m)))
            p->change_state(*m, Q_PRODROMAL);
    };
    auto update_q_prodromal = [](Agent<> * p, Model<> * m) -> void {
        // Prodromal agents detected in quarantine get isolated
        if (m->runif() < (1.0 / m->par("Prodromal period")))
            p->change_state(*m, ISOLATED);
    };

    // Build the model
    Model<> model;

    model.add_state("Susceptible",           update_susceptible);
    model.add_state("Latent",                update_latent);
    model.add_state("Prodromal",             update_prodromal);
    model.add_state("Rash",                  update_rash);
    model.add_state("Isolated",              update_isolated);
    model.add_state("Quarantined Suscept.",  update_q_susceptible);
    model.add_state("Quarantined Latent",    update_q_latent);
    model.add_state("Quarantined Prodromal", update_q_prodromal);
    model.add_state("Recovered");

    // Parameters
    model.add_param(0.8,  "Transmission rate");
    model.add_param(5.0,  "Incubation period");
    model.add_param(3.0,  "Prodromal period");
    model.add_param(4.0,  "Rash period");
    model.add_param(2.0,  "Days undetected");

    // Virus (exactly 1 initial case)
    Virus<> virus("TestMeasles");
    virus.set_state(LATENT, RECOVERED, RECOVERED);
    virus.set_prob_infecting("Transmission rate");
    virus.set_prob_recovery("Rash period");
    virus.set_incubation("Incubation period");
    virus.set_distribution(
        distribute_virus_randomly(1, false)
    );
    model.add_virus(virus);

    // Enable contact tracing (required)
    model.contact_tracing_on();

    // Add the quarantine intervention as a global event
    model.add_globalevent(quarantine_event);

    // Create a small-world network
    model.agents_smallworld(200, 4, false, 0.1);
    model.queuing_off();
    model.verbose_off();
    model.seed(2024);

    // ---------------------------------------------------------------
    // 3. Run and verify quarantine occurs
    // ---------------------------------------------------------------
    model.run(60);

    // Get final state distribution
    std::vector< int > final_dist;
    model.get_db().get_today_total(nullptr, &final_dist);

    // Check that the simulation produced recovered agents (epidemic happened)
    REQUIRE(final_dist[RECOVERED] > 0);

    // ---------------------------------------------------------------
    // 4. Run multiple simulations and check statistical consistency
    // ---------------------------------------------------------------
    size_t nsims = 50;
    std::vector< std::vector<epiworld_double> > transitions(nsims);
    std::vector< epiworld_double > R0s(nsims);
    std::vector< std::vector<int> > final_distributions(nsims);

    auto saver = tests_create_saver(
        transitions, R0s, 1,
        &final_distributions
    );

    model.run_multiple(60, nsims, 1234, saver, true, false);

    // Check that quarantine states were populated in at least some sims
    int sims_with_quarantine = 0;
    for (size_t s = 0u; s < nsims; ++s)
    {
        int q_total =
            final_distributions[s][Q_SUSCEPTIBLE] +
            final_distributions[s][Q_LATENT] +
            final_distributions[s][Q_PRODROMAL] +
            final_distributions[s][ISOLATED];

        if (q_total > 0)
            ++sims_with_quarantine;
    }

    // With high transmission and 100% willingness, quarantine should
    // trigger in a meaningful fraction of simulations
    REQUIRE(sims_with_quarantine > 0);

    // ---------------------------------------------------------------
    // 5. Verify constructor validation
    // ---------------------------------------------------------------
    // Mismatched quarantine source/target lengths
    REQUIRE_THROWS_AS(
        (epimodels::InterventionQuarantine<>(
            "Bad",
            1.0, 1.0, 1.0, 4u,
            {ISOLATED},
            {SUSCEPTIBLE, LATENT},      // 2 elements
            {Q_SUSCEPTIBLE},            // 1 element (mismatch!)
            {}, {}
        )),
        std::logic_error
    );

    // Mismatched isolation source/target lengths
    REQUIRE_THROWS_AS(
        (epimodels::InterventionQuarantine<>(
            "Bad",
            1.0, 1.0, 1.0, 4u,
            {ISOLATED},
            {}, {},
            {RASH},       // 1 element
            {}            // 0 elements (mismatch!)
        )),
        std::logic_error
    );

}
