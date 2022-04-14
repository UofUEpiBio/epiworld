#include "../../include/epiworld/epiworld.hpp"
#include "../../include/epiworld/models/seirconnected.hpp"
#include "../../include/cxxopts/cxxopts.hpp"

int main(int argc, char* argv[]) {

    cxxopts::Options options("SEIR connected graph", "SEIR ABM with a fully connected graph.");

    options.add_options()
        ("d,days", "Duration in days", cxxopts::value<int>()->default_value("100"))
        ("n,nagents", "Number of agents (pop size)", cxxopts::value<int>()->default_value("20000"))
        ("p,preval", "Prevalence (prop)", cxxopts::value<epiworld_double>()->default_value(".1"))
        ("i,infectprob", "Probability of infection", cxxopts::value<epiworld_double>()->default_value(".95"))
        ("b,beta", "Instantaneous contagion rate (beta)", cxxopts::value<epiworld_double>()->default_value("1.5"))
        ("r,recprob", "Probability of recovery", cxxopts::value<epiworld_double>()->default_value(".1428"))
        ("l,latency", "Avg. incubation (latency) days", cxxopts::value<epiworld_double>()->default_value("7.0"))
        ("s,seed", "Pseudo-RNG seed", cxxopts::value<int>()->default_value("123"))
        ("e,experiments", "Number of experiments", cxxopts::value<int>()->default_value("2"))
        ("h,help", "Print usage")
        ;

    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
      std::cout << options.help() << std::endl;
      exit(0);
    }

    unsigned int ndays              = result["days"].as<int>();
    unsigned int popsize            = result["nagents"].as<int>();
    epiworld_double preval          = result["preval"].as<epiworld_double>();
    epiworld_double prob_infect     = result["infectprob"].as<epiworld_double>();
    epiworld_double beta            = result["beta"].as<epiworld_double>();
    epiworld_double incubation_days = result["latency"].as<epiworld_double>();
    epiworld_double prob_recovery   = result["recprob"].as<epiworld_double>();
    unsigned int nexperiments       = result["experiments"].as<int>();
    
    epiworld::Model<> model;

    set_up_seir_connected(
        model,           // Model object
        "a virus",       // Name of the virus
        preval,          // Initial prevalence
        beta,            // Reproductive number
        prob_infect,     // Prob of transmission
        incubation_days, // Number of incubation days
        prob_recovery    // Prob of recovery
    );

    // Adding a bernoulli graph as step 0
    model.pop_from_random(popsize, 1, false, .0);

    model.init(ndays, result["seed"].as<int>());

    // Setup
    std::vector< std::vector< int > > results(nexperiments);
    std::vector< std::vector< int > > dates(nexperiments);
    std::vector< std::string > labels;
    unsigned int nreplica = 0u;

    auto record =
        [&results,&dates,&nreplica,&labels](epiworld::Model<bool> * m)
        {

            if (nreplica == 0)
                m->get_db().get_hist_total(&dates[nreplica], &labels, &results[nreplica]);
            else
                m->get_db().get_hist_total(&dates[nreplica], nullptr, &results[nreplica]);

            nreplica++;

            return;

        };

    // Running and checking the results
    model.run_multiple(
        nexperiments, // How many experiments
        record,           // Function to call after each experiment
        true,             // Whether to reset the population
        true              // Whether to print a progress bar
    );

    model.print();

    // Writing the results to the disk
    std::ofstream fn("09-seir-connected-experiments.csv", std::ios_base::out);
    fn << "run_id,date,status,counts\n";
    
    for (int r = 0; r < static_cast<int>(nexperiments); ++r)
    {
        for (unsigned int s = 0u; s < labels.size(); ++s)
            fn << 
                r << "," << 
                dates[r][s] << "," << 
                labels[s] << "," << 
                results[r][s] << "\n";
                
    }

    model.write_data("","","total_hist.txt", "transmission.txt", "transition.txt");

    return 0;

}

