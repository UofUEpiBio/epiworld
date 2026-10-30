#include "tests.hpp"
#include <algorithm>
#include <limits>

using namespace epiworld;

// A graph (AdjList) holds each vertex's neighbors as compressed rows, built in
// one pass (a counting sort of the edge ends, see #274 and #339), and a network
// layer loads them as they are. The order of an agent's neighbors
// decides which transmitter roulette() picks, so the contract is exact:
//
// - undirected: i's neighbors are the distinct j with (i, j) or (j, i) in the
//   list, ascending. Repeats and reversed pairs collapse; a self-loop lists i
//   once in its own row.
// - directed: i's neighbors are the distinct targets of i's edges, ascending.
//
// This checks the contract on hand-picked edge cases and on a larger random
// list, plus the errors an invalid list raises.

// Neighbor ids of agent i, in the layer's order.
static std::vector< size_t > nbrs(Model<> & model, size_t i)
{
    std::vector< size_t > res;
    for (auto * p : tests_net(model).neighbors_view(i))
        res.push_back(static_cast< size_t >(p->get_id()));
    return res;
}

// What each agent's neighbors must be, computed from the raw edge list.
static std::vector< std::vector< size_t > > expected_rows(
    const std::vector< int > & source,
    const std::vector< int > & target,
    size_t n,
    bool directed
)
{
    std::vector< std::vector< size_t > > rows(n);
    for (size_t m = 0u; m < source.size(); ++m)
    {
        rows[source[m]].push_back(target[m]);
        if (!directed)
            rows[target[m]].push_back(source[m]);
    }
    for (auto & r : rows)
    {
        std::sort(r.begin(), r.end());
        r.erase(std::unique(r.begin(), r.end()), r.end());
    }
    return rows;
}

