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
    auto dfun = epiworld::distribute_entity_randomly<>(5000, false);
    Entity<> e1("Entity 1", dfun);
    Entity<> e2("Entity 2", dfun);
    
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
    REQUIRE(ntrue == static_cast<int>(model.size()));
    #endif

    // Distributing entities among agents in a small population:
    // On average, x% should have 1 and y% two
    // The rest should have 0
    int N = 1e5;
    int n = 1e4;
    epimodels::ModelSIRCONN<> model2(
        "Flu", // std::string vname,
        N, // epiworld_fast_uint n,
        0.01,  // epiworld_double prevalence,
        10.0,  // epiworld_double contact_rate,
        1.0,   // epiworld_double transmission_rate,
        1.0/2.0// epiworld_double recovery_rate
    );

    Entity<> e3("Entity 3", distribute_entity_randomly<>(n, false));
    Entity<> e4("Entity 4", distribute_entity_randomly<>(n, false));

    model2.add_entity(e3);
    model2.add_entity(e4);

    model2.run(10, 123);

    auto agents3 = model2.get_entity(0).get_agents();
    auto agents4 = model2.get_entity(1).get_agents();

    std::vector< int > counts(model2.size(), 0);
    for (const auto & a: agents3)
        counts[a]++;

    for (const auto & a: agents4)
        counts[a]++;
    
    double n0 = 0, n1 = 0;
    for (const auto & c: counts)
    {
        if (c == 0)
            n0++;
        else if (c == 1)
            n1++;
    }

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(std::fabs(n0 - (N - n * 2)) < 100);
    REQUIRE(std::fabs(n1 - 2 * n) < 100);
    #endif

    // Checking distribution via sets
    std::vector< std::vector< size_t > > dist = {
        model.get_entity(0).get_agents(),
        model.get_entity(1).get_agents()
    };

    // Creating a copy of the model
    epimodels::ModelSIRCONN<> model3(
        "Flu", // std::string vname,
        N, // epiworld_fast_uint n,
        0.01,  // epiworld_double prevalence,
        10.0,  // epiworld_double contact_rate,
        1.0,   // epiworld_double transmission_rate,
        1.0/2.0// epiworld_double recovery_rate
    );

    // Updating the distribution
    e1.set_distribution(distribute_entity_to_set<>(dist[0]));
    e2.set_distribution(distribute_entity_to_set<>(dist[1]));

    model3.add_entity(e1);
    model3.add_entity(e2);

    model3.run(50, 123);

    // Should match the results!
    std::vector< std::vector< size_t > > dist2 = {
        model3.get_entity(0).get_agents(),
        model3.get_entity(1).get_agents()
    };

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(dist[0] == dist2[0]);
    REQUIRE(dist[1] == dist2[1]);
    #endif
    
    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}