#include "tests.hpp"
#include <atomic>

using namespace epiworld;

// Model::get_agents_in_state() lists who is in each state, without scanning
// the population. Every day of a run -- with ties added and removed mid-run,
// in both transmission modes, and in the copies run_multiple() makes -- the
// lists must hold exactly the agents in each state: as many as the daily
// counts say, each once, each actually in that state.

namespace {

struct Tally {
    std::atomic< int > days{0};
    std::atomic< int > bad{0};
};

void check_index(Model<> * m, Tally & tally)
{

    std::vector< int > counts;
    m->get_db().get_today_total(nullptr, &counts);

    std::vector< int > seen(m->size(), 0);
    bool ok = true;
    for (size_t s = 0u; s < m->get_n_states(); ++s)
    {

        const auto & ids = m->get_agents_in_state(s);
        if (static_cast< int >(ids.size()) != counts[s])
            ok = false;

        for (auto id : ids)
        {
            if (m->get_agent(id).get_state() != s)
                ok = false;
            seen[id]++;
        }

    }

    for (auto n : seen)
        if (n != 1)
            ok = false;

    tally.days++;
    if (!ok)
        tally.bad++;

}

void rewire_some_ties(Model<> * m)
{
    int day = m->today();
    if ((day % 7) == 3)
        for (size_t i = 0u; i < 30u; ++i)
            m->add_edge(i, i + 500u);
    else if ((day % 7) == 5)
        for (size_t i = 0u; i < 30u; ++i)
            m->rm_edge(i, i + 500u);
}

} // namespace

EPIWORLD_TEST_CASE("Agents in state - the index follows the model", "[agents-in-state]") {

    for (auto mode : {"push", "pull"})
    {

        Tally tally;

        epimodels::ModelSEIR<> model("flu", 0.02, 0.3, 3.0, 0.2);
        model.seed(42);
        model.agents_smallworld(1000, 6, false, 0.05);
        model.verbose_off();
        model.set_transmission_mode(mode);

        // Not available before the first run
        REQUIRE_THROWS_AS(model.get_agents_in_state(0u), std::logic_error);

        model.add_globalevent(rewire_some_ties, "rewire some ties");
        model.add_globalevent(
            [&tally](Model<> * m) -> void { check_index(m, tally); },
            "check index"
        );

        model.run(50, 1);

        INFO("mode " << mode);
        REQUIRE(tally.days == 50);
        REQUIRE(tally.bad == 0);

        // The outbreak moved agents through every state
        REQUIRE(model.get_agents_in_state(3u).size() > 0u);
        REQUIRE_THROWS_AS(model.get_agents_in_state(4u), std::range_error);

        // Replicates, on several threads when available (each on its own copy)
        tally.days = 0;
        model.run_multiple(30, 8, 7, nullptr, true, false, 4);
        REQUIRE(tally.days == 8 * 30);
        REQUIRE(tally.bad == 0);

    }

}
