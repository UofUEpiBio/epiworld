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

// `max_households` bounds how far peer nominations may chain. Without it the
// merges percolate and the whole population lands in one bubble; with it the
// parameter is a usable dial on how much contact survives.
EPIWORLD_TEST_CASE("Bubbles - peer bubbles honour max_households", "[bubbles]") {

    size_t n = 1200u, hh_size = 3u;
    auto hh = make_households(n, hh_size);
    size_t n_households = n / hh_size;

    // Returns {largest bubble in households, fraction of cross-household edges kept}
    auto run_with_cap = [&](size_t cap) -> std::pair<size_t, double> {
        epimodels::ModelSEIR<> model("flu", 0.01, 0.1, 4.5, 1.0/8.0);
        model.agents_smallworld(n, 8, false, 0.05);

        Bubbles<> bubbles(
            hh, BubbleFlavor::Peer, 2u, 1.0, 0, -1, 0, "Social bubble", cap
        );
        bubbles.deploy(model);
        model.verbose_off();
        model.run(1, 77);

        const auto & bid = bubbles.get_bubble_id();

        std::map<int, std::set<size_t>> households_in_bubble;
        for (size_t a = 0u; a < n; ++a)
            households_in_bubble[bid[a]].insert(hh[a]);

        size_t largest = 0u;
        for (auto & kv : households_in_bubble)
            largest = std::max(largest, kv.second.size());

        int total = 0, kept = 0;
        for (auto & a : model.get_agents())
        {
            size_t ia = static_cast<size_t>(a.get_id());
            for (auto * nb : a.get_neighbors(model))
            {
                size_t ib = static_cast<size_t>(nb->get_id());
                if (ia >= ib || hh[ia] == hh[ib]) continue;
                ++total;
                if (bid[ia] == bid[ib]) ++kept;
            }
        }

        REQUIRE(total > 0);
        return {largest, static_cast<double>(kept) / static_cast<double>(total)};
    };

    auto cap2 = run_with_cap(2u);
    auto cap4 = run_with_cap(4u);

    // The cap is respected...
    REQUIRE(cap2.first <= 2u);
    REQUIRE(cap4.first <= 4u);

    // ...and never swallows the population (the uncapped bug produced a single
    // bubble holding every household).
    REQUIRE(cap4.first < n_households);

    // A larger cap lets more contact survive.
    REQUIRE(cap4.second > cap2.second);

    // Neither setting is a no-op: some external contact survives, but not all.
    REQUIRE(cap2.second > 0.0);
    REQUIRE(cap4.second < 1.0);

}
