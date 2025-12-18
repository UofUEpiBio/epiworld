#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test that verifies hospitalization tracking using a GlobalEvent.
 * 
 * This test creates a GlobalEvent that independently tracks hospitalizations
 * by monitoring agent state changes. At the end of the simulation, we compare
 * the GlobalEvent's tracking with the HospitalizationsTracker to ensure they match.
 */
EPIWORLD_TEST_CASE(
    "Hospitalizations tracker validation with GlobalEvent",
    "[HospitalizationsTrackerValidation]"
) {
    
    // Data structure to track hospitalizations via GlobalEvent
    // Key: (date, virus_id, tool_id), Value: sum of tool_weight
    std::map<std::tuple<int, int, int>, double> global_event_tracking;
    
    // Track each agent's previous state to detect transitions
    std::vector<unsigned int> previous_states;
    
    // Create the model
    int n_seeds = 5;
    int simulation_days = 30;
    epimodels::ModelMeaslesSchool<> model(
        200,    // Number of agents
        n_seeds, // Number of initial cases
        5.0,     // Contact rate
        0.3,     // Transmission rate
        0.9,     // Vaccination efficacy
        0.3,     // Vaccination reduction recovery rate
        5.0,     // Incubation period
        3.0,     // Prodromal period
        4.0,     // Rash period
        2.0,     // Days undetected
        0.1,     // Hospitalization rate
        5.0,     // Hospitalization duration
        0.5,     // Proportion vaccinated
        -1,      // Quarantine period (disabled)
        0.0,     // Quarantine willingness
        -1       // Isolation period (disabled)
    );
    
    // Initialize previous states
    previous_states.resize(model.size(), 0u);
    
    // Create a GlobalEvent to track hospitalizations
    auto track_hospitalizations = [&global_event_tracking, &previous_states](
        epiworld::Model<>* m
    ) -> void {
        
        int current_day = m->today();
        
        // Check each agent for state transitions to hospitalized states
        for (size_t i = 0; i < m->size(); ++i)
        {
            auto & agent = m->get_agent(i);
            unsigned int current_state = agent.get_state();
            unsigned int prev_state = previous_states[i];
            
            // Check if agent transitioned to HOSPITALIZED (11) or DETECTED_HOSPITALIZED (6)
            bool now_hospitalized = (current_state == 6u || current_state == 11u);
            bool was_hospitalized = (prev_state == 6u || prev_state == 11u);
            
            if (now_hospitalized && !was_hospitalized)
            {
                // New hospitalization detected!
                int v_id = -1;
                auto & virus = agent.get_virus();
                if (virus != nullptr)
                    v_id = virus->get_id();
                
                size_t n_tools = agent.get_n_tools();
                
                if (n_tools == 0u)
                {
                    auto key = std::make_tuple(current_day, v_id, -1);
                    global_event_tracking[key] += 1.0;
                }
                else
                {
                    double weight = 1.0 / static_cast<double>(n_tools);
                    for (size_t t = 0u; t < n_tools; ++t)
                    {
                        int t_id = agent.get_tool(static_cast<int>(t))->get_id();
                        auto key = std::make_tuple(current_day, v_id, t_id);
                        global_event_tracking[key] += weight;
                    }
                }
            }
            
            // Update previous state
            previous_states[i] = current_state;
        }
    };
    
    // Add the GlobalEvent to run at the end of each day
    model.add_globalevent(track_hospitalizations, "Hospitalization Tracker");
    
    // Run the model
    model.run(simulation_days, 12345);
    
    // Get hospitalization data from the built-in tracker
    std::vector<int> dates;
    std::vector<int> virus_ids;
    std::vector<int> tool_ids;
    std::vector<double> tool_weights;
    
    model.get_hospitalizations(dates, virus_ids, tool_ids, tool_weights);
    
    // Convert the built-in tracker data to a map for comparison
    // (only non-zero entries)
    std::map<std::tuple<int, int, int>, double> tracker_data;
    for (size_t i = 0; i < dates.size(); ++i)
    {
        if (tool_weights[i] > 0.0)
        {
            auto key = std::make_tuple(dates[i], virus_ids[i], tool_ids[i]);
            tracker_data[key] = tool_weights[i];
        }
    }
    
    #ifndef CATCH_CONFIG_MAIN
    std::cout << "GlobalEvent tracking results:" << std::endl;
    for (const auto & entry : global_event_tracking)
    {
        std::cout << "Date: " << std::get<0>(entry.first)
                  << ", Virus ID: " << std::get<1>(entry.first)
                  << ", Tool ID: " << std::get<2>(entry.first)
                  << ", Weight: " << entry.second
                  << std::endl;
    }
    
    std::cout << "\nBuilt-in tracker results (non-zero):" << std::endl;
    for (const auto & entry : tracker_data)
    {
        std::cout << "Date: " << std::get<0>(entry.first)
                  << ", Virus ID: " << std::get<1>(entry.first)
                  << ", Tool ID: " << std::get<2>(entry.first)
                  << ", Weight: " << entry.second
                  << std::endl;
    }
    #endif
    
    // Compare the two tracking methods
    #ifdef CATCH_CONFIG_MAIN
    // Both maps should have the same number of non-zero entries
    REQUIRE(global_event_tracking.size() == tracker_data.size());
    
    // Each entry should match
    for (const auto & entry : global_event_tracking)
    {
        auto it = tracker_data.find(entry.first);
        REQUIRE(it != tracker_data.end());
        REQUIRE(std::abs(it->second - entry.second) < 0.0001);
    }
    #endif
    
    #ifndef CATCH_CONFIG_MAIN
    // Verify manually
    bool match = true;
    if (global_event_tracking.size() != tracker_data.size())
    {
        std::cout << "ERROR: Size mismatch! GlobalEvent: " << global_event_tracking.size()
                  << ", Tracker: " << tracker_data.size() << std::endl;
        match = false;
    }
    else
    {
        for (const auto & entry : global_event_tracking)
        {
            auto it = tracker_data.find(entry.first);
            if (it == tracker_data.end())
            {
                std::cout << "ERROR: Key not found in tracker!" << std::endl;
                match = false;
                break;
            }
            if (std::abs(it->second - entry.second) >= 0.0001)
            {
                std::cout << "ERROR: Value mismatch!" << std::endl;
                match = false;
                break;
            }
        }
    }
    
    if (match)
        std::cout << "\nVALIDATION PASSED: Both tracking methods match!" << std::endl;
    
    return 0;
    #endif
    
}
