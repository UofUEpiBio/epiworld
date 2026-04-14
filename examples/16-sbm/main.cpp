#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main() {

    // Creating an SIR model
    epimodels::ModelSIR<> model(
        "a virus", // Name of the virus
        0.01,      // Initial prevalence
        0.3,       // Transmission rate
        0.5        // Recovery rate
    );

    // Defining three population groups with different mixing patterns.
    // The mixing matrix M(g,h) controls the expected number of connections
    // agents in group g have with agents in group h.
    //
    // Row sums give the expected degree per group (when the balance
    // condition M(g,h)*n_g = M(h,g)*n_h holds):
    //   group 0 (urban):    4.5 + 3.0 + 2.5 = 10
    //   group 1 (suburban): 2.0 + 5.5 + 2.5 = 10
    //   group 2 (rural):    1.0 + 1.5 + 7.5 = 10
    //
    // Notice:
    //   - Urban agents mix more between groups.
    //   - Rural agents mostly connect within their own group.
    std::vector<size_t> block_sizes = {2000, 3000, 5000};
    std::vector<double> mixing_matrix = {
        4.5, 3.0, 2.5,
        2.0, 5.5, 2.5,
        1.0, 1.5, 7.5
    };

    model.agents_sbm(block_sizes, mixing_matrix, true);

    // Running and checking the results
    model.run(50, 123);
    model.print();

    return 0;

}
