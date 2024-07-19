#include "tests.hpp"

double p_0_approx_sim_global;
double p_0_approx_analy_global;


using namespace epiworld;

EPIWORLD_TEST_CASE("Generation interval", "[gen-int]")
{

    // Overall parameters
    static const double contact_rate = 10;
    static const int S = 10000;
    static const size_t max_days = 200;
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
    size_t nsims = 50000u;
    std::vector< size_t > sim_days;

    double p_0 = 0.0;
    double n_p_0 = 0.0;

    std::vector< double > distribution(max_days, 0.0);
    for (size_t i = 0u; i < nsims; ++i)
    {

        // Iterating through days
        for (size_t j = 0u; j < max_days; ++j)
        {
            // Recovery?
            if (model.runif() < p_r)
            {
                break;
            }

            // Sampling individuals
            int ncontacts = model.rbinom(S, p_c);

            n_p_0 += 1.0;
            p_0 += 1.0;
            if (ncontacts == 0)
            {
                continue;
            }

            // Infecting at least one individual
            double p_at_least_one = 1.0 - std::pow(1.0 - p_i, ncontacts);

            if (model.runif() < p_at_least_one)
            {
                p_0 -= 1.0;
                sim_days.push_back(j + 1);
                distribution[j] += 1.0;
                break;
            }

        }
        
    }

    p_0_approx_sim_global = p_0/n_p_0;


    // Computing the mean
    double mean = std::accumulate(sim_days.begin(), sim_days.end(), 0.0)/static_cast<double>(sim_days.size());

    // Using the math model
    double expected_mean = epiworld::gen_int_mean(
        S, p_c, p_i, p_r, max_days, max_contacts
    );

    // Checking the mean
    for (auto & d : distribution)
    {
        d /= static_cast<double>(sim_days.size());
    }

    std::vector< double > distribution_expected(max_days * 5, 0.0);
    p_0_approx_analy_global = -1.0;
    double normalizing_constant = -1.0;
    for (size_t i = 0u; i < (max_days * 5); ++i)
    {
        distribution_expected[i] = epiworld::dgenint(
            i + 1.0, S, p_c, p_i, p_r, p_0_approx_analy_global, 
            normalizing_constant, max_contacts, max_days
        );
    }

    // Printing out the means
    printf("Mean GI (simulated)  : %.4f\n", mean);
    printf("Mean GI (expected)   : %.4f\n", expected_mean);
    printf("Ratio (sim/expected) : %.4f\n", mean/expected_mean);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(moreless(mean/expected_mean, 1.00, 0.01));
    #endif

    // Printing out the first 20 of the distribution
    printf("         Expected | Simulated | AbsDiff\n");
    for (size_t i = 0u; i < 15u; ++i)
    {
        printf(
            "P(G=%2i) = %.4f  | %.4f    | %.4f\n",
            static_cast<int>(i),
            distribution_expected[i],
            distribution[i],
            std::abs(distribution_expected[i] - distribution[i])
            );

        #ifdef CATCH_CONFIG_MAIN
        REQUIRE_FALSE(moreless(distribution_expected[i], distribution[i], 0.01));
        #endif
    }

    
    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}