#include "tests.hpp"
#include <algorithm>

using namespace epiworld;

// What completing a bubble must leave alone, and what it refuses to do: the
// grouping rules, a second policy's bubble, another event's edits to the
// network, and rewiring.
EPIWORLD_TEST_CASE("Bubbles - ties do not interfere", "[bubbles]") {

    // -- 1. Grouping never sees the ties ------------------------------------
    //
    // Both grouping rules read the contact network to decide which households
    // may bubble together. A clique left in the network -- a policy's own,
    // standing while it repartitions, or another policy's -- would make its
    // members look connected and pull the grouping towards them, so
    // BubbleFlavor would silently depend on BubbleTies, and one policy on
    // another.
    //
    // Two policies on one model: bubbles of three households, and a Peer rule
    // over an unrelated grouping (classrooms, say, scattered across the
    // network), each redrawn on its own schedule -- so each repartitions while
    // its own clique and the other's are standing. With no prevalence nothing
    // else can differ, and realizing a bubble as ties draws no random numbers,
    // so both must group exactly as they do when neither touches the network.
    //
    // (Household bubbles of two would not tell: the pair is adjacent already,
    // so completing it adds nothing the household graph did not have. A third
    // household need not be tied to the first, and the Peer rule counts an
    // agent's contacts per household rather than de-duplicating them.)
    {
        size_t n = 300u;
        std::vector< size_t > homes(n), classes(n);
        for (size_t i = 0u; i < n; ++i)
        {
            homes[i]   = i / 3u;
            classes[i] = (i * 37u) % 97u;
        }

        struct Seen {
            std::vector< std::vector<int> > homes, classes;
            size_t homes_held   = 0u;
            size_t classes_held = 0u;
        };

        auto partitions_of = [&](BubbleTies ties) -> Seen {

            epimodels::ModelSEIR<> model("flu", n, 0.0, 0.1, 4.5, 1.0/8.0);
            model.seed(4700);
            model.add_layer(layer::network(
                "network", rgraph_smallworld(n, 6, 0.05, false, model)
            ));

            Bubbles<> by_home(
                homes, BubbleFlavor::Household, 3u, 0.0, 0, -1, 3,
                "home bubble", 2u, "home factor", ties, "network"
            );
            Bubbles<> by_class(
                classes, BubbleFlavor::Peer, 1u, 0.0, 0, -1, 4,
                "class bubble", 4u, "class factor", ties, "network"
            );
            model.add_globalevent(by_home);
            model.add_globalevent(by_class);

            Seen seen;
            model.add_globalevent(
                [&seen](Model<> * m) -> void {

                    auto * a = Bubbles<>::get_from(*m, "home bubble");
                    auto * b = Bubbles<>::get_from(*m, "class bubble");

                    seen.homes.push_back(a->get_bubble_id());
                    seen.classes.push_back(b->get_bubble_id());

                    seen.homes_held = std::max(
                        seen.homes_held, a->get_created_ties().size()
                    );
                    seen.classes_held = std::max(
                        seen.classes_held, b->get_created_ties().size()
                    );

                },
                "record the partitions"
            );

            model.verbose_off();
            model.run(20);
            return seen;

        };

        auto plain  = partitions_of(BubbleTies::Existing);
        auto clique = partitions_of(BubbleTies::Complete);

        // Both policies really were holding ties.
        REQUIRE(clique.homes_held > 0u);
        REQUIRE(clique.classes_held > 0u);

        REQUIRE(plain.homes.size() == clique.homes.size());

        for (size_t d = 0u; d < plain.homes.size(); ++d)
        {
            REQUIRE_THAT(clique.homes[d], Catch::Equals(plain.homes[d]));
            REQUIRE_THAT(clique.classes[d], Catch::Equals(plain.classes[d]));
        }

        // The partitions really did move on, so this is not comparing one
        // epoch with itself.
        bool homes_moved = false, classes_moved = false;
        for (size_t d = 1u; d < plain.homes.size(); ++d)
        {
            homes_moved   = homes_moved   || (plain.homes[d] != plain.homes[0]);
            classes_moved = classes_moved || (plain.classes[d] != plain.classes[0]);
        }

        REQUIRE(homes_moved);
        REQUIRE(classes_moved);
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

            epimodels::ModelSEIR<> model("flu", 3, 0.0, 0.1, 4.5, 1.0/8.0);
            model.seed(31);
            std::vector< int > source = {0, 1};
            std::vector< int > target = {1, 2};
            model.add_layer(layer::network(
                "network", AdjList(source, target, 3, false)
            ));

            // A closes on day 3, B on day 6.
            Bubbles<> a(
                hh, BubbleFlavor::Household, 1u, 0.0, 0, 3, 0,
                "bubble A", 2u, "factor A", BubbleTies::Complete, "network"
            );
            Bubbles<> b(
                hh, BubbleFlavor::Household, 1u, 0.0, 0, 6, 0,
                "bubble B", 2u, "factor B", BubbleTies::Complete, "network"
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
                    present.push_back(tests_net(*m).has_edge(0u, 2u) ? 1 : 0);
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

    // -- 3. What something else takes away stays away -----------------------
    //
    // An event that cuts ties -- isolating an agent, say -- means it, and the
    // bubble does not put them back, whether it created the tie (0--2) or the
    // network already had it (0--1). A tie it created stays on its books,
    // though: when the isolation ends and the ties are restored, 0--2 is still
    // the bubble's and comes down with it, while 0--1 is the network's and
    // stays.
    {
        std::vector< size_t > hh = {0u, 0u, 0u};

        epimodels::ModelSEIR<> model("flu", 3, 0.0, 0.1, 4.5, 1.0/8.0);
        model.seed(77);
        std::vector< int > source = {0, 1};
        std::vector< int > target = {1, 2};
        model.add_layer(layer::network(
            "network", AdjList(source, target, 3, false)
        ));

        Bubbles<> bubbles(
            hh, BubbleFlavor::Household, 1u, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete, "network"
        );
        model.add_globalevent(bubbles);

        // Registered after the policy, so it acts after the bubble has had its
        // turn: agent 0 is isolated on day 4 and released on day 6.
        model.add_globalevent(
            [](Model<> * m) -> void {
                if (m->today() == 4)
                {
                    tests_net(*m).rm_edge(0u, 1u);
                    tests_net(*m).rm_edge(0u, 2u);
                }
                else if (m->today() == 6)
                {
                    tests_net(*m).add_edge(0u, 1u);
                    tests_net(*m).add_edge(0u, 2u);
                }
            },
            "isolate agent 0"
        );

        std::vector<int> bubble_tie, network_tie;
        model.add_globalevent(
            [&bubble_tie, &network_tie](Model<> * m) -> void {
                bubble_tie.push_back(tests_net(*m).has_edge(0u, 2u) ? 1 : 0);
                network_tie.push_back(tests_net(*m).has_edge(0u, 1u) ? 1 : 0);
            },
            "watch agent 0"
        );

        model.verbose_off();
        model.run(8);

        // Gone while agent 0 is isolated (days 4 and 5), back once released. On
        // the last day the bubble withdraws its own tie and leaves the other.
        std::vector<int> expected_bubble  = {1, 1, 1, 0, 0, 1, 1, 0};
        std::vector<int> expected_network = {1, 1, 1, 0, 0, 1, 1, 1};

        REQUIRE_THAT(bubble_tie,  Catch::Equals(expected_bubble));
        REQUIRE_THAT(network_tie, Catch::Equals(expected_network));

        // Which is the network the run started with.
        REQUIRE(tests_net(model).has_edge(0u, 1u));
        REQUIRE(tests_net(model).has_edge(1u, 2u));
        REQUIRE_FALSE(tests_net(model).has_edge(0u, 2u));
    }

    // -- 4. Rewiring and directed networks ------------------------------------
    //
    // A rewiring function moves ties between agents, so a tie the intervention
    // created could be moved out from under it and never withdrawn. It is the
    // function that is refused, not the proportion: Model::rewire() calls it on
    // every step, and nothing obliges it to honour a proportion of zero.
    //
    // Completing a bubble also edits ties at both ends, which a directed
    // network -- whose ties are kept by their source only -- cannot take.
    {
        std::vector< size_t > hh = {0u, 0u, 0u, 1u, 1u, 1u};

        auto attempt = [&hh](
            BubbleTies ties,
            std::function< void(Model<> &) > configure,
            bool directed = false
        ) -> void {

            epimodels::ModelSEIR<> model("flu", 6, 0.0, 0.1, 4.5, 1.0/8.0);
            model.seed(20);
            std::vector< int > source = {0, 1, 3, 4, 2};
            std::vector< int > target = {1, 2, 4, 5, 3};
            model.add_layer(layer::network(
                "network", AdjList(source, target, 6, directed)
            ));

            configure(model);

            Bubbles<> bubbles(
                hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
                "Social bubble", 2u, "Bubble transmission factor", ties,
                "network"
            );
            model.add_globalevent(bubbles);
            model.verbose_off();
            model.run(3);

        };

        auto degseq = [](Model<> & m) -> void {
            m.add_param(0.1, "Rewiring proportion");
            tests_net(m).set_rewiring(graph::rewire_degseq("Rewiring proportion"));
        };

        // A function of the user's own, which changes nothing.
        auto own_function = [](Model<> & m) -> void {
            tests_net(m).set_rewiring(
                [](layer::Network<> &, Model<> &) -> void {}
            );
        };

        // A rewiring rate with no function to apply it rewires nothing.
        auto proportion_only = [](Model<> & m) -> void {
            m.add_param(0.1, "Rewiring proportion");
        };

        REQUIRE_THROWS_AS(
            attempt(BubbleTies::Complete, degseq), std::logic_error
        );
        REQUIRE_THROWS_AS(
            attempt(BubbleTies::Complete, own_function), std::logic_error
        );
        REQUIRE_NOTHROW(attempt(BubbleTies::Complete, proportion_only));

        // The same ties, as a directed network.
        auto nothing = [](Model<> &) -> void {};

        // Refused up front, by the intervention -- not halfway through the run
        // by add_edge(), which refuses directed networks too.
        REQUIRE_THROWS_WITH(
            attempt(BubbleTies::Complete, nothing, true),
            Catch::Matchers::Contains("needs an undirected network layer")
        );

        // The default realization does not touch the network, so it is fine.
        REQUIRE_NOTHROW(attempt(BubbleTies::Existing, degseq));
        REQUIRE_NOTHROW(attempt(BubbleTies::Existing, nothing, true));
    }

}
