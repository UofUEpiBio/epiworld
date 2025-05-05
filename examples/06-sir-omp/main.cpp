#ifdef _OPENMP
#include <omp.h>
#endif

#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main() {

    epiworld_fast_uint nthreads = 4u;
    omp_set_num_threads(nthreads);
    std::vector< epimodels::ModelSIR<> > Models;

    for (epiworld_fast_uint i = 0u; i < nthreads; ++i)
        Models.push_back(epimodels::ModelSIR<>(
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
        Models[i] = Models[0u];
    printf("done.\n");

    auto start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel shared(Models)
    {

        // Initializing and printing
        int nthread = omp_get_thread_num();
        // sir.print();
        Models[nthread].verbose_off();

        // Running and checking the results
        Models[nthread].run(100, 123 * nthread);
        
    }
    auto end = std::chrono::high_resolution_clock::now();

    for (auto & m : Models)
        m.print();

    for (auto & m : Models)
        m.get_elapsed("milliseconds");

    
    printf(
        "Elapsed time: %lld milliseconds\n",
        (long long int)(std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count()
    );

    return 0;
}
