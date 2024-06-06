#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Entity member", "[Entity]") {

    // Generating ModelSIRCONN model
    epimodels::ModelSIRCONN<> model(
        "Flu", // std::string vname,
        10000, // epiworld_fast_uint n,
        0.01,  // epiworld_double prevalence,
        40.0,  // epiworld_double contact_rate,
        1.0,   // epiworld_double transmission_rate,
        1.0/2.0// epiworld_double recovery_rate
    );

    // Generating two entities, 100 distribution
    auto dfun = epiworld::entity_to_unassigned_agents<>;
    Entity<> e1("Entity 1", 5000, false, dfun);
    Entity<> e2("Entity 2", 5000, false, dfun);
    
    model.add_entity(e1);
    model.add_entity(e2);

    model.run(50, 123);

    // Extracting entities
    auto agents1 = model.get_entity(0).get_agents();
    auto agents2 = model.get_entity(1).get_agents();

    std::vector< bool > ids(model.size(), false);
    int nrepeats = 0;
    for (const auto & a: agents1)
    {
        if (ids[a])
            nrepeats++;
        else
            ids[a] = true;
    }
    for (const auto & a: agents2)
    {
        if (ids[a])
            nrepeats++;
        else
            ids[a] = true;
    }

    // Accumulate ids if they are true
    int ntrue = 0;
    for (const auto & id: ids)
        if (id)
            ntrue++;

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(nrepeats == 0);
    REQUIRE(ntrue == model.size());
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}