#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main() {
    epimodels::ModelSIS<> model(
        "a virus", // Name of the virus
        0.01, // Initial prevalence
        0.9, // Infectiousness
        0.5 // Recovery rate
    );

    // Adding a bernoulli graph as step 0
    model.agents_from_adjlist(rgraph_smallworld(10000, 5, .001, false, model));

    // Running and checking the results
    model.run(100, 123);
    model.print();

    return 0;
}