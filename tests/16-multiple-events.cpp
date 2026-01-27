#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Multiple events", "[multi-events]") {

    epiworld::Model<> model_0;

    EPI_NEW_UPDATEFUN_LAMBDA(update_twice, int) {

        p->change_state(m, 1);

        if (m->runif() > .5)
            p->change_state(m, m->state_of("State 2"));

    };

    EPI_NEW_UPDATEFUN_LAMBDA(update_once, int) {

        if (m->runif() > .5)
            p->change_state(m, m->state_of("State 2"));

    };

    EPI_NEW_UPDATEFUN_LAMBDA(update_back, int) {

        p->change_state(m, m->state_of("State 1"));
        p->change_state(m, m->state_of("State 0"));

    };

    model_0.add_state("State 0", update_twice);
    model_0.add_state("State 1", update_once);
    model_0.add_state("State 2", update_back);

    model_0.agents_empty_graph(100);
    model_0.queuing_off();
    model_0.run(2000, 1);
    model_0.print(false);

    // Extracting the transition probabilities
    auto tprobs = model_0.get_db().get_transition_probability(false, true);
    std::vector<epiworld_double> tprob_expected = {0.0, 0.0, 1.0, 0.5, .5, 0.0, 0.5, 0.5, 0.0};

    REQUIRE_THAT(
        tprobs,
        Catch::Approx(tprob_expected).margin(0.05)
    );



}
