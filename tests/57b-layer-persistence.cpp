#include "tests.hpp"

using namespace epiworld;

// Persistence in a within-group mixing layer (Phase 4c). A transit layer has
// 50 trips (groups) of 60 riders on 10 routes (pools of five trips). With
// q = 1 every transit transmission is between riders of the same usual trip
// (and the run is the one without persistence); with q = 0 every rider takes
// another trip of its route each day, so transmissions cross trips but never
// routes; with q = 0.7 about 30% of the riders are away on a given day. A q
// per agent keeps some riders always home and sends others always away. An
// assignment function takes everyone out of the layer on odd days. The queue
// follows the moves (a run with it matches one without it), and the errors
// say what is missing.

namespace {

constexpr size_t N = 3000u;
constexpr size_t TRIPS = 50u;
constexpr size_t PER_ROUTE = 5u;
constexpr int DAYS = 40;

size_t usual_trip(size_t i) { return i / (N / TRIPS); }
size_t route(size_t trip) { return trip / PER_ROUTE; }

std::shared_ptr< layer::Mixing > transit()
{
    auto t = layer::mixing("transit");
    std::vector< size_t > trips(N);
    for (size_t i = 0u; i < N; ++i)
        trips[i] = usual_trip(i);
    t->set_groups(assign::labels(trips), TRIPS);
    t->set_contact_prob(0.05);

    std::vector< size_t > pools(TRIPS);
    for (size_t g = 0u; g < TRIPS; ++g)
        pools[g] = route(g);
    t->set_pools(pools);
    return t;
}

std::unique_ptr< epimodels::ModelSIR > build(std::shared_ptr< layer::Mixing > t)
{
    auto model = std::make_unique< epimodels::ModelSIR >(
        "Virus", N, 0.01, 0.3, 0.1
    );
    model->verbose_off();
    model->add_layer(layer::network("home", graph::smallworld(4, 0.05)));
    model->add_layer(t);
    return model;
}

struct Transmissions {
    std::vector< int > date, source, target, virus, expo, layer;
};

bool same(const Transmissions & a, const Transmissions & b)
{
    return (a.date == b.date) && (a.source == b.source) &&
        (a.target == b.target) && (a.layer == b.layer);
}

Transmissions transmissions(Model & model)
{
    Transmissions tx;
    model.get_db().get_transmissions(
        tx.date, tx.source, tx.target, tx.virus, tx.expo, tx.layer
    );
    return tx;
}

// Per day, the share of riders not in their usual trip (from a global event)
std::vector< double > away_share;
void count_away(Model * m)
{
    auto & t = *m->get_layer("transit").as_mixing();
    size_t away = 0u;
    for (size_t i = 0u; i < N; ++i)
        away += (t.get_group(i) != t.get_usual_group(i)) ? 1u : 0u;
    away_share.push_back(static_cast< double >(away) / static_cast< double >(N));
}

}

