#include <iostream>
#include <omp.h>
#include "../include/epiworld/models/sir.hpp"

int main() {

    omp_set_num_threads(4);
    std::vector< epiworld::Model<bool> > models(4);
    auto start = std::chrono::high_resolution_clock::now();
#pragma omp parallel shared(models)
{
    epiworld::Model<bool> sir;
    set_up_sir(
        sir,    // Model object
        "a virus", // Name of the virus
        .001,        // Initial prevalence
        .5,        // Efficacy rate
        .5,        // Recovery rate
        .95        // Post immunity
    );

    // Adding a bernoulli graph as step 0
    printf("Generating random graph ...");fflush(stdout);
    sir.pop_from_adjlist(randgraph);
    std::cout << "Done";

    // Initializing and printing
    int nthread = omp_get_thread_num();
    sir.init(100, 123 * nthread);
    // sir.print();
    sir.verbose_off();

    // Running and checking the results
    sir.run();
    #pragma omp critical
    sir.print();
    // models.push_back(sir);
}
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Elapsed time: " <<
        (std::chrono::duration_cast<std::chrono::seconds>(end - start)).count() << 
        " seconds" << std::endl;

    return 0;
}