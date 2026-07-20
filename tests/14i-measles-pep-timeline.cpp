#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;
using MS = measles::ModelMeaslesSchool<>;

// This test isolates the *timeline* logic of the measles PEP intervention.
//
// Public health does not trace individual contacts here. When a case is
// identified, the whole school is assumed to have been exposed, and the only
// question is how long ago that exposure started: MMR/IG must be given within
// a few days of the exposure. Given an index case with rash onset on day `d`,
// the index is considered infectious from `d - prodromal_period`, and the
// reference date is the FIRST day on or after that on which the class actually
// encountered the index. Contact tracing is used only to date that first
// encounter (which matters when the class is not in session every day), never
// to decide who was exposed.
//
// To exercise this deterministically we build a school model with no initial
// infections (so the dynamics leave contact tracing empty), advance the clock
// to a known day, then inject a controlled exposure history by hand before
// invoking the intervention.

namespace {

constexpr int TODAY = 10;
constexpr int PEP_WINDOW = 3;

MS make_school()
{
    MS model(
        20,  // Number of agents
        0,   // Number of initial cases (none: keeps the run inert)
        5.0, // Contact rate
        0.1, // Transmission rate
        0.9, // Vaccination efficacy
        0.3, // Vaccination reduction recovery rate
        7.0, // Incubation period
        4.0, // Prodromal period
        5.0, // Rash period
        3,   // Days undetected
        0.1, // Hospitalization rate
        7.0, // Hospitalization duration
        0.0, // Proportion vaccinated (none, to keep agents tool-free)
        21,  // Quarantine period
        1.0, // Quarantine willingness
        4    // Isolation period
    );

    model.verbose_off();
    model.run(TODAY, 3123);

    return model;
}

// Susceptible agents are the PEP target here so that we only need to check
// whether the PEP tool was administered.
measles::InterventionMeaslesPEP<> make_pep()
{
    return measles::InterventionMeaslesPEP<>(
        "Post-exposure prophylaxis for measles",
        1.0,       // PEP MMR efficacy
        1.0,       // PEP IG efficacy
        4.0 * 7.0, // PEP IG half-life (mean)
        7.0 / 2.0, // PEP IG half-life (sd)
        1.0,       // PEP MMR willingness (everyone willing)
        1.0,       // PEP IG willingness (everyone willing)
        static_cast<double>(PEP_WINDOW),
        static_cast<double>(PEP_WINDOW),
        {MS::SUSCEPTIBLE},
        {MS::SUSCEPTIBLE},
        {MS::SUSCEPTIBLE}
    );
}

size_t count_pep(MS & model)
{
    size_t n = 0u;
    for (size_t i = 0u; i < model.size(); ++i)
    {
        auto & agent = model.get_agent(i);
        if (agent.has_tool("PEP MMR") || agent.has_tool("PEP IG"))
            ++n;
    }

    return n;
}

} // namespace

EPIWORLD_TEST_CASE(
    "Measles PEP dates the exposure from the first encounter",
    "[ModelMeaslesPEP]"
) {

    // -----------------------------------------------------------------
    // (1) The class first met the index 2 days ago: within the window, so
    //     the WHOLE school is offered PEP -- including agents that were
    //     never recorded as having met the index.
    // -----------------------------------------------------------------
    {
        auto model = make_school();
        auto pep = make_pep();

        REQUIRE(model.today() == TODAY);

        auto & ct = model.get_contact_tracing();
        ct.add_contact(0, 1, 8); // first encounter: day 8
        ct.add_contact(0, 2, 9);

        model.add_triggering_agent(model, model.get_agent(0), 1);

        REQUIRE(count_pep(model) == 0u);

        pep(&model, model.today());
        model.events_run();

        // today - first_seen = 10 - 8 = 2 <= 3
        REQUIRE(model.get_agent(1).has_tool("PEP MMR"));
        REQUIRE(model.get_agent(2).has_tool("PEP MMR"));

        // Agent 7 never met the index, but the whole school is treated as
        // exposed, so it is offered PEP just the same.
        REQUIRE(model.get_agent(7).has_tool("PEP MMR"));
        REQUIRE(count_pep(model) == model.size());
    }

    // -----------------------------------------------------------------
    // (2) The class first met the index 8 days ago, and met it again only
    //     yesterday. The FIRST encounter is what starts the clock, so it
    //     is too late for PEP -- even though the most recent encounter is
    //     well inside the window.
    // -----------------------------------------------------------------
    {
        auto model = make_school();
        auto pep = make_pep();

        auto & ct = model.get_contact_tracing();
        ct.add_contact(0, 1, 2); // first encounter: day 2
        ct.add_contact(0, 2, 9); // most recent encounter: day 9

        model.add_triggering_agent(model, model.get_agent(0), 1);

        pep(&model, model.today());
        model.events_run();

        // today - first_seen = 10 - 2 = 8 > 3 (dating this from the last
        // encounter instead would have given 10 - 9 = 1 and dosed everyone)
        REQUIRE(count_pep(model) == 0u);
    }

    // -----------------------------------------------------------------
    // (3) Encounters that happened before the index was considered
    //     infectious do not expose anyone, so they must not be used to
    //     date the exposure.
    // -----------------------------------------------------------------
    {
        auto model = make_school();
        auto pep = make_pep();

        auto & ct = model.get_contact_tracing();
        ct.add_contact(0, 1, 3); // before infectious onset: ignored
        ct.add_contact(0, 2, 9); // first encounter while infectious

        // Rash onset minus the prodromal period lands on day 7.
        model.add_triggering_agent(model, model.get_agent(0), 7);

        pep(&model, model.today());
        model.events_run();

        // first_seen = 9 (not 3), so today - first_seen = 1 <= 3
        REQUIRE(count_pep(model) == model.size());
    }

    // -----------------------------------------------------------------
    // (4) Two cases identified on the same day. Public health works from
    //     the EARLIEST exposure: the co-detected case whose rash started
    //     earlier dictates how much time is left, even though the other
    //     case only met the class yesterday.
    // -----------------------------------------------------------------
    {
        auto model = make_school();
        auto pep = make_pep();

        auto & ct = model.get_contact_tracing();
        ct.add_contact(0, 1, 2);   // case A: first encounter day 2
        ct.add_contact(10, 2, 9);  // case B: first encounter day 9

        model.add_triggering_agent(model, model.get_agent(0), 1);
        model.add_triggering_agent(model, model.get_agent(10), 1);

        pep(&model, model.today());
        model.events_run();

        // earliest first_seen = 2, so today - 2 = 8 > 3. Taking the most
        // recent case instead would have given 10 - 9 = 1 and dosed the
        // whole school.
        REQUIRE(count_pep(model) == 0u);
    }

}
