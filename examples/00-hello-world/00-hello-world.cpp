
#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main()
{

    // Creating a model
    Model<> model;

    // Adding the tool and virus
    Virus<> virus("covid 19");
    virus.set_post_immunity(1.0);
    model.add_virus_n(virus, 50);
    
    Tool<> tool("vaccine");
    model.add_tool(tool, .5);

    // Generating a random pop 
    model.pop_from_random(10000);

    // Initializing setting days and seed
    model.init(100, 123);

    // Running the model
    model.run();
    model.print();

    // Writing the data
    model.write_data(
        "variant_info.txt",
        "variant_hist.txt",
        "total_hist.txt",
        "transmissions.txt",
        "transitions.txt"
        );
  
}
