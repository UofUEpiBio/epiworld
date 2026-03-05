#include "epiworld.hpp"

using namespace epiworld;

int main()
{

    // Creating a model with three statuses:
    // - Susceptible: Status 0
    // - Infected: Status 1
    // - Recovered: Status 2
    Model<> model;
    model.add_state("Susceptible", default_update_susceptible<>);
    model.add_state("Infected", default_update_exposed<>);
    model.add_state("Recovered");

    // Designing a virus: This virus will:
    // - Have a 10% transmission rate
    // - Have a 30% recovery rate
    // - Infected individuals become "Infected" (status 1)
    // - Recovered individuals become "Recovered" (status 2)
    // 100 individuals will have the virus from the beginning.
    Virus<> virus("covid 19", 100, false);

    virus.set_prob_infecting(.10);
    virus.set_prob_recovery(.30);
    
    virus.set_state(1, 2);

    model.add_virus(virus);
    
    // Generating a random pop from a smallworld network
    model.agents_smallworld(100000, 10L, false, .01);

    // Running the model
    model.run(100, 122);
    model.print();
  
}