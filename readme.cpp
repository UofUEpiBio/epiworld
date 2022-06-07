#include "epiworld.hpp"

using namespace epiworld;

int main()
{

    // Creating a model with three statuses:
    // - Susceptible: Status 0
    // - Infected: Status 1
    // - Recovered: Status 2
    Model<> model;
    model.add_status("Susceptible", default_update_susceptible<>);
    model.add_status("Infected", default_update_exposed<>);
    model.add_status("Recovered");

    // Desgining a virus: This virus will:
    // - Have a 90% transmission rate
    // - Have a 50% recovery rate
    // - Infected individuals become "Infected" (status 1)
    // - Recovered individuals become "Recovered" (status 2)
    // 100 individuals will have the virus from the beginning.
    Virus<> virus("covid 19");

    virus.set_prob_infecting(.95);
    virus.set_prob_recovery(.14);
    
    virus.set_status(1, 2);

    model.add_virus_n(virus, 100);
    
    // Generating a random pop from a smallworld network
    model.agents_smallworld(100000);

    // Initializing setting days and seed
    model.init(100, 123);

    // Running the model
    model.run();
    model.print();
  
}