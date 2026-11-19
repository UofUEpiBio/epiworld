#include "tests.hpp"

using namespace epiworld;

// Push for mixing layers (Phase 5). A push draws, for each carrier, the agents
// it meets with the probability their pulls would meet it, so each agent's
// exposures have the same distribution either way:
//
// - One carrier among N susceptibles, transmission probability p: on day 1 a
//   contact matrix with contact rate c infects c * p agents on average, and
//   groups of m with contact probability q infect (m - 1) * q * p, pushed or
//   pulled.
// - A model with a network, a contact matrix and households has the same mean
//   final size pushed and pulled.
// - The automatic mode pushes while carriers are few (mixing layers no longer
//   force a pull); a weekday schedule and a traced household layer behave the
//   same pushed.

namespace {

constexpr size_t N = EPI_TEST_SIZE(2000u, 600u);
constexpr size_t REPS = EPI_TEST_SIZE(400u, 100u);

// A model with one carrier and the given layers
template< typename AddLayers >
std::unique_ptr< epimodels::ModelSIR > one_carrier(double p, AddLayers add)
{
    // A share of 1.5 / N places floor(1.5) = 1 carrier
    auto model = std::make_unique< epimodels::ModelSIR >(
        "Virus", N, 1.5 / static_cast< double >(N), p, 0.0
    );
    model->verbose_off();
    add(*model);
    return model;
}

// Mean infections on day 1 over REPS runs
template< typename Build >
double day_one(Build build, const char * mode)
{
    double total = 0.0;
    for (size_t r = 0u; r < REPS; ++r)
    {
        auto model = build();
        model->set_transmission_mode(mode);
        model->run(1, static_cast< int >(r + 1u));
        std::vector< int > date, source, target, virus, expo, layer;
        model->get_db().get_transmissions(date, source, target, virus, expo, layer);
        for (size_t k = 0u; k < source.size(); ++k)
            total += (source[k] >= 0) ? 1.0 : 0.0;
    }
    return total / static_cast< double >(REPS);
}

}

