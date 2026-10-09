#include "tests.hpp"

using namespace epiworld;

// Whole-model behaviour of the transmission step on a heterogeneous network:
// household cliques plus a heavy-tailed layer of random ties, with a large
// outbreak.
//
// 1. Queuing stays a pure optimisation in every mode (pull, push, auto): on
//    and off give identical daily histories and transmission lists.
// 2. The built-in pull scan is the same draw as the plain scan of the
//    neighbors: a susceptible state that walks the neighbors' agents itself
//    (`plain_scan_update` below) gives exactly the run of the built-in state
//    in "pull" mode, so making that scan cheaper did not touch the random
//    stream.
// 3. The neighbor view used by that scan refuses indices past its end.
//
// (The automatic choice itself is in 34e.)

namespace {

// The reference pull: walks every neighbor's agent (no carrier flags), skips
// the latent ones (state 1) and the ones without a virus, and draws at most
// one virus.
void plain_scan_update(Agent<> * p, Model<> * m)
{

    size_t n = 0u;
    for (auto * neighbor : p->neighbors_view(*m))
    {

        auto & v = neighbor->get_virus();
        if ((neighbor->get_state() == 1u) || (v == nullptr))
            continue;

        m->array_double_tmp[n] =
            (1.0 - p->get_susceptibility_reduction(v, *m)) *
            v->get_prob_infecting(m) *
            (1.0 - neighbor->get_transmission_reduction(v, *m));
        m->array_virus_tmp[n++] = &(*v);

    }

    if (n == 0u)
        return;

    int which = roulette(n, m);
    if (which >= 0)
        p->set_virus(*m, *m->array_virus_tmp[which]);

}

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
        model.set_state_function(0u, plain_scan_update);
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

    // 3. The neighbor view checks its indices ---------------------------------
    {
        Model<> model;
        model.add_state("Susceptible", default_update_susceptible<>);
        model.agents_smallworld(20, 4, false, 0.0);
        model.verbose_off();
        model.run(1, 1);

        auto view = model.get_agent(0).neighbors_view(model);
        REQUIRE(view.size() == 4u);
        REQUIRE(view.id(3u) == view.ids()[3]);
        REQUIRE(view.agent(3u)->get_id() == view.id(3u));
        REQUIRE_THROWS_AS(view.id(4u), std::out_of_range);
        REQUIRE_THROWS_AS(view.agent(4u), std::out_of_range);
    }

}
