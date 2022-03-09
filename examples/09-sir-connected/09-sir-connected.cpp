#include "../../include/epiworld/models/sirconnected.hpp"
#include "../../include/cxxopts/cxxopts.hpp"


int main(int argc, char* argv[]) {

    cxxopts::Options options("SIR connected graph", "SIR ABM with a fully connected graph.");

    options.add_options()
        ("d,days", "Duration in days", cxxopts::value<int>()->default_value("100"))
        ("n,nagents", "Number of agents (pop size)", cxxopts::value<int>()->default_value("20000"))
        ("p,preval", "Prevalence (prop)", cxxopts::value<epiworld_double>()->default_value(".1"))
        ("i,infectprob", "Probability of infection", cxxopts::value<epiworld_double>()->default_value(".95"))
        ("b,beta", "Reproductive number", cxxopts::value<epiworld_double>()->default_value("1.5"))
        ("r,recprob", "Probability of recovery", cxxopts::value<epiworld_double>()->default_value(".1428"))
        ("s,seed", "Pseudo-RNG seed", cxxopts::value<int>()->default_value("123"))
        ;

    auto result = options.parse(argc, argv);

    unsigned int ndays            = result["days"].as<int>();
    unsigned int popsize          = result["nagents"].as<int>();
    epiworld_double preval        = result["preval"].as<epiworld_double>();
    epiworld_double prob_infect   = result["infectprob"].as<epiworld_double>();
    epiworld_double beta          = result["beta"].as<epiworld_double>();
    epiworld_double prob_recovery = result["recprob"].as<epiworld_double>();
    
    epiworld::Model<> model;

    set_up_sir_connected(
        model,        // Model object
        "a virus",    // Name of the virus
        preval,       // Initial prevalence
        beta,         // Reproductive number
        prob_infect,  // Prob of transmission
        prob_recovery // Prob of recovery
    );

    // Adding a bernoulli graph as step 0
    model.pop_from_random(popsize, 1, false, .0);
    model.queuing_off(); // No queuing need

    model.init(ndays, result["seed"].as<int>());

    // Running and checking the results
    model.run();
    model.print();

    model.write_data("","","total_hist.txt", "transmission.txt", "transition.txt");

    return 0;

}

