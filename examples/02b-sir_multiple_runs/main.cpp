#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main() {
    int total_replicates = 100;
    int pop_size = 10000;

    epimodels::ModelSIR<> sir(
        "a virus", // Name of the virus
        0.01, // Initial prevalence
        0.9, // Infectiousness
        0.3 // Recovery rate
    );

    // Adding a bernoulli graph as step 0
    sir.agents_from_adjlist(rgraph_smallworld(pop_size, 5, .01, false, sir));

    // Function to record each replicate results
    std::vector<std::vector<int>> results(total_replicates);
    std::vector<std::string> labels;

    auto record = [&results, &labels](size_t iternum, epiworld::Model<>* m) {
        if (iternum == 0u)
            m->get_db().get_today_total(nullptr, &results[iternum]);
        else
            m->get_db().get_today_total(&labels, &results[iternum]);

        return;
    };

    // Running
    sir.run_multiple(
        60,
        total_replicates, // How many experiments
        123,
        record, // Function to call after each experiment
        true, // Whether to reset the population
        true // Whether to print a progress bar
    );

    sir.print();

    // How long did it took?
    sir.get_elapsed();

    // Writing the results to the disk
    std::ofstream fn("02b-sir-multiple-runs.csv", std::ios_base::out);
    fn << "run_id";
    for (epiworld_fast_uint s = 0u; s < labels.size(); ++s)
        fn << "," << labels[s];

    fn << "\n";

    for (int r = 0; r < total_replicates; ++r) {
        fn << r;
        for (epiworld_fast_uint s = 0u; s < labels.size(); ++s)
            fn << "," << results[r][s];

        fn << "\n";
    }

    return 0;
}
