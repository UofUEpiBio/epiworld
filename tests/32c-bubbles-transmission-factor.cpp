#include "tests.hpp"

using namespace epiworld;

// Builds a household id vector: agents 0..n-1 grouped in households of `hh_size`.
static std::vector<size_t> make_households(size_t n, size_t hh_size)
{
    std::vector<size_t> hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / hh_size;
    return hh;
}

// Transmissions split into within- and out-of-bubble.
struct TransmissionCounts {
    int within = 0;
    int outside = 0;
};

EPIWORLD_TEST_CASE(
    "Bubbles - transmission factor dampens out-of-bubble contact", "[bubbles]"
) {

    size_t n = 400u, hh_size = 4u, group = 2u;
    auto hh = make_households(n, hh_size);

    // Runs the model with the factor given at construction; `override_factor`
    // (when non-negative) is instead written to the model parameter before the
    // run, so the tool must pick it up from the model.
    auto run_with = [&](
        epiworld_double factor, epiworld_double override_factor
    ) -> TransmissionCounts {

        epimodels::ModelSEIR<> model("flu", 0.1, 0.2, 4.5, 1.0/8.0);
        model.seed(2024);
        model.agents_smallworld(n, 8, false, 0.10);

        Bubbles<> bubbles(hh, BubbleFlavor::Household, group, factor, 0, -1, 0);
        model.add_globalevent(bubbles);

        if (override_factor >= 0.0)
            model.add_param(override_factor, bubbles.get_param_name(), true);

        model.verbose_off();
        model.run(80);

        // The intervention registers the factor as a model parameter when it
        // sets itself up, but a value already in the model is left alone.
        REQUIRE(
            model.get_param(bubbles.get_param_name()) ==
            (override_factor >= 0.0 ? override_factor : factor)
        );

        const auto & bid = Bubbles<>::get_from(model)->get_bubble_id();

        std::vector<int> date, source, target, virus, sexp;
        model.get_db().get_transmissions(date, source, target, virus, sexp);

        TransmissionCounts counts;
        for (size_t i = 0u; i < source.size(); ++i)
        {
            if (source[i] < 0) // seed case (external)
                continue;

            if (bid[static_cast<size_t>(source[i])] ==
                bid[static_cast<size_t>(target[i])])
                ++counts.within;
            else
                ++counts.outside;
        }

        return counts;

    };

    // factor = 0: a perfectly observed bubble. Contact outside it is cut, while
    // contact *within* the bubble is untouched -- the outbreak still spreads
    // inside bubbles.
    TransmissionCounts strict = run_with(0.0, -1.0);
    REQUIRE(strict.outside == 0);
    REQUIRE(strict.within > 0);

    // factor = 1: the bubble imposes nothing, so contacts across bubbles
    // transmit as they would without the intervention.
    TransmissionCounts off = run_with(1.0, -1.0);
    REQUIRE(off.outside > 0);

    // A soft contact reduction sits in between: some out-of-bubble
    // transmission, but less than with no policy at all.
    TransmissionCounts soft = run_with(0.25, -1.0);
    REQUIRE(soft.outside > 0);
    REQUIRE(soft.outside < off.outside);

    // The factor is read from the model on every exposure, so the model's
    // parameter -- not the value the object was built with -- governs the run.
    TransmissionCounts from_param = run_with(0.0, 1.0);
    REQUIRE(from_param.outside == off.outside);
    REQUIRE(from_param.within == off.within);

}
