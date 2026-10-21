#include "tests.hpp"

using namespace epiworld;

// A model may have several network layers: susceptible agents pull from all of
// them, carriers push along all of them, and the queue counts ties in all of
// them. An empty second layer changes nothing, draw for draw, and splitting a
// network into two layers gives the same epidemic in distribution.

namespace {

// Final number of agents ever infected, one entry per replicate
std::vector< double > attack(Model<> & model, const std::string & mode)
{

    std::vector< double > out;
    model.set_transmission_mode(mode);
    model.run_multiple(
        60, 300, 2026,
        [&out](size_t, Model<> * m) -> void {
            std::vector< int > counts;
            m->get_db().get_today_total(nullptr, &counts);
            out.push_back(static_cast< double >(counts[1] + counts[2]));
        },
        true, false, 1
    );

    return out;

}

std::pair< double, double > mean_se(const std::vector< double > & x)
{
    double m = 0.0, v = 0.0;
    for (double xi : x) m += xi;
    m /= static_cast< double >(x.size());
    for (double xi : x) v += (xi - m) * (xi - m);
    v /= static_cast< double >(x.size() - 1u);
    return {m, std::sqrt(v / static_cast< double >(x.size()))};
}

}

EPIWORLD_TEST_CASE("Network layers - several in one model", "[layer-network]") {

    const size_t n = 1000u;

    // The ties, drawn once
    epimodels::ModelSIR<> draw("flu", n, 0.0, 0.0, 0.0);
    draw.seed(46);
    std::vector< int > source, target;
    layer::Network<>("all", rgraph_smallworld(n, 6, 0.1, false, draw))
        .get_edgelist(source, target);

    auto make = [&](size_t n_layers) {

        auto model = std::make_unique< epimodels::ModelSIR<> >(
            "flu", n, 0.01, 0.1, 0.2
        );
        model->verbose_off();

        std::vector< int > s[2], t[2];
        for (size_t k = 0u; k < source.size(); ++k)
        {
            size_t l = (n_layers == 2u) ? (k % 2u) : 0u;
            s[l].push_back(source[k]);
            t[l].push_back(target[k]);
        }

        model->add_layer(std::make_unique< layer::Network<> >(
            "a", AdjList(s[0], t[0], static_cast< int >(n), false)
        ));
        model->add_layer(std::make_unique< layer::Network<> >(
            "b", AdjList(s[1], t[1], static_cast< int >(n), false)
        ));

        return model;

    };

    // -- one network plus an empty one: the same run, draw for draw --------
    for (std::string mode : {"pull", "push"})
    {
        epimodels::ModelSIR<> single("flu", n, 0.01, 0.1, 0.2);
        single.verbose_off();
        single.add_layer(std::make_unique< layer::Network<> >(
            "a", AdjList(source, target, static_cast< int >(n), false)
        ));
        single.set_transmission_mode(mode);
        single.run(60, 123);

        auto with_empty = make(1u);
        with_empty->set_transmission_mode(mode);
        with_empty->run(60, 123);

        std::vector< int > h1, h2;
        single.get_db().get_hist_total(nullptr, nullptr, &h1);
        with_empty->get_db().get_hist_total(nullptr, nullptr, &h2);
        REQUIRE(h1 == h2);

        std::vector< int > d, src, tgt, v, se;
        single.get_db().get_transmissions(d, src, tgt, v, se);
        REQUIRE(d.size() > 50u);
    }

    // -- the network split into two layers: the same in distribution -------
    // Odd and even ties go to different layers, so an agent's contacts come
    // in a different order (which changes the random stream) but are the same.
    auto one = make(1u);
    auto two = make(2u);
    REQUIRE(two->get_network_layers().size() == 2u);

    for (std::string mode : {"pull", "push"})
    {
        auto a = mean_se(attack(*one, mode));
        auto b = mean_se(attack(*two, mode));

        REQUIRE(a.first > 100.0); // the epidemic does take off
        REQUIRE(
            std::abs(a.first - b.first) <
            4.0 * std::sqrt(a.second * a.second + b.second * b.second)
        );
    }

}