EPIWORLD_TEST_CASE("Push for mixing layers", "[transmission]") {

    const double p = 0.5;

    // A contact matrix: c * p a day
    const double c = 8.0;
    auto matrix = [&]() {
        return one_carrier(p, [&](Model & m) {
            auto l = layer::mixing("public");
            l->set_groups(assign::labels(std::vector< size_t >(N, 0u)), 1u);
            l->set_contact_matrix({c});
            m.add_layer(l);
        });
    };

    // Within groups of 10: (m - 1) * q * p a day
    const size_t size = 10u;
    const double q = 0.6;
    auto within = [&]() {
        return one_carrier(p, [&](Model & m) {
            std::vector< size_t > g(N);
            for (size_t i = 0u; i < N; ++i)
                g[i] = i / size;
            auto l = layer::mixing("households");
            l->set_groups(assign::labels(g), N / size);
            l->set_contact_prob(q);
            m.add_layer(l);
        });
    };

    // The standard error of a mean of REPS binomial counts is under 0.25 here
    const double tol = 4.0 * std::sqrt(c * p / static_cast< double >(REPS));
    for (const char * mode : {"pull", "push"})
    {
        REQUIRE_THAT(day_one(matrix, mode), Catch::Matchers::WithinAbs(c * p, tol));
        REQUIRE_THAT(
            day_one(within, mode),
            Catch::Matchers::WithinAbs((size - 1u) * q * p, tol)
        );
    }

    // An asymmetric matrix and unequal groups (30% and 70%): infections by
    // the group of the infected on day 1, from 5% carriers, pushed and pulled
    auto by_group = [&](const char * mode) {
        std::array< double, 2 > out = {0.0, 0.0};
        for (size_t r = 0u; r < REPS; ++r)
        {
            auto model = std::make_unique< epimodels::ModelSIR >(
                "Virus", N, 0.05, 0.3, 0.0
            );
            model->verbose_off();
            std::vector< size_t > g(N);
            for (size_t i = 0u; i < N; ++i)
                g[i] = (i < (3u * N) / 10u) ? 0u : 1u;
            auto l = layer::mixing("public");
            l->set_groups(assign::labels(g), 2u);
            l->set_contact_matrix({6.0, 1.0, 4.0, 0.5});
            model->add_layer(l);
            model->set_transmission_mode(mode);
            model->run(1, static_cast< int >(r + 1u));

            std::vector< int > date, source, target, virus, expo, layer;
            model->get_db().get_transmissions(date, source, target, virus, expo, layer);
            for (size_t k = 0u; k < source.size(); ++k)
                if (source[k] >= 0)
                    out[g[static_cast< size_t >(target[k])]] += 1.0 / REPS;
        }
        return out;
    };

    const auto bg_pull = by_group("pull");
    const auto bg_push = by_group("push");
    for (size_t h = 0u; h < 2u; ++h)
    {
        REQUIRE(bg_pull[h] > 1.0);
        REQUIRE_THAT(
            bg_push[h],
            Catch::Matchers::WithinAbs(bg_pull[h], 6.0 * std::sqrt(2.0 * bg_pull[h] / REPS))
        );
    }

    // A full model: the same mean final size pushed and pulled
    auto full = []() {
        auto model = std::make_unique< epimodels::ModelSIR >(
            "Virus", N, 0.005, 0.08, 1.0 / 7.0
        );
        model->verbose_off();
        model->add_layer(layer::network("work", graph::smallworld(4, 0.1)));

        auto pub = layer::mixing("public");
        pub->set_groups(assign::shares({{0, 0.5}, {1, 0.5}}), 2u);
        pub->set_contact_matrix({2.0, 1.0, 1.0, 2.0});
        model->add_layer(pub);

        std::vector< size_t > g(N);
        for (size_t i = 0u; i < N; ++i)
            g[i] = i / 4u;
        auto home = layer::mixing("home");
        home->set_groups(assign::labels(g), N / 4u);
        home->set_contact_prob(0.5);
        model->add_layer(home);
        return model;
    };

    auto final_size = [&](const char * mode) {
        auto model = full();
        model->set_transmission_mode(mode);
        std::vector< double > sizes;
        model->run_multiple(60, REPS / 4u, 123, [&](size_t, Model * m) {
            std::vector< int > counts;
            m->get_db().get_today_total(nullptr, nullptr, &counts);
            sizes.push_back(static_cast< double >(N - static_cast< size_t >(counts[0])));
        }, true, false, 1);
        double mean = 0.0, var = 0.0;
        for (double x : sizes) mean += x / sizes.size();
        for (double x : sizes) var += (x - mean) * (x - mean) / (sizes.size() - 1u);
        return std::make_pair(mean, var / sizes.size());
    };

    const auto pulled = final_size("pull");
    const auto pushed = final_size("push");
    REQUIRE(pulled.first > 0.1 * N);
    REQUIRE_THAT(
        pushed.first,
        Catch::Matchers::WithinAbs(pulled.first, 4.0 * std::sqrt(pulled.second + pushed.second))
    );

    // Mixing layers no longer force a pull; auto pushes early
    {
        auto model = full();
        model->set_transmission_mode("auto");
        model->run(1, 5);
        REQUIRE(model->get_last_transmission_mode() == TransmissionMode::push);

        model->set_transmission_mode("push");
        model->run(10, 5);
        REQUIRE(model->get_last_transmission_mode() == TransmissionMode::push);
    }

    // A weekday schedule and a traced household layer, pushed
    {
        auto model = full();
        model->set_transmission_mode("push");
        model->get_layer("public").set_schedule({true, true, true, true, true, false, false});
        model->get_layer("home").enable_tracing(100u);
        model->run(40, 9);

        std::vector< int > date, source, target, virus, expo, layer;
        model->get_db().get_transmissions(date, source, target, virus, expo, layer);
        size_t n_public = 0u, n_home = 0u;
        for (size_t k = 0u; k < source.size(); ++k)
        {
            if (source[k] < 0)
                continue;
            if (layer[k] == 1)
            {
                ++n_public;
                REQUIRE(((date[k] - 1) % 7) < 5);
            }
            if (layer[k] == 2)
            {
                ++n_home;
                REQUIRE(source[k] / 4 == target[k] / 4);
            }
        }
        REQUIRE(n_public > 5u);
        REQUIRE(n_home > 5u);

        // The log has only housemates
        auto & log = model->get_layer("home").get_tracing();
        size_t logged = 0u;
        for (size_t a = 0u; a < log.get_n_agents(); ++a)
            for (size_t k = 0u; k < std::min(log.get_n_contacts(a), log.get_max_contacts()); ++k)
            {
                ++logged;
                REQUIRE(log.get_contact(a, k).first / 4 == a / 4);
            }
        REQUIRE(logged > 10u);
    }

}
