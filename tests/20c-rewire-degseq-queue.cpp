#include "tests.hpp"

using namespace epiworld;

// Rewiring the network during a run must not disturb the queueing system.
//
// With a rewiring function set, Model::rewire() runs on every step, and
// rewire_degseq() moves ties with Agent::swap_neighbors(): (i-j) and (k-l)
// become (i-l) and (k-j). The queue counts, for every agent, the registered
// agents among itself and its neighbors. The count is built when an agent is
// registered and unwound when it is deregistered, both walking the neighbors
// the agent has *at that moment*, so a swap in between that the queue is not
// told about leaves the two walks disagreeing. Before swap_neighbors() told
// it, an agent whose count drifted to zero was skipped by update_state(): its
// neighbors' infections never reached it, and an infected agent stranded there
// never recovered.
//
// As in 33b-edges-queue.cpp, the check is that queuing is a pure optimisation:
// a run with it on and a run with it off must agree exactly. That holds in
// every transmission mode (pushing consults the queue too), and in this model
// "auto" switches between the two.
EPIWORLD_TEST_CASE("Rewire degseq - mid-run rewiring keeps the queue honest", "[rewire_degseq]") {

    struct Run {
        std::vector< int > hist_date, hist_counts;
        std::vector< std::string > hist_state;
        std::vector< int > date, source, target, virus, source_exposure_date;
        std::vector< int > net_source_0, net_target_0; // Network before the run
        std::vector< int > net_source, net_target;     // ... and after it
        int n_push = 0, n_pull = 0;
    };

    auto simulate = [](const std::string & mode, bool queuing) -> Run {

        Run r;

        epimodels::ModelSIR<> model("v", 0.01, 0.3, 0.2);
        model.verbose_off();
        model.agents_smallworld(2000, 6, false, 0.05);
        model.set_rewire_fun(
            [](std::vector< Agent<> > * a, Model<> * m, epiworld_double p) -> void {
                rewire_degseq(a, m, p);
            }
        );
        model.set_rewire_prop(0.1);
        model.set_transmission_mode(mode);

        if (!queuing)
            model.queuing_off();

        // Draws no random numbers, so it leaves the run as it was
        model.add_globalevent([&r](Model<> * m) -> void {
            if (m->get_last_transmission_mode() == TransmissionMode::push)
                ++r.n_push;
            else
                ++r.n_pull;
        }, "count steps");

        model.write_edgelist(r.net_source_0, r.net_target_0);

        model.run(60, 123);

        model.get_db().get_hist_total(&r.hist_date, &r.hist_state, &r.hist_counts);
        model.get_db().get_transmissions(
            r.date, r.source, r.target, r.virus, r.source_exposure_date
        );
        model.write_edgelist(r.net_source, r.net_target);

        return r;

    };

    for (std::string mode : {"pull", "auto", "push"})
    {

        Run on  = simulate(mode, true);
        Run off = simulate(mode, false);

        INFO("transmission mode: " << mode);

        // Not vacuous: an outbreak happened, on a network rewired under it
        bool rewired =
            (on.net_source != on.net_source_0) ||
            (on.net_target != on.net_target_0);

        REQUIRE(on.date.size() > 1000u);
        REQUIRE(rewired);

        if (mode == "auto")
        {
            REQUIRE(on.n_push > 0);
            REQUIRE(on.n_pull > 0);
        }

        REQUIRE_THAT(off.hist_date, Catch::Equals(on.hist_date));
        REQUIRE_THAT(off.hist_state, Catch::Equals(on.hist_state));
        REQUIRE_THAT(off.hist_counts, Catch::Equals(on.hist_counts));

        REQUIRE_THAT(off.date, Catch::Equals(on.date));
        REQUIRE_THAT(off.source, Catch::Equals(on.source));
        REQUIRE_THAT(off.target, Catch::Equals(on.target));
        REQUIRE_THAT(off.virus, Catch::Equals(on.virus));
        REQUIRE_THAT(off.source_exposure_date, Catch::Equals(on.source_exposure_date));

        // Both runs drew the same random numbers, rewiring included
        REQUIRE_THAT(off.net_source, Catch::Equals(on.net_source));
        REQUIRE_THAT(off.net_target, Catch::Equals(on.net_target));

    }

}
