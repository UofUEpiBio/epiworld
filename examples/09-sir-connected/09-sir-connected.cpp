// #define EPI_DEBUG
#include <iostream>
#include "../../include/epiworld/epiworld.hpp"
#include "../../include/cxxopts/cxxopts.hpp"
using namespace epiworld;

int main(int argc, char* argv[]) {

    cxxopts::Options options("SIR connected graph", "SIR ABM with a fully connected graph.");

    options.add_options()
        ("d,days", "Duration in days", cxxopts::value<int>()->default_value("100"))
        ("n,nagents", "Number of agents (pop size)", cxxopts::value<int>()->default_value("20000"))
        ("p,preval", "Prevalence (prop)", cxxopts::value<epiworld_double>()->default_value(".1"))
        ("i,infectprob", "Probability of infection", cxxopts::value<epiworld_double>()->default_value(".95"))
        ("b,beta", "Instantaneous contagion rate (beta)", cxxopts::value<epiworld_double>()->default_value("1.5"))
        ("r,recprob", "Probability of recovery", cxxopts::value<epiworld_double>()->default_value(".1428"))
        ("s,seed", "Pseudo-RNG seed", cxxopts::value<int>()->default_value("123"))
        ("e,experiments", "Number of experiments", cxxopts::value<int>()->default_value("4"))
        ("t,threads", "Number of threads", cxxopts::value<int>()->default_value("2"))
        ("h,help", "Print usage")
        ;

    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
      std::cout << options.help() << std::endl;
      exit(0);
    }

    epiworld_fast_uint ndays        = result["days"].as<int>();
    epiworld_fast_uint popsize      = result["nagents"].as<int>();
    epiworld_double preval          = result["preval"].as<epiworld_double>();
    epiworld_double prob_infect     = result["infectprob"].as<epiworld_double>();
    epiworld_double beta            = result["beta"].as<epiworld_double>();
    epiworld_double prob_recovery   = result["recprob"].as<epiworld_double>();
    int threads = result["threads"].as<int>();
    epiworld_fast_uint nexperiments = result["experiments"].as<int>(); 

    epiworld::epimodels::ModelSIRCONN<> model(
        "a virus",    // Name of the virus
        popsize,
        preval,       // Initial prevalence
        beta,         // Reproductive number
        prob_infect,  // Prob of transmission
        prob_recovery // Prob of recovery
    );

    // // Adding a bernoulli graph as step 0
    // model.agents_smallworld(popsize, 1, false, .0);

    model.init(ndays, result["seed"].as<int>());

    // Setup
    std::vector< std::vector< int > > results(nexperiments);
    std::vector< std::vector< int > > dates(nexperiments);
    std::vector< std::string > labels;
    epiworld_fast_uint nreplica = 0u;

    auto record =
        [&results,&dates,&nreplica,&labels](size_t s, epiworld::Model<> * m)
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
        record,       // Function to call after each experiment
        true,         // Whether to reset the population
        true         // Whether to print a progress bar
        #ifdef _OPENMP
        ,threads 
        #endif
    );

    model.print();

    // Writing the results to the disk
    std::ofstream fn("09-sir-connected-experiments.csv", std::ios_base::out);
    fn << "run_id,date,status,counts\n";
    
    for (int r = 0; r < static_cast<int>(nexperiments); ++r)
    {
        for (epiworld_fast_uint s = 0u; s < labels.size(); ++s)
            fn << 
                r << "," << 
                dates[r][s] << "," << 
                labels[s] << "," << 
                results[r][s] << "\n";
                
    }

    model.write_data("","","","","total_hist.txt", "transmission.txt", "transition.txt", "");

    return 0;

}

