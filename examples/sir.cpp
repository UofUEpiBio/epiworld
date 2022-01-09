#include <iostream>
#include "../include/epiworld/models/sir.hpp"

int main() {

    epiworld::Model<> model;

    set_up_sir(
        model,    // Model object
        "a virus", // Name of the virus
        .001,        // Initial prevalence
        .5,        // Efficacy rate
        .5,        // Recovery rate
        .95        // Post immunity
    );

    // Adding a bernoulli graph as step 0
    printf("Generating random graph... ");fflush(stdout);
    model.pop_from_adjlist(
        epiworld::rgraph_smallworld(1e6, 5, .001, false, model)
    );
    printf("done.\n");

    auto start = std::chrono::high_resolution_clock::now();

    model.init(100, 123);
    // Running and checking the results
    model.run();
    
    auto end = std::chrono::high_resolution_clock::now();

    model.print();

    model.get_elapsed("milliseconds");

    
    std::cout << "Elapsed time: " <<
        (std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count() << 
        " seconds" << std::endl;

    return 0;
}