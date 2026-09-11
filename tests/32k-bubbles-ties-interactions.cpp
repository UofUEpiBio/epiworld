#include "tests.hpp"
#include <set>
#include <algorithm>

using namespace epiworld;

static std::vector<size_t> make_households(size_t n, size_t hh_size)
{
    std::vector<size_t> hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / hh_size;
    return hh;
}

// Three ways completing a bubble can interfere with something else, all of them
// silent: the ties feeding back into the next partition, a second policy losing
// a contact it still wants, and a bubble pushing an agent past what the virus
// sampler can weigh.
EPIWORLD_TEST_CASE("Bubbles - ties do not interfere", "[bubbles]") {

    // -- 1. Repartitioning must not see the ties ----------------------------
    //
    // Both grouping rules read the contact network to decide which households
    // may bubble together. A clique left standing while the next partition is
    // drawn makes the last bubble's members look connected, pulling the new
    // bubble towards the same membership -- so BubbleFlavor would silently
    // depend on BubbleTies.
    //
    // With no prevalence nothing else can differ, and realizing the bubble as
    // ties draws no random numbers, so the two must partition identically at
    // every epoch.
    {
        size_t n = 300u, hh_size = 3u;
        auto hh = make_households(n, hh_size);

        // group_size >= 3 and the Peer rule are the cases that can tell: with two
        // households per bubble the pair is adjacent already, so completing it
        // adds nothing the household graph did not have. A third household need
        // not be tied to the first, and the Peer rule counts an agent's
        // contacts per household rather than de-duplicating them, so both see
        // the clique.
        auto partitions_of = [&](BubbleTies ties, BubbleFlavor flavor,
                                 size_t group) -> std::vector< std::vector<int> > {

            epimodels::ModelSEIR<> model("flu", 0.0, 0.1, 4.5, 1.0/8.0);
            model.seed(4700);
            model.agents_smallworld(n, 6, false, 0.05);

            Bubbles<> bubbles(
                hh, flavor, group, 0.0, 0, -1, 4,
                "Social bubble", 4u, "Bubble transmission factor", ties
            );
            model.add_globalevent(bubbles);

            std::vector< std::vector<int> > seen;
            model.add_globalevent(
                [&seen](Model<> * m) -> void {
                    seen.push_back(Bubbles<>::get_from(*m)->get_bubble_id());
                },
                "record the partition"
            );

            model.verbose_off();
            model.run(20);
            return seen;

        };

        struct Case { BubbleFlavor flavor; size_t group; };
        std::vector< Case > cases = {
            {BubbleFlavor::Household, 3u},
            {BubbleFlavor::Peer, 1u}
        };

        for (auto & c : cases)
        {

            auto plain  = partitions_of(BubbleTies::Existing, c.flavor, c.group);
            auto clique = partitions_of(BubbleTies::Complete, c.flavor, c.group);

            REQUIRE(plain.size() == clique.size());

            for (size_t d = 0u; d < plain.size(); ++d)
                REQUIRE_THAT(clique[d], Catch::Equals(plain[d]));

            // The partition really did move on, so this is not comparing one
            // epoch with itself.
            bool moved = false;
            for (size_t d = 1u; d < plain.size(); ++d)
                if (plain[d] != plain[0])
                    moved = true;

            REQUIRE(moved);

        }
    }

    // -- 2. Two policies, one tie -------------------------------------------
    //
    // A tie both policies want is on the books of whichever created it. When
    // that policy's window closes it must not take the tie away from the other
    // one, which is still holding its bubble open.
    //
    // Three agents on a path in a single household: 0--1--2. Both policies want
    // 0--2, and only the first to run creates it.
    {
        std::vector< size_t > hh = {0u, 0u, 0u};

        auto run_pair = [&](bool a_first) -> std::vector<int> {

            epimodels::ModelSEIR<> model("flu", 0.0, 0.1, 4.5, 1.0/8.0);
            model.seed(31);
            std::vector< int > source = {0, 1};
            std::vector< int > target = {1, 2};
            model.agents_from_edgelist(source, target, 3, false);

            // A closes on day 3, B on day 6.
            Bubbles<> a(
                hh, BubbleFlavor::Household, 1u, 0.0, 0, 3, 0,
                "bubble A", 2u, "factor A", BubbleTies::Complete
            );
            Bubbles<> b(
                hh, BubbleFlavor::Household, 1u, 0.0, 0, 6, 0,
                "bubble B", 2u, "factor B", BubbleTies::Complete
            );

            // Order decides which policy creates 0--2 and which tears down
            // first; neither should matter.
            if (a_first)
            {
                model.add_globalevent(a);
                model.add_globalevent(b);
            }
            else
            {
                model.add_globalevent(b);
                model.add_globalevent(a);
            }

            std::vector<int> present;
            model.add_globalevent(
                [&present](Model<> * m) -> void {
                    present.push_back(m->has_edge(0u, 2u) ? 1 : 0);
                },
                "watch 0--2"
            );

            model.verbose_off();
            model.run(8);
            return present;

        };

        // Recorded at the end of day d, so entry d-1 is the network day d+1
        // uses. B keeps its bubble open until day 6, so 0--2 must be there for
        // every step up to then, and gone afterwards.
        std::vector<int> expected = {1, 1, 1, 1, 0, 0, 0, 0};

        REQUIRE_THAT(run_pair(true),  Catch::Equals(expected));
        REQUIRE_THAT(run_pair(false), Catch::Equals(expected));
    }

    // -- 2b. A tie taken away by something else is put back -----------------
    //
    // Handing ties over covers the case where the other holder is a bubble
    // policy. Nothing covers the case where the model itself takes a tie away,
    // and the intervention is not told. The clique is therefore re-asserted
    // every day rather than assumed to still be standing.
    {
        std::vector< size_t > hh = {0u, 0u, 0u};

        epimodels::ModelSEIR<> model("flu", 0.0, 0.1, 4.5, 1.0/8.0);
        model.seed(77);
        std::vector< int > source = {0, 1};
        std::vector< int > target = {1, 2};
        model.agents_from_edgelist(source, target, 3, false);

        Bubbles<> bubbles(
            hh, BubbleFlavor::Household, 1u, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete
        );
        model.add_globalevent(bubbles);

        // Registered after the policy, so on day 4 it removes 0--2 *after* the
        // bubble has already had its turn.
        model.add_globalevent(
            [](Model<> * m) -> void {
                if (m->today() == 4)
                    m->rm_edge(0u, 2u);
            },
            "take a tie away"
        );

        std::vector<int> present;
        model.add_globalevent(
            [&present](Model<> * m) -> void {
                present.push_back(m->has_edge(0u, 2u) ? 1 : 0);
            },
            "watch 0--2"
        );

        model.verbose_off();
        model.run(8);

        // Gone at the end of day 4, back by the end of day 5, and the run still
        // leaves the network as it found it.
        std::vector<int> expected = {1, 1, 1, 0, 1, 1, 1, 0};
        REQUIRE_THAT(present, Catch::Equals(expected));

        REQUIRE_FALSE(model.has_edge(0u, 2u));
        REQUIRE(model.has_edge(0u, 1u));
        REQUIRE(model.has_edge(1u, 2u));
    }

    // -- 3. The sampler's limit is a degree, not a bubble size --------------
    //
    // roulette() weighs at most array_double_tmp.size() / 2 candidates, because
    // it uses two slots each. Completing a bubble adds ties on top of whatever
    // an agent already has outside it, so a bubble that is itself within the
    // limit can still push a well-connected member over it.
    {
        epimodels::ModelSEIR<> model("flu", 0.0, 0.1, 4.5, 1.0/8.0);
        size_t limit = model.array_double_tmp.size() / 2u;   // 1024

        // One household of exactly `limit` agents -- allowed on its own, since
        // completing it gives each member limit-1 ties -- plus two outsiders
        // tied to agent 0, which puts agent 0 at limit+1.
        size_t n = limit + 2u;

        std::vector< size_t > hh(n, 0u);
        hh[limit] = 1u;
        hh[limit + 1u] = 2u;

        std::vector< int > source, target;
        for (size_t i = 1u; i < limit; ++i)      // household is a path
        {
            source.push_back(static_cast<int>(i - 1u));
            target.push_back(static_cast<int>(i));
        }
        source.push_back(0); target.push_back(static_cast<int>(limit));
        source.push_back(0); target.push_back(static_cast<int>(limit + 1u));

        model.seed(5);
        model.agents_from_edgelist(source, target, static_cast<int>(n), false);
        model.verbose_off();

        Bubbles<> bubbles(
            hh, BubbleFlavor::Household, 1u, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete
        );
        model.add_globalevent(bubbles);

        // Agent 0 would end up with limit+1 neighbors, one past what the
        // sampler can weigh, even though the bubble itself is exactly at the
        // limit. Refused up front rather than throwing from inside the sampler
        // partway through a run.
        REQUIRE_THROWS_AS(model.run(3), std::length_error);
    }

}
