#include "tests.hpp"
#include <numeric>

using namespace epiworld;

// A mixing layer with a contact probability per pair mixes within groups only:
// here 60 households of 10 agents added to an SIR model. The model's queue sees
// the layer, so a run with the queue is the same run, draw for draw, as one
// without it -- also when agents change households during the run. The layer
// is the same run as one built with the equivalent diagonal contact matrix.
// With q = 1 every household is a clique; with q < 1 each housemate is met
// with probability q. Probabilities set between runs are kept; set during a
// run, they last for that run.
namespace {

const size_t n = 600u;
const size_t n_groups = 60u;

std::vector< size_t > households()
{
    std::vector< size_t > g(n);
    for (size_t i = 0u; i < n; ++i)
        g[i] = i / 10u;
    return g;
}

// SIR model; S = 0, I = 1, R = 2
std::unique_ptr< epimodels::ModelSIR<> > make(
    std::function< void(layer::Mixing<> &) > contacts,
    double prevalence = 0.02,
    double transmission = 0.2,
    double recovery = 0.1
)
{

    auto model = std::make_unique< epimodels::ModelSIR<> >(
        "flu", n, prevalence, transmission, recovery
    );
    model->verbose_off();

    auto hh = layer::mixing<>("households", {1}, {0, 1, 2});
    hh->set_groups(households(), n_groups);
    contacts(*hh);
    model->add_layer(hh);

    return model;

}

void half(layer::Mixing<> & l) { l.set_contact_prob(0.5); }

// The same contacts as a matrix: 4.5 contacts a day with the 9 housemates
void matrix(layer::Mixing<> & l)
{
    std::vector< double > cm(n_groups * n_groups, 0.0);
    for (size_t g = 0u; g < n_groups; ++g)
        cm[g * n_groups + g] = 4.5;
    l.set_contact_matrix(cm);
}

// Final states, by agent
std::vector< int > states(Model<> & m)
{
    std::vector< int > out;
    for (const auto & a : m.get_agents())
        out.push_back(static_cast< int >(a.get_state()));
    return out;
}

}

