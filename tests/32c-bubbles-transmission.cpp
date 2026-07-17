#include "tests.hpp"

using namespace epiworld;

// Builds a household id vector: agents 0..n-1 grouped in households of `hh_size`.
static std::vector<size_t> make_households(size_t n, size_t hh_size)
{
    std::vector<size_t> hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / hh_size;
    return hh;
}

// Counts edges of the contact network whose endpoints fall in different bubbles.
static int count_cross_bubble_edges(
    Model<> & m, const std::vector<int> & bid
)
{
    int cross = 0;
    for (auto & a : m.get_agents())
    {
        int ba = bid[static_cast<size_t>(a.get_id())];
        for (auto * nb : a.get_neighbors(m))
            if (bid[static_cast<size_t>(nb->get_id())] != ba)
                ++cross;
    }
    return cross / 2; // undirected
}

EPIWORLD_TEST_CASE("Bubbles - no cross-bubble transmission", "[bubbles]") {

    size_t n = 400u, hh_size = 4u, group = 2u;
    auto hh = make_households(n, hh_size);

    epimodels::ModelSEIR<> model("flu", 0.1, 0.2, 4.5, 1.0/8.0);
    model.seed(2024);
    model.agents_smallworld(n, 8, false, 0.10);

    Bubbles<> bubbles(hh, BubbleFlavor::Household, group, 1.0, 0, -1, 0);
    bubbles.deploy(model);
    model.verbose_off();
    model.run(80);

    const auto & bid = bubbles.get_bubble_id();

    // The network must contain cross-bubble contacts, otherwise the test is
    // vacuous (nothing to block).
    REQUIRE(count_cross_bubble_edges(model, bid) > 0);

    // Every transmission must be within a single bubble.
    std::vector<int> date, source, target, virus, sexp;
    model.get_db().get_transmissions(date, source, target, virus, sexp);

    int n_secondary = 0;
    for (size_t i = 0u; i < source.size(); ++i)
    {
        if (source[i] < 0) // seed case (external)
            continue;
        ++n_secondary;
        REQUIRE(bid[static_cast<size_t>(source[i])] ==
                bid[static_cast<size_t>(target[i])]);
    }

    // The outbreak actually produced secondary transmissions.
    REQUIRE(n_secondary > 0);

}
