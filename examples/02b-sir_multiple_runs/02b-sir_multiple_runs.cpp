#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"
#include "../../include/epiworld/models/sir.hpp"

int main() {

    int total_replicates = 100;
    int pop_size         = 10000;

    epiworld::Model<> sir;
    set_up_sir(
        sir,    // Model object
        "a virus", // Name of the virus
        0.01,      // Initial prevalence
        1.0,       // Infectiousness
        0.5,       // Efficacy rate
        0.5,       // Recovery rate
        0.95       // Post immunity
    );

    // Adding a bernoulli graph as step 0
    sir.pop_from_adjlist(
        epiworld::rgraph_smallworld(pop_size, 5, .01, false, sir)
    );

    // Initializing and printing
    sir.init(60, 123);
    sir.print();

    // Function to record each replicate results 
    std::vector< std::vector< int > > results(total_replicates);
    std::vector< std::string > labels;
    unsigned int nreplica = 0u;

    auto record =
        [&results,&nreplica,&labels](epiworld::Model<bool> * m)
        {

            if (nreplica == 0u)
                m->get_db().get_today_total(nullptr, &results[nreplica++]);
            else
                m->get_db().get_today_total(&labels, &results[nreplica++]);

            return;

        };

    // Running
    sir.run_multiple(
        total_replicates, // How many experiments
        record,           // Function to call after each experiment
        true,             // Whether to reset the population
        true              // Whether to print a progress bar
    );

    // How long did it took?
    sir.get_elapsed();

    // Writing the results to the disk
    std::ofstream fn("02b-sir-multiple-runs.csv", std::ios_base::out);
    fn << "run_id";
    for (unsigned int s = 0u; s < labels.size(); ++s)
        fn << "," << labels[s];
    
    fn << "\n";

    for (int r = 0; r < total_replicates; ++r)
    {
        fn << r;
        for (unsigned int s = 0u; s < labels.size(); ++s)
            fn << "," << results[r][s];

        fn << "\n";

    }

    return 0;
}
