#include "tests.hpp"

using namespace epiworld;

// A layer keeps its own contact log once enable_tracing() is called. With
// certain transmission, every transmission through a traced layer is in that
// layer's log; an untraced layer keeps none. The log starts empty at every run,
// and pull and push log the same contacts.

namespace {

using Log = std::set< std::pair< size_t, size_t > >;

// Every (carrier, contact) pair in the log
Log pairs_of(ContactTracing & ct)
{
    Log out;
    for (size_t a = 0u; a < ct.get_n_agents(); ++a)
        for (const auto & rec : ct.get_contacts(a))
            out.insert({a, rec.get_contact_id()});
    return out;
}

}

EPIWORLD_TEST_CASE("Layer contact tracing", "[layer-tracing]") {

    // Two networks and a households layer (cliques); only two are traced
    epimodels::ModelSIR<> model("Virus", 600, 0.02, 1.0, 0.3);

    auto home = layer::network("home", graph::smallworld<>(4, 0.1));
    auto work = layer::network("work", graph::smallworld<>(6, 0.3));
    auto hh = layer::mixing<>("households");
    std::vector< size_t > groups(600);
    for (size_t i = 0u; i < 600u; ++i)
        groups[i] = i / 4u;
    hh->set_groups(groups, 150u);
    hh->set_contact_prob(1.0);

    home->enable_tracing(1000u);
    hh->enable_tracing(1000u);

    model.add_layer(home);
    model.add_layer(work);
    model.add_layer(hh);

    REQUIRE_FALSE(work->is_tracing());
    REQUIRE_THROWS_AS(work->get_tracing(), std::logic_error);
    REQUIRE_THROWS_AS(home->enable_tracing(0u), std::invalid_argument);

    model.verbose_off();
    model.run(30, 54);

    std::vector< int > date, source, target, virus, expo, layer;
    model.get_db().get_transmissions(date, source, target, virus, expo, layer);

    REQUIRE(home->get_tracing().get_n_agents() == model.size());
    const Log home_log = pairs_of(home->get_tracing());
    const Log hh_log = pairs_of(hh->get_tracing());

    // Every transmission through a traced layer is in that layer's log
    size_t n_traced = 0u, n_work = 0u;
    for (size_t i = 0u; i < source.size(); ++i)
    {
        if (layer[i] < 0)
            continue;

        const std::pair< size_t, size_t > p(source[i], target[i]);
        if (layer[i] == static_cast< int >(home->get_id()))
        {
            REQUIRE(home_log.count(p) == 1u);
            ++n_traced;
        }
        else if (layer[i] == static_cast< int >(hh->get_id()))
        {
            REQUIRE(hh_log.count(p) == 1u);
            ++n_traced;
        }
        else
            ++n_work;
    }

    REQUIRE(n_traced > 0u);
    REQUIRE(n_work > 0u);

    // Households: a contact is always a housemate
    for (const auto & p : hh_log)
        REQUIRE(p.first / 4u == p.second / 4u);

    // Every run starts with an empty log: the same seed, the same log
    model.run(30, 54);
    REQUIRE(pairs_of(home->get_tracing()) == home_log);

    size_t total = 0u;
    for (size_t a = 0u; a < model.size(); ++a)
        total += home->get_tracing().get_n_contacts(a);
    size_t total_again = 0u;
    model.run(30, 54);
    for (size_t a = 0u; a < model.size(); ++a)
        total_again += home->get_tracing().get_n_contacts(a);
    REQUIRE(total == total_again);

    // Pull and push log the same contacts (no transmission: the population
    // never changes, so both modes see the same carriers every day)
    auto run = [](TransmissionMode mode) {

        epimodels::ModelSIR<> m("Virus", 400, 0.05, 0.0, 0.0);
        auto net = layer::network("network", graph::smallworld<>(6, 0.1));
        net->enable_tracing(1000u);
        m.add_layer(net);
        m.set_transmission_mode(mode);
        m.verbose_off();
        m.run(5, 331);

        std::vector< std::vector< std::pair< size_t, int > > > out(m.size());
        auto & ct = net->get_tracing();
        for (size_t a = 0u; a < m.size(); ++a)
        {
            for (size_t k = 0u; k < ct.get_n_contacts(a); ++k)
                out[a].push_back(ct.get_contact(a, k));
            std::sort(out[a].begin(), out[a].end());
        }

        return std::make_pair(out, m.get_last_transmission_mode());

    };

    auto pull = run(TransmissionMode::pull);
    auto push = run(TransmissionMode::push);
    REQUIRE(pull.second == TransmissionMode::pull);
    REQUIRE(push.second == TransmissionMode::push);
    REQUIRE(pull.first == push.first);

    size_t n_logged = 0u;
    for (const auto & row : push.first)
        n_logged += row.size();
    REQUIRE(n_logged > 0u);

    // Turning tracing off drops the log
    home->disable_tracing();
    REQUIRE_FALSE(home->is_tracing());
    REQUIRE_THROWS_AS(home->get_tracing(), std::logic_error);

}
