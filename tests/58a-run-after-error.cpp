#include "tests.hpp"

using namespace epiworld;

// A run that ends in an exception (here, from a global event) leaves the model
// ready for the next one: not running, so settings made between runs work, and
// the next run is the same as a run of a fresh model. (With GCC 13 at -O2 the
// "running" flag used to stay set.)

EPIWORLD_TEST_CASE("A run that throws", "[model]") {

    auto build = []() {
        auto model = std::make_unique< epimodels::ModelSIR >(
            "Virus", 1000, 0.01, 0.3, 0.1
        );
        model->verbose_off();
        model->add_param(1.0, "Weight");
        model->add_layer(layer::network("home", graph::smallworld(4, 0.1)));
        return model;
    };

    // The case that showed the bug: a global event that makes a setting
    // during the run (an error), then a setting between runs
    {
        auto m = build();
        m->add_globalevent([](Model * x) {
            x->set_param_cycle("Weight", {1.0});
        }, "Set", 3);
        REQUIRE_THROWS_WITH(
            m->run(5, 1), Catch::Matchers::Contains("between runs")
        );
        REQUIRE_THROWS_AS(
            m->set_param_cycle("Weight", {}), std::length_error
        );
        REQUIRE_FALSE(m->is_running());
    }

    auto model = build();
    model->add_globalevent(
        [](Model *) { throw std::runtime_error("boom"); }, "Throw", 3
    );
    REQUIRE_THROWS_WITH(model->run(10, 1), Catch::Matchers::Contains("boom"));
    REQUIRE_FALSE(model->is_running());

    // Settings made between runs
    model->rm_globalevent("Throw");
    REQUIRE_NOTHROW(model->set_param_cycle("Weight", {1.0, 0.5}));
    model->clear_param_cycle("Weight");
    // (the layers too: a function schedule is refused during a run)
    REQUIRE_NOTHROW(model->get_layer("home").set_schedule(
        std::function< bool(int) >([](int d) { return d % 2 == 1; })
    ));
    model->get_layer("home").clear_schedule();

    // The next run is the run of a fresh model
    model->run(20, 5);
    auto fresh = build();
    fresh->run(20, 5);

    std::vector< int > d1, s1, t1, v1, e1, l1, d2, s2, t2, v2, e2, l2;
    model->get_db().get_transmissions(d1, s1, t1, v1, e1, l1);
    fresh->get_db().get_transmissions(d2, s2, t2, v2, e2, l2);
    REQUIRE(d1.size() > 10u);
    REQUIRE(d1 == d2);
    REQUIRE(s1 == s2);
    REQUIRE(t1 == t2);

}
