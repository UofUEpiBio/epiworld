
#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main()
{

    // Creating a model
    epiworld::Model<int> model;

    // Declaring the three statuses in the model
    model.add_status("Susceptible", epiworld::default_update_susceptible<int>);
    model.add_status("Exposed", epiworld::default_update_exposed<int>);
    model.add_status("Removed");

    // Adding the tool and virus
    epiworld::Virus<int> virus("covid 19");
    virus.set_post_immunity(1.0);
    virus.set_status(1,2,2);
    model.add_virus_n(virus, 50);
    
    epiworld::Tool<int> tool("vaccine");
    model.add_tool(tool, .5);

    // Generating a random pop 
    model.population_smallworld(10000);

    // Initializing setting days and seed
    model.init(100, 123);

    // Running the model
    model.run();
    model.print();

    // Writing the data
    model.write_data(
        "variant_info.txt",
        "variant_hist.txt",
        "tool_info.txt",
        "tool_hist.txt",
        "total_hist.txt",
        "transmissions.txt",
        "transitions.txt"
        );
  
}
