#include "tests.hpp"
#include "measles-stages.hpp"

using namespace epiworld;

// A natural history with fixed durations (SEIRS: exposed 2 days, infectious
// 3, immune 4) and two statuses (Free, Isolated), run end to end:
//
// - stages change on the days their drawn durations end, and leaving the
//   infectious stage removes the virus;
// - a second infection starts a new episode, with its own entry days;
// - a stage change scheduled from a stage the agent has left is dropped;
// - the stage hook runs in the step of the change (isolation at the start
//   of the infectious stage);
// - a copy of the model gives the same history;
// - the stage counts, the joint table and the statuses add up every day.
EPIWORLD_TEST_CASE("Natural history - stages episodes and hooks", "[NaturalHistory]") {

    const size_t n = 20u;
    Model<> model(n);
    model.verbose_off();
    model.add_layer(layer::network(
        "network", AdjList(std::vector< int >{}, std::vector< int >{}, n, false)
    ));

    const int FREE = model.add_state("Free");
    const int ISOLATED = model.add_state(
        "Isolated",
        [](Agent<> * p, Model<> * m) -> void {
            // Released when the virus is gone
            if (p->get_virus() == nullptr)
                p->change_status(*m, 0);
        }
    );

    auto days = [](int d) -> StageDurationFun<> {
        return [d](Agent<> *, Model<> *) { return d; };
    };

    NaturalHistory<> nh("SEIRS");
    const int S = nh.add_stage("Susceptible");
    const int E = nh.add_stage("Exposed");
    const int I = nh.add_stage("Infectious");
    const int R = nh.add_stage("Immune");
    nh.set_duration(E, days(2), I);
    nh.set_duration(I, days(3), R); // R is the recovery stage: virus removed
    nh.set_duration(R, days(4), S); // waning
    nh.set_infection(E, R);
    REQUIRE_THROWS(nh.add_stage("Exposed"));
    REQUIRE_THROWS(nh.set_duration(9, days(1)));

    Virus<> virus("Virus");
    virus.set_natural_history(0);
    REQUIRE_THROWS(model.add_virus(virus)); // the model has no natural history
    REQUIRE(model.add_natural_history(nh) == 0u);
    REQUIRE_THROWS(model.add_natural_history(nh)); // same name

    // The states are the pairs of a status and a stage
    REQUIRE(model.get_n_states() == 8u);
    REQUIRE(model.get_states()[0] == "Free Susceptible");
    REQUIRE(model.get_states()[7] == "Isolated Immune");

    // Agent 0 is infected on day 0, and again on day 10 (once susceptible)
    virus.set_distribution([](Virus<> & v, Model<> * m) -> void {
        m->get_agent(0).set_virus(*m, v);
    });
    model.add_virus(virus);
    model.add_globalevent(
        [](Model<> * m) -> void {
            m->get_agent(0).set_virus(*m, m->get_virus(0));
        },
        "Reinfect", 10
    );

    // Agent 1 gets two stage changes from Susceptible on day 1: the first
    // applies, the second is stale
    model.add_globalevent(
        [S, R, E](Model<> * m) -> void {
            auto & a = m->get_agent(1);
            REQUIRE(a.get_stage(*m) == S);
            a.change_stage(*m, R);
            a.change_stage(*m, E);
        },
        "Two changes", 1
    );

    // Isolation starts with the infectious stage, in the same step
    std::vector< int > hook_days;
    model.set_stage_hook(
        [&hook_days, I, ISOLATED](
            Agent<> * p, size_t k, int, int to, Model<> * m
        ) -> void {
            REQUIRE(k == 0u);
            if (to == I)
            {
                hook_days.push_back(m->today());
                p->change_status(*m, ISOLATED);
            }
        }
    );

    model.run(20, 1);

    // The stage changes of agent 0 (the only infected agent)
    auto on = [&](const std::string & from, const std::string & to) {
        auto counts = stage_transitions_by_day(model, from, to);
        std::vector< int > out;
        for (size_t d = 0u; d < counts.size(); ++d)
            for (int c = 0; c < counts[d]; ++c)
                out.push_back(static_cast< int >(d));
        return out;
    };

    REQUIRE(on("Susceptible", "Exposed") == std::vector< int >{0, 10});
    REQUIRE(on("Exposed", "Infectious") == std::vector< int >{2, 12});
    REQUIRE(on("Infectious", "Immune") == std::vector< int >{5, 15});

    // Waning: agent 0 on days 9 and 19, agent 1 on day 5 (immune from day 1)
    REQUIRE(on("Immune", "Susceptible") == std::vector< int >{5, 9, 19});
    REQUIRE(on("Susceptible", "Immune") == std::vector< int >{1});

    // The hook isolated agent 0 the day it became infectious, and it was
    // released once the virus was gone
    REQUIRE(hook_days == std::vector< int >{2, 12});
    REQUIRE(status_transitions_by_day(model, "Free", "Isolated")[2] == 1);
    REQUIRE(status_transitions_by_day(model, "Isolated", "Free")[6] == 1);

    // Agent 0's second episode, with its own entry days
    const auto & h = model.get_natural_history();
    const auto & a0 = model.get_agent(0);
    REQUIRE(h.get_episode(a0) == 2);
    REQUIRE(h.entered(a0, E) == 10);
    REQUIRE(h.entered(a0, I) == 12);
    REQUIRE(h.entered(a0, R) == 15);
    REQUIRE(h.entered(a0, S) == 19);
    REQUIRE(h.get_episode(model.get_agent(1)) == 0);
    REQUIRE(a0.get_virus() == nullptr);
    REQUIRE(a0.get_status(model) == FREE);

    // Bookkeeping: the joint table adds up to the stage counts, to the
    // statuses, and to the population, every day
    std::vector< int > date_j, counts_j, date_s, counts_s;
    std::vector< std::string > status_j, stage_j, stage_s;
    model.get_db().get_hist_joint(0u, date_j, status_j, stage_j, counts_j);
    model.get_db().get_hist_stage(0u, date_s, stage_s, counts_s);

    std::vector< int > date_t, counts_t;
    std::vector< std::string > status_t;
    model.get_db().get_hist_status(date_t, status_t, counts_t);

    const size_t ndays = model.get_ndays() + 1u;
    REQUIRE(date_s.size() == ndays * nh.size());
    REQUIRE(date_j.size() == ndays * nh.size() * model.get_n_statuses());
    for (size_t d = 0u; d < ndays; ++d)
    {
        int total = 0;
        for (size_t s = 0u; s < nh.size(); ++s)
        {
            int by_stage = 0;
            for (size_t st = 0u; st < model.get_n_statuses(); ++st)
                by_stage += counts_j[
                    (d * nh.size() + s) * model.get_n_statuses() + st
                ];
            REQUIRE(by_stage == counts_s[d * nh.size() + s]);
            total += by_stage;
        }
        REQUIRE(total == static_cast< int >(n));

        for (size_t st = 0u; st < model.get_n_statuses(); ++st)
        {
            int by_status = 0;
            for (size_t s = 0u; s < nh.size(); ++s)
                by_status += counts_j[
                    (d * nh.size() + s) * model.get_n_statuses() + st
                ];
            REQUIRE(by_status == counts_t[d * model.get_n_statuses() + st]);
        }
    }

    // A copy gives the same history (its natural history included)
    Model<> copy(model);
    copy.run(20, 1);
    std::vector< int > date_c, counts_c;
    std::vector< std::string > stage_c;
    copy.get_db().get_hist_stage(0u, date_c, stage_c, counts_c);
    REQUIRE(counts_c == counts_s);
    REQUIRE(copy.get_natural_history().get_episode(copy.get_agent(0)) == 2);

}
