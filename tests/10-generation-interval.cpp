#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Generation interval", "[gen-int]")
{

    // Overall parameters
    static const double contact_rate = 100;
    static const int S = 10000;
    static const size_t max_days = 400;
    static const size_t max_contacts = S/2;
    static const double p_r = 1.0/7.0;
    static const double p_i = 0.05;

    // Derived parameters
    static const double p_c = contact_rate/static_cast<double>(S);

    // Creating model engine
    epimodels::ModelSIRCONN<> model(
        "a virus", S, p_r, contact_rate, p_i, p_r
    );

    model.seed(3123);
    
    // Building a simple simulation for SIR connected model
    size_t nsims = 500000u;
    std::vector< size_t > sim_days;
    for (size_t i = 0u; i < nsims; ++i)
    {

        // Iterating through days
        for (size_t j = 0u; j < max_days; ++j)
        {
            // Recovery?
            if (model.runif() < p_r)
                break;

            // Sampling individuals
            int ncontacts = model.rbinom(S, p_c);

            if (ncontacts == 0)
                continue;

            // Infecting at least one individual
            double p_at_least_one = 1.0 - std::pow(1.0 - p_i, ncontacts);

            if (model.runif() < p_at_least_one)
            {
                sim_days.push_back(j);
                break;
            }

        }
        
    }

    // Computing the mean
    double mean = std::accumulate(sim_days.begin(), sim_days.end(), 0.0)/static_cast<double>(sim_days.size());

    // Using the math model
    double expected_mean = epiworld::gen_int_mean(
        S, p_c, p_i, p_r, max_days, max_contacts
    );

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}