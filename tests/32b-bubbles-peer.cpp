#include "tests.hpp"
#include <set>
#include <map>

using namespace epiworld;

// Builds a household id vector: agents 0..n-1 grouped in households of `hh_size`.
static std::vector<size_t> make_households(size_t n, size_t hh_size)
{
    std::vector<size_t> hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / hh_size;
    return hh;
}

// Households making up each bubble.
static std::map<int, std::set<size_t>> households_per_bubble(
    const std::vector<size_t> & hh, const std::vector<int> & bid
)
{
    std::map<int, std::set<size_t>> res;
    for (size_t a = 0u; a < bid.size(); ++a)
        res[bid[a]].insert(hh[a]);
    return res;
}

EPIWORLD_TEST_CASE("Bubbles - peer partition respects households", "[bubbles]") {

    size_t n = 300u, hh_size = 3u;
    auto hh = make_households(n, hh_size);
    size_t n_households = n / hh_size;

    // Household contact graph: h1 ~ h2 iff some member of h1 is connected to a
    // member of h2. Filled from the (deterministic) network below.
    std::map<size_t, std::set<size_t>> hh_adj;

    // Partition produced by the peer rule under a given cap on the number of
    // households a bubble may hold.
    auto partition_with_cap = [&](size_t cap) -> std::vector<int> {
        epimodels::ModelSEIR<> model("flu", 0.05, 0.1, 4.5, 1.0/8.0);
        model.seed(7);
        model.agents_smallworld(n, 8, false, 0.15);

        Bubbles<> bubbles(
            hh, BubbleFlavor::Peer, 1u, 0.0, 0, -1, 0, "Social bubble", cap
        );
        model.add_globalevent(bubbles);
        model.verbose_off();
        model.run(5);

        for (auto & a : model.get_agents())
        {
            size_t ia = static_cast<size_t>(a.get_id());
            for (auto * nb : a.get_neighbors(model))
            {
                size_t ib = static_cast<size_t>(nb->get_id());
                if (hh[ia] != hh[ib])
                    hh_adj[hh[ia]].insert(hh[ib]);
            }
        }

        return Bubbles<>::get_from(model)->get_bubble_id();
    };

    std::vector<int> bid = partition_with_cap(2u);

    // Households never split across bubbles.
    for (size_t a = 0u; a < n; ++a)
        REQUIRE(bid[a] == bid[(a / hh_size) * hh_size]);

    auto bubbles_2 = households_per_bubble(hh, bid);

    // Peer nominations merge households, so some households share a bubble.
    REQUIRE(bubbles_2.size() < n_households);

    // ...but the merges must NOT percolate. With the default cap of two
    // households per bubble, no bubble may exceed it and there must be at least
    // n_households / 2 bubbles. (Uncapped union-find collapsed the whole
    // population into a single bubble, imposing no restriction at all.)
    for (auto & kv : bubbles_2)
        REQUIRE(kv.second.size() <= 2u);

    REQUIRE(bubbles_2.size() >= (n_households + 1u) / 2u);

    // Every household sharing a bubble must be genuinely connected: with a cap
    // of two, the pair is joined by at least one member-to-member contact.
    // Bubbling households that share no contact would be a no-op, since the
    // intervention can only suppress transmission along existing edges.
    for (auto & kv : bubbles_2)
    {
        if (kv.second.size() < 2u)
            continue;
        auto it = kv.second.begin();
        size_t h1 = *it++, h2 = *it;
        REQUIRE(hh_adj[h1].count(h2) == 1u);
    }

    // `max_households` is the dial on bubble size: raising it lets the
    // nominations chain further, but never beyond the cap.
    auto bubbles_4 = households_per_bubble(hh, partition_with_cap(4u));

    size_t largest = 0u;
    for (auto & kv : bubbles_4)
        largest = std::max(largest, kv.second.size());

    REQUIRE(largest <= 4u);
    REQUIRE(largest > 2u);              // the cap is a dial, not a constant
    REQUIRE(bubbles_4.size() < bubbles_2.size()); // ...and fewer, larger bubbles

}
