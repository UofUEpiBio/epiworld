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

    // Now test with run_multiple() using 2 threads
    // Factory function to create and configure a model with entities and global events
    auto create_test_model = [&contact_matrix]() -> epimodels::ModelSEIRMixing<> {
        epimodels::ModelSEIRMixing<> m(
            "TestVirus",
            100,
            0.01,
            5.0,
            0.5,
            2.0,
            0.3,
            contact_matrix
        );
        m.seed(12345);
        m.verbose_off();

        // Add entities
        Entity<> e0("Entity0", distribute_entity_to_range<>(0, 50));
        Entity<> e1("Entity1", distribute_entity_to_range<>(50, 100));
        m.add_entity(e0);
        m.add_entity(e1);

        // Add global event
        // Note: No moved flag needed since date parameter ensures it only runs on day 5
        m.add_globalevent(
            [](Model<>* model_ptr) -> void {
                Entity<> & entity0 = model_ptr->get_entity(0);
                Entity<> & entity1 = model_ptr->get_entity(1);

                for (size_t i = 0; i < 5; ++i)
                {
                    Agent<> & agent = model_ptr->get_agent(i);
                    agent.rm_entity(entity0, model_ptr);
                    agent.add_entity(entity1, model_ptr);
                }

                for (size_t i = 5; i < 10; ++i)
                {
                    Agent<> & agent = model_ptr->get_agent(i);
                    agent.rm_entity(entity0, model_ptr);
                }
            },
            "Move agents between entities",
            5
        );

        return m;
    };

    // Create savers to capture results from both runs
    auto saver_1thread = epiworld::make_save_run<>(
        "26-entity-add-rm-saves/main_out_1thread_%li",
        true,  // total_hist
        false, // variant_info
        false, // variant_hist
        false, // tool_info
        false, // tool_hist
        false, // transmission
        false, // transition
        false, // reproductive
        false, // generation
        false  // outbreak_size
    );

    auto saver_2thread = epiworld::make_save_run<>(
        "26-entity-add-rm-saves/main_out_2thread_%li",
        true,  // total_hist
        false, // variant_info
        false, // variant_hist
        false, // tool_info
        false, // tool_hist
        false, // transmission
        false, // transition
        false, // reproductive
        false, // generation
        false  // outbreak_size
    );

    // Create models using the factory function
    auto model_1thread = create_test_model();
    auto model_2thread = create_test_model();

    // Run multiple simulations with 1 thread and 2 threads
    model_1thread.run_multiple(10, 10, 1231, saver_1thread, true, false, 1);
    model_2thread.run_multiple(10, 10, 1231, saver_2thread, true, false, 2);

    // Compare the results from both runs
    for (size_t i = 0u; i < 10u; ++i)
    {
        std::string file_1thread = "26-entity-add-rm-saves/main_out_1thread_" + std::to_string(i) + "_total_hist.csv";
        std::string file_2thread = "26-entity-add-rm-saves/main_out_2thread_" + std::to_string(i) + "_total_hist.csv";

        auto content_1thread = file_reader(file_1thread);
        auto content_2thread = file_reader(file_2thread);

        #ifdef CATCH_CONFIG_MAIN
        REQUIRE_THAT(content_1thread, Catch::Equals(content_2thread));
        #else
        if (content_1thread != content_2thread)
        {
            printf_epiworld(
                "Files %s and %s are different\n",
                file_1thread.c_str(), file_2thread.c_str()
            );
            throw std::logic_error("Multi-threading test failed - results differ!");
        }
        #endif
    }

    #ifdef CATCH_CONFIG_MAIN
    // Success message for multi-threading test
    #else
    printf_epiworld("Multi-threading test completed successfully!\n");
    #endif

}
