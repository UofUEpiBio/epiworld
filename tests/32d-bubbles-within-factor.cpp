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

EPIWORLD_TEST_CASE("Bubbles - within_factor scales/blocks transmission", "[bubbles]") {

    size_t n = 300u, hh_size = 3u;
    auto hh = make_households(n, hh_size);
    size_t n_households = n / hh_size;

    auto count_secondary = [&](epiworld_double within_factor) -> int {
        epimodels::ModelSEIR<> model("flu", 0.1, 0.3, 4.5, 1.0/8.0);
        model.seed(99);
        model.agents_smallworld(n, 8, false, 0.10);

        // One big bubble: everyone shares a bubble, so within_factor governs
        // all transmission.
        Bubbles<> bubbles(
            hh, BubbleFlavor::Household, n_households, within_factor, 0, -1, 0
        );
        bubbles.deploy(model);
        model.verbose_off();
        model.run(60);

        std::vector<int> date, source, target, virus, sexp;
        model.get_db().get_transmissions(date, source, target, virus, sexp);
        int secondary = 0;
        for (auto s : source)
            if (s >= 0) ++secondary;
        return secondary;
    };

    // within_factor = 0 fully blocks transmission (reduction = 1.0).
    REQUIRE(count_secondary(0.0) == 0);

    // within_factor = 1 leaves transmission unrestricted -> outbreak grows.
    REQUIRE(count_secondary(1.0) > 0);

}
