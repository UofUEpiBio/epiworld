#ifndef CATCH_CONFIG_MAIN
// #define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

int main() {

    size_t n_agents = 5000;
    
    // More intensive test with larger population and 50 groups
    std::vector< double > contact_matrix(50 * 50, 0.0);
    
    // Create contact matrix with rows summing to 1 and random elements
    std::mt19937 gen(123); // Set seed for reproducibility
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    
    for (int i = 0; i < 50; ++i) {
        double row_sum = 0.0;
        // Generate random values for the row
        for (int j = 0; j < 50; ++j) {
            contact_matrix[j * 50 + i] = dis(gen);
            row_sum += contact_matrix[j * 50 + i];
        }
        // Normalize the row to sum to 1
        for (int j = 0; j < 50; ++j) {
            contact_matrix[j * 50 + i] /= row_sum;
        }
    }

    epimodels::ModelSIRMixing<> model(
        "Flu", // std::string vname,
        n_agents, // epiworld_fast_uint n, increased population
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
    std::vector< double > distributions = {
        0.028, 0.007, 0.023, 0.023, 0.002, 0.003, 0.028, 0.004, 0.018, 0.034,
        0.023, 0.023, 0, 0.002, 0.012, 0.002, 0.005, 0.002, 0.045, 0.026,
        0.029, 0.001, 0.016, 0.024, 0.03, 0.038, 0.001, 0.035, 0.018, 0.033,
        0.032, 0.021, 0.039, 0.036, 0.014, 0.036, 0.006, 0.004, 0.028, 0.035,
        0, 0.035, 0.017, 0.032, 0.005, 0.045, 0.006, 0.018, 0.029
    };

    size_t distributed = 0;
    for (int i = 0; i < (distributions.size() + 1); ++i)
    {

        if (i == distributions.size())
        {
            entities.emplace_back(
                "Entity " + std::to_string(i + 1), 
                dist_factory<>(
                    distributed,
                    n_agents
                )
            );
        } else {

            entities.emplace_back(
                "Entity " + std::to_string(i + 1), 
                dist_factory<>(
                    distributed,
                    distributed + n_agents * distributions[i]
                )
            );

            distributed += n_agents * distributions[i];
        }
        model.add_entity(entities.back());
    }

    // Running for longer period
    model.run_multiple(
        100, // epiworld_fast_uint ndays,
        20, // epiworld_fast_uint nexperiments,
        33, // int seed_,
        nullptr, // std::function<void(size_t,Model<>)> fun,
        true, // bool reset,
        true, // bool verbose,
        4 // int nthreads
    );
    
    return 0;
}
