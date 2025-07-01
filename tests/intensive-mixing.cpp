#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

int main() {
    
    // More intensive test with larger population and 50 groups
    std::vector< double > contact_matrix(50 * 50, 0.0);
    
    // Create identity matrix (each group only contacts itself)
    for (int i = 0; i < 50; ++i) {
        contact_matrix[i * 50 + i] = 1.0;
    }

    epimodels::ModelSIRMixing<> model(
        "Flu", // std::string vname,
        100000, // epiworld_fast_uint n, increased population
        0.01,// epiworld_double prevalence,
        80.0,// epiworld_double contact_rate,
        1.0,// epiworld_double transmission_rate,
        1.0/3.0,// epiworld_double recovery_rate,
        contact_matrix
    );

    // Copy the original virus
    Virus<> v1 = model.get_virus(0);
    model.rm_virus(0);
    v1.set_distribution(dist_virus<>(0));

    model.add_virus(v1);

    // Creating fifty groups (2000 agents each)
    std::vector<Entity<>> entities;
    for (int i = 0; i < 50; ++i) {
        entities.emplace_back(
            "Entity " + std::to_string(i + 1), 
            dist_factory<>(i * 2000, (i + 1) * 2000)
        );
        model.add_entity(entities.back());
    }

    // Running for longer period
    model.run(100, 123);
    
    return 0;
}
