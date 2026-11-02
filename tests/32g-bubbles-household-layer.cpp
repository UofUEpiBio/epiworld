#include "tests.hpp"
#include <algorithm>

using namespace epiworld;

// Two households of three, joined by the single tie 2--3:
//
//   household 0: 0 -- 1 -- 2            household 1: 3 -- 4 -- 5
//                          \___________________/
//
// Under BubbleTies::Complete the bubble is a group of the household layer
// (within-group mixing, q = 1): outside the policy window each household meets
// in full, and while it is in force the whole bubble does.
static void two_households(Model<> & model)
{
    model.add_layer(layer::network(
        "network", AdjList({0, 1, 3, 4, 2}, {1, 2, 4, 5, 3}, 6, false)
    ));
}

static std::vector< size_t > households(size_t n, size_t size)
{
    std::vector< size_t > hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / size;
    return hh;
}

EPIWORLD_TEST_CASE("Bubbles - complete bubbles in the household layer", "[bubbles]") {

    const std::vector< size_t > hh = households(6u, 3u);

    // -- 1. The layer: one group per household, everyone meets ------------
    Bubbles<> policy(hh, BubbleFlavor::Household, 2u, 0.0, 2, 5, 0);
    REQUIRE_THROWS_WITH(policy.make_layer(), Catch::Matchers::Contains("name"));
    policy.set_ties(BubbleTies::Complete, "household");

    auto layer = policy.make_layer();
    REQUIRE(layer->get_name() == "household");
    REQUIRE(layer->get_n_groups() == 2u);
    REQUIRE(layer->within_groups());

    // -- 2. Agents move into the bubble while the policy is in force -------
    // Window [2, 5): the moves made by the daily event take effect at the end
    // of the step, so the groups seen on day d are the ones of day d's draws.
    epimodels::ModelSIR<> model("flu", 6, 0.0, 0.0, 0.0);
    model.verbose_off();
    two_households(model);
    model.add_layer(layer);
    model.add_globalevent(policy);

    std::vector< std::vector< size_t > > seen;
    model.add_globalevent(
        [&seen](Model<> * m) -> void {
            auto & l = m->get_layer_as< layer::Mixing<> >("household");
            std::vector< size_t > g(6u);
            for (size_t i = 0u; i < 6u; ++i)
                g[i] = l.get_group(i);
            seen.push_back(g);
        },
        "record the groups"
    );

    for (int rep = 0; rep < 2; ++rep)
    {
        // The second run starts from the households again (the reset
        // restores the layer)
        seen.clear();
        model.run(6, 20);

        // The two households are connected, so they bubble together
        const auto & bid = Bubbles<>::get_from(model)->get_bubble_id();
        REQUIRE(bid[0] == bid[5]);
        const size_t b = static_cast< size_t >(bid[0]);

        REQUIRE(seen.size() == 6u);
        for (size_t d = 1u; d <= 6u; ++d)
            for (size_t i = 0u; i < 6u; ++i)
                REQUIRE(seen[d - 1u][i] == (((d >= 2u) && (d < 5u)) ? b : hh[i]));
    }

    // -- 3. Who infects whom -----------------------------------------------
    // Agent 0 carries the virus, transmission is certain, nobody recovers,
    // and the network layer does not transmit (weight 0; the grouping still
    // reads it). Every transmission comes from the household layer. Returns
    // who got infected; `h < 0` builds the layer without rate parameters.
    auto infected = [&](
        BubbleTies ties, double h, double b, int ndays, int seed
    ) -> std::vector< bool > {

        epimodels::ModelSIR<> m("flu", 6, 0.0, 1.0, 0.0);
        m.verbose_off();
        m.get_virus(0).set_distribution(distribute_virus_to_set<>({0u}));
        m.add_param(0.0, "Network weight");
        two_households(m);
        m.get_layer("network").set_weight("Network weight");

        Bubbles<> p(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0);
        p.set_ties(BubbleTies::Complete, "household");
        if (h < 0.0)
            m.add_layer(p.make_layer());
        else
        {
            m.add_param(h, "Household contact probability");
            m.add_param(b, "Bubble contact probability");
            m.add_layer(p.make_layer(
                "Household contact probability", "Bubble contact probability"
            ));
        }
        p.set_ties(ties, "household");
        m.add_globalevent(p);

        m.run(static_cast< epiworld_fast_uint >(ndays), seed);

        std::vector< int > d, src, tgt, v, se, lay;
        m.get_db().get_transmissions(d, src, tgt, v, se, lay);
        const int hl = static_cast< int >(m.get_layer_id("household"));
        std::vector< bool > res(6u, false);
        for (size_t k = 0u; k < tgt.size(); ++k)
        {
            res[static_cast< size_t >(tgt[k])] = true;
            REQUIRE(lay[k] == ((src[k] >= 0) ? hl : -1));
        }

        return res;

    };

    auto count = [](const std::vector< bool > & x) {
        return static_cast< size_t >(std::count(x.begin(), x.end(), true));
    };

    // Within its household only, the virus reaches 3 agents; completed into
    // one bubble from day 1, all 6. Rates of 1 are the default layer, and a
    // bubble rate of 0 keeps the households apart.
    REQUIRE(count(infected(BubbleTies::Existing, -1.0, 0.0, 10, 3)) == 3u);
    REQUIRE(count(infected(BubbleTies::Complete, -1.0, 0.0, 10, 3)) == 6u);
    REQUIRE(
        infected(BubbleTies::Complete, 1.0, 1.0, 10, 3) ==
        infected(BubbleTies::Complete, -1.0, 0.0, 10, 3)
    );
    REQUIRE(count(infected(BubbleTies::Complete, 1.0, 0.0, 10, 3)) == 3u);

    // On day 1 only agent 0 transmits: a housemate is infected with the
    // household rate (0.6), an agent of the other household with the bubble
    // rate (0.3)
    {
        const int nsims = 4000;
        double housemate = 0.0, other = 0.0;
        for (int sim = 0; sim < nsims; ++sim)
        {
            auto x = infected(BubbleTies::Complete, 0.6, 0.3, 1, sim);
            housemate += x[1];
            other     += x[3];
        }

        REQUIRE_THAT(housemate / nsims, Catch::Matchers::WithinAbs(0.6, 0.03));
        REQUIRE_THAT(other / nsims, Catch::Matchers::WithinAbs(0.3, 0.03));
    }

    // -- 4. Redrawn bubbles: the groups follow the partition ---------------
    // 300 agents in households of 3 on a small world, bubbles of 2
    // households redrawn every 4 days in [3, 20). Every day the layer's
    // groups are the previous day's partition (moves wait for the end of the
    // step) while the policy is in force, and the households otherwise.
    {
        const size_t n = 300u;
        const auto hh300 = households(n, 3u);

        epimodels::ModelSIR<> m("flu", n, 0.01, 0.2, 0.1);
        m.verbose_off();
        m.add_layer(layer::network("network", graph::smallworld<>(6, 0.1)));

        Bubbles<> p(hh300, BubbleFlavor::Household, 2u, 0.0, 3, 20, 4);
        p.set_ties(BubbleTies::Complete, "household");
        m.add_layer(p.make_layer());
        m.add_globalevent(p);

        std::vector< int > previous;
        size_t checked = 0u, merged = 0u;
        m.add_globalevent(
            [&](Model<> * mm) -> void {

                auto & l = mm->get_layer_as< layer::Mixing<> >("household");
                const auto * b = Bubbles<>::get_from(*mm);
                const int day = static_cast< int >(mm->today());

                const bool in_force = (day >= 3) && (day < 20);
                for (size_t i = 0u; i < n; ++i)
                    REQUIRE(l.get_group(i) == (
                        in_force ? static_cast< size_t >(previous[i]) : hh300[i]
                    ));

                // members_of() agrees with the groups, and bubbles hold
                // more than one household
                for (size_t g = 0u; g < l.get_n_groups(); ++g)
                {
                    for (size_t i : l.members_of(g))
                        REQUIRE(l.get_group(i) == g);

                    if (in_force && (l.members_of(g).size() > 3u))
                        ++merged;
                }

                previous = b->get_bubble_id();
                ++checked;

            },
            "check the groups"
        );

        m.run(25, 77);

        REQUIRE(checked == 25u);
        REQUIRE(merged > 0u);
    }

}
