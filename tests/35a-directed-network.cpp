#include "tests.hpp"
#include <algorithm>

using namespace epiworld;

// A network built as directed stays directed: each tie `source -> target` is
// kept by its source only, so the model reports exactly the ties it was given,
// one-way ties stay one-way, and the edge-editing API (which works on both ends
// of a tie) refuses the model. Model::directed used to be left unset, which
// silently turned every directed network into an undirected one.

// The model's ties as the public API reports them, sorted.
static std::vector< std::pair<int,int> > ties_of(Model<> & model)
{
    std::vector< int > source, target;
    model.write_edgelist(source, target);

    std::vector< std::pair<int,int> > res;
    for (size_t i = 0u; i < source.size(); ++i)
        res.emplace_back(source[i], target[i]);

    std::sort(res.begin(), res.end());
    return res;
}

// In- and out-degree of every agent, from the ties the model reports.
static std::pair< std::vector<int>, std::vector<int> > degrees_of(Model<> & model)
{
    std::vector< int > indeg(model.size(), 0), outdeg(model.size(), 0);
    for (const auto & t : ties_of(model))
    {
        outdeg[t.first]++;
        indeg[t.second]++;
    }
    return {indeg, outdeg};
}

EPIWORLD_TEST_CASE("Directed networks - ties are kept as given", "[directed]") {

    // A one-way cycle 0 -> 1 -> 2 -> 0, a reciprocated pair 3 <-> 4, a hub (5)
    // pointing at 0..3, and an isolate (6). The tie 0 -> 1 is listed twice.
    std::vector< int > source = {0, 1, 2, 3, 4, 5, 5, 5, 5, 0};
    std::vector< int > target = {1, 2, 0, 4, 3, 0, 1, 2, 3, 1};
    int n = 7;

    std::vector< std::pair<int,int> > given = {
        {0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 3}, {5, 0}, {5, 1}, {5, 2}, {5, 3}
    };

    // -- directed ------------------------------------------------------------
    epimodels::ModelSIR<> model("a virus", 0.1, 0.5, 0.3);
    model.verbose_off();
    model.agents_from_edgelist(source, target, n, true);

    REQUIRE(model.is_directed());

    // Exactly the ties given (the repeat collapses), not their symmetrization.
    REQUIRE(ties_of(model) == given);

    // The source lists the target, and not the other way around.
    REQUIRE(model.has_edge(0u, 1u));
    REQUIRE_FALSE(model.has_edge(1u, 0u));
    REQUIRE(model.has_edge(3u, 4u));
    REQUIRE(model.has_edge(4u, 3u));
    REQUIRE(model.get_agent(5u).get_n_neighbors() == 4u);
    REQUIRE(model.get_agent(0u).get_n_neighbors() == 1u);
    REQUIRE(model.get_agent(6u).get_n_neighbors() == 0u);

    // add_edge()/rm_edge() change both ends of a tie; not for a directed model.
    REQUIRE_THROWS(model.add_edge(1u, 6u));
    REQUIRE_THROWS(model.rm_edge(0u, 1u));
    REQUIRE(ties_of(model) == given);

    // The network survives runs unchanged.
    model.run_multiple(10, 3, 123, nullptr, true, false, 1);
    REQUIRE(model.is_directed());
    REQUIRE(ties_of(model) == given);

    // -- the same ties, undirected -------------------------------------------
    // Every tie is reported once, with the reciprocated pair merged into one.
    epimodels::ModelSIR<> model_u("a virus", 0.1, 0.5, 0.3);
    model_u.verbose_off();
    model_u.agents_from_edgelist(source, target, n, false);

    std::vector< std::pair<int,int> > undirected = {
        {0, 1}, {0, 2}, {0, 5}, {1, 2}, {1, 5}, {2, 5}, {3, 4}, {3, 5}
    };

    REQUIRE_FALSE(model_u.is_directed());
    REQUIRE(ties_of(model_u) == undirected);
    REQUIRE(model_u.has_edge(1u, 0u));
    REQUIRE(model_u.get_agent(0u).get_n_neighbors() == 3u);

    // -- rebuilding the network replaces the direction -----------------------
    // The run_multiple() above backed the directed network up. The next runs
    // must use the new network, not bring the old one back.
    model.agents_from_edgelist(source, target, n, false);
    REQUIRE_FALSE(model.is_directed());
    model.run_multiple(10, 2, 124, nullptr, true, false, 1);
    REQUIRE_FALSE(model.is_directed());
    REQUIRE(model.size() == static_cast<size_t>(n));
    REQUIRE(ties_of(model) == undirected);
    REQUIRE(model.add_edge(1u, 6u));

    model.agents_from_edgelist(source, target, n, true);
    REQUIRE(model.is_directed());

    model.agents_empty_graph(n);
    REQUIRE_FALSE(model.is_directed());
    REQUIRE(ties_of(model).empty());
    REQUIRE(model.add_edge(0u, 1u));

    // -- generated networks: the model holds exactly the AdjList's ties -------
    {
        epimodels::ModelSIR<> m("a virus", 0.1, 0.5, 0.3);
        m.seed(77);
        AdjList al = rgraph_bernoulli(300u, 0.02, true, m);
        m.agents_from_adjlist(al);

        std::vector< std::pair<int,int> > expected;
        for (size_t i = 0u; i < al.get_dat().size(); ++i)
            for (const auto & link : al.get_dat()[i])
                expected.emplace_back(static_cast<int>(i), link.first);

        std::sort(expected.begin(), expected.end());

        REQUIRE(m.is_directed());
        REQUIRE(expected.size() == al.ecount());
        REQUIRE(ties_of(m) == expected);
    }

    // A directed small world is a ring of k out-ties per agent (plus rewiring),
    // not 2k ties at both ends.
    {
        epimodels::ModelSIR<> m("a virus", 0.1, 0.5, 0.3);
        m.seed(78);
        m.agents_smallworld(200u, 4u, true, 0.0);

        REQUIRE(m.is_directed());
        REQUIRE(ties_of(m).size() == 200u * 4u);
        for (size_t i = 0u; i < 200u; ++i)
        {
            REQUIRE(m.get_agent(i).get_n_neighbors() == 4u);
            REQUIRE(m.has_edge(i, (i + 1u) % 200u));
            REQUIRE_FALSE(m.has_edge((i + 1u) % 200u, i));
        }
    }

    // -- rewiring keeps ties one-way and both degree sequences ----------------
    {
        epimodels::ModelSIR<> m("a virus", 0.05, 0.5, 0.3);
        m.seed(79);
        m.verbose_off();
        m.agents_smallworld(300u, 6u, true, 0.0);

        auto ties0 = ties_of(m);
        auto deg0 = degrees_of(m);

        m.set_rewire_fun([](std::vector< Agent<> > * a, Model<> * mm, epiworld_double p) {
            rewire_degseq(a, mm, p);
        });
        m.set_rewire_prop(0.2);
        m.run(20, 80);

        auto ties1 = ties_of(m);
        REQUIRE(m.is_directed());
        REQUIRE(ties1.size() == ties0.size());
        REQUIRE(ties1 != ties0);
        REQUIRE(degrees_of(m) == deg0);

        // No self-ties and no repeated ties.
        REQUIRE(std::adjacent_find(ties1.begin(), ties1.end()) == ties1.end());
        for (const auto & t : ties1)
            REQUIRE(t.first != t.second);
    }

}
