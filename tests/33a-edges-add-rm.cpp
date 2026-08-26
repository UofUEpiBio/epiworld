#include "tests.hpp"
#include <set>
#include <algorithm>

using namespace epiworld;

// Tie mutation has to stay behind Model::add_edge()/rm_edge(), because only
// those keep the queueing system in step. Agent::add_neighbor() edits the
// network and nothing else, so a global event calling it directly would leave a
// newly exposed agent at queue count zero and see it silently skipped by
// update_state().
//
// Access checking happens during template argument deduction, so a non-public
// member makes this substitution fail rather than the build. If someone ever
// moves these back into the public section, the static_asserts below fail and
// say why.
template< typename T, typename = void >
struct calls_add_neighbor : std::false_type {};

template< typename T >
struct calls_add_neighbor<
    T,
    std::void_t< decltype(
        std::declval< T & >().add_neighbor(std::declval< T & >())
    ) >
> : std::true_type {};

template< typename T, typename = void >
struct calls_rm_neighbor : std::false_type {};

template< typename T >
struct calls_rm_neighbor<
    T,
    std::void_t< decltype(
        std::declval< T & >().rm_neighbor(std::declval< T & >())
    ) >
> : std::true_type {};

static_assert(
    !calls_add_neighbor< Agent<> >::value,
    "Agent::add_neighbor must stay non-public: it edits ties without telling "
    "the queue. Use Model::add_edge()."
);

static_assert(
    !calls_rm_neighbor< Agent<> >::value,
    "Agent::rm_neighbor must stay non-public: it edits ties without telling "
    "the queue. Use Model::rm_edge()."
);

// Sorted (source, target) pairs of the model's contact network, as the public
// API reports them.
static std::vector< std::pair<int,int> > edges_of(Model<> & model)
{
    std::vector< int > source, target;
    model.write_edgelist(source, target);

    std::vector< std::pair<int,int> > res;
    res.reserve(source.size());
    for (size_t i = 0u; i < source.size(); ++i)
        res.emplace_back(
            std::min(source[i], target[i]), std::max(source[i], target[i])
        );

    std::sort(res.begin(), res.end());
    return res;
}

