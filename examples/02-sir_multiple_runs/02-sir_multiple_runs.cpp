#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"
#include "../../include/epiworld/models/sir.hpp"

int main() {

    int total_replicates = 100;

    epiworld::Model<bool> sir;
    set_up_sir(
        sir,       // Model object
        "a virus", // Name of the virus
        0.01,      // Initial prevalence
        1.0,       // Infectiousness
        0.5,       // Efficacy rate
        0.5,       // Recovery rate
        0.95       // Post immunity
    );

    // Adding a bernoulli graph as step 0
    sir.pop_from_adjlist(
        epiworld::rgraph_smallworld(1000, 5, .01, false, sir)
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