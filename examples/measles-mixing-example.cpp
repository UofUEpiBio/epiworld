#include "../include/epiworld/epiworld.hpp"
#include <iostream>

using namespace epiworld;

// Helper function to distribute agents to entities
template<typename TSeq = EPI_DEFAULT_TSEQ>
EntityToAgentFun<TSeq> dist_factory(int from, int to) {
    return [from, to](Entity<> & e, Model<> * m) -> void {
        auto & agents = m->get_agents();
        for (size_t i = from; i < to; ++i)
        {
            e.add_agent(&agents[i], m);
        }
        return;
    };
}

int main() {
    
    // Example: Measles outbreak in a single population with mixing
    
    // Contact matrix for a single homogeneous population (all agents mix equally)
    std::vector<double> contact_matrix = {1.0};
    
    // Create the model
    epimodels::ModelMeaslesMixing<> model(
        1000,             // Population size
        0.005,            // Initial prevalence (0.5% infected)
        4.0,              // Contact rate (4 contacts per day)
        0.9,              // Transmission rate (90% transmission per contact)
        0.95,             // Vaccine efficacy (95% effective)
        0.5,              // Vaccine recovery enhancement (50% faster recovery)
        10.0,             // Incubation period (10 days)
        4.0,              // Prodromal period (4 days, infectious)
        5.0,              // Rash period (5 days, detectable)
        contact_matrix,   // Contact matrix
        0.1,              // Hospitalization rate (10%)
        7.0,              // Hospitalization period (7 days)
        2.0,              // Days undetected (2 days average detection time)
        14,               // Quarantine period (14 days)
        0.8,              // Quarantine willingness (80% compliance)
        0.9,              // Isolation willingness (90% compliance)
        10,               // Isolation period (10 days)
        0.7,              // Vaccination rate (70% vaccinated)
        0.8,              // Contact tracing success rate (80%)
        3u                // Contact tracing days prior (3 days)
    );

    // Add a single population entity
    model.add_entity(Entity<>("Population", dist_factory<>(0, 1000)));

    // Run the simulation for 60 days
    model.run(60, 123);
    
    // Print results
    model.print();
    
    std::cout << "\n=== Model Summary ===" << std::endl;
    std::cout << "This model demonstrates measles transmission with:" << std::endl;
    std::cout << "- Population mixing using contact matrices" << std::endl;
    std::cout << "- Disease progression: Susceptible → Exposed → Prodromal → Rash" << std::endl;
    std::cout << "- Prodromal individuals are infectious" << std::endl;
    std::cout << "- Detection occurs during rash period" << std::endl;
    std::cout << "- Contact tracing and quarantine measures" << std::endl;
    std::cout << "- Vaccination effects included" << std::endl;
    
    return 0;
}