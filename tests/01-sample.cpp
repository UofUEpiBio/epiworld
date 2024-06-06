
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
    Virus<int> virus("covid 19", 50, false);
    virus.set_post_immunity(1.0);
    virus.set_status(1,2,3);
    virus.set_prob_death(.01);
    model.add_virus(virus);
    
    epiworld::Tool<int> tool("vaccine");
    model.add_tool(tool, .5);

    // Generating a random pop 
    model.agents_smallworld(10000);

    // Running the model
    model.run(100, 123);
    model.print();

    (void) model.get_db().transition_probability();

    AgentsSample<int> agents(model, .05 * 10000);


    printf_epiworld("Total sampled: %lu\n", agents.size());
    for (auto & a: agents)
        a->print(true);


  
}
