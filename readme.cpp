#include "epiworld.hpp"

using namespace epiworld;

int main()
{

    Model<> model(100000);

    // Designing a virus and its natural history: three stages,
    // Susceptible, Infected and Recovered. A contact with an infected agent
    // transmits with probability 0.1, and infected agents recover with daily
    // probability 0.3. 100 individuals have the virus from the beginning.
    Virus<> virus("covid 19");
    virus.add_stage("Susceptible");
    virus.add_stage("Infected", stage::infectious);
    virus.add_stage("Recovered");
    virus.infection(0, 1, 0.10);
    virus.rate(1, 2, 0.30).effect(Effect::recovery);

    model.add_virus(virus, assign::count(100));
    
    // Generating a random pop from a smallworld network
    model.add_layer(layer::network(
        "contacts", graph::smallworld(10, .01)
    ));

    // Running the model
    model.run(100, 122);
    model.print();
  
}