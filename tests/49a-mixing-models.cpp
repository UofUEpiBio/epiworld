#include "tests.hpp"

using namespace epiworld;

// ModelSIR with a mixing layer is the model ModelSIRMixing was (removed in
// 1.0.0): the same runs, draw for draw. The
// histories below were recorded with those models (epiworld 1ab03e2, 1,000
// agents in two groups, seed 1912) with libstdc++. The binomial and Poisson
// draws come from the standard library, whose algorithms differ between
// implementations, so elsewhere the test only checks the counts add up.
EPIWORLD_TEST_CASE("A model plus a mixing layer is the old mixing model", "[mixing-models]") {

    const size_t n = 1000u;
    std::vector< size_t > groups(n);
    for (size_t i = 0u; i < n; ++i)
        groups[i] = i % 2u;

    auto add_mixing = [&groups](Model<> & model) {
        auto mixing = layer::mixing<>("mixing");
        mixing->set_groups(assign::labels(groups), 2u);
        mixing->set_contact_matrix({6.0, 1.0, 1.0, 4.0});
        model.add_layer(mixing);
    };

    auto history = [](Model<> & model) {
        model.verbose_off();
        model.run(20, 1912);
        std::vector< int > counts;
        model.get_db().get_hist_total(nullptr, nullptr, &counts);
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

    epimodels::ModelSIR<> sir("v", n, 0.02, 0.1, 1.0 / 7.0);
    add_mixing(sir);
    const std::vector< int > sir_expected = {
        980, 20, 0, 966, 32, 2, 946, 47, 7, 917, 66, 17, 889, 82, 29,
        839, 122, 39, 772, 171, 57, 693, 231, 76, 614, 287, 99, 529,
        323, 148, 453, 347, 200, 365, 383, 252, 308, 390, 302, 258, 377,
        365, 216, 373, 411, 178, 367, 455, 140, 348, 512, 116, 330, 554,
        103, 293, 604, 88, 272, 640, 76, 242, 682
    };
    check(history(sir), sir_expected, 3u);

    // ModelSEIR draws its latent period when an agent enters it (a natural
    // history), so its draws differ from the old model's: only the counts
    // must add up
    epimodels::ModelSEIR<> seir("v", n, 0.02, 0.1, 3.0, 1.0 / 7.0);
    add_mixing(seir);
    const auto seir_got = history(seir);
    REQUIRE(seir_got.size() == 21u * 4u);
    for (size_t d = 0u; d < seir_got.size(); d += 4u)
        REQUIRE(std::accumulate(
            seir_got.begin() + d, seir_got.begin() + d + 4u, 0
        ) == static_cast< int >(n));

}
