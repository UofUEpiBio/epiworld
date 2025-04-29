#include <omp.h>

// #define EPI_DEBUG

#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main(int argc, char * argv[]) {

    auto nthreads = 4;
    auto nsims    = 100;
    if (argc == 3)
    {
        nthreads = strtol(argv[1], nullptr, 0);
        nsims    = strtol(argv[2], nullptr, 0);
    }

    omp_set_num_threads(nthreads);
    epimodels::ModelSIR<> model(
        "a virus", // Name of the virus
        0.01,      // Initial prevalence
        0.9,       // Infectiousness
        0.3        // Recovery rate
    );

    model.agents_from_adjlist(
        rgraph_smallworld(100000, 4, .01, false, model)
    );

    auto sav = make_save_run<>(std::string("%03lu-episim.txt"));
    model.run_multiple(100, nsims, 222, sav, true, true, nthreads);

    model.print();

    return 0;
}
