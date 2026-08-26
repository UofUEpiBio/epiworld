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

EPIWORLD_TEST_CASE("Bubbles - rewiring advances epochs", "[bubbles]") {

    size_t n = 300u, hh_size = 3u;
    auto hh = make_households(n, hh_size);

    epimodels::ModelSEIR<> model("flu", 0.05, 0.1, 4.5, 1.0/8.0);
    model.seed(5);
    model.agents_smallworld(n, 6, false, 0.05);

    int start = 10, rewire = 7, ndays = 60;
    Bubbles<> bubbles(hh, BubbleFlavor::Household, 2u, 0.0, start, -1, rewire);
    model.add_globalevent(bubbles);
    model.verbose_off();
    model.run(ndays);

    // The intervention recomputes the partition at each rewiring epoch. The
    // model owns the intervention, so the epoch is read off the model.
    int expected_epoch = (ndays - start) / rewire;
    REQUIRE(Bubbles<>::get_from(model)->get_last_epoch() == expected_epoch);

}
