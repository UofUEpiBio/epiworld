#include "tests.hpp"

using namespace epiworld;

static std::vector< size_t > households(size_t n, size_t size)
{
    std::vector< size_t > hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / size;
    return hh;
}

// What a Complete bubble leaves alone (the grouping), and what it refuses:
// a missing household layer, and two policies moving agents in one layer.
EPIWORLD_TEST_CASE("Bubbles - household layer rules", "[bubbles]") {

    const size_t n = 300u;
    const auto hh = households(n, 3u);

    // A household policy and a Peer one over another grouping (classrooms,
    // scattered across the network), each redrawn on its own schedule.
    std::vector< size_t > room(n);
    for (size_t i = 0u; i < n; ++i)
        room[i] = (i * 7u) % 50u;

    auto make = [&](BubbleTies ties) {

        auto m = std::make_unique< epimodels::ModelSIR<> >(
            "flu", n, 0.0, 0.2, 0.1
        );
        m->verbose_off();
        m->add_layer(layer::network("network", graph::smallworld<>(6, 0.1)));

        Bubbles<> home(hh, BubbleFlavor::Household, 3u, 0.0, 2, 30, 5,
            "Home bubble", 2u, "Home factor", ties, "household");
        Bubbles<> school(room, BubbleFlavor::Peer, 2u, 0.0, 4, -1, 3,
            "School bubble", 3u, "School factor", ties, "school");

        if (ties == BubbleTies::Complete)
        {
            m->add_layer(home.make_layer());
            m->add_layer(school.make_layer());
        }

        m->add_globalevent(home);
        m->add_globalevent(school);
        return m;

    };

    // -- 1. Grouping never sees the household layers -------------------------
    // With no prevalence nothing else differs, and a q = 1 layer draws no
    // random numbers: both policies must group exactly as they do when
    // neither moves anyone.
    auto partitions = [&](BubbleTies ties) {

        auto m = make(ties);
        std::vector< std::vector< int > > res;
        m->add_globalevent(
            [&res](Model<> * mm) -> void {
                for (const char * name : {"Home bubble", "School bubble"})
                    res.push_back(Bubbles<>::get_from(*mm, name)->get_bubble_id());
            },
            "record the partitions"
        );
        m->run(40, 123);
        return res;

    };

    REQUIRE(partitions(BubbleTies::Existing) == partitions(BubbleTies::Complete));

    // -- 2. Complete needs its layer, and a layer of its own -----------------
    // No household layer, or a network layer by that name: neither will do
    for (bool network_named_household : {false, true})
    {
        epimodels::ModelSIR<> m("flu", n, 0.01, 0.2, 0.1);
        m.verbose_off();
        m.add_layer(layer::network("network", graph::smallworld<>(6, 0.1)));
        if (network_named_household)
            m.add_layer(layer::network("household", graph::smallworld<>(2, 0.0)));

        Bubbles<> p(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete, "household");
        m.add_globalevent(p);

        REQUIRE_THROWS_WITH(m.run(5, 1), Catch::Matchers::Contains("make_layer"));
    }

    // A within-group layer by that name whose groups are not the households:
    // fewer groups, or as many but with other members
    for (size_t n_groups : {50u, 100u})
    {
        epimodels::ModelSIR<> m("flu", n, 0.01, 0.2, 0.1);
        m.verbose_off();
        m.add_layer(layer::network("network", graph::smallworld<>(6, 0.1)));

        std::vector< size_t > other(n);
        for (size_t i = 0u; i < n; ++i)
            other[i] = i % n_groups;

        auto wrong = layer::mixing<>("household");
        wrong->set_groups(assign::labels(other), n_groups);
        wrong->set_contact_prob(1.0);
        m.add_layer(wrong);

        Bubbles<> p(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete, "household");
        m.add_globalevent(p);

        REQUIRE_THROWS_WITH(m.run(5, 1), Catch::Matchers::Contains("one group per household"));
    }

    {
        epimodels::ModelSIR<> m("flu", n, 0.01, 0.2, 0.1);
        m.verbose_off();
        m.add_layer(layer::network("network", graph::smallworld<>(6, 0.1)));
        Bubbles<> a(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
            "Bubble A", 2u, "Factor A", BubbleTies::Complete, "household");
        Bubbles<> b(hh, BubbleFlavor::Household, 3u, 0.0, 0, -1, 0,
            "Bubble B", 2u, "Factor B", BubbleTies::Complete, "household");
        m.add_layer(a.make_layer());
        m.add_globalevent(a);
        m.add_globalevent(b);

        REQUIRE_THROWS_WITH(m.run(5, 1), Catch::Matchers::Contains("its own layer"));
    }

    // -- 3. Two policies, two layers: both run, every day ---------------------
    auto m = make(BubbleTies::Complete);
    REQUIRE_NOTHROW(m->run_multiple(20, 4, 5));

}
