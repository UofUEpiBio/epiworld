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

EPIWORLD_TEST_CASE("Bubbles - peer partition respects households", "[bubbles]") {

    size_t n = 300u, hh_size = 3u;
    auto hh = make_households(n, hh_size);

    epimodels::ModelSEIR<> model("flu", 0.05, 0.1, 4.5, 1.0/8.0);
    model.seed(7);
    model.agents_smallworld(n, 8, false, 0.15);

    Bubbles<> bubbles(hh, BubbleFlavor::Peer, 1u, 1.0, 0, -1, 0);
    bubbles.deploy(model);
    model.verbose_off();
    model.run(5);

    const auto & bid = bubbles.get_bubble_id();

    // Households never split across bubbles.
    for (size_t a = 0u; a < n; ++a)
        REQUIRE(bid[a] == bid[(a / hh_size) * hh_size]);

    // With k = 1 external peer, bubbles merge at least some households.
    std::set<int> distinct(bid.begin(), bid.end());
    REQUIRE(distinct.size() < (n / hh_size)); // fewer bubbles than households

}
