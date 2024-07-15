#include <iostream>
#include <vector>
#include <math.h>
#include <random>
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

// Implementing the factorial function
inline double log_factorial(int n)
{
    if (n == 0)
        return 0.0;
    return std::log(static_cast<double>(n)) + log_factorial(n-1);
}

inline double dpois(int k, double lambda, int max_n = 100, bool as_log = false)
{

    if (max_n < k)
        throw std::runtime_error("max_n must be greater than k");

    double res = k * std::log(lambda) - lambda - log_factorial(
        std::min(k, max_n)
        );
    
    return as_log ? res : std::exp(res);
}

/**
 * @brief Compute the probability of the generation interval
 * 
 * @details
 * If `p_0_approx` is negative, it will be computed using the Poisson
 * distribution.
 * 
 * @param g Generation interval
 * @param S Population size
 * @param p_c Probability of contact
 * @param p_i Probability of infection
 * @param p_r Probability of recovery
 * @param p_0_approx Approximation of the probability of not being infected
 * @param max_n Maximum number of contacts
 * 
 * @return The probability of the generation interval
 * 
 */
double dgenint(
    int g,
    double S,
    double p_c,
    double p_i,
    double p_r,
    double & p_0_approx,
    int max_n = 500
    ) {

    if (p_0_approx < 0.0)
    {

        p_0_approx = 0.0;
        for (int i = 0; i < max_n; ++i)
        {

            p_0_approx +=
                dpois(i, S * p_c, max_n, false) *
                std::pow(1.0 - p_i, static_cast<double>(i)) ;

        }
    }

    double g_dbl = static_cast<double>(g);

    return
        std::pow(1 - p_r, g_dbl) *
        std::pow(p_0_approx, g_dbl - 1.0) * 
        (1.0 - p_0_approx);

}

// Mean of the generation interval
/**
 * @brief Compute the mean of the generation interval
 * @param S Population size
 * @param p_c Probability of contact
 * @param p_i Probability of infection
 * @param p_r Probability of recovery
 * @param max_n Maximum number of contacts
 * 
 * @return The mean of the generation interval
 */
double gen_int_mean(
    double S,
    double p_c,
    double p_i,
    double p_r,
    int max_n = 200
    ) {

    double mean = 0.0;
    double p_0_approx = -1.0;
    for (int i = 0; i < max_n; ++i)
    {
        mean += 
            static_cast<double>(i) *
            dgenint(
                static_cast<double>(i), S, p_c, p_i, p_r, p_0_approx, max_n
                );

        if (i == 0)
            std::cout << "p_0_approx: " << p_0_approx << std::endl;
    }

    return mean;

}

int main()
{

    // Model parameters
    double r   = 5.0; // Contact rate
    double popsize = 1e4;
    double p_contact = r/popsize; // Probability of contact
    double p_i = .1; // Probability of infection
    double p_r = 1.0/15.0; // Probability of recovery

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
        0.00000000009,
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

    double theory_mean = gen_int_mean(popsize, p_contact, p_i, p_r);
    std::cout << "Mean  (theory) +1: " << theory_mean + 1.0 << std::endl;
    

    return 0;


}