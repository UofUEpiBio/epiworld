#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test that Agent::add_entity and Agent::rm_entity work correctly
 * 
 * This test verifies that:
 * 1. Entities can be distributed to agents initially (50/50 split)
 * 2. Agents can be removed from one entity and added to another during simulation
 * 3. Final entity sizes match expected values after modifications
 */
EPIWORLD_TEST_CASE("Entity add/rm operations", "[entity][add_entity][rm_entity]") {

    // Contact matrix for 2 groups (identity = no cross-group contact)
    std::vector<double> contact_matrix = {
        1.0, 0.0,
        0.0, 1.0
    };

    // Create a simple SEIR mixing model with 100 agents
    epimodels::ModelSEIRMixing<> model(
        "TestVirus",    // vname
        100,            // n (number of agents)
        0.01,           // prevalence
        5.0,            // contact_rate
        0.5,            // transmission_rate
        2.0,            // avg_incubation_days
        0.3,            // recovery_rate
        contact_matrix
    );

    model.seed(12345);
    model.verbose_off();

    // Create two entities with 50 agents each
    // Entity 0: agents 0-49
    // Entity 1: agents 50-99
    Entity<> e0("Entity0", distribute_entity_to_range<>(0, 50));
    Entity<> e1("Entity1", distribute_entity_to_range<>(50, 100));

    model.add_entity(e0);
    model.add_entity(e1);

    // Track which agents we moved
    bool moved = false;

    // Add a global event on day 5 that:
    // - Removes 5 agents (ids 0-4) from Entity 0
    // - Adds those same 5 agents to Entity 1
    // - Also removes 5 more agents (ids 5-9) from Entity 0 without adding to any entity
    model.add_globalevent(
        [&moved](Model<>* m) -> void {
            if (moved)
                return;
            
            moved = true;

            Entity<> & entity0 = m->get_entity(0);
            Entity<> & entity1 = m->get_entity(1);

            // Move agents 0-4 from Entity 0 to Entity 1
            for (size_t i = 0; i < 5; ++i)
            {
                Agent<> & agent = m->get_agent(i);
                agent.rm_entity(entity0, m);
                agent.add_entity(entity1, m);
            }

            // Remove agents 5-9 from Entity 0 (leave them without entity)
            for (size_t i = 5; i < 10; ++i)
            {
                Agent<> & agent = m->get_agent(i);
                agent.rm_entity(entity0, m);
            }
        },
        "Move agents between entities",
        5  // Run on day 5
    );

    // Run the model for 10 days (single replicate)
    model.run(10);

    // Get final entity sizes
    Entity<> & final_entity0 = model.get_entity(0);
    Entity<> & final_entity1 = model.get_entity(1);

    size_t size0 = final_entity0.size();
    size_t size1 = final_entity1.size();

    // Count agents with no entities
    size_t no_entity_count = 0;
    for (const auto & agent : model.get_agents())
    {
        if (agent.get_n_entities() == 0)
            no_entity_count++;
    }

    #ifdef CATCH_CONFIG_MAIN
    // Entity 0 started with 50, lost 10 (5 moved + 5 removed) = 40
    REQUIRE(size0 == 40);
    
    // Entity 1 started with 50, gained 5 = 55
    REQUIRE(size1 == 55);
    
    // 5 agents should have no entity
    REQUIRE(no_entity_count == 5);

    // Verify the specific agents moved correctly
    // Agents 0-4 should be in Entity 1
    for (size_t i = 0; i < 5; ++i)
    {
        const Agent<> & agent = model.get_agent(i);
        REQUIRE(agent.get_n_entities() == 1);
        REQUIRE(agent.get_entity(0).get_id() == 1);
    }

    // Agents 5-9 should have no entity
    for (size_t i = 5; i < 10; ++i)
    {
        const Agent<> & agent = model.get_agent(i);
        REQUIRE(agent.get_n_entities() == 0);
    }

    // Agents 10-49 should still be in Entity 0
    for (size_t i = 10; i < 50; ++i)
    {
        const Agent<> & agent = model.get_agent(i);
        REQUIRE(agent.get_n_entities() == 1);
        REQUIRE(agent.get_entity(0).get_id() == 0);
    }

    // Agents 50-99 should still be in Entity 1
    for (size_t i = 50; i < 100; ++i)
    {
        const Agent<> & agent = model.get_agent(i);
        REQUIRE(agent.get_n_entities() == 1);
        REQUIRE(agent.get_entity(0).get_id() == 1);
    }
    #else
    printf_epiworld(
        "Entity add/rm test completed:\n"
        "  Entity 0 size: %zu (expected 40)\n"
        "  Entity 1 size: %zu (expected 55)\n"
        "  Agents with no entity: %zu (expected 5)\n",
        size0, size1, no_entity_count
    );
    
    if (size0 != 40 || size1 != 55 || no_entity_count != 5)
    {
        throw std::logic_error("Entity add/rm test failed!");
    }
    #endif

}
