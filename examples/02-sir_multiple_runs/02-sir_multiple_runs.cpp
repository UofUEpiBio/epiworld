#include "../../include/epiworld/epiworld.hpp"
#include <iostream>

using namespace epiworld;

int main() {
    int total_replicates = 100;

    epimodels::ModelSIR<> sir(
        "a virus", // Name of the virus
        0.01, // Initial prevalence
        0.9, // Infectiousness
        0.3 // Recovery rate
    );

    // Adding a bernoulli graph as step 0
    sir.agents_from_adjlist(rgraph_smallworld(1000, 5, .01, false, sir));

    // Running and checking the results
    std::vector<std::vector<int>> results(total_replicates);
    std::vector<std::string> labels;
    sir.set_backup();
    sir.verbose_off();

    sir.seed(123);
    for (int r = 0; r < total_replicates; ++r) {
        sir.run(60);
        sir.get_db().get_today_total(&labels, &results[r]);
        sir.reset();
        std::cout << "Replicate " << r << " done" << std::endl;
    }

    sir.get_elapsed();

    for (epiworld_fast_uint s = 0u; s < labels.size(); ++s)
        printf("%s, ", labels[s].c_str());
    printf("\n");

    for (epiworld_fast_uint r = 0u; r < 10; ++r) {
        for (epiworld_fast_uint s = 0u; s < labels.size(); ++s)
            printf("%i, ", results[r][s]);

        printf("\n");
    }

    return 0;
}