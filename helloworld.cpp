#include "epiworld.hpp"

using namespace epiworld;

int main()
{

    // epiworld already comes with a couple
    // of models, like the SIR
    epimodels::ModelSIR hello(
        "COVID-19", // Name of the virus
        100000,     // Number of agents
        0.01,       // Initial prevalence
        0.1,        // Transmission probability
        0.3         // Recovery probability
        );

    // Contacts come from layers: here, a small-world network, drawn anew
    // for every run
    hello.add_layer(layer::network(
        "contacts", graph::smallworld(10, .01)
    ));

    // Running the model and printing the results
    // Setting the number of days (100) and seed (122)
    hello.run(100, 122);
    hello.print();

    return 0;

}
