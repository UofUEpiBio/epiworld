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
    
    std::cout << "Testing ModelMeaslesMixingRiskQuarantine..." << std::endl;
    
    try {
        // Simple contact matrix (single group, all mixing)
        std::vector<double> contact_matrix = {1.0};
        
        epimodels::ModelMeaslesMixingRiskQuarantine<> model(
            100,         // Number of agents (smaller for quick test)
            0.05,        // Initial prevalence
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

        // Adding a single entity (population group)
        model.add_entity(Entity<>("Population", dist_factory<>(0, 100)));

        // Setting the distribution function of the initial cases
        Virus<> virus = model.get_virus(0);
        model.rm_virus(0);
        virus.set_distribution(dist_virus<>(5));
        model.add_virus(virus);
        
        // Setting initial states - this needs to be called after entities are added
        model.initial_states({1.0, 0.0});

        std::cout << "Model created successfully!" << std::endl;

        // Run simulation
        model.run(20, 123);
        std::cout << "Model ran successfully!" << std::endl;
        
        // Check basic results
        auto db = model.get_db();
        std::vector<int> totals;
        db.get_today_total(nullptr, &totals);

        std::cout << "Final state distribution:" << std::endl;
        for (size_t i = 0; i < totals.size(); ++i) {
            std::cout << "State " << i << ": " << totals[i] << " agents" << std::endl;
        }

        // Check risk levels were assigned
        auto risk_levels = model.get_quarantine_risk_levels();
        int high_risk = 0, medium_risk = 0, low_risk = 0;
        for (size_t i = 0; i < risk_levels.size(); ++i) {
            if (risk_levels[i] == model.RISK_HIGH) high_risk++;
            else if (risk_levels[i] == model.RISK_MEDIUM) medium_risk++;
            else low_risk++;
        }
        
        std::cout << "Risk level assignments:" << std::endl;
        std::cout << "High risk: " << high_risk << std::endl;
        std::cout << "Medium risk: " << medium_risk << std::endl;
        std::cout << "Low risk: " << low_risk << std::endl;

        std::cout << "Test completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}