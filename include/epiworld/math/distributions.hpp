#ifndef EPIWORLD_MATH_DISTRIBUTIONS_HPP
#define EPIWORLD_MATH_DISTRIBUTIONS_HPP

// Implementing the factorial function
/**
 * @brief Compute the log of the factorial
 * 
 * @param n Number
 * 
 * @return The log of the factorial
 */
inline double log_factorial(int n)
{
    if (n == 0)
        return 0.0;
    return std::log(static_cast<double>(n)) + log_factorial(n-1);
}

/**
 * @brief Compute the Poisson probability
 * 
 * @param k Number of events
 * @param lambda Rate
 * @param max_n Maximum number of events
 * @param as_log Return the log of the probability
 * 
 * @return The Poisson probability
 */
inline double dpois(
    int k,
    double lambda,
    int max_n = 100,
    bool as_log = false
    )
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
inline double dgenint(
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

            p_0_approx += std::exp(
                dpois(i, S * p_c, max_n, true) +
                std::log(1.0 - p_i) * static_cast<double>(i)
                );

        }
    }

    double g_dbl = static_cast<double>(g);

    return std::exp(
        std::log(1 - p_r) * g_dbl +
        std::log(p_0_approx) * (g_dbl - 1.0) +
        std::log(1.0 - p_0_approx)
        );

}

// Mean of the generation interval
/**
 * @brief Compute the mean of the generation interval
 * @param S Population size.
 * @param p_c Probability of contact.
 * @param p_i Probability of infection.
 * @param p_r Probability of recovery.
 * @param max_days Maximum number of days.
 * @param max_n Maximum number of contacts.
 * 
 * @return The mean of the generation interval
 */
inline double gen_int_mean(
    double S,
    double p_c,
    double p_i,
    double p_r,
    int max_days = 200,
    int max_n = 200
    ) {

    double mean = 0.0;
    double p_0_approx = -1.0;
    for (int i = 1; i < max_days; ++i)
    {
        mean += 
            static_cast<double>(i) *
            dgenint(
                i, S, p_c, p_i, p_r, p_0_approx, max_n
                );

    }

    return mean;

}

#endif