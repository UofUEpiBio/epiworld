#include "tests.hpp"
#include <set>
#include <map>
#include <algorithm>

using namespace epiworld;

static std::set< std::pair<int,int> > edges_of(Model<> & model)
{
    std::vector< int > source, target;
    model.write_edgelist(source, target);

    std::set< std::pair<int,int> > res;
    for (size_t i = 0u; i < source.size(); ++i)
        res.emplace(
            std::min(source[i], target[i]), std::max(source[i], target[i])
        );

    return res;
}

// Two households of three, joined by the single tie 2--3:
//
//   household 0: 0 -- 1 -- 2            household 1: 3 -- 4 -- 5
//                          \___________________/
//
// Neither household is complete (0 and 2 are not tied, nor are 3 and 5), and the
// two households share exactly one contact. This is the case the Complete
// realization exists for: the policy says these two households have bubbled, so
// they should meet -- all of them.
static void two_households(Model<> & model)
{
    std::vector< int > source = {0, 1, 3, 4, 2};
    std::vector< int > target = {1, 2, 4, 5, 3};
    model.agents_from_edgelist(source, target, 6, false);
}

// Under BubbleTies::Complete a bubble stops being only a transmission rule and
// becomes a clique. Checked first on a hand-built network, where the arithmetic
// can be done by eye, then at scale on a real one. Both are inspected from
// inside the run, since the ties are withdrawn when it ends.
EPIWORLD_TEST_CASE("Bubbles - complete ties", "[bubbles]") {

    // -- 1. Two households that bubbled because of a single tie -------------
    {
        std::vector< size_t > hh = {0u, 0u, 0u, 1u, 1u, 1u};

        epimodels::ModelSEIR<> base("flu", 0.0, 0.1, 4.5, 1.0/8.0);
        base.seed(20);
        two_households(base);
        auto baseline = edges_of(base);

        // Existing: the bubble is only a transmission rule.
        epimodels::ModelSEIR<> keep("flu", 0.0, 0.1, 4.5, 1.0/8.0);
        keep.seed(20);
        two_households(keep);

        Bubbles<> as_rule(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0);
        keep.add_globalevent(as_rule);
        keep.verbose_off();
        keep.run(5);

        // Nothing was added: 2--3 is still the only contact between the
        // households, and neither household got completed.
        REQUIRE(edges_of(keep) == baseline);
        REQUIRE_FALSE(keep.has_edge(0u, 2u));
        REQUIRE_FALSE(keep.has_edge(0u, 3u));

        // Complete: the bubble becomes a clique.
        epimodels::ModelSEIR<> full("flu", 0.0, 0.1, 4.5, 1.0/8.0);
        full.seed(20);
        two_households(full);

        Bubbles<> as_ties(
            hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete
        );
        full.add_globalevent(as_ties);
        full.verbose_off();

        bool checked = false;
        full.add_globalevent(
            [&checked](Model<> * m) -> void {

                if (m->today() != 3)
                    return;

                // Everyone is in one bubble here, so every pair should be tied
                // -- including the two that were not tied inside their own
                // household.
                const auto & bid = Bubbles<>::get_from(*m)->get_bubble_id();
                for (size_t i = 0u; i < 6u; ++i)
                    for (size_t j = i + 1u; j < 6u; ++j)
                        if (bid[i] == bid[j])
                            REQUIRE(m->has_edge(i, j));

                // 6 agents, all in one bubble -> C(6,2) = 15 ties.
                REQUIRE(m->get_agent(0u).get_n_neighbors() == 5u);

                // Within a household: 0--2 and 3--5 did not exist before.
                REQUIRE(m->has_edge(0u, 2u));
                REQUIRE(m->has_edge(3u, 5u));

                // Across households: every one of the 9 pairs, not just 2--3.
                for (size_t a = 0u; a < 3u; ++a)
                    for (size_t b = 3u; b < 6u; ++b)
                        REQUIRE(m->has_edge(a, b));

                // Only the ties it actually created are on the intervention's
                // books: 15 pairs minus the 5 the network already had.
                REQUIRE(
                    Bubbles<>::get_from(*m)->get_created_ties().size() ==
                    15u - 5u
                );

                checked = true;

            },
            "check the bubble network"
        );

        full.run(6);

        REQUIRE(checked);

        // Both households ended up in one bubble (they are connected), so this
        // is a meaningful comparison rather than a degenerate one.
        const auto & bid = Bubbles<>::get_from(full)->get_bubble_id();
        REQUIRE(bid[0] == bid[5]);

        // And the run left the network exactly as it found it.
        REQUIRE(edges_of(full) == baseline);
        REQUIRE(Bubbles<>::get_from(full)->get_created_ties().empty());
    }

    // -- 2. The same at scale -----------------------------------------------
    {
        size_t n = 300u, hh_size = 3u;

        std::vector< size_t > hh(n);
        for (size_t i = 0u; i < n; ++i)
            hh[i] = i / hh_size;

        // The same network, built the same way, with nothing done to it.
        epimodels::ModelSEIR<> base("flu", 0.05, 0.1, 4.5, 1.0/8.0);
        base.seed(913);
        base.agents_smallworld(n, 6, false, 0.05);
        auto baseline = edges_of(base);

        epimodels::ModelSEIR<> model("flu", 0.05, 0.1, 4.5, 1.0/8.0);
        model.seed(913);
        model.agents_smallworld(n, 6, false, 0.05);

        REQUIRE(edges_of(model) == baseline);   // same seed, same network

        Bubbles<> bubbles(
            hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete
        );
        model.add_globalevent(bubbles);
        model.verbose_off();

        bool checked = false;
        model.add_globalevent(
            [&checked, &baseline, &hh, n](Model<> * m) -> void {

                if (m->today() != 4)
                    return;

                auto * policy = Bubbles<>::get_from(*m);
                const auto & bid = policy->get_bubble_id();

                // Group the agents by bubble.
                std::map< int, std::vector<size_t> > members;
                for (size_t a = 0u; a < n; ++a)
                    members[bid[a]].push_back(a);

                // (a) Every bubble is a clique.
                size_t within_pairs = 0u;
                for (auto & kv : members)
                {
                    auto & who = kv.second;
                    for (size_t i = 0u; i < who.size(); ++i)
                        for (size_t j = i + 1u; j < who.size(); ++j)
                        {
                            REQUIRE(m->has_edge(who[i], who[j]));
                            ++within_pairs;
                        }
                }

                // Bubbles of 2 households of 3 -> mostly C(6,2) = 15 pairs
                // each; enough of them that the check is not vacuous.
                REQUIRE(within_pairs > n);

                // (b) Households got completed too -- that is the part a
                //     transmission-only rule cannot express.
                size_t completed_within_household = 0u;
                for (size_t a = 0u; a < n; ++a)
                    for (size_t b = a + 1u; b < n; ++b)
                    {
                        if (hh[a] != hh[b])
                            continue;

                        REQUIRE(m->has_edge(a, b));

                        auto pair = std::make_pair(
                            static_cast<int>(a), static_cast<int>(b)
                        );
                        if (baseline.find(pair) == baseline.end())
                            ++completed_within_household;
                    }

                REQUIRE(completed_within_household > 0u);

                // (c) Nothing new crosses a bubble: every tie that was not in
                //     the baseline joins two agents of the same bubble.
                auto now = edges_of(*m);
                for (auto & e : now)
                {
                    if (baseline.find(e) != baseline.end())
                        continue;

                    REQUIRE(
                        bid[static_cast<size_t>(e.first)] ==
                        bid[static_cast<size_t>(e.second)]
                    );
                }

                // (d) The books balance: the network grew by exactly the ties
                //     the intervention recorded, and it recorded only
                //     genuinely new ones.
                const auto & created = policy->get_created_ties();
                REQUIRE(now.size() == baseline.size() + created.size());

                for (auto & tie : created)
                {
                    REQUIRE(bid[tie.first] == bid[tie.second]);

                    auto pair = std::make_pair(
                        static_cast<int>(std::min(tie.first, tie.second)),
                        static_cast<int>(std::max(tie.first, tie.second))
                    );
                    REQUIRE(baseline.find(pair) == baseline.end());
                }

                checked = true;

            },
            "inspect the bubble network"
        );

        model.run(8);

        REQUIRE(checked);

        // The run put the network back.
        REQUIRE(edges_of(model) == baseline);
    }

}
