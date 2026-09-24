#include "tests.hpp"
#include <set>
#include <algorithm>

using namespace epiworld;

static std::vector<size_t> make_households(size_t n, size_t hh_size)
{
    std::vector<size_t> hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / hh_size;
    return hh;
}

static std::set< std::pair<int,int> > edges_of(Model<> & model)
{
    std::vector< int > source, target;
    model.write_edgelist(source, target);

    std::set< std::pair<int,int> > res;
    for (size_t i = 0u; i < source.size(); ++i)
        res.emplace(
            std::min(source[i], target[i]), std::max(source[i], target[i])
        );

    return res;
}

// Ties the intervention puts up have to come back down: when the policy lifts,
// when the run ends, before the next run starts, and whenever the bubble is
// redrawn -- which happens mid-run, while agents are infectious.
EPIWORLD_TEST_CASE("Bubbles - ties are withdrawn", "[bubbles]") {

    // -- 1. When the policy lifts, when the run ends, and before the next ----
    //
    // Model::run() takes no population backup, so anything left behind would
    // still be there on the next run and would be captured by the backup
    // run_multiple() takes -- ties would accumulate run over run.
    {
        size_t n = 240u;
        auto hh = make_households(n, 4u);

        epimodels::ModelSEIR<> base("flu", 0.05, 0.1, 4.5, 1.0/8.0);
        base.seed(404);
        base.agents_smallworld(n, 6, false, 0.05);
        auto baseline = edges_of(base);

        epimodels::ModelSEIR<> model("flu", 0.05, 0.1, 4.5, 1.0/8.0);
        model.seed(404);
        model.agents_smallworld(n, 6, false, 0.05);

        int start = 3, end = 9;

        Bubbles<> bubbles(
            hh, BubbleFlavor::Household, 2u, 0.0, start, end, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete
        );
        model.add_globalevent(bubbles);
        model.verbose_off();

        // Watch the edge count across the policy window. The tool starts
        // damping on `start` and stops on `end`, and the ties follow on exactly
        // the same days.
        std::vector< std::pair<int, size_t> > sizes;
        model.add_globalevent(
            [&sizes](Model<> * m) -> void {
                std::vector< int > s, t;
                m->write_edgelist(s, t);
                sizes.emplace_back(m->today(), s.size());
            },
            "record edge counts"
        );

        model.run(14);

        REQUIRE(sizes.size() == 14u);

        size_t nbase = baseline.size();
        for (auto & rec : sizes)
        {
            int day = rec.first;
            // Global events run after the day's transitions, so what is
            // recorded on day d is the network day d+1 will use.
            bool in_force = ((day + 1) >= start) && ((day + 1) < end);

            if (in_force)
                REQUIRE(rec.second > nbase);
            else
                REQUIRE(rec.second == nbase);
        }

        // (a) The policy lifted mid-run, and (b) the run ended clean.
        REQUIRE(edges_of(model) == baseline);
        REQUIRE(Bubbles<>::get_from(model)->get_created_ties().empty());

        // (c) A second run does not inherit anything from the first.
        model.run(14);
        REQUIRE(edges_of(model) == baseline);

        // (d) And neither does run_multiple, whose replicates work on copies of
        //     the population taken after the runs above.
        model.run_multiple(12, 4, 99, make_save_run<int>(), true, false, 1);
        REQUIRE(edges_of(model) == baseline);

        // (e) A policy that never lifts is still withdrawn when the run ends --
        //     there is no end-of-run callback for a global event, so the last
        //     day is where it has to happen.
        epimodels::ModelSEIR<> never_lifts("flu", 0.05, 0.1, 4.5, 1.0/8.0);
        never_lifts.seed(404);
        never_lifts.agents_smallworld(n, 6, false, 0.05);

        Bubbles<> forever(
            hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete
        );
        never_lifts.add_globalevent(forever);
        never_lifts.verbose_off();

        size_t on_last_day = 0u;
        never_lifts.add_globalevent(
            [&on_last_day](Model<> * m) -> void {
                if (m->today() != 9)
                    return;
                std::vector< int > s, t;
                m->write_edgelist(s, t);
                on_last_day = s.size();
            },
            "record the second-to-last day"
        );

        never_lifts.run(10);

        REQUIRE(on_last_day > nbase);           // still up the day before the end
        REQUIRE(edges_of(never_lifts) == baseline);
        REQUIRE(Bubbles<>::get_from(never_lifts)->get_created_ties().empty());

        // (f) restore_network() cleans up a hand-driven loop that stopped early.
        epimodels::ModelSEIR<> partial("flu", 0.05, 0.1, 4.5, 1.0/8.0);
        partial.seed(404);
        partial.agents_smallworld(n, 6, false, 0.05);

        Bubbles<> always_on(
            hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete
        );
        partial.add_globalevent(always_on);
        partial.verbose_off();
        partial.reset();                 // sets the policy up, ties and all

        auto * policy = Bubbles<>::get_from(partial);
        REQUIRE_FALSE(policy->get_created_ties().empty());
        REQUIRE(edges_of(partial).size() > baseline.size());

        policy->restore_network(&partial);
        REQUIRE(policy->get_created_ties().empty());
        REQUIRE(edges_of(partial) == baseline);

        // Calling it again is harmless.
        policy->restore_network(&partial);
        REQUIRE(edges_of(partial) == baseline);
    }

    // -- 2. Redrawn mid-run without anything losing count --------------------
    //
    // With rewiring on, a Complete bubble tears down a clique and builds
    // another one every epoch, while agents are infectious. The queueing system
    // counts each agent's active neighbors, and every tie that appears or
    // disappears has to be reflected in that count, or agents silently stop
    // being visited by update_state().
    //
    // Nothing observable says "the queue is wrong", so the check is the one
    // epiworld relies on elsewhere (01-sir.cpp, 33b-edges-queue.cpp): queuing is
    // a pure optimisation, so the same model with it on and off must agree
    // exactly. Pushing never consults the queue, so the check is made pulling,
    // and again in the automatic mode users get by default.
    {
        size_t n = 600u;
        auto hh = make_households(n, 3u);

        struct Outcome {
            std::vector< int > counts;
            int last_epoch;
        };

        auto run_with = [&](TransmissionMode mode, bool queuing) -> Outcome {

            epimodels::ModelSEIR<> model("flu", 0.05, 0.15, 4.5, 1.0/8.0);
            model.seed(8181);
            model.agents_smallworld(n, 6, false, 0.05);
            model.set_transmission_mode(mode);

            if (!queuing)
                model.queuing_off();

            // Bubbles redrawn every 5 days, and leaky enough that out-of-bubble
            // contact still transmits -- so the run depends on both the ties
            // and the damping.
            Bubbles<> bubbles(
                hh, BubbleFlavor::Household, 2u, 0.3, 2, -1, 5,
                "Social bubble", 2u, "Bubble transmission factor",
                BubbleTies::Complete
            );
            model.add_globalevent(bubbles);
            model.verbose_off();
            model.run(45);

            Outcome res;
            model.get_db().get_hist_total(nullptr, nullptr, &res.counts);
            res.last_epoch = Bubbles<>::get_from(model)->get_last_epoch();
            return res;

        };

        for (auto mode : {TransmissionMode::pull, TransmissionMode::automatic})
        {

            auto with_queue    = run_with(mode, true);
            auto without_queue = run_with(mode, false);

            // An outbreak actually happened, so the comparison is not vacuous.
            REQUIRE(with_queue.counts.size() > 0u);
            REQUIRE(with_queue.counts.back() > 0);

            REQUIRE_THAT(without_queue.counts, Catch::Equals(with_queue.counts));

            // The partition really did move on -- otherwise no tie surgery
            // happened mid-run and this would be testing nothing.
            REQUIRE(with_queue.last_epoch == (45 - 2) / 5);

        }
    }

}
