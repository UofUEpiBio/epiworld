#include "tests.hpp"

using namespace epiworld;

// Exposed agents carry the virus but are not infectious yet. With an
// incubation so long that nobody leaves Exposed, a fully transmissible
// virus must not spread past the agents it started in -- in every
// transmission mode, for the SEIR and the SEIRD models.
EPIWORLD_TEST_CASE("Exposed agents do not transmit", "[seir-exposed]") {

    const int n = 1000;
    const int n_seeds = 10;

    for (const std::string mode : {"pull", "push"}) {
        for (const bool with_death : {false, true}) {

            std::unique_ptr< Model<> > model;
            if (with_death)
                model = std::make_unique< epimodels::ModelSEIRD<> >(
                    "virus", n, 0.01, 1.0, 1e9, 0.1, 0.01
                );
            else
                model = std::make_unique< epimodels::ModelSEIR<> >(
                    "virus", n, 0.01, 1.0, 1e9, 0.1
                );

            model->add_layer(layer::network(
                "network", rgraph_smallworld(n, 10, 0.1, false, *model)
            ));
            model->set_transmission_mode(mode);
            model->verbose_off();
            model->run(30, 1912);

            // Susceptible, Exposed, Infected
            CHECK(model->get_db().get_today_total("Susceptible") == n - n_seeds);
            CHECK(model->get_db().get_today_total("Exposed") == n_seeds);
            CHECK(model->get_db().get_today_total("Infected") == 0);

        }
    }

}
