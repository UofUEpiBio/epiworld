#include "tests.hpp"
#include <algorithm>

using namespace epiworld;

// A network layer built as directed stays directed: each tie `source -> target`
// is kept by its source only, so the layer reports exactly the ties it was
// given, one-way ties stay one-way, and the edge-editing API (which works on
// both ends of a tie) refuses the layer.

// The model's ties as the public API reports them, sorted.
static std::vector< std::pair<int,int> > ties_of(Model<> & model)
{
    std::vector< int > source, target;
    tests_net(model).get_edgelist(source, target);

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
    epimodels::ModelSIR<> model("a virus", n, 0.1, 0.5, 0.3);
    model.verbose_off();
    model.add_layer(layer::network(
        "network", AdjList(source, target, n, true)
    ));

    REQUIRE(tests_net(model).is_directed());

    // Exactly the ties given (the repeat collapses), not their symmetrization.
    REQUIRE(ties_of(model) == given);

    // The source lists the target, and not the other way around.
    REQUIRE(tests_net(model).has_edge(0u, 1u));
    REQUIRE_FALSE(tests_net(model).has_edge(1u, 0u));
    REQUIRE(tests_net(model).has_edge(3u, 4u));
    REQUIRE(tests_net(model).has_edge(4u, 3u));
    REQUIRE(tests_net(model).get_degree(5u) == 4u);
    REQUIRE(tests_net(model).get_degree(0u) == 1u);
    REQUIRE(tests_net(model).get_degree(6u) == 0u);

    // add_edge()/rm_edge() change both ends of a tie; not for a directed layer.
    REQUIRE_THROWS(tests_net(model).add_edge(1u, 6u));
    REQUIRE_THROWS(tests_net(model).rm_edge(0u, 1u));
    REQUIRE(ties_of(model) == given);

    // The network survives runs unchanged.
    model.run_multiple(10, 3, 123, nullptr, true, false, 1);
    REQUIRE(tests_net(model).is_directed());
    REQUIRE(ties_of(model) == given);

    // -- the same ties, undirected -------------------------------------------
    // Every tie is reported once, with the reciprocated pair merged into one.
    epimodels::ModelSIR<> model_u("a virus", n, 0.1, 0.5, 0.3);
    model_u.verbose_off();
    model_u.add_layer(layer::network(
        "network", AdjList(source, target, n, false)
    ));

    std::vector< std::pair<int,int> > undirected = {
        {0, 1}, {0, 2}, {0, 5}, {1, 2}, {1, 5}, {2, 5}, {3, 4}, {3, 5}
    };

    REQUIRE_FALSE(tests_net(model_u).is_directed());
    REQUIRE(ties_of(model_u) == undirected);
    REQUIRE(tests_net(model_u).has_edge(1u, 0u));
    REQUIRE(tests_net(model_u).get_degree(0u) == 3u);

    // -- generated networks: the layer holds exactly the AdjList's ties -------
    {
        epimodels::ModelSIR<> m("a virus", 300u, 0.1, 0.5, 0.3);
        m.seed(77);
        AdjList al = rgraph_bernoulli(300u, 0.02, true, m);
        m.add_layer(layer::network("network", al));

        std::vector< std::pair<int,int> > expected;
        for (size_t i = 0u; i < al.vcount(); ++i)
            for (int j : al(i))
                expected.emplace_back(static_cast<int>(i), j);

        std::sort(expected.begin(), expected.end());

        REQUIRE(tests_net(m).is_directed());
        REQUIRE(expected.size() == al.ecount());
        REQUIRE(ties_of(m) == expected);
    }

    // An undirected Bernoulli graph: symmetric, no self-ties, and about
    // p * n * (n - 1) / 2 ties (sd ~ 30 here)
    {
        epimodels::ModelSIR<> m("a virus", 300u, 0.1, 0.5, 0.3);
        m.seed(77);
        m.add_layer(layer::network(
            "network", rgraph_bernoulli(300u, 0.02, false, m)
        ));

        auto ties = ties_of(m);
        REQUIRE_FALSE(tests_net(m).is_directed());
        REQUIRE(ties.size() > 897u - 150u);
        REQUIRE(ties.size() < 897u + 150u);
        for (const auto & t : ties)
        {
            REQUIRE(t.first != t.second);
            REQUIRE(tests_net(m).has_edge(t.second, t.first));
        }

        size_t degree_sum = 0u;
        for (size_t i = 0u; i < m.size(); ++i)
            degree_sum += tests_net(m).get_degree(i);
        REQUIRE(degree_sum == 2u * ties.size());
    }

    // A directed small world is a ring of k out-ties per agent (plus rewiring),
    // not 2k ties at both ends.
    {
        epimodels::ModelSIR<> m("a virus", 200u, 0.1, 0.5, 0.3);
        m.seed(78);
        m.add_layer(layer::network(
            "network", rgraph_smallworld(200u, 4u, 0.0, true, m)
        ));

        REQUIRE(tests_net(m).is_directed());
        REQUIRE(ties_of(m).size() == 200u * 4u);
        for (size_t i = 0u; i < 200u; ++i)
        {
            REQUIRE(tests_net(m).get_degree(i) == 4u);
            REQUIRE(tests_net(m).has_edge(i, (i + 1u) % 200u));
            REQUIRE_FALSE(tests_net(m).has_edge((i + 1u) % 200u, i));
        }
    }

    // -- rewiring keeps ties one-way and both degree sequences ----------------
    {
        epimodels::ModelSIR<> m("a virus", 300u, 0.05, 0.5, 0.3);
        m.seed(79);
        m.verbose_off();
        m.add_layer(layer::network(
            "network", rgraph_smallworld(300u, 6u, 0.0, true, m)
        ));

        auto ties0 = ties_of(m);
        auto deg0 = degrees_of(m);

        m.add_param(0.2, "Rewiring proportion");
        tests_net(m).set_rewiring(graph::rewire_degseq("Rewiring proportion"));
        m.run(20, 80);

        auto ties1 = ties_of(m);
        REQUIRE(tests_net(m).is_directed());
        REQUIRE(ties1.size() == ties0.size());
        REQUIRE(ties1 != ties0);
        REQUIRE(degrees_of(m) == deg0);

        // No self-ties and no repeated ties.
        REQUIRE(std::adjacent_find(ties1.begin(), ties1.end()) == ties1.end());
        for (const auto & t : ties1)
            REQUIRE(t.first != t.second);
    }

}
