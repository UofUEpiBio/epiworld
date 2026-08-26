#include "tests.hpp"

using namespace epiworld;

// Rewires a fixed set of ties on a fixed schedule. Deterministic on purpose: it
// draws no random numbers, so both runs below do exactly the same surgery and
// any difference between them has to come from the queue.
static void rewire_some_ties(Model<> * m)
{

    int day = m->today();

    if ((day % 10) == 5)
    {
        for (size_t i = 0u; i < 60u; ++i)
            m->add_edge(i, i + 900u);
    }
    else if ((day % 10) == 8)
    {
        for (size_t i = 0u; i < 60u; ++i)
            m->rm_edge(i, i + 900u);
    }

}

// Changing the contact network mid-run must not disturb the queueing system.
//
// The queue is a per-agent count of how many of its neighbors are active, built
// when an agent is registered and unwound when it is deregistered -- both
// walking the neighbors the agent has *at that moment*. A tie appearing or
// disappearing in between would leave the two walks disagreeing, and an agent
// whose count drifted to zero is quietly skipped by update_state(): infections
// that should have happened simply do not.
//
// Nothing observable says "the queue is wrong", so the check is the one epiworld
// already relies on elsewhere (see 01-sir.cpp): queuing is supposed to be a pure
// optimisation, so a run with it on and a run with it off must agree exactly.
EPIWORLD_TEST_CASE("Edges - mid-run ties keep the queue honest", "[edges]") {

    size_t n = 2000u;

    auto build = [&](bool queuing) -> std::vector< int > {

        epimodels::ModelSIR<> model("a virus", 0.02, 0.9, 0.3);
        model.seed(3131);
        model.agents_smallworld(n, 6, false, 0.02);

        if (!queuing)
            model.queuing_off();

        model.add_globalevent(rewire_some_ties, "rewire some ties");
        model.verbose_off();
        model.run(60);

        std::vector< int > counts;
        model.get_db().get_hist_total(nullptr, nullptr, &counts);
        return counts;

    };

    std::vector< int > with_queue    = build(true);
    std::vector< int > without_queue = build(false);

    // An outbreak actually happened -- otherwise the comparison is vacuous.
    REQUIRE(with_queue.size() > 0u);
    REQUIRE(with_queue.back() > 0);

    REQUIRE_THAT(without_queue, Catch::Equals(with_queue));

    // The same holds when the surgery happens around a model that is copied per
    // replicate: run_multiple hands each replicate its own copy of the agents,
    // queue included.
    auto build_multi = [&](bool queuing) -> std::vector< int > {

        epimodels::ModelSIR<> model("a virus", 0.02, 0.9, 0.3);
        model.seed(717);
        model.agents_smallworld(n, 6, false, 0.02);

        if (!queuing)
            model.queuing_off();

        model.add_globalevent(rewire_some_ties, "rewire some ties");
        model.verbose_off();
        model.run_multiple(40, 4, 555, make_save_run<int>(), true, false, 1);

        std::vector< int > counts;
        model.get_db().get_hist_total(nullptr, nullptr, &counts);
        return counts;

    };

    REQUIRE_THAT(build_multi(false), Catch::Equals(build_multi(true)));

}
