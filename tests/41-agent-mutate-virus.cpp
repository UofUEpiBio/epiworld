#include "tests.hpp"

using namespace epiworld;

// Agent::mutate_virus() applies the virus' mutation with the model (#270)
EPIWORLD_TEST_CASE("Agent::mutate_virus()", "[mutation]") {

    // No transmission and no recovery: the seeded agents stay infected
    epimodels::ModelSIRCONN<> model("a virus", 50, 0.1, 2.0, 0.0, 0.0);
    model.verbose_off();

    model.get_virus(0).set_mutation(
        [](Agent<> *, Virus<> & v, Model<> *) -> bool {
            v.set_sequence(v.get_sequence() + 1);
            return true;
        });

    model.run(0, 123);
    REQUIRE(model.get_n_viruses() == 1u);

    Agent<> * infected = nullptr;
    Agent<> * susceptible = nullptr;
    for (auto & a : model.get_agents())
    {
        if (a.get_virus() != nullptr)
            infected = &a;
        else
            susceptible = &a;
    }

    REQUIRE(infected != nullptr);
    REQUIRE(susceptible != nullptr);

    // The mutated virus is a new variant, recorded in the database
    infected->mutate_virus(model);
    REQUIRE(model.get_n_viruses() == 2u);
    REQUIRE(infected->get_virus()->get_id() == 1);

    REQUIRE_THROWS_AS(susceptible->mutate_virus(model), std::logic_error);

}
