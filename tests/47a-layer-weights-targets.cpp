#include "tests.hpp"

using namespace epiworld;

// Several layers in one model draw one infection from all their contacts, and
// each transmission records the layer it came through. A layer's weight scales
// the odds of infection through it, a layer can be restricted to some viruses,
// and a mixing layer works next to a network (the model then pulls).

namespace {

// An SIR-like model over `n` agents with default (all-layer) pulling
Model<> make_model(size_t n)
{
    Model<> m(n);
    m.add_state("Susceptible", default_update_susceptible<>);
    m.add_state("Infected", default_update_exposed<>);
    m.add_state("Recovered");
    m.verbose_off();
    return m;
}

Virus<> make_virus(std::string name, double p_inf, double p_rec = 0.0)
{
    Virus<> v(name, 0.0, true);
    v.set_state(1, 2, 2);
    v.set_prob_infecting(p_inf);
    v.set_prob_recovery(p_rec);
    return v;
}

struct Transmissions {
    std::vector< int > date, source, target, virus, exposure, layer;
};

Transmissions transmissions(Model<> & m)
{
    Transmissions t;
    m.get_db().get_transmissions(
        t.date, t.source, t.target, t.virus, t.exposure, t.layer
    );
    return t;
}

}

EPIWORLD_TEST_CASE("Layers - weights, virus targets and layer ids", "[layer-weights]") {

    // -- weights scale the odds -----------------------------------------------
    // Pairs (2k, 2k+1); the even agent carries the virus, and the odd one
    // meets it once on day 0 with probability p = 0.2 through the layer. A
    // weight w turns that into w p / (1 - p + w p).
    {
        const size_t pairs = 20000u;
        const size_t n = 2u * pairs;
        const double p = 0.2;

        std::vector< int > source, target;
        std::vector< size_t > evens;
        for (size_t k = 0u; k < pairs; ++k)
        {
            source.push_back(static_cast< int >(2u * k));
            target.push_back(static_cast< int >(2u * k + 1u));
            evens.push_back(2u * k);
        }
        AdjList graph(source, target, static_cast< int >(n), false);

        auto infected_odds = [&](const char * param, double w) -> double {

            Model<> m = make_model(n);
            Virus<> v = make_virus("flu", p);
            v.set_distribution(distribute_virus_to_set<>(evens));
            m.add_virus(v);
            m.add_param(w, "Weight");
            auto net = layer::network("pairs", graph);
            if (param != nullptr)
                net->set_weight(param);
            m.add_layer(net);
            m.run(1, 2026);

            return (
                static_cast< double >(m.get_db().get_today_total("Infected")) -
                static_cast< double >(pairs)
            ) / static_cast< double >(pairs);

        };

        auto expect = [&](double w) -> double { return w * p / (1.0 - p + w * p); };
        auto se = [&](double q) -> double {
            return std::sqrt(q * (1.0 - q) / static_cast< double >(pairs));
        };

        const double none = infected_odds(nullptr, 1.0);
        REQUIRE(std::abs(none - p) < 4.0 * se(p));

        for (double w : {0.5, 3.0})
        {
            const double got = infected_odds("Weight", w);
            INFO("weight " << w);
            REQUIRE(std::abs(got - expect(w)) < 4.0 * se(expect(w)));
        }

        // Weight 0 switches the layer off; weight 1 changes nothing
        REQUIRE(infected_odds("Weight", 0.0) == 0.0);
        REQUIRE(infected_odds("Weight", 1.0) == none);
    }

    // -- virus targets and layer ids ------------------------------------------
    // Two layers over the same agents; flu A spreads through layer 0 only and
    // flu B through layer 1 only. Every transmission names the layer it used,
    // and the seeds belong to none.
    {
        const size_t n = 2000u;
        Model<> m = make_model(n);

        Virus<> a = make_virus("flu A", 0.2, 0.1);
        a.set_distribution(distribute_virus_randomly<>(0.01));
        Virus<> b = make_virus("flu B", 0.2, 0.1);
        b.set_distribution(distribute_virus_randomly<>(0.01));
        m.add_virus(a);
        m.add_virus(b);
        m.seed(77);

        auto net_a = layer::network("a", rgraph_smallworld(n, 6, 0.1, false, m));
        auto net_b = layer::network("b", rgraph_smallworld(n, 6, 0.1, false, m));
        net_a->add_virus_target(m.get_virus(0));
        net_b->add_virus_target(m.get_virus(1));
        m.add_layer(net_a);
        m.add_layer(net_b);

        m.run(60, 3);
        auto t = transmissions(m);

        size_t seeds = 0u, through[2][2] = {{0u, 0u}, {0u, 0u}};
        for (size_t i = 0u; i < t.date.size(); ++i)
        {
            if (t.source[i] < 0)
            {
                REQUIRE(t.layer[i] == -1);
                ++seeds;
                continue;
            }
            REQUIRE(t.layer[i] >= 0);
            REQUIRE(t.layer[i] <= 1);
            ++through[t.virus[i]][t.layer[i]];
        }

        REQUIRE(seeds > 0u);
        REQUIRE(through[0][0] > 50u);
        REQUIRE(through[1][1] > 50u);
        REQUIRE(through[0][1] == 0u);
        REQUIRE(through[1][0] == 0u);

        // A virus the model does not have cannot be a target
        Virus<> loose = make_virus("loose", 0.1);
        REQUIRE_THROWS_AS(net_a->add_virus_target(loose), std::logic_error);
    }

    // -- a mixing layer next to a network -------------------------------------
    // Both layers transmit, with either transmission mode requested; the
    // mixing layer needs the model to visit every agent, so it pulls.
    {
        const size_t n = 3000u;

        for (std::string mode : {"pull", "push"})
        {

            Model<> m = make_model(n);
            Virus<> v = make_virus("flu", 0.05, 0.1);
            v.set_distribution(distribute_virus_randomly<>(0.01));
            m.add_virus(v);
            m.seed(5);

            m.add_layer(layer::network(
                "ties", rgraph_smallworld(n, 4, 0.1, false, m)
            ));
            auto mixing = layer::mixing("crowd", {1}, {0, 1, 2});
            mixing->set_groups(std::vector< size_t >(n, 0u), 1u);
            mixing->set_contact_matrix({2.0});
            m.add_layer(mixing);

            m.set_transmission_mode(mode);
            m.run(40, 9);
            REQUIRE(m.get_last_transmission_mode() == TransmissionMode::pull);

            auto t = transmissions(m);
            size_t from[2] = {0u, 0u};
            for (size_t i = 0u; i < t.date.size(); ++i)
                if (t.source[i] >= 0)
                {
                    REQUIRE(t.layer[i] >= 0);
                    REQUIRE(t.layer[i] <= 1);
                    ++from[t.layer[i]];
                }

            INFO("mode " << mode);
            REQUIRE(from[0] > 50u);
            REQUIRE(from[1] > 50u);
        }
    }

}
