#include <iostream>
#include <omp.h>
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
    int threads                     = result["threads"].as<int>();
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

    // Setup
    std::vector< std::vector< int > > results(nexperiments);
    std::vector< std::vector< int > > date(nexperiments);
    std::vector< std::string > labels;
    int nreplica = -1;

    auto record =
        [&results,&date,&nreplica,&labels](size_t s, epiworld::Model<> * m)
        {

            #pragma omp critical
            nreplica++;

            if (nreplica == 0)
                m->get_db().get_hist_total(&date[nreplica], &labels, &results[nreplica]);
            else
                m->get_db().get_hist_total(&date[nreplica], nullptr, &results[nreplica]);

            return;

        };

    // Running and checking the results
    model.run_multiple(
        ndays,
        nexperiments, // How many experiments
        result["seed"].as<int>(),
        record,       // Function to call after each experiment
        true,         // Whether to reset the population
        true          // Whether to print a progress bar
        #ifdef _OPENMP
        ,threads 
        #endif
    );

    model.print();

    // Writing the results to the disk
    std::ofstream fn("09-sir-connected-experiments.csv", std::ios_base::out);
    fn << "run_id,date,state,counts\n";
    
    for (int r = 0; r < static_cast<int>(nexperiments); ++r)
    {
        for (epiworld_fast_uint s = 0u; s < labels.size(); ++s)
            fn << 
                r << "," << 
                date[r][s] << "," << 
                labels[s] << "," << 
                results[r][s] << "\n";
                
    }

    model.write_data(
        "","","","",
        "total_hist.txt", "transmission.txt", "transition.txt", "", "", "", "", ""
        );


    // We can compute the expected generation time
    auto gen_time = model.generation_time_expected();

    // Compute observed generation interval
    std::vector< int > agent_id;
    std::vector< int > virus_id;
    std::vector< int > time;
    std::vector< int > gentim;
    model.get_db().generation_time(
        agent_id,
        virus_id,
        time,
        gentim
    );

    // Averaging out at the time level
    std::vector< double > gen_time_observed;
    std::vector< double > gen_time_observed_count;

    for (size_t i = 0; i < gentim.size(); i++)
    {
        if (gentim[i] >= 0)
        {
            if (time[i] >= gen_time_observed.size())
            {
                gen_time_observed.resize(time[i] + 1, 0.0);
                gen_time_observed_count.resize(time[i] + 1, 0.0);
            }

            gen_time_observed[time[i]] += gentim[i];
            gen_time_observed_count[time[i]] += 1.0;
        }
    }

    for (size_t i = 0; i < gen_time_observed.size(); i++)
    {
        if (gen_time_observed_count[i] > 0)
            gen_time_observed[i] /= gen_time_observed_count[i];
    }


    return 0;

}

