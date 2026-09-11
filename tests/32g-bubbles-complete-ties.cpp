#include "tests.hpp"
#include <set>
#include <map>
#include <algorithm>

using namespace epiworld;

static std::vector<size_t> make_households(size_t n, size_t hh_size)
{
    std::vector<size_t> hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / hh_size;
    return hh;
}

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

// Under BubbleTies::Complete the bubble stops being only a transmission rule and
// becomes a clique. Checked on a real network rather than a hand-built one, and
// from inside the run, since the ties are withdrawn when it ends.
EPIWORLD_TEST_CASE("Bubbles - complete ties", "[bubbles]") {

    size_t n = 300u, hh_size = 3u, group = 2u;
    auto hh = make_households(n, hh_size);

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
        hh, BubbleFlavor::Household, group, 0.0, 0, -1, 0,
        "Social bubble", 2u, "Bubble transmission factor", BubbleTies::Complete
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

            // 1. Every bubble is a clique.
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

            // Bubbles of 2 households of 3 -> mostly C(6,2) = 15 pairs each;
            // enough of them that the check is not vacuous.
            REQUIRE(within_pairs > n);

            // 2. Households got completed too -- that is the part a
            //    transmission-only rule cannot express.
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

            // 3. Nothing new crosses a bubble: every tie that was not in the
            //    baseline joins two agents of the same bubble.
            auto now = edges_of(*m);
            for (auto & e : now)
            {
                if (baseline.find(e) != baseline.end())
                    continue;

                REQUIRE(bid[e.first] == bid[e.second]);
            }

            // 4. The books balance: the network grew by exactly the ties the
            //    intervention recorded, and it recorded only genuinely new ones.
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

            // 5. within_pairs counts every pair a bubble should tie; the ones
            //    not created are exactly the ones the network already had.
            REQUIRE(within_pairs >= created.size());

            checked = true;

        },
        "inspect the bubble network"
    );

    model.run(8);

    REQUIRE(checked);

    // The run put the network back.
    REQUIRE(edges_of(model) == baseline);

}
