#include "../../include/epiworld/models/sirconnected.hpp"



int main() {

    epiworld::Model<> model;

    set_up_sir_connected(
        model,     // Model object
        "a virus", // Name of the virus
        0.01,      // Initial prevalence
        2.0,       // Reproductive number
        0.9,       // Prob of transmission
        0.2        // Prob of recovery
    );

    // Adding a bernoulli graph as step 0
    model.pop_from_random(10000, 1, false, .0);
    model.queuing_off(); // No queuing need

    model.init(100, 123);

    // Running and checking the results
    model.run();
    model.print();

    return 0;

}

