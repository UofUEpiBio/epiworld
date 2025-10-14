
// #define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main()
{

    // Creating a model
    epiworld::Model<int> model;

    // Declaring the three states in the model
    model.add_state("Susceptible", epiworld::default_update_susceptible<int>);
    auto exposed_state = model.add_state("Exposed", epiworld::default_update_exposed<int>);
    auto recovered_state = model.add_state("Recovered");
    auto removed_state = model.add_state("Removed");

    // Adding the tool and virus
    epiworld::Virus<int> virus("covid 19");
    virus.set_distribution(
        distribute_virus_randomly<int>(50, false));

    virus.set_post_immunity(1.0);
    virus.set_state(exposed_state, recovered_state, removed_state);
    virus.set_prob_death(.01);
    model.add_virus(virus);

    epiworld::Tool<int> tool("vaccine", .5, true);
    model.add_tool(tool);

    // Generating a random pop
    model.agents_smallworld(10000, 20, false, .01);

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
        "reproductive.txt",
        "generation_time.txt"
        );

}
