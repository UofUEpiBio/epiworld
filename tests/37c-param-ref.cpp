#include "tests.hpp"

using namespace epiworld;

// Builds an SEIR-like model whose parameters are added in `order`, so the same
// names sit at different positions in different models.
static void build_model(
    Model<> & model,
    const std::vector< std::string > & order,
    Virus<> & virus
) {

    std::map< std::string, epiworld_double > values = {
        {"beta", 0.2}, {"incubation", 0.25}, {"gamma", 0.15}, {"padding", 0.99}
    };
    for (auto & name : order)
        model.add_param(values[name], name);

    model.add_virus(virus, assign::share(0.01));

    model.seed(8812);
    model.add_layer(layer::network(
        "network", rgraph_smallworld(2000, 8, 0.1, false, model)
    ));
    model.verbose_off();

}

/**
 * @brief One ParamRef used by models with different parameter layouts
 *
 * The same virus (its transitions read "beta", "incubation" and "gamma")
 * runs in models whose parameters were added in different orders. Each model must read its own values, even when runs
 * alternate between models and after a model adds a parameter.
 */
EPIWORLD_TEST_CASE("ParamRef across models", "[parameters][ParamRef]") {

    Virus<> virus("a virus");
    virus.add_stage("Susceptible");
    virus.add_stage("Exposed", stage::carrier);
    virus.add_stage("Infected", stage::infectious);
    virus.add_stage("Recovered");
    virus.infection(0, 1, "beta");
    virus.rate(1, 2, "incubation");
    virus.rate(2, 3, "gamma");

    Model<> model_a(2000);
    Model<> model_b(2000);
    build_model(
        model_a, {"beta", "incubation", "gamma", "padding"}, virus
    );
    build_model(
        model_b, {"padding", "gamma", "incubation", "beta"}, virus
    );

    REQUIRE(model_a.get_param_id("beta").idx != model_b.get_param_id("beta").idx);
    REQUIRE(model_a.get_param_layout_id() != model_b.get_param_layout_id());

    std::vector< int > hist_a, hist_b, hist_a2, hist_b2;

    model_a.run(60, 331);
    model_a.get_db().get_hist_total(nullptr, nullptr, nullptr, &hist_a);

    model_b.run(60, 331);
    model_b.get_db().get_hist_total(nullptr, nullptr, nullptr, &hist_b);

    // Same values, so the same simulation
    REQUIRE(hist_a == hist_b);

    // Alternating back, and after adding a parameter (a new layout)
    model_a.add_param(0.0, "unused");
    model_a.run(60, 331);
    model_a.get_db().get_hist_total(nullptr, nullptr, nullptr, &hist_a2);
    REQUIRE(hist_a2 == hist_a);

    model_b.run(60, 331);
    model_b.get_db().get_hist_total(nullptr, nullptr, nullptr, &hist_b2);
    REQUIRE(hist_b2 == hist_b);

    std::vector< int > today;
    model_a.get_db().get_today_total(nullptr, nullptr, &today);
    REQUIRE(today[3u] > 0); // Some agents recovered

    // A change made by name is seen through the cached positions
    model_b.set_param("beta", 0.0);
    model_b.run(60, 331);
    std::vector< int > date, source, target, v, se, se_layer;
    model_b.get_db().get_transmissions(date, source, target, v, se, se_layer);
    int n_transmissions = 0;
    for (auto s : source)
        if (s >= 0)
            ++n_transmissions;
    REQUIRE(n_transmissions == 0);

    // A ParamRef on its own
    ParamRef gamma("gamma");
    REQUIRE_THAT(gamma(model_a), Catch::Matchers::WithinAbs(0.15, 1e-6));
    REQUIRE_THAT(gamma(model_b), Catch::Matchers::WithinAbs(0.15, 1e-6));
    REQUIRE(gamma.id(model_a).idx == model_a.get_param_id("gamma").idx);
    REQUIRE(gamma.id(model_b).idx == model_b.get_param_id("gamma").idx);
    REQUIRE_THROWS_WITH(
        ParamRef("Not a parameter")(model_a),
        Catch::Matchers::Contains("'Not a parameter'")
    );

}