EPIWORLD_TEST_CASE("Persistence in a mixing layer", "[layer-mixing]") {

    // q = 1: fixed cohorts, and the same run as without persistence
    {
        auto t = transit();
        t->set_persistence(1.0);
        auto model = build(t);
        model->run(DAYS, 3);
        const auto tx = transmissions(*model);

        size_t n_transit = 0u;
        for (size_t k = 0u; k < tx.source.size(); ++k)
            if ((tx.source[k] >= 0) && (tx.layer[k] == 1))
            {
                ++n_transit;
                REQUIRE(
                    usual_trip(static_cast< size_t >(tx.source[k])) ==
                    usual_trip(static_cast< size_t >(tx.target[k]))
                );
            }
        REQUIRE(n_transit > 20u);

        auto plain = build(transit());
        plain->run(DAYS, 3);
        REQUIRE(same(transmissions(*plain), tx));
    }

    // q = 0: another trip of the route every day
    {
        auto t = transit();
        t->set_persistence(0.0);
        auto model = build(t);
        model->run(DAYS, 3);
        const auto tx = transmissions(*model);

        size_t crossing = 0u;
        for (size_t k = 0u; k < tx.source.size(); ++k)
            if ((tx.source[k] >= 0) && (tx.layer[k] == 1))
            {
                const size_t a = usual_trip(static_cast< size_t >(tx.source[k]));
                const size_t b = usual_trip(static_cast< size_t >(tx.target[k]));
                REQUIRE(route(a) == route(b));
                crossing += (a != b) ? 1u : 0u;
            }
        REQUIRE(crossing > 10u);

        // Everyone is away (the last day's draw), in its own route
        for (size_t i = 0u; i < N; ++i)
        {
            REQUIRE(t->get_usual_group(i) == usual_trip(i));
            REQUIRE(t->get_group(i) != usual_trip(i));
            REQUIRE(route(t->get_group(i)) == route(usual_trip(i)));
        }
    }

    // q = 0.7: about 30% away on a given day
    {
        auto t = transit();
        t->set_persistence(0.7);
        auto model = build(t);
        model->add_globalevent(count_away, "Count");
        away_share.clear();
        model->run(DAYS, 3);
        REQUIRE(away_share.size() == static_cast< size_t >(DAYS));
        double mean = 0.0;
        for (double s : away_share)
            mean += s / static_cast< double >(away_share.size());
        REQUIRE_THAT(mean, Catch::Matchers::WithinAbs(0.3, 0.01));
    }

    // A q per agent: the first half always home, the second always away
    {
        auto t = transit();
        std::vector< double > q(N, 1.0);
        for (size_t i = N / 2u; i < N; ++i)
            q[i] = 0.0;
        t->set_persistence(q);
        auto model = build(t);
        model->run(10, 3);
        for (size_t i = 0u; i < N; ++i)
            REQUIRE((t->get_group(i) == usual_trip(i)) == (i < N / 2u));
    }

    // An assignment: out of the layer on odd days
    {
        auto t = transit();
        t->set_assignment([](size_t, size_t usual, Model & m) {
            return ((m.today() + 1) % 2 == 0) ? usual : layer::Mixing::NO_GROUP;
        });
        REQUIRE(t->has_assignment());
        auto model = build(t);
        model->run(DAYS, 3);
        const auto tx = transmissions(*model);
        size_t n_transit = 0u;
        for (size_t k = 0u; k < tx.source.size(); ++k)
            if ((tx.source[k] >= 0) && (tx.layer[k] == 1))
            {
                ++n_transit;
                REQUIRE(tx.date[k] % 2 == 0);
            }
        REQUIRE(n_transit > 10u);
    }

    // The queue follows the moves
    {
        auto with = transit();
        with->set_persistence(0.5);
        auto queued = build(with);
        queued->run(DAYS, 7);

        auto without = transit();
        without->set_persistence(0.5);
        auto unqueued = build(without);
        unqueued->queuing_off();
        unqueued->run(DAYS, 7);

        REQUIRE(same(transmissions(*queued), transmissions(*unqueued)));
    }

    // Errors
    {
        auto t = layer::mixing("transit");
        std::vector< size_t > trips(N);
        for (size_t i = 0u; i < N; ++i)
            trips[i] = usual_trip(i);
        t->set_groups(assign::labels(trips), TRIPS);
        t->set_contact_prob(0.05);
        t->set_persistence(0.5);
        auto model = build(t);
        REQUIRE_THROWS_WITH(model->run(5, 1), Catch::Matchers::Contains("set_pools()"));

        t->set_pools(std::vector< size_t >(TRIPS, 0u));
        t->set_assignment([](size_t, size_t usual, Model &) { return usual; });
        REQUIRE_THROWS_WITH(model->run(5, 1), Catch::Matchers::Contains("both"));

        t->clear_assignment();
        t->set_persistence(std::vector< double >{0.5, 0.5});
        REQUIRE_THROWS_AS(model->run(5, 1), std::length_error);

        REQUIRE_THROWS_AS(t->set_persistence(1.5), std::range_error);

        auto matrix = layer::mixing("public");
        matrix->set_groups(assign::labels(std::vector< size_t >(N, 0u)), 1u);
        matrix->set_contact_matrix({2.0});
        matrix->set_persistence(0.5);
        auto other = build(transit());
        other->add_layer(matrix);
        REQUIRE_THROWS_WITH(other->run(5, 1), Catch::Matchers::Contains("within groups"));
    }

}
