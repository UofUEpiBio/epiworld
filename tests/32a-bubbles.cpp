#include "tests.hpp"
#include <set>
#include <map>
#include <unordered_map>
#include <algorithm>

using namespace epiworld;

// Builds a household id vector: agents 0..n-1 grouped in households of `hh_size`.
static std::vector<size_t> make_households(size_t n, size_t hh_size)
{
    std::vector<size_t> hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / hh_size;
    return hh;
}

EPIWORLD_TEST_CASE("Bubbles - household partition structure", "[bubbles]") {

    size_t n = 300u, hh_size = 3u, group = 2u;
    auto hh = make_households(n, hh_size);

    epimodels::ModelSEIR<> model("flu", 0.05, 0.1, 4.5, 1.0/8.0);
    model.seed(11);
    model.agents_smallworld(n, 6, false, 0.05);

    Bubbles<> bubbles(hh, BubbleFlavor::Household, group, 1.0, 0, -1, 0);
    bubbles.deploy(model);
    model.verbose_off();
    model.run(5);

    const auto & bid = bubbles.get_bubble_id();
    REQUIRE(bid.size() == n);

    // Agents in the same household share a bubble.
    for (size_t a = 0u; a < n; ++a)
        REQUIRE(bid[a] == bid[(a / hh_size) * hh_size]);

    // Each bubble contains at most `group` households (and the partition is a
    // function of households, so households never split across bubbles).
    std::unordered_map<int, std::set<size_t>> households_in_bubble;
    for (size_t a = 0u; a < n; ++a)
        households_in_bubble[bid[a]].insert(hh[a]);

    size_t max_hh = 0u;
    for (auto & kv : households_in_bubble)
        max_hh = std::max(max_hh, kv.second.size());

    REQUIRE(max_hh <= group);

    // Bubbles are grown along existing ties, so a household may end up in a
    // smaller bubble (or alone) when it has no unassigned connected partner.
    // Hence there are at least ceil(n_households / group) bubbles.
    size_t n_households = n / hh_size;
    size_t min_bubbles = (n_households + group - 1u) / group;
    REQUIRE(households_in_bubble.size() >= min_bubbles);

    // Household contact graph: h1 ~ h2 iff some member of h1 is connected to a
    // member of h2.
    std::map<size_t, std::set<size_t>> hh_adj;
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

    // Every bubble must be CONNECTED in the household graph. Grouping
    // households that share no tie would be a no-op, since the intervention can
    // only suppress transmission along existing edges (never create them).
    for (auto & kv : households_in_bubble)
    {
        const std::set<size_t> & members = kv.second;
        if (members.size() < 2u)
            continue;

        // BFS from an arbitrary member, restricted to the bubble.
        std::set<size_t> seen;
        std::vector<size_t> stack{*members.begin()};
        seen.insert(*members.begin());
        while (!stack.empty())
        {
            size_t cur = stack.back();
            stack.pop_back();
            for (size_t nb : hh_adj[cur])
                if (members.count(nb) && !seen.count(nb))
                {
                    seen.insert(nb);
                    stack.push_back(nb);
                }
        }

        REQUIRE(seen.size() == members.size());
    }

}
