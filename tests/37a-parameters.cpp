#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Parameter lookups accept any string-like name
 *
 * `par()`, `get_param()`, `set_param()`, `has_param()`, and `operator()` take
 * the name as a `std::string_view`, so string literals, `std::string`, and
 * `std::string_view` all work, including names longer than the short-string
 * buffer. Unknown names still throw, naming the parameter.
 */

/**
 * @brief Reading and writing parameters by position (ParamId)
 *
 * `get_param_id()` returns a position that `par_at()` and `set_param_at()`
 * use directly. Positions stay valid in copies of the model, and writes
 * through them change the simulation exactly as `set_param()` does.
 */

// Builds an SEIR-like model whose parameters are added in `order`, so the same
// names sit at different positions in different models.
static void build_model(
    Model & model,
    const std::vector< std::string > & order,
    Virus & virus
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

// Parameters: lookup by name, position and reference
//
// One test case; each former test file is a block below, with the name it had.
EPIWORLD_TEST_CASE("Parameters: lookup by name; position and reference", "[parameters][ParamId][ParamRef]") {

    // ---- 37a-parameter-lookup.cpp: Parameter lookup by name
    {

    epimodels::ModelSIR model("a virus", 100, 0.01, 0.5, 0.25);

    const std::string long_name = "Transmission probability"; // 24 characters
    const std::string_view long_view = long_name;

    REQUIRE_THAT(model.par("Transmission probability"), Catch::Matchers::WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(model.par(long_name), Catch::Matchers::WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(model.par(long_view), Catch::Matchers::WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(model.get_param(long_view), Catch::Matchers::WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(model("Recovery rate"), Catch::Matchers::WithinAbs(0.25, 1e-6));

    const Model & cmodel = model;
    REQUIRE_THAT(cmodel.get_param("Recovery rate"), Catch::Matchers::WithinAbs(0.25, 1e-6));
    REQUIRE_THAT(cmodel("Recovery rate"), Catch::Matchers::WithinAbs(0.25, 1e-6));

    REQUIRE(model.has_param(long_view));
    REQUIRE_FALSE(model.has_param("Transmission"));

    model.set_param(long_view, 0.9);
    REQUIRE_THAT(model.par(long_name), Catch::Matchers::WithinAbs(0.9, 1e-6));

    // Overwriting through add_param updates the same entry
    model.add_param(0.1, long_name, true);
    REQUIRE_THAT(model.par(long_view), Catch::Matchers::WithinAbs(0.1, 1e-6));
    REQUIRE(model.params().size() == 2u);

    REQUIRE_THROWS_WITH(
        model.par("Not a parameter"),
        Catch::Matchers::Contains("'Not a parameter'")
    );
    REQUIRE_THROWS_WITH(
        model.get_param("Not a parameter"),
        Catch::Matchers::Contains("Not a parameter")
    );
    REQUIRE_THROWS_WITH(
        model.set_param("Not a parameter", 1.0),
        Catch::Matchers::Contains("'Not a parameter'")
    );
    REQUIRE_THROWS_WITH(
        model("Not a parameter"),
        Catch::Matchers::Contains("'Not a parameter'")
    );

    // Results are the same whichever way the parameters were set
    epimodels::ModelSIR model_a("a virus", 2000, 0.01, 0.1, 0.25);
    epimodels::ModelSIR model_b("a virus", 2000, 0.01, 0.9, 0.9);
    model_b.set_param(long_view, 0.1);
    model_b.set_param("Recovery rate", 0.25);

    model_a.add_layer(layer::network(
        "network", rgraph_smallworld(2000, 6, 0.1, false, model_a)
    ));
    model_b.add_layer(layer::network(
        "network", rgraph_smallworld(2000, 6, 0.1, false, model_b)
    ));
    model_a.verbose_off();
    model_b.verbose_off();
    model_a.run(60, 123);
    model_b.run(60, 123);

    std::vector< int > hist_a, hist_b;
    model_a.get_db().get_hist_total(nullptr, nullptr, nullptr, &hist_a);
    model_b.get_db().get_hist_total(nullptr, nullptr, nullptr, &hist_b);
    REQUIRE(hist_a == hist_b);

    std::vector< int > today;
    model_a.get_db().get_today_total(nullptr, nullptr, &today);
    REQUIRE(today[0u] < 2000); // Something happened
    }

    // ---- 37b-param-id.cpp: Parameters by position
    {

    epimodels::ModelSIR model("a virus", 2000, 0.01, 0.9, 0.25);
    model.seed(2231);
    model.add_layer(layer::network(
        "network", rgraph_smallworld(2000, 6, 0.1, false, model)
    ));
    model.verbose_off();

    ParamId beta = model.get_param_id("Transmission probability");
    ParamId gamma = model.get_param_id("Recovery rate");

    REQUIRE(model.get_n_params() == 2u);
    REQUIRE_THAT(model.par_at(beta), Catch::Matchers::WithinAbs(0.9, 1e-6));
    REQUIRE_THAT(model.par_at(gamma), Catch::Matchers::WithinAbs(0.25, 1e-6));

    // Writes by position and by name reach the same value
    model.set_param_at(beta, 0.1);
    REQUIRE_THAT(model.par("Transmission probability"), Catch::Matchers::WithinAbs(0.1, 1e-6));
    model.set_param("Recovery rate", 0.3);
    REQUIRE_THAT(model.par_at(gamma), Catch::Matchers::WithinAbs(0.3, 1e-6));

    // Adding a parameter keeps the existing positions
    model.add_param(0.5, "Another parameter");
    REQUIRE(model.get_param_id("Transmission probability").idx == beta.idx);
    REQUIRE(model.params().size() == 3u);

    // Positions are valid in copies
    Model copy(model);
    REQUIRE_THAT(copy.par_at(beta), Catch::Matchers::WithinAbs(0.1, 1e-6));
    REQUIRE(copy.get_param_layout_id() == model.get_param_layout_id());

    // Equality compares parameters by name, not by position
    Model order_1(0), order_2(0);
    order_1.add_param(0.1, "a");
    order_1.add_param(0.2, "b");
    order_2.add_param(0.2, "b");
    order_2.add_param(0.1, "a");
    REQUIRE(order_1 == order_2);
    order_2.set_param("a", 0.3);
    #ifndef EPI_DEBUG
    // (With EPI_DEBUG, a mismatch throws, naming what differs)
    REQUIRE_FALSE(order_1 == order_2);
    #else
    REQUIRE_THROWS(order_1 == order_2);
    #endif

    REQUIRE_THROWS_AS(model.par_at(ParamId{99u}), std::out_of_range);
    REQUIRE_THROWS_AS(model.set_param_at(ParamId{99u}, 1.0), std::out_of_range);
    REQUIRE_THROWS_WITH(
        model.get_param_id("Not a parameter"),
        Catch::Matchers::Contains("'Not a parameter'")
    );

    // End to end: stopping transmission mid-run through set_param_at
    // leaves no transmissions after that day
    const int stop_day = 10;
    model.add_globalevent(
        [beta](Model * m) -> void { m->set_param_at(beta, 0.0); },
        "stop transmission",
        stop_day
    );
    model.run(40, 55);

    std::vector< int > date, source, target, virus, source_exposure, source_exposure_layer;
    model.get_db().get_transmissions(date, source, target, virus, source_exposure, source_exposure_layer);

    int before = 0, after = 0;
    for (auto d : date)
    {
        if (d <= stop_day)
            ++before;
        else
            ++after;
    }

    REQUIRE(before > 0);
    REQUIRE(after == 0);

    // The same run with the change made by name gives the same history
    epimodels::ModelSIR model_name("a virus", 2000, 0.01, 0.1, 0.3);
    model_name.seed(2231);
    model_name.add_layer(layer::network(
        "network", rgraph_smallworld(2000, 6, 0.1, false, model_name)
    ));
    model_name.verbose_off();
    model_name.add_param(0.5, "Another parameter");
    model_name.add_globalevent(
        [](Model * m) -> void { m->set_param("Transmission probability", 0.0); },
        "stop transmission",
        stop_day
    );
    model_name.run(40, 55);

    std::vector< int > hist_id, hist_name;
    model.get_db().get_hist_total(nullptr, nullptr, nullptr, &hist_id);
    model_name.get_db().get_hist_total(nullptr, nullptr, nullptr, &hist_name);
    REQUIRE(hist_id == hist_name);
    }

    // ---- 37c-param-ref.cpp: ParamRef across models
    {

    Virus virus("a virus");
    virus.add_stage("Susceptible");
    virus.add_stage("Exposed", stage::carrier);
    virus.add_stage("Infected", stage::infectious);
    virus.add_stage("Recovered");
    virus.infection(0, 1, "beta");
    virus.rate(1, 2, "incubation");
    virus.rate(2, 3, "gamma");

    Model model_a(2000);
    Model model_b(2000);
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

}
