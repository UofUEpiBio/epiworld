#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("distribute_tool_virus_to_entities_counts", "[distribute-entities-counts]") {

    // Create a basic SIR model
    epimodels::ModelSIRCONN<> model(
        "test virus", 500, 0.0, 0.9, 0.0, 0.0
    );

    // Set seed for reproducibility
    model.seed(124);

    // Create 5 entities with 100 agents each (500 total)
    std::vector<int> entity_ranges = {0, 100, 200, 300, 400, 500};
    for (size_t i = 0; i < entity_ranges.size() - 1; ++i) {
        Entity<> e("Entity " + std::to_string(i), 
                   dist_factory<>(entity_ranges[i], entity_ranges[i+1]));
        model.add_entity(e);
    }

    // Create tool with distribution function (using counts, not proportions)
    Tool<> tool("Test Tool");
    tool.set_transmission_reduction(0.0);
    tool.set_susceptibility_reduction(0.0);
    std::vector<double> tool_counts = {10.0, 5.0, 2.0, 1.0, 0.0};
    tool.set_distribution(
        distribute_tool_to_entities<>(tool_counts, false)
    );
    model.add_tool(tool);

    // Create virus with distribution function (using counts, not proportions)
    Virus<> virus = model.get_virus(0);
    virus.set_distribution(
        distribute_virus_to_entities<>(tool_counts, false)
    );
    model.rm_virus(0);
    model.add_virus(virus);

    // Run model multiple times to collect statistics
    int n_runs = 100;
    std::vector<int> virus_counts_by_entity(5, 0);
    std::vector<int> tool_counts_by_entity(5, 0);

    // Create saver to record initial state
    auto saver = [&virus_counts_by_entity, &tool_counts_by_entity](
        size_t, Model<>* m
    ) -> void {
        // Record counts at time 0
        for (size_t e = 0; e < m->get_entities().size(); ++e) {
            for (const Agent<> & agent : m->get_entity(e)) {
                if (agent.get_virus() != nullptr) {
                    virus_counts_by_entity[e]++;
                }
                if (agent.get_n_tools() > 0) {
                    tool_counts_by_entity[e]++;
                }
            }
        }
    };

    // Run multiple times
    model.run_multiple(1, n_runs, 124, saver, true, true, 1);

    // Calculate averages
    std::vector<double> avg_virus_by_entity(5);
    std::vector<double> avg_tool_by_entity(5);
    for (size_t i = 0; i < 5; ++i) {
        avg_virus_by_entity[i] = static_cast<double>(virus_counts_by_entity[i]) / n_runs;
        avg_tool_by_entity[i] = static_cast<double>(tool_counts_by_entity[i]) / n_runs;
    }

    // Expected values for each entity (exact counts since we're using counts, not proportions)
    std::vector<double> expected_counts = {10.0, 5.0, 2.0, 1.0, 0.0};

    // With exact counts, the results should be deterministic (no variation)
    #ifdef CATCH_CONFIG_MAIN
    for (size_t i = 0; i < 5; ++i) {
        // Check that virus distribution matches exactly
        REQUIRE(avg_virus_by_entity[i] == expected_counts[i]);

        // Check that tool distribution matches exactly
        REQUIRE(avg_tool_by_entity[i] == expected_counts[i]);
    }
    #endif
}
