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
EPIWORLD_TEST_CASE("Parameter lookup by name", "[parameters]") {

    epimodels::ModelSIR<> model("a virus", 0.01, 0.5, 0.25);

    const std::string long_name = "Transmission rate"; // 17 characters
    const std::string_view long_view = long_name;

    REQUIRE_THAT(model.par("Transmission rate"), Catch::Matchers::WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(model.par(long_name), Catch::Matchers::WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(model.par(long_view), Catch::Matchers::WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(model.get_param(long_view), Catch::Matchers::WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(model("Recovery rate"), Catch::Matchers::WithinAbs(0.25, 1e-6));

    const Model<> & cmodel = model;
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
    epimodels::ModelSIR<> model_a("a virus", 0.01, 0.1, 0.25);
    epimodels::ModelSIR<> model_b("a virus", 0.01, 0.9, 0.9);
    model_b.set_param(long_view, 0.1);
    model_b.set_param("Recovery rate", 0.25);

    model_a.agents_smallworld(2000, 6, false, 0.1);
    model_b.agents_smallworld(2000, 6, false, 0.1);
    model_a.verbose_off();
    model_b.verbose_off();
    model_a.run(60, 123);
    model_b.run(60, 123);

    std::vector< int > hist_a, hist_b;
    model_a.get_db().get_hist_total(nullptr, nullptr, &hist_a);
    model_b.get_db().get_hist_total(nullptr, nullptr, &hist_b);
    REQUIRE(hist_a == hist_b);

    std::vector< int > today;
    model_a.get_db().get_today_total(nullptr, &today);
    REQUIRE(today[0u] < 2000); // Something happened

}
