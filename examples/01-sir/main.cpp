// #define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"
// #include "../../include/epiworld/models/sir.hpp"

using namespace epiworld;

int main() {

    epiworld::Model<> model = models::sir(
        "a virus", // Name of the virus
        0.01,      // Initial prevalence
        0.9,       // Infectiousness
        0.5       // Recovery rate
    );

    // Adding a bernoulli graph as step 0
    model.agents_from_adjlist(
        epiworld::rgraph_smallworld(10000, 5, .001, false, model)
    );

    model.init(100, 123);

    // Running and checking the results
    model.run();
    model.print();

    return 0;

}