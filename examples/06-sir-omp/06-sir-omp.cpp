#include <iostream>
#include <omp.h>
#include "../include/epiworld/models/sir.hpp"

int main() {

    unsigned int nthreads = 4u;
    omp_set_num_threads(nthreads);
    std::vector< epiworld::Model<bool> > models(nthreads);

    for (unsigned int i = 0u; i < nthreads; ++i)
        set_up_sir(
            models[i], // Model object
            "a virus", // Name of the virus
            0.01,      // Initial prevalence
            1.0,       // Infectiousness
            0.5,       // Efficacy rate
            0.5,       // Recovery rate
            0.95       // Post immunity
        );

    // Adding a bernoulli graph as step 0
    printf("Generating random graph... ");fflush(stdout);
    models[0u].pop_from_adjlist(
        epiworld::rgraph_smallworld(250000, 5, .01, false, models[0u])
    );

    for (unsigned i = 1u; i < nthreads; ++i)
        models[i].clone_population(models[0u]);
    printf("done.\n");

    auto start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel shared(models)
    {

        // Initializing and printing
        int nthread = omp_get_thread_num();
        models[nthread].init(100, 123 * nthread);
        // sir.print();
        models[nthread].verbose_off();

        // Running and checking the results
        models[nthread].run();
        
    }
    auto end = std::chrono::high_resolution_clock::now();

    for (auto & m : models)
        m.print();

    for (auto & m : models)
        m.get_elapsed("milliseconds");

    
    std::cout << "Elapsed time: " <<
        (std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count() << 
        " seconds" << std::endl;

    return 0;
}