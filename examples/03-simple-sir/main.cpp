// #define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

int main()
{
  // Creating a model
  epiworld::Model model(1000);

  // Creating a virus, with its natural history
  epiworld::Virus covid19("covid 19");
  covid19.add_stage("Susceptible");
  covid19.add_stage("Infected", epiworld::stage::infectious);
  covid19.add_stage("Removed");
  covid19.infection(0, 1, 0.8);
  covid19.rate(1, 2, 0.5).effect(epiworld::Effect::recovery);

  // Creating a tool
  epiworld::Tool vax("vaccine");
  vax.set_susceptibility_reduction(.95);

  // Adding the tool and virus
  model.add_virus(covid19, epiworld::assign::share(0.05));

  model.add_tool(vax, epiworld::assign::share(.5));

  // Generating a random pop
  model.add_layer(epiworld::layer::network(
      "network", epiworld::rgraph_smallworld(1000, 5, 0.01, false, model)
  ));

  // Running the model
  model.run(100, 123123);

  model.print();

  return 0;

}
