// #define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

int main()
{
  // Creating a model
  epiworld::Model<> model;

  model.add_state("Susceptible", epiworld::default_update_susceptible<>);
  auto infected_state = model.add_state("Infected", epiworld::default_update_exposed<>);
  auto removed_state = model.add_state("Removed");

  // Creating a virus
  epiworld::Virus<> covid19("covid 19", 0.05, true);
  covid19.set_prob_infecting(0.8);
  covid19.set_state(infected_state, removed_state, removed_state);

  // Creating a tool
  epiworld::Tool<> vax("vaccine", .5, true);
  vax.set_susceptibility_reduction(.95);

  // Adding the tool and virus
  model.add_virus(covid19);

  model.add_tool(vax);

  // Generating a random pop
  model.agents_from_adjlist(
    epiworld::rgraph_smallworld(1000, 5, 0.01, false, model)
  );

  // Running the model
  model.run(100, 123123);

  model.print();

  return 0;

}
