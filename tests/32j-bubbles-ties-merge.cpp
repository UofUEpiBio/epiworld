#include "tests.hpp"
#include <set>
#include <algorithm>

using namespace epiworld;

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

EPIWORLD_TEST_CASE("Bubbles - merged households are fully connected", "[bubbles]") {

    std::vector< size_t > hh = {0u, 0u, 0u, 1u, 1u, 1u};

    // -- Existing: the bubble is only a transmission rule -------------------
    epimodels::ModelSEIR<> base("flu", 0.0, 0.1, 4.5, 1.0/8.0);
    base.seed(20);
    two_households(base);
    auto baseline = edges_of(base);

    epimodels::ModelSEIR<> keep("flu", 0.0, 0.1, 4.5, 1.0/8.0);
    keep.seed(20);
    two_households(keep);

    Bubbles<> as_rule(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0);
    keep.add_globalevent(as_rule);
    keep.verbose_off();
    keep.run(5);

    // Nothing was added: 2--3 is still the only contact between the households,
    // and neither household got completed.
    REQUIRE(edges_of(keep) == baseline);
    REQUIRE_FALSE(keep.has_edge(0u, 2u));
    REQUIRE_FALSE(keep.has_edge(0u, 3u));

    // -- Complete: the bubble becomes a clique ------------------------------
    epimodels::ModelSEIR<> full("flu", 0.0, 0.1, 4.5, 1.0/8.0);
    full.seed(20);
    two_households(full);

    Bubbles<> as_ties(
        hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
        "Social bubble", 2u, "Bubble transmission factor", BubbleTies::Complete
    );
    full.add_globalevent(as_ties);
    full.verbose_off();

    // The whole partition is one bubble, so a global event run mid-policy sees
    // all 15 pairs tied. Checking from inside the run matters: the ties are
    // withdrawn when the run ends.
    bool checked = false;
    full.add_globalevent(
        [&checked, &hh](Model<> * m) -> void {

            if (m->today() != 3)
                return;

            // Everyone is in one bubble here, so every pair should be tied --
            // including the two that were not tied inside their own household.
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
                Bubbles<>::get_from(*m)->get_created_ties().size() == 15u - 5u
            );

            checked = true;

        },
        "check the bubble network"
    );

    full.run(6);

    REQUIRE(checked);

    // Both households ended up in one bubble (they are connected), so this is a
    // meaningful comparison rather than a degenerate one.
    const auto & bid = Bubbles<>::get_from(full)->get_bubble_id();
    REQUIRE(bid[0] == bid[5]);

    // And the run left the network exactly as it found it.
    REQUIRE(edges_of(full) == baseline);
    REQUIRE(Bubbles<>::get_from(full)->get_created_ties().empty());

    // -- what Complete refuses to do ----------------------------------------
    //
    // (There is also a guard against directed models, but it cannot be reached
    // from here: Model::directed is never set by agents_from_adjlist(), so
    // is_directed() reports false even for a network built with
    // agents_from_edgelist(..., directed = true). The guard is kept for when
    // that is wired up.)

    // Degree-sequence rewiring swaps neighbors between agents, so a tie this
    // created could be swapped out from under it and never withdrawn.
    {
        epimodels::ModelSEIR<> rewired("flu", 0.0, 0.1, 4.5, 1.0/8.0);
        rewired.seed(20);
        two_households(rewired);
        rewired.set_rewire_fun(rewire_degseq<>);
        rewired.set_rewire_prop(0.1);

        Bubbles<> with_rewire(
            hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete
        );
        rewired.add_globalevent(with_rewire);
        rewired.verbose_off();

        REQUIRE_THROWS_AS(rewired.run(3), std::logic_error);

        // The default realization does not touch the network, so it is fine.
        epimodels::ModelSEIR<> ok("flu", 0.0, 0.1, 4.5, 1.0/8.0);
        ok.seed(20);
        two_households(ok);
        ok.set_rewire_fun(rewire_degseq<>);
        ok.set_rewire_prop(0.1);

        Bubbles<> plain(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0);
        ok.add_globalevent(plain);
        ok.verbose_off();
        REQUIRE_NOTHROW(ok.run(3));
    }

    // A bubble large enough to make the clique unreasonable is refused rather
    // than quietly overrunning the sampler's fixed scratch array.
    {
        size_t big = 2400u;
        std::vector< size_t > pairs(big);
        for (size_t i = 0u; i < big; ++i)
            pairs[i] = i / 2u;          // households of two

        epimodels::ModelSEIR<> huge("flu", 0.0, 0.1, 4.5, 1.0/8.0);
        huge.seed(20);
        huge.agents_smallworld(big, 6, false, 0.05);

        // group_size large enough to swallow the whole connected network.
        Bubbles<> one_big_bubble(
            pairs, BubbleFlavor::Household, big, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete
        );
        huge.add_globalevent(one_big_bubble);
        huge.verbose_off();

        REQUIRE_THROWS_AS(huge.run(3), std::length_error);
    }

}
