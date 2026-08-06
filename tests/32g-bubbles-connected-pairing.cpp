#include "tests.hpp"
#include <set>

using namespace epiworld;

// Builds a household id vector: agents 0..n-1 grouped in households of `hh_size`.
static std::vector<size_t> make_households(size_t n, size_t hh_size)
{
    std::vector<size_t> hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / hh_size;
    return hh;
}

// Counts cross-household edges that survive the bubble policy, i.e. whose two
// endpoints share a bubble.
static void count_edges(
    Model<> & m,
    const std::vector<size_t> & hh,
    const std::vector<int> & bid,
    int & total,
    int & kept
) {
    total = 0;
    kept  = 0;
    for (auto & a : m.get_agents())
    {
        size_t ia = static_cast<size_t>(a.get_id());
        for (auto * nb : a.get_neighbors(m))
        {
            size_t ib = static_cast<size_t>(nb->get_id());
            if (ia >= ib)          continue; // undirected: count once
            if (hh[ia] == hh[ib])  continue; // household-internal
            ++total;
            if (bid[ia] == bid[ib]) ++kept;
        }
    }
}

// Household bubbles are grown along EXISTING ties. Grouping households that
// share no contact would be a no-op (the intervention only suppresses
// transmission along existing edges, it never creates them), which would make
// `group_size` inert -- indistinguishable from a household-only lockdown.
// This test guards against that regression.
EPIWORLD_TEST_CASE("Bubbles - household pairing follows existing ties", "[bubbles]") {

    size_t n = 3000u, hh_size = 3u;
    auto hh = make_households(n, hh_size);

    auto kept_fraction = [&](size_t group) -> double {
        epimodels::ModelSEIR<> model("flu", 0.01, 0.1, 4.5, 1.0/8.0);
        model.agents_smallworld(n, 8, false, 0.05);

        Bubbles<> bubbles(hh, BubbleFlavor::Household, group, 1.0, 0, -1, 0);
        bubbles.deploy(model);
        model.verbose_off();
        model.run(1, 123);

        int total = 0, kept = 0;
        count_edges(model, hh, bubbles.get_bubble_id(), total, kept);
        REQUIRE(total > 0); // the network has cross-household contacts
        return static_cast<double>(kept) / static_cast<double>(total);
    };

    // A bubble of one household is a strict lockdown: no external contact survives.
    REQUIRE(kept_fraction(1u) == 0.0);

    // Bubbling two households must retain a substantial share of external
    // contacts. With random (tie-blind) pairing this was ~0.1%.
    double f2 = kept_fraction(2u);
    REQUIRE(f2 > 0.10);

    // Larger bubbles retain progressively more contact.
    double f4 = kept_fraction(4u);
    REQUIRE(f4 > f2);

}
