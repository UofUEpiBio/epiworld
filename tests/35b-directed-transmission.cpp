#include "tests.hpp"

using namespace epiworld;

// In a directed network, a tie i -> j puts j among i's neighbors, and agents
// catch viruses from their neighbors: i is exposed to j. So j can infect i,
// never the reverse, and every recorded transmission runs along a tie from the
// infected agent to its infector.
//
// Directed networks always pull, and they update every agent instead of the
// queued ones: the queue marks the neighbors of an agent that becomes
// infectious, which here are the agents it is exposed to, not the ones it can
// infect. Results must not depend on the queue or on the requested transmission
// mode.

namespace {

struct Outcome {
    std::vector< int > date, source, target, virus, exposure, hist;
    std::vector< epiworld_fast_uint > states;
};

Outcome outcome_of(Model<> & model)
{
    Outcome o;
    model.get_db().get_transmissions(
        o.date, o.source, o.target, o.virus, o.exposure
    );

    std::vector< int > date;
    std::vector< std::string > state;
    model.get_db().get_hist_total(&date, &state, &o.hist);

    o.states = model.get_agents_states();
    return o;
}

void require_same(const Outcome & a, const Outcome & b)
{
    REQUIRE(a.date == b.date);
    REQUIRE(a.source == b.source);
    REQUIRE(a.target == b.target);
    REQUIRE(a.virus == b.virus);
    REQUIRE(a.exposure == b.exposure);
    REQUIRE(a.hist == b.hist);
    REQUIRE(a.states == b.states);
}

// Every transmission (seeds have source -1) went from an agent to one that
// lists it among its neighbors. Returns how many there were.
size_t require_along_ties(Model<> & model, const Outcome & o)
{
    size_t n = 0u;
    for (size_t k = 0u; k < o.source.size(); ++k)
    {
        if (o.source[k] < 0)
            continue;

        INFO("transmission " << o.source[k] << " -> " << o.target[k]);
        REQUIRE(model.has_edge(
            static_cast< size_t >(o.target[k]),
            static_cast< size_t >(o.source[k])
        ));
        ++n;
    }
    return n;
}

} // namespace

EPIWORLD_TEST_CASE("Directed networks - infection follows the ties", "[directed]") {

    // -- a one-way path ------------------------------------------------------
    // 0 -> 1 -> ... -> 9. With certain transmission and no recovery, a virus
    // seeded at 9 climbs the path one agent a day; seeded at 0 it goes nowhere.
    // Undirected, it reaches everyone either way.
    const int n = 10;
    std::vector< int > source, target;
    for (int i = 0; i + 1 < n; ++i)
    {
        source.push_back(i);
        target.push_back(i + 1);
    }

    auto run_path = [&](bool directed, size_t seed_agent, bool queuing) {
        epimodels::ModelSIR<> model("a virus", 0.0, 1.0, 0.0);
        model.verbose_off();
        model.agents_from_edgelist(source, target, n, directed);
        model.get_virus(0u).set_distribution(
            distribute_virus_to_set<>({seed_agent})
        );
        if (!queuing)
            model.queuing_off();
        model.run(2 * n, 42);
        return outcome_of(model);
    };

    const epiworld_fast_uint infected = 1u; // Susceptible, Infected, Recovered
    std::vector< epiworld_fast_uint > everyone(n, infected);

    for (bool queuing : {true, false})
    {

        INFO("queuing " << (queuing ? "on" : "off"));

        // Seeded at the end: on day d, agent 10 - d infects agent 9 - d.
        Outcome up = run_path(true, n - 1, queuing);
        REQUIRE(up.states == everyone);

        std::vector< int > date = {0}, from = {-1}, to = {n - 1};
        for (int d = 1; d < n; ++d)
        {
            date.push_back(d);
            from.push_back(n - d);
            to.push_back(n - 1 - d);
        }
        REQUIRE(up.date == date);
        REQUIRE(up.source == from);
        REQUIRE(up.target == to);

        // Seeded at the start: agent 0 is exposed to 1, but nobody to 0.
        Outcome down = run_path(true, 0u, queuing);
        std::vector< epiworld_fast_uint > only_0(n, 0u);
        only_0[0u] = infected;
        REQUIRE(down.states == only_0);
        REQUIRE(down.source == std::vector< int >({-1}));

        // Undirected, the same seed reaches everyone.
        REQUIRE(run_path(false, 0u, queuing).states == everyone);

    }

    // -- a random directed network -------------------------------------------
    // The same network and seeds, with queuing on and off and each transmission
    // mode requested: all runs must match, and they all pull.
    auto run_random = [](bool queuing, const char * mode) {
        epimodels::ModelSEIR<> model("a virus", 0.01, 0.5, 3.0, 0.2);
        model.verbose_off();
        model.seed(2026);
        model.agents_bernoulli(1500u, 0.004, true);
        if (!queuing)
            model.queuing_off();
        model.set_transmission_mode(mode);
        model.run(60, 99);

        REQUIRE(model.is_directed());
        REQUIRE(model.get_last_transmission_mode() == TransmissionMode::pull);

        Outcome o = outcome_of(model);

        // Enough transmissions for "they all follow a tie" to mean something.
        REQUIRE(require_along_ties(model, o) > 100u);

        return o;
    };

    Outcome reference = run_random(true, "pull");
    require_same(run_random(false, "pull"), reference);
    require_same(run_random(true, "push"), reference);
    require_same(run_random(true, "auto"), reference);
    require_same(run_random(false, "push"), reference);

    // -- mutation does not depend on the queue either --------------------------
    // Carriers of a virus that never registers in the queue (`NoOne`) must
    // still be offered a mutation every day. Along the one-way path seeded at
    // 9, agent 9 - d is infected on day d and nobody recovers, so the carriers
    // at the end of day d are agents 9 - d .. 9: 2 + 3 + ... + 10 over days
    // 1-9, then all 10 on each of days 10-15.
    for (bool queuing : {true, false})
    {

        epimodels::ModelSIR<> model("a virus", 0.0, 1.0, 0.0);
        model.verbose_off();
        model.agents_from_edgelist(source, target, n, true);

        size_t offered = 0u;
        auto & v = model.get_virus(0u);
        v.set_distribution(distribute_virus_to_set<>({static_cast<size_t>(n - 1)}));
        v.set_queue(Queue<int>::NoOne, Queue<int>::NoOne, Queue<int>::NoOne);
        v.set_mutation([&offered](Agent<> *, Virus<> &, Model<> *) -> bool {
            ++offered;
            return false;
        });

        if (!queuing)
            model.queuing_off();

        model.run(15, 42);

        INFO("queuing " << (queuing ? "on" : "off"));
        REQUIRE(model.get_agents_states() == everyone);
        REQUIRE(offered == 54u + 6u * 10u);

    }

}
