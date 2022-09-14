#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;
 
int main() {

    epimodels::ModelSEIR<> model(
        "a virus", // Name of the virus
        0.01,      // Initial prevalence
        0.9,       // Infectiousness
        7,       // Incubation
        0.5        // Recovery rate
    );

    // Adding a bernoulli graph as step 0
    model.agents_from_adjlist(
        rgraph_smallworld(500000, 5, .001, false, model)
    );

    model.init(50, 123);

    // Running and checking the results
    model.run();
    model.print();

    return 0;

}
