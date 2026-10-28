#include "tests.hpp"

using namespace epiworld;

// ModelSIR and ModelSEIR with a mixing layer are the models ModelSIRMixing and
// ModelSEIRMixing were (removed in 1.0.0): the same runs, draw for draw. The
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
        mixing->set_groups(groups, 2u);
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

    epimodels::ModelSEIR<> seir("v", n, 0.02, 0.1, 3.0, 1.0 / 7.0);
    add_mixing(seir);
    const std::vector< int > seir_expected = {
        980, 20, 0, 0, 980, 14, 6, 0, 969, 23, 7, 1, 966, 22, 11, 1,
        961, 19, 18, 2, 956, 20, 21, 3, 943, 22, 29, 6, 921, 37, 32, 10,
        893, 54, 38, 15, 871, 57, 54, 18, 846, 62, 66, 26, 813, 82, 67,
        38, 775, 94, 83, 48, 747, 78, 119, 56, 703, 98, 123, 76, 664,
        104, 135, 97, 605, 131, 151, 113, 566, 123, 177, 134, 515, 136,
        199, 150, 463, 142, 215, 180, 417, 148, 225, 210
    };
    check(history(seir), seir_expected, 4u);

}
