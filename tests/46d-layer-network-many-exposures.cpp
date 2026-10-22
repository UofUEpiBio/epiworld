#include "tests.hpp"
#include <set>

using namespace epiworld;

// An agent's exposures add up across its network layers, with no ceiling: here
// agent 0 is the hub of three star layers whose 1,000 leaves are all infected,
// so it is exposed 3,000 times a day -- more than the 2,048 slots the sampler's
// scratch arrays used to have. Under roulette its chance of infection in a day
// is R / (1 + R), with R the sum of the odds p / (1 - p) of its contacts, and
// every contact is equally likely to be the infector. With p = 0.5 the product
// of (1 - p) underflows, which is what roulette used to normalize by: pulling
// then always picked the last contact.
EPIWORLD_TEST_CASE("Network layers - many exposures", "[layer-network]") {

    const size_t n = 1001u;
    const size_t nsims = 400u;

    std::vector< int > source(n - 1u, 0), target;
    std::vector< size_t > leaves;
    for (size_t i = 1u; i < n; ++i)
    {
        target.push_back(static_cast< int >(i));
        leaves.push_back(i);
    }

    auto make = [&](double p) {

        auto model = std::make_unique< epimodels::ModelSIR<> >(
            "flu", n, 0.0, p, 0.0
        );
        model->verbose_off();
        model->get_virus(0).set_distribution(distribute_virus_to_set<>(leaves));
        for (std::string name : {"a", "b", "c"})
            model->add_layer(layer::network(
                name, AdjList(source, target, static_cast< int >(n), false)
            ));

        return model;

    };

    for (std::string mode : {"pull", "push"})
    {

        // -- how often the hub is infected ---------------------------------
        {
            const double p = 0.0005;
            const double odds = 3.0 * (n - 1u) * p / (1.0 - p);
            const double expected = odds / (1.0 + odds);

            auto model = make(p);
            model->set_transmission_mode(mode);

            size_t infected = 0u;
            model->run_multiple(
                1, nsims, 46,
                [&infected](size_t, Model<> * m) -> void {
                    if (m->get_agent(0u).get_virus() != nullptr)
                        ++infected;
                },
                true, false, 1
            );

            double share = static_cast< double >(infected) / nsims;
            double se    = std::sqrt(expected * (1.0 - expected) / nsims);
            REQUIRE(std::abs(share - expected) < 4.0 * se);
        }

        // -- who infects it, and through which layer -----------------------
        {
            auto model = make(0.5);
            model->set_transmission_mode(mode);

            std::set< int > infectors;
            std::vector< size_t > by_layer(3u, 0u);
            model->run_multiple(
                1, nsims, 46,
                [&](size_t, Model<> * m) -> void {
                    std::vector< int > d, src, tgt, v, se, lay;
                    m->get_db().get_transmissions(d, src, tgt, v, se, lay);
                    for (size_t k = 0u; k < tgt.size(); ++k)
                        if ((tgt[k] == 0) && (src[k] >= 0))
                        {
                            infectors.insert(src[k]);
                            ++by_layer[static_cast< size_t >(lay[k])];
                        }
                },
                true, false, 1
            );

            // R = 3,000, so the hub is infected in (almost) every replicate.
            // 400 uniform draws from 1,000 leaves give ~330 distinct ones.
            REQUIRE(by_layer[0] + by_layer[1] + by_layer[2] == nsims);
            REQUIRE(infectors.size() > 250u);

            const double third = nsims / 3.0;
            const double se    = std::sqrt(nsims * (1.0 / 3.0) * (2.0 / 3.0));
            for (size_t l = 0u; l < 3u; ++l)
                REQUIRE(std::abs(by_layer[l] - third) < 4.0 * se);
        }

    }

}
