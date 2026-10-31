#include "tests.hpp"

using namespace epiworld;

// A run depends only on its seed: the distributions that keep state between
// calls (the normal draws are made in pairs) are reset when the model is
// seeded. A global event draws one normal per run, which leaves a spare
// normal behind; the same seed must give the same draw after a run with
// another seed. And the binomial set with n = 0 or p = 0 draws 0 (its
// Poisson shortcut needs a positive mean).
EPIWORLD_TEST_CASE(
    "Random numbers: seeding resets the distributions",
    "[RngSeedReset]"
) {

    size_t n = 1000u;
    epimodels::ModelSIR<> model("flu", n, 0.01, 0.5, 0.2);
    model.add_layer(layer::network(
        "network", rgraph_smallworld(n, 5, 0.01, false, model)
    ));
    model.verbose_off();

    std::vector< double > draws;
    model.add_globalevent(
        [&draws](Model<> * m) -> void { draws.push_back(m->rnorm()); },
        "One normal draw",
        1
    );

    // Without the reset, the second run's draw would be the spare normal
    // the first run left behind
    model.run(5, 123);
    model.run(5, 123);
    model.run(5, 456);
    model.run(5, 123);

    REQUIRE(draws.size() == 4u);
    REQUIRE(draws[1] == draws[0]);
    REQUIRE(draws[3] == draws[0]);
    REQUIRE(draws[2] != draws[0]);

    model.set_rand_binom(0, 0.005);
    for (int i = 0; i < 10; ++i)
        REQUIRE(model.rbinom() == 0);

    model.set_rand_binom(100, 0.0);
    for (int i = 0; i < 10; ++i)
        REQUIRE(model.rbinom() == 0);

}
