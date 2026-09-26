#include "tests.hpp"

using namespace epiworld;

static size_t n_edges(Model<> & model)
{
    std::vector< int > source, target;
    model.write_edgelist(source, target);
    return source.size();
}

// run(0) sets a model up without simulating. Its only step is the setup, so a
// Complete bubble in force from day 0 must not leave its clique behind: nothing
// would ever withdraw it, and the backup run_multiple() takes would carry it
// into every replicate (#271).
EPIWORLD_TEST_CASE("Bubbles - a zero-day run leaves the network as it found it", "[bubbles]") {

    // Path 0-1-2-3-4-5 (5 ties); households {0,1,2} and {3,4,5}. One bubble of
    // both households, completed to a clique of 15 ties, in force on [0, 3).
    epimodels::ModelSIR<> model("x", 0.0, 0.0, 0.0);
    model.agents_from_edgelist({0, 1, 2, 3, 4}, {1, 2, 3, 4, 5}, 6, false);
    model.verbose_off();

    Bubbles<> bubbles({0, 0, 0, 1, 1, 1}, BubbleFlavor::Household, 2, 0.0, 0, 3);
    bubbles.set_ties(BubbleTies::Complete);
    model.add_globalevent(bubbles);

    std::vector< size_t > by_day;
    model.add_globalevent([&by_day](Model<> * m) {
        std::vector< int > source, target;
        m->write_edgelist(source, target);
        by_day.push_back(source.size());
    }, "count ties");

    REQUIRE(n_edges(model) == 5u);

    model.run(0, 1);
    REQUIRE(n_edges(model) == 5u);
    REQUIRE(Bubbles<>::get_from(model)->get_created_ties().empty());

    // Every replicate sees the clique while the policy is in force, and the
    // original network once it lifts.
    model.run_multiple(6, 2, 1, nullptr, true, false, 1);

    std::vector< size_t > expected = {15, 5, 5, 5, 5, 5, 15, 5, 5, 5, 5, 5};
    REQUIRE(by_day == expected);
    REQUIRE(n_edges(model) == 5u);

}
