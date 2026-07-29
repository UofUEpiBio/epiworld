#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;
using MS = measles::ModelMeaslesSchool<>;

// The exposed group offered PEP is, by default, the whole population: when
// a case is identified, everyone is assumed to have been exposed. That is
// reasonable for a single classroom, but misleading for a population that
// is really a set of separate communities.
//
// The intervention therefore takes an optional group (e.g. classroom)
// membership vector, one entry per agent. PEP is then offered only within
// the group(s) the identified case(s) belong to, and each group is timed
// from its own exposure. An empty vector keeps the original behavior.
//
// As in 14i, the exposure history is injected by hand so the rules can be
// checked deterministically.

namespace {

constexpr int TODAY = 10;
constexpr int PEP_WINDOW = 3;
constexpr size_t N_AGENTS = 20u;
constexpr size_t GROUP_SIZE = 10u;

// Agents 0-9 are in group 0, agents 10-19 in group 1.
std::vector< int > two_groups()
{
    std::vector< int > groups(N_AGENTS, 0);
    for (size_t i = GROUP_SIZE; i < N_AGENTS; ++i)
        groups[i] = 1;

    return groups;
}

MS make_school()
{
    MS model(
        N_AGENTS, // Number of agents
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

    model.verbose_off();
    model.run(TODAY, 3123);

    return model;
}

// Susceptible agents are the PEP target here so that we only need to check
// whether the PEP tool was administered.
measles::InterventionMeaslesPEP<> make_pep(std::vector< int > groups = {})
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
        {MS::SUSCEPTIBLE},
        groups
    );
}

// Number of agents holding PEP, restricted to ids in [from, to).
size_t count_pep(MS & model, size_t from = 0u, size_t to = N_AGENTS)
{
    size_t n = 0u;
    for (size_t i = from; i < to; ++i)
    {
        auto & agent = model.get_agent(i);
        if (agent.has_tool("PEP MMR") || agent.has_tool("PEP IG"))
            ++n;
    }

    return n;
}

} // namespace

EPIWORLD_TEST_CASE(
    "Measles PEP can be restricted to the case's group",
    "[ModelMeaslesPEP]"
) {

    // -----------------------------------------------------------------
    // (1) No groups given: the whole population is one exposed group, so
    //     everyone is offered PEP. This is the default and must not
    //     change.
    // -----------------------------------------------------------------
    {
        auto model = make_school();
        auto pep = make_pep();

        model.get_contact_tracing().add_contact(0, 1, 9);
        model.add_triggering_agent(model, model.get_agent(0), 5);

        pep(&model, model.today());
        model.events_run();

        REQUIRE(count_pep(model) == N_AGENTS);
    }

    // -----------------------------------------------------------------
    // (2) With groups, only the group the identified case belongs to is
    //     offered PEP. The index (agent 0) is in group 0, so group 1 is
    //     left alone even though the timing is identical.
    // -----------------------------------------------------------------
    {
        auto model = make_school();
        auto pep = make_pep(two_groups());

        model.get_contact_tracing().add_contact(0, 1, 9);
        model.add_triggering_agent(model, model.get_agent(0), 5);

        pep(&model, model.today());
        model.events_run();

        REQUIRE(count_pep(model, 0u, GROUP_SIZE) == GROUP_SIZE);
        REQUIRE(count_pep(model, GROUP_SIZE, N_AGENTS) == 0u);
    }

    // -----------------------------------------------------------------
    // (3) A case in the other group is dosed instead when it is the one
    //     identified. Nothing about group 0 is special.
    // -----------------------------------------------------------------
    {
        auto model = make_school();
        auto pep = make_pep(two_groups());

        model.get_contact_tracing().add_contact(10, 11, 9);
        model.add_triggering_agent(model, model.get_agent(10), 5);

        pep(&model, model.today());
        model.events_run();

        REQUIRE(count_pep(model, 0u, GROUP_SIZE) == 0u);
        REQUIRE(count_pep(model, GROUP_SIZE, N_AGENTS) == GROUP_SIZE);
    }

    // -----------------------------------------------------------------
    // (4) Each group is timed from its OWN exposure.
    //
    //     Two cases are identified today. Group 0 was first exposed on
    //     day 9 (today - 9 = 1, within the window); group 1 on day 2
    //     (today - 2 = 8, long past it). Group 0 is dosed and group 1 is
    //     not.
    //
    //     Without per-group dating the earliest exposure across all cases
    //     would govern (day 2), and NOBODY would be dosed -- group 1's
    //     stale case would deny PEP to group 0.
    // -----------------------------------------------------------------
    {
        auto model = make_school();
        auto pep = make_pep(two_groups());

        auto & ct = model.get_contact_tracing();
        ct.add_contact(0, 1, 9);   // group 0: recent
        ct.add_contact(10, 11, 2); // group 1: stale

        model.add_triggering_agent(model, model.get_agent(0), 5);
        model.add_triggering_agent(model, model.get_agent(10), 1);

        pep(&model, model.today());
        model.events_run();

        REQUIRE(count_pep(model, 0u, GROUP_SIZE) == GROUP_SIZE);
        REQUIRE(count_pep(model, GROUP_SIZE, N_AGENTS) == 0u);
    }

    // -----------------------------------------------------------------
    // (5) Both groups are dosed when both were exposed recently, so
    //     restricting by group does not mean only one group can ever be
    //     covered.
    // -----------------------------------------------------------------
    {
        auto model = make_school();
        auto pep = make_pep(two_groups());

        auto & ct = model.get_contact_tracing();
        ct.add_contact(0, 1, 9);
        ct.add_contact(10, 11, 8);

        model.add_triggering_agent(model, model.get_agent(0), 5);
        model.add_triggering_agent(model, model.get_agent(10), 5);

        pep(&model, model.today());
        model.events_run();

        REQUIRE(count_pep(model) == N_AGENTS);
    }

    // -----------------------------------------------------------------
    // (6) Group labels are arbitrary integers, not indices: they only
    //     have to match between agents of the same group.
    // -----------------------------------------------------------------
    {
        auto model = make_school();

        std::vector< int > groups(N_AGENTS, -7);
        for (size_t i = GROUP_SIZE; i < N_AGENTS; ++i)
            groups[i] = 41;

        auto pep = make_pep(groups);

        model.get_contact_tracing().add_contact(0, 1, 9);
        model.add_triggering_agent(model, model.get_agent(0), 5);

        pep(&model, model.today());
        model.events_run();

        REQUIRE(count_pep(model, 0u, GROUP_SIZE) == GROUP_SIZE);
        REQUIRE(count_pep(model, GROUP_SIZE, N_AGENTS) == 0u);
    }

    // -----------------------------------------------------------------
    // (7) A group vector of the wrong length is a user error: it is
    //     indexed by agent id, so a short one would read out of bounds.
    // -----------------------------------------------------------------
    {
        auto model = make_school();
        auto pep = make_pep(std::vector< int >(N_AGENTS - 1u, 0));

        model.get_contact_tracing().add_contact(0, 1, 9);
        model.add_triggering_agent(model, model.get_agent(0), 5);

        REQUIRE_THROWS_AS(pep(&model, model.today()), std::length_error);
    }

}
