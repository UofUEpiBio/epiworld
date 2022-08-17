#include <omp.h>

#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main() {

    unsigned int nthreads = 4u;
    omp_set_num_threads(nthreads);
    std::vector< models::ModelSIR<> > Models;

    for (unsigned int i = 0u; i < nthreads; ++i)
        Models.push_back(models::ModelSIR<>(
            "a virus", // Name of the virus
            0.01,      // Initial prevalence
            0.9,       // Infectiousness
            0.3        // Recovery rate
        ));

    // Adding a bernoulli graph as step 0
    printf("Generating random graph... ");fflush(stdout);
    Models[0u].agents_from_adjlist(
        rgraph_smallworld(250000, 5, .01, false, Models[0u])
    );

    for (unsigned i = 1u; i < nthreads; ++i)
        Models[i].clone_population(Models[0u]);
    printf("done.\n");

    auto start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel shared(Models)
    {

        // Initializing and printing
        int nthread = omp_get_thread_num();
        Models[nthread].init(100, 123 * nthread);
        // sir.print();
        Models[nthread].verbose_off();

        // Running and checking the results
        Models[nthread].run();
        
    }
    auto end = std::chrono::high_resolution_clock::now();

    for (auto & m : Models)
        m.print();

    for (auto & m : Models)
        m.get_elapsed("milliseconds");

    
    printf(
        "Elapsed time: %ld milliseconds\n",
        (std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count()
    );

    return 0;
}
