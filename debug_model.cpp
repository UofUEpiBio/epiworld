#include <iostream>
#include "include/epiworld/epiworld.hpp"

using namespace epiworld;

// Helper functions for testing
template<typename TSeq = int>
inline epiworld::EntityToAgentFun<TSeq> dist_factory(int from, int to) {
    return [from, to](epiworld::Entity<TSeq> & e, epiworld::Model<TSeq> * m) -> void {

            auto & agents = m->get_agents();
            for (int i = from; i < to; ++i)
            {
                agents[i].add_entity(e, m);
            }
            return;
        };
}

template<typename TSeq = int>
inline epiworld::VirusToAgentFun<TSeq> dist_virus(int i)
{
    return [i](epiworld::Virus<TSeq> & v, epiworld::Model<TSeq> * m) -> void {

            m->get_agents()[i].set_virus(v, m);
            return;
    };
}

int main() {
    
    std::cout << "Testing ModelMeaslesMixingRiskQuarantine creation..." << std::endl;
    
    try {
        // Simple contact matrix (single group, all mixing)
        std::vector<double> contact_matrix = {1.0};
        
        epimodels::ModelMeaslesMixingRiskQuarantine<> model(
            10,          // Number of agents (very small for debugging)
            0.1,         // Initial prevalence
            2.0,         // Contact rate
            0.2,         // Transmission rate
            0.9,         // Vaccination efficacy
            0.3,         // Vaccination reduction recovery rate
            7.0,         // Incubation period
            4.0,         // Prodromal period
            5.0,         // Rash period
            contact_matrix, // Contact matrix
            0.2,         // Hospitalization rate
            7.0,         // Hospitalization duration
            3.0,         // Days undetected
            21,          // Quarantine period high risk
            14,          // Quarantine period medium risk
            7,           // Quarantine period low risk
            .8,          // Quarantine willingness
            .8,          // Isolation willingness
            4,           // Isolation period
            0.0,         // Proportion vaccinated
            0.1,         // Detection rate during quarantine
            1.0,         // Contact tracing success rate
            4u           // Contact tracing days prior
        );

        std::cout << "Model object created successfully!" << std::endl;

        // Adding a single entity (population group)
        model.add_entity(Entity<>("Population", dist_factory<>(0, 10)));
        std::cout << "Entity added successfully!" << std::endl;

        // Check parameters
        std::cout << "Quarantine period high: " << model("Quarantine period high") << std::endl;
        std::cout << "Quarantine period medium: " << model("Quarantine period medium") << std::endl; 
        std::cout << "Quarantine period low: " << model("Quarantine period low") << std::endl;
        std::cout << "Detection rate quarantine: " << model("Detection rate quarantine") << std::endl;

        std::cout << "Test completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}