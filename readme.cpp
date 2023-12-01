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
    // - Have a 30% recovery rate
    // - Infected individuals become "Infected" (status 1)
    // - Recovered individuals become "Recovered" (status 2)
    // 100 individuals will have the virus from the beginning.
    Virus<> virus("covid 19");

    virus.set_prob_infecting(.90);
    virus.set_prob_recovery(.30);
    
    virus.set_status(1, 2);

    model.default_add_virus<TSeq>n(virus, 1000);
    
    // Generating a random pop from a smallworld network
    model.agents_smallworld(100000, 4L, false, .01);

    // Initializing setting days and seed
    model.init(100, 122);

    // Running the model
    model.run();
    model.print();
  
}