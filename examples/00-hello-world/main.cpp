
#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main()
{

    // Creating a model
    epiworld::Model<int> model;

    // Declaring the three states in the model
    model.add_state("Susceptible", epiworld::default_update_susceptible<int>);
    model.add_state("Exposed", epiworld::default_update_exposed<int>);
    model.add_state("Recovered");
    model.add_state("Removed");

    // Adding the tool and virus
    epiworld::Virus<int> virus("covid 19");
    virus.set_post_immunity(1.0);
    virus.set_state(1,2,3);
    virus.set_prob_death(.01);
    model.add_virus_n(virus, 50);
    
    epiworld::Tool<int> tool("vaccine");
    model.add_tool(tool, .5);

    // Generating a random pop 
    model.agents_smallworld(10000);

    // Running the model
    model.run(100, 123);
    model.print();

    // Writing the data
    model.write_data(
        "variant_info.txt",
        "variant_hist.txt",
        "tool_info.txt",
        "tool_hist.txt",
        "total_hist.txt",
        "transmissions.txt",
        "transitions.txt",
        "reproductive.txt"
        );
  
}
