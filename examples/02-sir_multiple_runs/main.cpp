#include "../../include/epiworld/epiworld.hpp"
#include <iostream>

using namespace epiworld;

int main() {

    int total_replicates = 100;

    Model<> sir = models::sir(
        "a virus", // Name of the virus
        0.01,      // Initial prevalence
        1.0,       // Infectiousness
        0.5,       // Efficacy rate
        0.5,       // Recovery rate
        0.95       // Post immunity
    );

    // Adding a bernoulli graph as step 0
    sir.agents_from_adjlist(
        rgraph_smallworld(1000, 5, .01, false, sir)
    );

    // Initializing and printing
    sir.init(60, 123);
    sir.print();

    // Running and checking the results
    std::vector< std::vector< int > > results(total_replicates);
    std::vector< std::string > labels;
    sir.set_backup();
    sir.verbose_off();

    for (int r = 0; r < total_replicates; ++r)
    {
        sir.run();
        sir.get_db().get_today_total(&labels, &results[r]);
        sir.reset();
        std::cout << "Replicate " << r << " done" << std::endl;
    }
    
    sir.get_elapsed();

    for (unsigned int s = 0u; s < labels.size(); ++s)
        printf("%s, ",labels[s].c_str());
    printf("\n");

    for (unsigned int r = 0u; r < 10; ++r)
    {
        for (unsigned int s = 0u; s < labels.size(); ++s)
            printf("%i, ", results[r][s]);

        printf("\n");
    }

    return 0;
}