#include "tests.hpp"

using namespace epiworld;

int main()
{

    std::vector< double > contact_matrix = {
        0.9, 0.1, 0.1,
        0.05, 0.8, 0.2,
        0.05, 0.1, 0.7
    };

    epimodels::ModelSEIRMixing<> model(
        "Flu", // std::string vname,
        9000, // epiworld_fast_uint n,
        1.0/9000.0,  // epiworld_double prevalence,
        20.0,  // epiworld_double contact_rate,
        0.1,   // epiworld_double transmission_rate,
        7.0,   // epiworld_double avg_incubation_days,
        1.0/7.0,// epiworld_double recovery_rate,
        contact_matrix
    );

    // Creating three groups
    Entity<> e1("Entity 1", distribute_entity_randomly<>(n, false));
    Entity<> e2("Entity 2", distribute_entity_randomly<>(n, false));
    Entity<> e3("Entity 3", distribute_entity_randomly<>(n, false));

    model.add_entity(e1);
    model.add_entity(e2);
    model.add_entity(e3);

    // Running and checking the results
    model.run(100, 13);
    model.print();

    // Checking entity assignment
    auto agents1 = model.get_entity(0).get_agents();
    auto agents2 = model.get_entity(1).get_agents();
    auto agents3 = model.get_entity(2).get_agents();

    
    return 0;

}