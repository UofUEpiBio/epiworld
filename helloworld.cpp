#include "epiworld.hpp"

using namespace epiworld;

int main()
{

    // epiworld already comes with a couple
    // of models, like the SIR
    Model<> hello = models::sir(
        "COVID-19", // Name of the virus
        0.1,        // Initial prevalence
        0.9,        // Transmission probability
        0.3         // Recovery probability
        );

    // We can simulate agents using a smallworld network
    // with 100,000 individuals, in this case
    hello.agents_smallworld(1000);

    // Setting the number of days (100) and seed (122)
    hello.init(100, 122);

    // Running the model and printing the results
    hello.run();
    hello.print();

    return 0;

}