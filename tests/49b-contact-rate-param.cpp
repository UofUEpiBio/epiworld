#include "tests.hpp"

using namespace epiworld;

// A mixing layer can take its contacts from a model parameter, read at the
// end of every step: a global event that sets "Contact rate" to 0 on day 10
// stops new exposures from day 11 on, while the same run without it goes on.
EPIWORLD_TEST_CASE("The contact rate is a model parameter", "[contact-rate]") {

    auto last_exposure = [](bool lockdown) {

        epimodels::ModelSIR<> model("v", 2000, 0.01, 0.1, 1.0 / 14.0);
        add_random_mixing(model, 5.0);

        if (lockdown)
            model.add_globalevent(
                epimodels::globalevent_set_param<int>("Contact rate", 0.0),
                "lockdown", 10
            );

        model.verbose_off();
        model.run(30, 2211);

        std::vector< int > date, source, target, virus, expo, layer;
        model.get_db().get_transmissions(date, source, target, virus, expo, layer);

        int last = -1;
        for (size_t i = 0u; i < date.size(); ++i)
            if (source[i] >= 0)
                last = std::max(last, date[i]);

        return last;

    };

    REQUIRE(last_exposure(true) == 10);
    REQUIRE(last_exposure(false) > 10);

}
