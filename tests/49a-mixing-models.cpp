#include "tests.hpp"

using namespace epiworld;

// ModelSIR with a mixing layer is the model ModelSIRMixing was (removed in
// 1.0.0). Until Phase 6 it gave the same runs, draw for draw, as that model
// (recorded with epiworld 1ab03e2); since then exposures are independent
// rather than combined by the roulette, so the history below was recorded
// again at Phase 6 (1,000 agents in two groups, seed 1912) with libstdc++. The binomial and Poisson
// draws come from the standard library, whose algorithms differ between
// implementations, so elsewhere the test only checks the counts add up.
EPIWORLD_TEST_CASE("A model plus a mixing layer is the old mixing model", "[mixing-models]") {

    const size_t n = 1000u;
    std::vector< size_t > groups(n);
    for (size_t i = 0u; i < n; ++i)
        groups[i] = i % 2u;

    auto add_mixing = [&groups](Model & model) {
        auto mixing = layer::mixing("mixing");
        mixing->set_groups(assign::labels(groups), 2u);
        mixing->set_contact_matrix({6.0, 1.0, 1.0, 4.0});
        model.add_layer(mixing);
    };

    // The old mixing models pulled (since Phase 5, "auto" may push a mixing
    // layer: same distribution, another random stream)
    auto history = [](Model & model) {
        model.verbose_off();
        model.set_transmission_mode("pull");
        model.run(20, 1912);
        std::vector< int > counts;
        model.get_db().get_hist_total(nullptr, nullptr, nullptr, &counts);
        return counts;
    };

    auto check = [n](
        const std::vector< int > & got,
        const std::vector< int > & expected,
        size_t n_states
    ) {
        #ifdef __GLIBCXX__
        REQUIRE_THAT(got, Catch::Equals(expected));
        #else
        REQUIRE(got.size() == expected.size());
        for (size_t d = 0u; d < got.size(); d += n_states)
            REQUIRE(std::accumulate(
                got.begin() + d, got.begin() + d + n_states, 0
            ) == static_cast< int >(n));
        #endif
    };

    epimodels::ModelSIR sir("v", n, 0.02, 0.1, 1.0 / 7.0);
    add_mixing(sir);
    const std::vector< int > sir_expected = {
        980, 20, 0, 966, 32, 2, 945, 48, 7, 917, 72, 11, 875, 103, 22,
        822, 142, 36, 755, 192, 53, 674, 247, 79, 580, 313, 107, 462, 388, 150,
        368, 425, 207, 296, 437, 267, 236, 429, 335, 186, 422, 392, 150, 391, 459,
        129, 351, 520, 105, 329, 566, 83, 297, 620, 72, 266, 662, 62, 238, 700,
        59, 205, 736
    };
    check(history(sir), sir_expected, 3u);

    // ModelSEIR draws its latent period when an agent enters it (a natural
    // history), so its draws differ from the old model's: only the counts
    // must add up
    epimodels::ModelSEIR seir("v", n, 0.02, 0.1, 3.0, 1.0 / 7.0);
    add_mixing(seir);
    const auto seir_got = history(seir);
    REQUIRE(seir_got.size() == 21u * 4u);
    for (size_t d = 0u; d < seir_got.size(); d += 4u)
        REQUIRE(std::accumulate(
            seir_got.begin() + d, seir_got.begin() + d + 4u, 0
        ) == static_cast< int >(n));

}
