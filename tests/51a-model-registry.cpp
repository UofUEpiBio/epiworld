#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

// Every registered model, core and measles: built from the registry with a
// value of its own for every argument, it carries those values as its
// parameters, its virus has a natural history, and it runs on the layer a
// user would add. A strain of the model's virus spreads through the same
// stages as the model's own.
EPIWORLD_TEST_CASE("Model registry - every model", "[registry]") {

    ModelRegistry<> registry;
    epimodels::register_models(registry);
    measles::register_models(registry);

    std::vector< std::string > ids;
    for (const auto & info : registry.list_models())
        ids.push_back(info.id);

    REQUIRE(ids == std::vector< std::string >{
        "SIS", "SIR", "SEIR", "SISD", "SIRD", "SEIRD", "SEIRQuarantine",
        "SURV", "MeaslesMixing", "MeaslesMixingRiskQuarantine", "MeaslesSchool"
    });

    for (const auto & info : registry.list_models())
    {
        INFO("model " << info.id);

        // A value of its own for every argument, near its default and in
        // its limits: a builder that passed two arguments in the wrong order
        // would show it in the parameters
        std::map< std::string, double > values;
        for (size_t k = 0u; k < info.params.size(); ++k)
        {
            const auto & p = info.params[k];
            const double step = p.integer ? 1.0 : 1e-3 * static_cast< double >(k + 1u);
            values[p.arg] = (p.value + step <= p.max) ? p.value + step : p.value - step;
        }

        auto model = registry.make_model(info.id, values);
        model->verbose_off();

        REQUIRE(model->size() == static_cast< size_t >(values["n"]));
        for (const auto & p : info.params)
        {
            INFO("argument " << p.arg);
            if (!p.name.empty())
                REQUIRE(model->get_param(p.name) == values[p.arg]);
        }

        REQUIRE(model->get_virus(0u).get_natural_history() != nullptr);

        if (!info.virus_name.empty())
            REQUIRE(
                registry.make_model(info.id, {}, "flu")->get_virus(0u).get_name()
                == "flu"
            );

        // The layer a user would add: the measles mixing models need a mixing
        // layer, the school mixes on its own, and the rest take a network
        if (info.id.find("MeaslesMixing") == 0u)
        {
            auto mix = layer::mixing<>("mixing");
            mix->set_groups(assign::labels<>(std::vector< size_t >(model->size(), 0u)), 1u);
            mix->set_contact_matrix({10.0});
            model->add_layer(mix);
        }
        else if (info.id != "MeaslesSchool")
            model->add_layer(layer::network("network", graph::smallworld<>(6, 0.1)));

        REQUIRE_NOTHROW(model->run(20, 123));
    }

    // The registry builds the model the constructor builds: same seed, same
    // epidemic
    auto from_registry = registry.make_model(
        "SEIR", {{"n", 2000}, {"transmission_prob", 0.2}}, "flu"
    );
    epimodels::ModelSEIR<> direct("flu", 2000, 0.01, 0.2, 7.0, 0.14);
    for (Model<> * m : {from_registry.get(), static_cast< Model<> * >(&direct)})
    {
        m->verbose_off();
        m->add_layer(layer::network("network", graph::smallworld<>(6, 0.1)));
        m->run(60, 42);
    }

    std::vector< int > d1, d2, n1, n2;
    std::vector< std::string > k1, k2, s1, s2;
    from_registry->get_db().get_hist_total(&d1, &k1, &s1, &n1);
    direct.get_db().get_hist_total(&d2, &k2, &s2, &n2);
    REQUIRE(n1 == n2);

    // A strain of the model's virus: its first cases are exposed, as with the
    // model's own virus, and it spreads
    auto model = registry.make_model("SEIR", {{"n", 2000}}, "flu");
    model->verbose_off();
    model->add_layer(layer::network("network", graph::smallworld<>(6, 0.1)));

    Virus<> other = model->get_virus(0u);
    other.set_name("other");
    model->add_virus(other, assign::count<>(20u));

    // Seeded at reset (a run of 0 days)
    model->run(0, 7);
    const int exposed = epimodels::ModelSEIR<>::EXPOSED;
    size_t seeded = 0u;
    for (const auto & a : model->get_agents())
        if ((a.get_virus() != nullptr) && (a.get_virus()->get_name() == "other"))
        {
            ++seeded;
            REQUIRE(model->get_stage(a) == exposed);
        }
    REQUIRE(seeded == 20u);

    model->run(60, 7);

    std::vector< int > date, source, target, virus, source_exposure_date, layer;
    model->get_db().get_transmissions(
        date, source, target, virus, source_exposure_date, layer
    );
    REQUIRE(std::count(virus.begin(), virus.end(), 1) > 20);

}