EPIWORLD_TEST_CASE("Edge list - neighbors are exact", "[edgelist]") {

    // -- edge cases ----------------------------------------------------------
    // 0 - 1 given three times, twice reversed; a self-loop at 2; agents 3 and
    // 5 are isolates; agent 4 is a hub above EPI_NEIGHBOR_INDEX_THRESHOLD,
    // with its ties listed in descending order.
    const size_t nhub = EPI_NEIGHBOR_INDEX_THRESHOLD + 8u;
    const size_t n    = 6u + nhub;

    std::vector< int > source = {0, 1, 1, 2, 2};
    std::vector< int > target = {1, 0, 0, 2, 0};
    for (size_t k = n - 1u; k >= 6u; --k)
    {
        source.push_back(4);
        target.push_back(static_cast< int >(k));
    }

    for (bool directed : {false, true})
    {

        AdjList graph(source, target, static_cast< int >(n), directed);

        epimodels::ModelSIR<> model("a virus", static_cast< int >(n), 0.1, 0.5, 0.3);
        model.verbose_off();
        model.add_layer(layer::network("network", graph));

        REQUIRE(tests_net(model).is_directed() == directed);
        REQUIRE(model.size() == n);

        // The graph holds the same rows as the layer, and counts the edges
        // as given
        auto rows = expected_rows(source, target, n, directed);
        for (size_t i = 0u; i < n; ++i)
        {
            REQUIRE(nbrs(model, i) == rows[i]);
            REQUIRE(tests_net(model).get_degree(i) == rows[i].size());

            auto row = graph(i);
            REQUIRE(std::vector< size_t >(row.begin(), row.end()) == rows[i]);
        }
        REQUIRE(graph.ecount() == source.size());
        REQUIRE_THROWS_AS(graph(n), std::range_error);

        if (!directed)
        {
            REQUIRE(nbrs(model, 0u) == std::vector< size_t >({1u, 2u}));
            REQUIRE(nbrs(model, 1u) == std::vector< size_t >({0u}));
        }
        else
        {
            REQUIRE(nbrs(model, 0u) == std::vector< size_t >({1u}));
            REQUIRE(nbrs(model, 1u) == std::vector< size_t >({0u}));
        }

        REQUIRE(nbrs(model, 2u) == std::vector< size_t >({0u, 2u}));
        REQUIRE(tests_net(model).has_edge(2u, 2u));
        REQUIRE(tests_net(model).get_degree(3u) == 0u);
        REQUIRE(tests_net(model).get_degree(5u) == 0u);
        REQUIRE(tests_net(model).get_degree(4u) == nhub);

        // The hub's lookups go through its index; every tie must be found.
        for (size_t k = 6u; k < n; ++k)
        {
            REQUIRE(tests_net(model).has_edge(4u, k));
            REQUIRE(tests_net(model).has_edge(k, 4u) == !directed);
        }
        REQUIRE_FALSE(tests_net(model).has_edge(4u, 3u));

        // Undirected: editing the hub keeps its index in step.
        if (!directed)
        {
            REQUIRE(tests_net(model).rm_edge(4u, 10u));
            REQUIRE_FALSE(tests_net(model).has_edge(4u, 10u));
            REQUIRE(tests_net(model).has_edge(4u, 11u));
            REQUIRE(tests_net(model).get_degree(4u) == nhub - 1u);
            REQUIRE(tests_net(model).add_edge(4u, 10u));
            REQUIRE(nbrs(model, 4u).back() == 10u);
        }

        // write_edgelist() reports the same ties (once each if undirected).
        std::vector< int > s, t;
        tests_net(model).get_edgelist(s, t);
        std::vector< std::pair< size_t, size_t > > got, want;
        for (size_t m = 0u; m < s.size(); ++m)
            if (directed)
                got.emplace_back(s[m], t[m]);
            else
                got.emplace_back(
                    std::min(s[m], t[m]), std::max(s[m], t[m])
                );
        for (size_t i = 0u; i < n; ++i)
            for (auto j : rows[i])
                if (directed || (i <= j))
                    want.emplace_back(i, j);
        std::sort(got.begin(), got.end());
        std::sort(want.begin(), want.end());
        REQUIRE(got == want);

    }

    // -- a larger random list --------------------------------------------------
    std::mt19937 gen(274u);
    const int nbig = 2000;
    std::vector< int > bs, bt;
    for (int m = 0; m < 20000; ++m)
    {
        int a = static_cast< int >(gen() % nbig);
        int b = static_cast< int >(gen() % nbig);
        bs.push_back(a);
        bt.push_back(b);
        if (m % 7 == 0) // some reversed repeats
        {
            bs.push_back(b);
            bt.push_back(a);
        }
    }

    for (bool directed : {false, true})
    {
        Model<> model(nbig);
        model.add_layer(layer::network(
            "network", AdjList(bs, bt, nbig, directed)
        ));
        auto rows = expected_rows(bs, bt, nbig, directed);
        for (size_t i = 0u; i < static_cast< size_t >(nbig); ++i)
            REQUIRE(nbrs(model, i) == rows[i]);
    }

    // -- errors ----------------------------------------------------------------
    // A bad list throws, and a graph of the wrong size is refused at reset.
    REQUIRE_THROWS_AS(AdjList({0, 1}, {1, 3}, 3, false), std::range_error);
    REQUIRE_THROWS_AS(AdjList({3, 1}, {1, 2}, 3, true), std::range_error);
    REQUIRE_THROWS_AS(
        AdjList(std::vector< int >{-1}, std::vector< int >{1}, 3, false),
        std::range_error
    );
    REQUIRE_THROWS_AS(AdjList({0, 1}, {1}, 3, false), std::length_error);
    REQUIRE_THROWS_AS(
        AdjList(std::vector< int >{}, std::vector< int >{}, -1, false),
        std::length_error
    );
    REQUIRE_THROWS_AS(
        AdjList(
            std::vector< int >{}, std::vector< int >{},
            std::numeric_limits< int >::min(), false
        ),
        std::length_error
    );

    Model<> model(3);
    model.add_state("Susceptible");
    model.add_layer(layer::network(
        "network", AdjList({0, 1}, {1, 2}, 3, false)
    ));

    Model<> wrong_size(4);
    wrong_size.add_state("Susceptible");
    wrong_size.add_layer(layer::network(
        "network", AdjList({0, 1}, {1, 2}, 3, false)
    ));
    wrong_size.verbose_off();
    REQUIRE_THROWS_AS(wrong_size.run(1, 1), std::length_error);

    REQUIRE(model.size() == 3u);
    REQUIRE(nbrs(model, 1u) == std::vector< size_t >({0u, 2u}));

    // An empty list gives isolates.
    Model<> empty(4);
    empty.add_layer(layer::network(
        "network", AdjList(std::vector< int >{}, std::vector< int >{}, 4, false)
    ));
    REQUIRE(tests_net(empty).size() == 4u);
    for (size_t i = 0u; i < 4u; ++i)
        REQUIRE(tests_net(empty).get_degree(i) == 0u);

}
