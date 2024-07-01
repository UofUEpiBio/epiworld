#include <iostream>
#include <vector>
#include <math.h>
#include <random>
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main()
{

    // Model parameters
    double r   = 5.0; // Contact rate
    double popsize = 5e5;
    double p_contact = r/popsize; // Probability of contact
    double p_i = .05; // Probability of infection
    double p_r = 1.0/7.0; // Probability of recovery

    size_t nsims = 1e6; // Number of simulations
    size_t nsteps = 100; // Number of days

    // Generation intervals: time to first transmission
    std::vector< double > genints;
    genints.reserve(nsims);

    // Random number generator
    std::mt19937 rng;
    std::uniform_real_distribution<double> unif(0.0, 1.0);
    std::binomial_distribution<int> binom(popsize, p_contact);
    rng.seed(0);

    for (size_t i = 0; i < nsims; i++)
    {
        
        for (size_t j = 0; j < nsteps; j++) {

            // Recovers
            if (unif(rng) < p_r)
                break;

            // How many contacts
            int ncontacts = binom(rng);

            // Infects at least 1
            double p = 1.0 - pow(1.0 - p_i, ncontacts);
            if (unif(rng) < p) {
                genints.push_back(j);
                break;
            }
        }
    }


    // Computing the mean
    double mean = 0.0;
    for (auto & genint : genints)
        mean += genint;

    mean /= genints.size();

    epiworld::epimodels::ModelSIRCONN<> model(
        "avirus",
        popsize,
        50.0/popsize,
        r,
        p_i,
        p_r
    );

    model.run(nsteps, 554);
    model.print();

    std::vector<int> agent, virus, time, gentime;
    model.get_db().generation_time(agent, virus, time, gentime);


    // Computing the mean of gentime conditioning on gentime >= 0
    double mean2 = 0.0;
    size_t n = 0;
    for (int i = 0; i < gentime.size()*3/4; i++)
    {
        if (gentime[i] >= 0)
        {
            mean2 += gentime[i];
            n++;
        }
    }

    mean2 /= n;

    // Repeating for ModelSEIRCONN
    epiworld::epimodels::ModelSEIRCONN<> model2(
        "avirus",
        popsize,
        50.0/popsize,
        r,
        p_i,
        0.000009,
        p_r
    );

    model2.run(nsteps, 554);
    model2.print();

    std::vector<int> agent2, virus2, time2, gentime2;
    model2.get_db().generation_time(agent2, virus2, time2, gentime2);


    // Computing the mean of gentime conditioning on gentime >= 0
    double mean_seirconn = 0.0;
    n = 0;
    for (int i = 0; i < gentime2.size()*3/4; i++)
    {
        if (gentime2[i] >= 0)
        {
            mean_seirconn += gentime2[i];
            n++;
        }
    }

    mean_seirconn /= n;

    // Printing both mean and mean2
    std::cout << "Mean  (naive)    : " << mean << std::endl;
    std::cout << "Mean2 (from sim) : " << mean2 << std::endl;
    std::cout << "Mean  (SEIRCONN) : " << mean_seirconn << std::endl;

    return 0;


}