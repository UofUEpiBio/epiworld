#include "tests.hpp"

using namespace epiworld;

static std::vector<size_t> make_households(size_t n, size_t hh_size)
{
    std::vector<size_t> hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / hh_size;
    return hh;
}

// With rewiring on, a Complete bubble tears down a clique and builds another one
// every epoch, in the middle of a run, while agents are infectious. That is the
// heaviest use the queueing system gets: it counts each agent's active
// neighbors, and every tie that appears or disappears has to be reflected in
// that count, or agents silently stop being visited by update_state().
//
// Nothing observable says "the queue is wrong", so the check is the one epiworld
// relies on elsewhere (01-sir.cpp, 33b-edges-queue.cpp): queuing is meant to be
// a pure optimisation, so the same model with it on and off must agree exactly.
EPIWORLD_TEST_CASE("Bubbles - ties keep the queue honest", "[bubbles]") {

    size_t n = 600u, hh_size = 3u;
    auto hh = make_households(n, hh_size);

    auto run_with = [&](bool queuing) -> std::vector< int > {

        epimodels::ModelSEIR<> model("flu", 0.05, 0.15, 4.5, 1.0/8.0);
        model.seed(8181);
        model.agents_smallworld(n, 6, false, 0.05);

        if (!queuing)
            model.queuing_off();

        // Bubbles redrawn every 5 days, and leaky enough that out-of-bubble
        // contact still transmits -- so the run depends on both the ties and
        // the damping.
        Bubbles<> bubbles(
            hh, BubbleFlavor::Household, 2u, 0.3, 2, -1, 5,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete
        );
        model.add_globalevent(bubbles);
        model.verbose_off();
        model.run(45);

        std::vector< int > counts;
        model.get_db().get_hist_total(nullptr, nullptr, &counts);
        return counts;

    };

    std::vector< int > with_queue    = run_with(true);
    std::vector< int > without_queue = run_with(false);

    // An outbreak actually happened, so the comparison is not vacuous.
    REQUIRE(with_queue.size() > 0u);
    REQUIRE(with_queue.back() > 0);

    REQUIRE_THAT(without_queue, Catch::Equals(with_queue));

    // The partition really did move on -- otherwise no tie surgery happened
    // mid-run and this would be testing nothing.
    epimodels::ModelSEIR<> probe("flu", 0.05, 0.15, 4.5, 1.0/8.0);
    probe.seed(8181);
    probe.agents_smallworld(n, 6, false, 0.05);

    Bubbles<> bubbles(
        hh, BubbleFlavor::Household, 2u, 0.3, 2, -1, 5,
        "Social bubble", 2u, "Bubble transmission factor", BubbleTies::Complete
    );
    probe.add_globalevent(bubbles);
    probe.verbose_off();
    probe.run(45);

    REQUIRE(Bubbles<>::get_from(probe)->get_last_epoch() == (45 - 2) / 5);

}
