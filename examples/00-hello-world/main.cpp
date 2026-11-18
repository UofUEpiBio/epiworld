
// #define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main()
{
    constexpr int n_agents = 5000;
    constexpr int initial_cases = n_agents / 200;

    // Creating a model
    epiworld::Model model(n_agents);

    // The virus and its natural history: infected agents recover or die
    // (competing daily rates); the recovered are immune (Recovered has no
    // infection)
    epiworld::Virus virus("covid 19");
    virus.add_stage("Susceptible");
    virus.add_stage("Infected", stage::infectious);
    virus.add_stage("Recovered");
    virus.add_stage("Removed");
    virus.set_contact(3, 0.0);
    virus.infection(0, 1, 0.5);
    virus.rate(1, 2, 0.5).effect(Effect::recovery);
    virus.rate(1, 3, 0.01).effect(Effect::death);
    model.add_virus(virus, assign::count(initial_cases));

    epiworld::Tool tool("vaccine");
    model.add_tool(tool, assign::share(.5));

    // Generating a random pop
    model.add_layer(layer::network(
        "network", rgraph_smallworld(n_agents, 20, .01, false, model)
    ));

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
        "generation_time.txt",
        "active_cases.txt",
        "outbreak_size.txt",
        ""
        );

}
