#include "tests.hpp"

using namespace epiworld;

// Builds an SEIR-like model whose parameters are added in `order`, so the same
// names sit at different positions in different models.
static void build_model(
    Model<> & model,
    const std::vector< std::string > & order,
    Virus<> & virus,
    const UpdateFun<> & update_exposed,
    const UpdateFun<> & update_infected
) {

    std::map< std::string, epiworld_double > values = {
        {"beta", 0.2}, {"incubation", 0.25}, {"gamma", 0.15}, {"padding", 0.99}
    };
    for (auto & name : order)
        model.add_param(values[name], name);

    model.add_state("Susceptible", default_update_susceptible<>);
    model.add_state("Exposed", update_exposed);
    model.add_state("Infected", update_infected);
    model.add_state("Recovered");

    model.add_virus(virus);

    model.seed(8812);
    model.agents_smallworld(2000, 8, false, 0.1);
    model.verbose_off();

}

/**
 * @brief One ParamRef used by models with different parameter layouts
 *
 * The same virus (set_prob_infecting("beta")) and the same update functions
 * (new_state_update_transition) run in models whose parameters were added
 * in different orders. Each model must read its own values, even when runs
 * alternate between models and after a model adds a parameter.
 */
EPIWORLD_TEST_CASE("ParamRef across models", "[parameters][ParamRef]") {

    Virus<> virus("a virus", 0.01, true);
    virus.set_state(1, 3, 3);
    virus.set_prob_infecting("beta");
    virus.set_prob_recovery("gamma");

    auto update_exposed = new_state_update_transition<>({"incubation"}, {2u});
    auto update_infected = new_state_update_transition<>({"gamma"}, {3u});

    Model<> model_a;
    Model<> model_b;
    build_model(
        model_a, {"beta", "incubation", "gamma", "padding"},
        virus, update_exposed, update_infected
    );
    build_model(
        model_b, {"padding", "gamma", "incubation", "beta"},
        virus, update_exposed, update_infected
    );

    REQUIRE(model_a.get_param_id("beta").idx != model_b.get_param_id("beta").idx);
    REQUIRE(model_a.get_param_layout_id() != model_b.get_param_layout_id());

    std::vector< int > hist_a, hist_b, hist_a2, hist_b2;

    model_a.run(60, 331);
    model_a.get_db().get_hist_total(nullptr, nullptr, &hist_a);

    model_b.run(60, 331);
    model_b.get_db().get_hist_total(nullptr, nullptr, &hist_b);

    // Same values, so the same simulation
    REQUIRE(hist_a == hist_b);

    // Alternating back, and after adding a parameter (a new layout)
    model_a.add_param(0.0, "unused");
    model_a.run(60, 331);
    model_a.get_db().get_hist_total(nullptr, nullptr, &hist_a2);
    REQUIRE(hist_a2 == hist_a);

    model_b.run(60, 331);
    model_b.get_db().get_hist_total(nullptr, nullptr, &hist_b2);
    REQUIRE(hist_b2 == hist_b);

    std::vector< int > today;
    model_a.get_db().get_today_total(nullptr, &today);
    REQUIRE(today[3u] > 0); // Some agents recovered

    // A change made by name is seen through the cached positions
    model_b.set_param("beta", 0.0);
    model_b.run(60, 331);
    std::vector< int > date, source, target, v, se;
    model_b.get_db().get_transmissions(date, source, target, v, se);
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
