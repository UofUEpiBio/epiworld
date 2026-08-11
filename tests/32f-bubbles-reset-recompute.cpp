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

// The partition is (re)computed at reset time (via the tool's distribution
// function), so it is fresh for each run/replicate and depends on that run's
// seed -- not a stale partition carried over from a previous run.
EPIWORLD_TEST_CASE("Bubbles - partition recomputed each run", "[bubbles]") {

    size_t n = 300u, hh_size = 3u;
    auto hh = make_households(n, hh_size);

    epimodels::ModelSEIR<> model("flu", 0.05, 0.1, 4.5, 1.0/8.0);
    model.agents_smallworld(n, 6, false, 0.05);

    Bubbles<> bubbles(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0);
    bubbles.deploy(model);
    model.verbose_off();

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
    // (With the previous eager-compute design, both runs reused the same stale
    // partition and this would fail.)
    bool differ = false;
    for (size_t a = 0u; a < n; ++a)
        if (p1[a] != p2[a]) { differ = true; break; }
    REQUIRE(differ);

}