EPIWORLD_TEST_CASE("Mixing layer - within groups", "[layer-mixing-within]") {

    // -- the queue sees the groups -----------------------------------------
    // Agents move to the next household on day 5 (an edit during the run),
    // so the queue must also follow the moves.
    auto move = [](Model<> * m) -> void {
        auto & l = m->get_layer_as< layer::Mixing<> >("households");
        for (size_t i = 0u; i < n; i += 7u)
            l.set_group(i, (l.get_group(i) + 1u) % n_groups);
    };

    std::vector< int > hist[2], src[2];
    for (bool queue : {true, false})
    {

        auto model = make(half);
        if (!queue)
            model->queuing_off();

        model->add_globalevent(move, "move", 5);
        model->run(60, 46);

        model->get_db().get_hist_total(nullptr, nullptr, &hist[queue]);
        std::vector< int > d, t, v, se, lay;
        model->get_db().get_transmissions(d, src[queue], t, v, se, lay);

        // Every transmission came through the layer, between housemates of
        // the day (the move makes some cross the original households)
        REQUIRE(src[queue].size() > 50u);
        for (size_t k = 0u; k < lay.size(); ++k)
            REQUIRE(lay[k] == (src[queue][k] < 0 ? -1 : 0));

    }
    REQUIRE(hist[0] == hist[1]);
    REQUIRE(src[0] == src[1]);

    // A move made while the agents update (here, by the carrier itself) waits
    // for the end of the step: the carrier's housemate still meets it (q = 1,
    // and the contact transmits for sure), with or without the queue
    for (bool queue : {true, false})
    {

        epimodels::ModelSIR<> tiny("flu", 3, 0.0, 1.0, 0.0);
        tiny.verbose_off();
        tiny.get_virus(0).set_distribution(distribute_virus_to_set<>({0}));

        auto hh = layer::mixing<>("hh", {1}, {0, 1, 2});
        hh->set_groups({0, 0, 1}, 2);
        hh->set_contact_prob(1.0);
        tiny.add_layer(hh);

        tiny.set_state_function(1, [](Agent<> * a, Model<> * m) -> void {
            if (a->get_id() == 0)
                m->get_layer_as< layer::Mixing<> >("hh").set_group(0, 1);
        });

        if (!queue)
            tiny.queuing_off();

        tiny.run(1, 46);

        REQUIRE(states(tiny) == std::vector< int >{1, 1, 0});
        REQUIRE(hh->get_group(0) == 1u);

    }

    // The edit lasted for that run only
    auto check = make(half);
    check->add_globalevent(move, "move", 5);
    check->run(10, 1);
    auto & hh = check->get_layer_as< layer::Mixing<> >("households");
    REQUIRE(hh.get_group(7u) == 1u);
    check->rm_globalevent("move");
    check->run(1, 1);
    REQUIRE(hh.get_group(7u) == 0u);
    REQUIRE(hh.members_of(0u).size() == 10u);

    // -- the equivalent diagonal matrix is the same run ---------------------
    // q = 0.5 per housemate is 4.5 contacts a day with 9 housemates
    auto with_q = make(half);
    auto with_matrix = make(matrix);
    with_q->run(60, 2026);
    with_matrix->run(60, 2026);
    REQUIRE(states(*with_q) == states(*with_matrix));

    // -- q = 1: everyone meets everyone, every day --------------------------
    // One carrier (agent 0), transmission and contact for sure: after a day,
    // its 9 housemates are infected, and nobody else
    auto clique = make(
        [](layer::Mixing<> & l) { l.set_contact_prob(1.0); }, 0.0, 1.0, 0.0
    );
    clique->get_virus(0).set_distribution(distribute_virus_to_set<>({0}));
    clique->run(1, 3);

    std::vector< int > expected(n, 0);
    std::fill(expected.begin(), expected.begin() + 10, 1);
    REQUIRE(states(*clique) == expected);

    // -- q < 1: each housemate is met with probability q --------------------
    // Same setup with q = 0.3: the carrier infects Binomial(9, 0.3) housemates
    auto pairs = make(
        [](layer::Mixing<> & l) { l.set_contact_prob(0.3); }, 0.0, 1.0, 0.0
    );
    pairs->get_virus(0).set_distribution(distribute_virus_to_set<>({0}));

    std::vector< double > infected;
    bool outside = false;
    pairs->run_multiple(
        1, 400, 2026,
        [&](size_t, Model<> * m) -> void {
            auto s = states(*m);
            infected.push_back(std::accumulate(s.begin() + 1, s.begin() + 10, 0.0));
            outside = outside || (std::accumulate(s.begin() + 10, s.end(), 0) > 0);
        },
        true, false, 1
    );

    double mean = 0.0;
    for (double x : infected)
        mean += x;
    mean /= infected.size();

    // SD of the mean: sqrt(9 * 0.3 * 0.7 / 400) = 0.069
    REQUIRE(!outside);
    REQUIRE(std::abs(mean - 9.0 * 0.3) < 4.0 * 0.069);

    // -- contact probabilities are edited like the contact matrix -----------
    auto edit = make(half);
    auto & layer = edit->get_layer_as< layer::Mixing<> >("households");

    // Between runs: the new baseline, one per group or one for all
    std::vector< double > q(n_groups, 0.1);
    q[0] = 0.9;
    layer.set_contact_prob(q);
    edit->run(1, 1);
    REQUIRE(layer.get_contact_prob() == q);

    // During a run: that run only
    edit->add_globalevent(
        [](Model<> * m) -> void {
            m->get_layer_as< layer::Mixing<> >("households").set_contact_prob(0.0);
        },
        "lockdown"
    );
    edit->run(2, 1);
    REQUIRE(layer.get_contact_prob() == std::vector< double >(n_groups, 0.0));
    edit->rm_globalevent("lockdown");
    edit->run(1, 1);
    REQUIRE(layer.get_contact_prob() == q);

    // A matrix replaces the probabilities between runs, but not during one
    edit->add_globalevent(
        [](Model<> * m) -> void {
            m->get_layer_as< layer::Mixing<> >("households")
                .set_contact_matrix({1.0});
        },
        "switch"
    );
    REQUIRE_THROWS_AS(edit->run(1, 1), std::logic_error);

    // Probabilities need the grouping (the queue reads it before the first
    // step)
    epimodels::ModelSIR<> bare("flu", n, 0.02, 0.2, 0.1);
    auto nogroups = layer::mixing<>("households", {1}, {0, 1, 2});
    nogroups->set_contact_prob(0.5);
    bare.add_layer(nogroups);
    bare.verbose_off();
    REQUIRE_THROWS_AS(bare.run(1, 1), std::logic_error);

    // One probability per group, or one for all, each in [0, 1]
    auto wrong = make(half);
    wrong->get_layer_as< layer::Mixing<> >("households")
        .set_contact_prob({0.1, 0.2});
    REQUIRE_THROWS_AS(wrong->run(1, 1), std::length_error);
    REQUIRE_THROWS_AS(layer.set_contact_prob(1.5), std::range_error);

    // Agents and groups out of range are errors, not reads past the end
    REQUIRE_THROWS_AS(layer.get_group(n), std::range_error);
    REQUIRE_THROWS_AS(layer.members_of(n_groups), std::range_error);
    REQUIRE_THROWS_AS(layer.set_group(0u, n_groups), std::range_error);

}
