#include "epiworld.hpp"

using namespace epiworld;

int main()
{

    // epiworld already comes with a couple
    // of models, like the SIR
    epimodels::ModelSIR<> hello(
        "COVID-19", // Name of the virus
        0.01,        // Initial prevalence
        0.9,        // Transmission probability
        0.3         // Recovery probability
        );

    // We can simulate agents using a smallworld network
    // with 100,000 individuals, in this case
    hello.agents_smallworld(100000, 4L, false, .01);

    // Running the model and printing the results
    // Setting the number of days (100) and seed (122)
    hello.run(100, 122);
    hello.print();

    return 0;

}