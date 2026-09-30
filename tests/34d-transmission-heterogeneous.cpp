#include "tests.hpp"

using namespace epiworld;

// Whole-model behaviour of the transmission step on a heterogeneous network:
// household cliques plus a heavy-tailed layer of random ties, with a large
// outbreak.
//
// 1. Queuing stays a pure optimisation in every mode (pull, push, auto): on
//    and off give identical daily histories and transmission lists.
// 2. The built-in pull scan is the same draw as the plain scan of the
//    neighbors: a susceptible state written with
//    `sampler::make_sample_virus_neighbors()` (which always walks the
//    neighbors' agents) gives exactly the run of the built-in state in "pull"
//    mode, so making that scan cheaper did not touch the random stream.
//
// (The automatic choice itself is in 34e.)

namespace {

struct Run {
    std::vector< int > counts, date, source, target;
    bool operator==(const Run & o) const {
        return (counts == o.counts) && (date == o.date) &&
            (source == o.source) && (target == o.target);
    }
};

// A SEIR whose latent (exposed) agents do not transmit. With `plain_scan`, the
// susceptible state is the sampling function that walks the neighbors
// directly instead of the built-in one.
Run simulate(
    const std::string & mode,
    bool queuing,
    bool plain_scan
)
{

    epimodels::ModelSEIR<> model("virus", 20.0 / 3000.0, 0.1, 3.0, 1.0 / 7.0);

    if (plain_scan)
    {
        auto pick = sampler::make_sample_virus_neighbors<>({1u});
        model.set_state_function(
            0u,
            [pick](Agent<> * p, Model<> * m) -> void {
                Virus<> * v = pick(p, m);
                if (v != nullptr)
                    p->set_virus(*m, *v);
            }
        );
    }
    else
        model.set_state_function(0u, sampler::make_update_susceptible<>({1u}));

    model.seed(31);
    tests_heterogeneous_network(model, 3000u);

    if (!queuing)
        model.queuing_off();

    model.set_transmission_mode(mode);
    model.verbose_off();

    model.run(80, 77);

    Run r;
    model.get_db().get_hist_total(nullptr, nullptr, &r.counts);
    std::vector< int > virus, sexp;
    model.get_db().get_transmissions(r.date, r.source, r.target, virus, sexp);
    return r;

}

} // namespace

EPIWORLD_TEST_CASE("Transmission - heterogeneous network", "[transmission]") {

    // 1. Queuing on == queuing off, in every mode ------------------------------
    for (auto mode : {"pull", "push", "auto"})
    {
        Run with_queue = simulate(mode, true, false);
        Run without_queue = simulate(mode, false, false);

        INFO("mode " << mode);
        // A large outbreak: a third of the agents or more
        REQUIRE(with_queue.date.size() > 1000u);
        REQUIRE(with_queue == without_queue);
    }

    // 2. The built-in pull scan is the plain scan -----------------------------
    for (bool queuing : {true, false})
    {
        Run builtin = simulate("pull", queuing, false);
        Run plain = simulate("pull", queuing, true);

        INFO("queuing " << queuing);
        REQUIRE(builtin.date.size() > 1000u);
        REQUIRE(builtin == plain);
    }

}
