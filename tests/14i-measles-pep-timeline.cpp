#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;
using MS = measles::ModelMeaslesSchool<>;

// This test isolates the *timeline* logic of the measles PEP intervention.
//
// The eligibility for PEP is a class-level decision: it depends on how long
// ago the class was last exposed to the index case (i.e., the last time the
// index was seen in school while infectious), compared to today (the day the
// case is detected and PEP is administered). It must NOT depend on when a
// particular classmate contacted the index within the infectious window.
//
// To exercise this deterministically we build a school model with no initial
// infections (so the dynamics leave the contact-tracing structure empty),
// advance the clock to a known day, then inject a controlled set of contacts
// and triggering (index) agents by hand before invoking the intervention.
EPIWORLD_TEST_CASE("Measles PEP respects the exposure timeline", "[ModelMeaslesPEP]") {

    constexpr int n_agents = 20;
    constexpr int mmr_window = 3;
    constexpr int ig_window = 3;

    MS model(
        n_agents, // Number of agents
        0,        // Number of initial cases (none: keeps the run inert)
        5.0,      // Contact rate
        0.1,      // Transmission rate
        0.9,      // Vaccination efficacy
        0.3,      // Vaccination reduction recovery rate
        7.0,      // Incubation period
        4.0,      // Prodromal period
        5.0,      // Rash period
        3,        // Days undetected
        0.1,      // Hospitalization rate
        7.0,      // Hospitalization duration
        0.0,      // Proportion vaccinated (none, to keep agents tool-free)
        21,       // Quarantine period
        1.0,      // Quarantine willingness
        4         // Isolation period
    );

    // Susceptible agents are the PEP target here so that we only need to
    // check whether the PEP tool was administered.
    measles::InterventionMeaslesPEP<> pep(
        "Post-exposure prophylaxis for measles",
        1.0,                       // PEP MMR efficacy
        1.0,                       // PEP IG efficacy
        4.0 * 7.0,                 // PEP IG half-life (mean)
        7.0 / 2.0,                 // PEP IG half-life (sd)
        1.0,                       // PEP MMR willingness (everyone willing)
        1.0,                       // PEP IG willingness (everyone willing)
        static_cast<double>(mmr_window),
        static_cast<double>(ig_window),
        {MS::SUSCEPTIBLE},
        {MS::SUSCEPTIBLE},
        {MS::SUSCEPTIBLE}
    );

    // Advance the clock without any disease dynamics. With zero initial
    // cases nothing gets infected, so contact tracing stays empty and no
    // triggering agents are recorded during the run.
    const int today = 10;
    model.verbose_off();
    model.run(today, 3123);

    REQUIRE(model.today() == today);

    // ---------------------------------------------------------------
    // Inject a controlled exposure history.
    //
    //   Index A (agent 0), last seen by the class on day 9:
    //     - agent 1 contacted on day 9 (the last exposure)
    //     - agent 2 contacted on day 2 (early in the infectious window)
    //
    //   Index B (agent 10), last seen by the class on day 3:
    //     - agent 11 contacted on day 3 (the last exposure)
    //
    // With today == 10 and a 3-day window:
    //   * Index A's cohort: today - last_seen = 10 - 9 = 1 <= 3  -> PEP
    //     Both agent 1 AND agent 2 are offered PEP, even though agent 2's
    //     own contact was 8 days ago. This is the class-level behavior.
    //   * Index B's cohort: today - last_seen = 10 - 3 = 7  > 3  -> no PEP
    //     Agent 11 is NOT offered PEP, even though its contact happened
    //     within `window` days of the index becoming infectious.
    // ---------------------------------------------------------------
    auto & ct = model.get_contact_tracing();

    // Index A
    ct.add_contact(0, 1, 9);
    ct.add_contact(0, 2, 2);

    // Index B
    ct.add_contact(10, 11, 3);

    // Public health considers each index infectious from day 1 onwards.
    model.add_triggering_agent(model, model.get_agent(0), 1);
    model.add_triggering_agent(model, model.get_agent(10), 1);

    // Sanity: none of the classmates carry a PEP tool yet.
    REQUIRE_FALSE(model.get_agent(1).has_tool("PEP MMR"));
    REQUIRE_FALSE(model.get_agent(2).has_tool("PEP MMR"));
    REQUIRE_FALSE(model.get_agent(11).has_tool("PEP MMR"));

    // Administer PEP.
    pep(&model, model.today());
    model.events_run();

    // Index A cohort: still within the window -> both classmates get PEP,
    // regardless of *when* within the infectious window they were exposed.
    REQUIRE(model.get_agent(1).has_tool("PEP MMR"));
    REQUIRE(model.get_agent(2).has_tool("PEP MMR"));

    // Index B cohort: the last class exposure is older than the window, so
    // no PEP is administered (neither MMR nor IG).
    REQUIRE_FALSE(model.get_agent(11).has_tool("PEP MMR"));
    REQUIRE_FALSE(model.get_agent(11).has_tool("PEP IG"));

}
