#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main() {

    epimodels::ModelSIR<> model(
        "a virus", // Name of the virus
        0.01,      // Initial prevalence
        0.9,       // Infectiousness
        0.5       // Recovery rate
    );

    // Adding a bernoulli graph as step 0
    model.agents_from_adjlist(
        rgraph_smallworld(50000, 20, .01, false, model)
    );

    // Running and checking the results
    model.run(50, 123);
    model.print();

    return 0;

}
