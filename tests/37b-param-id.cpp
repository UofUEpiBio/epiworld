#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Reading and writing parameters by position (ParamId)
 *
 * `get_param_id()` returns a position that `par_at()` and `set_param_at()`
 * use directly. Positions stay valid in copies of the model, and writes
 * through them change the simulation exactly as `set_param()` does.
 */
EPIWORLD_TEST_CASE("Parameters by position", "[parameters][ParamId]") {

    epimodels::ModelSIR<> model("a virus", 0.01, 0.9, 0.25);
    model.seed(2231);
    model.agents_smallworld(2000, 6, false, 0.1);
    model.verbose_off();

    ParamId beta = model.get_param_id("Transmission rate");
    ParamId gamma = model.get_param_id("Recovery rate");

    REQUIRE(model.get_n_params() == 2u);
    REQUIRE_THAT(model.par_at(beta), Catch::Matchers::WithinAbs(0.9, 1e-6));
    REQUIRE_THAT(model.par_at(gamma), Catch::Matchers::WithinAbs(0.25, 1e-6));

    // Writes by position and by name reach the same value
    model.set_param_at(beta, 0.1);
    REQUIRE_THAT(model.par("Transmission rate"), Catch::Matchers::WithinAbs(0.1, 1e-6));
    model.set_param("Recovery rate", 0.3);
    REQUIRE_THAT(model.par_at(gamma), Catch::Matchers::WithinAbs(0.3, 1e-6));

    // Adding a parameter keeps the existing positions
    model.add_param(0.5, "Another parameter");
    REQUIRE(model.get_param_id("Transmission rate").idx == beta.idx);
    REQUIRE(model.params().size() == 3u);

    // Positions are valid in copies
    Model<> copy(model);
    REQUIRE_THAT(copy.par_at(beta), Catch::Matchers::WithinAbs(0.1, 1e-6));
    REQUIRE(copy.get_param_layout_id() == model.get_param_layout_id());

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
        [beta](Model<> * m) -> void { m->set_param_at(beta, 0.0); },
        "stop transmission",
        stop_day
    );
    model.run(40, 55);

    std::vector< int > date, source, target, virus, source_exposure;
    model.get_db().get_transmissions(date, source, target, virus, source_exposure);

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
    epimodels::ModelSIR<> model_name("a virus", 0.01, 0.1, 0.3);
    model_name.seed(2231);
    model_name.agents_smallworld(2000, 6, false, 0.1);
    model_name.verbose_off();
    model_name.add_param(0.5, "Another parameter");
    model_name.add_globalevent(
        [](Model<> * m) -> void { m->set_param("Transmission rate", 0.0); },
        "stop transmission",
        stop_day
    );
    model_name.run(40, 55);

    std::vector< int > hist_id, hist_name;
    model.get_db().get_hist_total(nullptr, nullptr, &hist_id);
    model_name.get_db().get_hist_total(nullptr, nullptr, &hist_name);
    REQUIRE(hist_id == hist_name);

}
