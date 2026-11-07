#include "tests.hpp"

using namespace epiworld;

// Agent::mutate_virus() applies the virus' mutation with the model (#270).
// Each mutation is a new variant: the next id, a child of the variant it
// changed, in the same lineage. The next run starts from the model's viruses.
EPIWORLD_TEST_CASE("Agent::mutate_virus()", "[mutation]") {

    // No transmission and no recovery: the seeded agents stay infected
    epimodels::ModelSIR model("a virus", 50, 0.1, 0.0, 0.0);
    add_random_mixing(model, 2.0);
    model.verbose_off();

    model.get_virus(0).set_mutation(
        [](Agent *, Virus &, Model *) -> bool { return true; });

    model.run(0, 123);
    REQUIRE(model.get_n_viruses() == 1u);

    Agent * infected = nullptr;
    Agent * susceptible = nullptr;
    for (auto & a : model.get_agents())
    {
        if (a.get_virus() != nullptr)
            infected = &a;
        else
            susceptible = &a;
    }

    REQUIRE(infected != nullptr);
    REQUIRE(susceptible != nullptr);

    // The mutated virus is a new variant, recorded in the database, and so
    // is the variant of the variant (the same change, a new id)
    infected->mutate_virus(model);
    infected->mutate_virus(model);
    REQUIRE(model.get_n_viruses() == 3u);
    REQUIRE(infected->get_virus()->get_id() == 2);
    REQUIRE(infected->get_virus()->get_lineage_id() == 0);

    auto info = model.get_db().get_run_outputs({"virus_info"}).at("virus_info");
    REQUIRE(info.colnames == std::vector< std::string >(
        {"virus_id", "virus", "date_recorded", "parent"}
    ));
    REQUIRE(std::get< std::vector< int > >(info.columns[0u]) == std::vector< int >({0, 1, 2}));
    REQUIRE(std::get< std::vector< int > >(info.columns[3u]) == std::vector< int >({-99, 0, 1}));

    REQUIRE_THROWS_AS(susceptible->mutate_virus(model), std::logic_error);

    // The next run drops the variants
    model.run(0, 123);
    REQUIRE(model.get_n_viruses() == 1u);

}