// Ties can be added to and removed from a model that already has a network, and
// removing what was added puts the network back exactly as it was. The agent
// with the largest degree is deliberately pushed past
// EPI_NEIGHBOR_INDEX_THRESHOLD so that both the scanned and the indexed lookup
// paths are exercised in the same model.
EPIWORLD_TEST_CASE("Edges - add and remove ties", "[edges]") {

    size_t n = 200u;
    size_t hub = 0u;                 // tied to everyone in [1, hub_degree]
    size_t hub_degree = EPI_NEIGHBOR_INDEX_THRESHOLD + 8u;

    std::vector< int > source, target;

    // A path over agents [1, n - 1), stopping short of the last agent so that
    // it is left isolated on purpose.
    for (size_t i = 1u; i < n - 2u; ++i)
    {
        source.push_back(static_cast<int>(i));
        target.push_back(static_cast<int>(i + 1u));
    }

    // A hub whose degree crosses the indexing threshold.
    for (size_t i = 1u; i <= hub_degree; ++i)
    {
        source.push_back(static_cast<int>(hub));
        target.push_back(static_cast<int>(i));
    }

    epimodels::ModelSIR<> model("a virus", 0.05, 0.9, 0.3);
    model.seed(4321);
    model.agents_from_edgelist(source, target, static_cast<int>(n), false);
    model.verbose_off();

    REQUIRE(model.get_agent(hub).get_n_neighbors() == hub_degree);
    REQUIRE(model.get_agent(n - 1u).get_n_neighbors() == 0u);

    auto baseline = edges_of(model);

    // -- the view over an agent with no ties --------------------------------
    // An isolate has nothing to point at, so the empty view is a pair of null
    // pointers; its size must not be computed by subtracting them.
    {
        auto view = model.get_agent(n - 1u).neighbors_view(model);
        REQUIRE(view.size() == 0u);
        REQUIRE(view.empty());
        REQUIRE(view.begin() == view.end());

        size_t visited = 0u;
        for (auto * nb : view)
        {
            (void) nb;
            ++visited;
        }
        REQUIRE(visited == 0u);
    }

    // A default-constructed view is empty in the same way.
    {
        NeighborsView<> view;
        REQUIRE(view.size() == 0u);
        REQUIRE(view.empty());
        REQUIRE(view.begin() == view.end());
    }

    // And where there *are* ties, the view agrees with get_neighbors().
    {
        std::vector< int > from_view, from_vector;
        for (auto * nb : model.get_agent(hub).neighbors_view(model))
            from_view.push_back(nb->get_id());
        for (auto * nb : model.get_agent(hub).get_neighbors(model))
            from_vector.push_back(nb->get_id());

        REQUIRE(model.get_agent(hub).neighbors_view(model).size() == hub_degree);
        REQUIRE_THAT(from_view, Catch::Equals(from_vector));
    }

    // -- has_edge agrees with the network we asked for ------------------------
    REQUIRE(model.has_edge(hub, 1u));
    REQUIRE(model.has_edge(1u, hub));            // ties are undirected
    REQUIRE_FALSE(model.has_edge(hub, n - 1u));

    // -- adding ------------------------------------------------------------
    // A tie to the isolate, one that crosses the hub's indexed list, and one
    // between two ordinary (scanned) agents.
    REQUIRE(model.add_edge(hub, n - 1u));
    REQUIRE(model.add_edge(n - 1u, 5u));
    REQUIRE(model.add_edge(10u, 50u));

    REQUIRE(model.get_agent(n - 1u).get_n_neighbors() == 2u);
    REQUIRE(model.get_agent(hub).get_n_neighbors() == hub_degree + 1u);

    // Both ends see the tie.
    REQUIRE(model.has_edge(hub, n - 1u));
    REQUIRE(model.has_edge(n - 1u, hub));

    // Adding again is a no-op, in either direction.
    REQUIRE_FALSE(model.add_edge(hub, n - 1u));
    REQUIRE_FALSE(model.add_edge(n - 1u, hub));
    REQUIRE(model.get_agent(n - 1u).get_n_neighbors() == 2u);

    REQUIRE(edges_of(model).size() == baseline.size() + 3u);

    // -- removing ----------------------------------------------------------
    // A tie that is not there cannot be removed.
    REQUIRE_FALSE(model.rm_edge(20u, 100u));

    REQUIRE(model.rm_edge(hub, n - 1u));
    REQUIRE_FALSE(model.has_edge(hub, n - 1u));
    REQUIRE_FALSE(model.has_edge(n - 1u, hub));
    REQUIRE_FALSE(model.rm_edge(hub, n - 1u));   // already gone

    REQUIRE(model.rm_edge(5u, n - 1u));          // removing from the other end
    REQUIRE(model.rm_edge(10u, 50u));

    // Back to exactly the network we started with -- same ties, and the
    // survivors in the same order, which is what decides who infects whom.
    REQUIRE(model.get_agent(n - 1u).get_n_neighbors() == 0u);
    REQUIRE(model.get_agent(hub).get_n_neighbors() == hub_degree);
    REQUIRE(edges_of(model) == baseline);

    // -- ties removed from the middle of a list keep the rest in order -------
    // Drop three of the hub's ties, including one before the threshold and one
    // after it, and check the remaining ids are the original ones minus those.
    std::vector< size_t > dropped = {3u, EPI_NEIGHBOR_INDEX_THRESHOLD, hub_degree};

    std::vector< int > expected;
    for (size_t i = 1u; i <= hub_degree; ++i)
        if (std::find(dropped.begin(), dropped.end(), i) == dropped.end())
            expected.push_back(static_cast<int>(i));

    for (size_t d : dropped)
        REQUIRE(model.rm_edge(hub, d));

    std::vector< int > remaining;
    for (auto * nb : model.get_agent(hub).get_neighbors(model))
        remaining.push_back(nb->get_id());

    REQUIRE_THAT(remaining, Catch::Equals(expected));

    // -- invalid arguments --------------------------------------------------
    REQUIRE_THROWS(model.add_edge(1u, 1u));      // self-loop
    REQUIRE_THROWS(model.add_edge(0u, n));       // out of range
    REQUIRE_THROWS(model.rm_edge(n + 10u, 0u));

    // -- and the model still runs on the edited network ---------------------
    model.run(20);
    REQUIRE(model.today() == 20);

}
