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

// Adding the intervention to the model is the whole installation: it sets
// itself up on the first day of *every* run, so the partition is drawn from
// that run's seed and the tool is back on the agents after the reset that
// stripped it -- no stale partition carried over, and no setup step for the
// user to remember.
EPIWORLD_TEST_CASE("Bubbles - set up on every run", "[bubbles]") {

    size_t n = 300u, hh_size = 3u;
    auto hh = make_households(n, hh_size);

    epimodels::ModelSEIR<> model("flu", 0.05, 0.1, 4.5, 1.0/8.0);
    model.agents_smallworld(n, 6, false, 0.05);

    Bubbles<> bubbles(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0);
    model.add_globalevent(bubbles);
    model.verbose_off();

    // Nothing is installed before the run: the model's own copy of the
    // intervention has no partition yet.
    REQUIRE(Bubbles<>::get_from(model)->get_bubble_id().empty());

    model.run(5, 111);
    std::vector<int> p1 = Bubbles<>::get_from(model)->get_bubble_id();

    model.run(5, 222);
    std::vector<int> p2 = Bubbles<>::get_from(model)->get_bubble_id();

    REQUIRE(p1.size() == n);
    REQUIRE(p2.size() == n);

    // Both are valid partitions (households never split across bubbles).
    for (size_t a = 0u; a < n; ++a)
    {
        REQUIRE(p1[a] == p1[(a / hh_size) * hh_size]);
        REQUIRE(p2[a] == p2[(a / hh_size) * hh_size]);
    }

    // Different seeds shuffle households differently -> the partitions differ.
    // (A partition computed once and kept would make both runs identical.)
    bool differ = false;
    for (size_t a = 0u; a < n; ++a)
        if (p1[a] != p2[a]) { differ = true; break; }
    REQUIRE(differ);

    // Every agent carries the bubble tool at the end of the second run: reset()
    // takes it away, so this only holds if the intervention hands it out on
    // each run rather than once.
    for (auto & a : model.get_agents())
        REQUIRE(a.has_tool("Social bubble"));

    // The object the user built stays a template: the model works off its own
    // copy, which is where the partition lives.
    REQUIRE(bubbles.get_bubble_id().empty());

}
