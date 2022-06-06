
#define EPI_DEBUG
#include "../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main()
{

    // Creating a model
    Model<int> model;

    // Declaring the three statuses in the model
    model.add_status("Susceptible", epiworld::default_update_susceptible<int>);
    model.add_status("Exposed", epiworld::default_update_exposed<int>);
    model.add_status("Recovered");
    model.add_status("Removed");

    // Adding the tool and virus
    Virus<int> virus("covid 19");
    virus.set_post_immunity(1.0);
    virus.set_status(1,2,3);
    virus.set_prob_death(.01);
    model.add_virus_n(virus, 50);
    
    epiworld::Tool<int> tool("vaccine");
    model.add_tool(tool, .5);

    // Generating a random pop 
    model.agents_smallworld(10000);

    // Initializing setting days and seed
    model.init(100, 123);

    // Running the model
    model.run();
    model.print();

    (void) model.get_db().transition_probability();

    Agents<int> agents(model);
    agents.sample(.05);


    printf_epiworld("Total sampled: %lu\n", agents.size());
    for (auto & a: agents)
        a->print(true);


  
}
