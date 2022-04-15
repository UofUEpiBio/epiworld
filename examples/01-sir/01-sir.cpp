#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"
#include "../../include/epiworld/models/sir.hpp"

int main() {

    epiworld::Model<> model;

    set_up_sir(
        model,     // Model object
        "a virus", // Name of the virus
        0.01,      // Initial prevalence
        1.0,       // Infectiousness
        0.5,       // Efficacy rate
        0.5,       // Recovery rate
        0.95       // Post immunity
    );

    // Adding a bernoulli graph as step 0
    model.pop_from_adjlist(
        epiworld::rgraph_smallworld(100000, 5, .001, false, model)
    );

    model.init(100, 123);

    // Running and checking the results
    model.run();
    model.print();

    return 0;

}