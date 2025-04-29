#include "tests.hpp"

inline epiworld_double calc_mean(const std::vector<epiworld_double> & x)
{
    return std::accumulate(x.begin(), x.end(), 0.0)/static_cast<epiworld_double>(x.size());
}

inline epiworld_double calc_variance(const std::vector<epiworld_double> & x)
{
    epiworld_double mean = calc_mean(x);
    return std::accumulate(
        x.begin(), x.end(), 0.0,
        [mean](epiworld_double acc, epiworld_double y) {
            return acc + (y - mean)*(y - mean);
        }
        )/(static_cast<epiworld_double>(x.size()) - 1);
}

using namespace epiworld;

EPIWORLD_TEST_CASE("Random numbers", "[rand-nums]")
{

    Model<> model;
    model.seed(3312);

    // Generating random numbers
    size_t n = 100000u;
    std::vector< epiworld_double > num_normals(n);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rnorm();

    // Computing the mean and variance
    epiworld_double m_norm = calc_mean(num_normals);
    epiworld_double v_norm = calc_variance(num_normals);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(moreless(m_norm, 0.00, 0.025));
    REQUIRE_FALSE(moreless(v_norm, 1.00, 0.025));
    #endif

    // Repeating with runif
    model.set_rand_unif(-1.0, 1.0);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.runif();

    // Computing the mean and variance
    epiworld_double m_unif = calc_mean(num_normals);
    epiworld_double v_unif = calc_variance(num_normals);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(moreless(m_unif, 0.00, 0.025));
    REQUIRE_FALSE(moreless(v_unif, 4.0/12.0, 0.025));
    #endif

    // Now with gamma
    model.set_rand_gamma(1.5, 2.0);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rgamma();

    // Computing the mean and variance
    epiworld_double m_gamma = calc_mean(num_normals);
    epiworld_double v_gamma = calc_variance(num_normals);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(moreless(m_gamma, 1.5*2.0, 0.025));
    REQUIRE_FALSE(moreless(v_gamma, 1.5*2.0*2.0, 0.025));
    #endif
    
    // Looking at the exponential
    model.set_rand_exp(1.0);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rexp();

    // Computing the mean and variance
    epiworld_double m_exp = calc_mean(num_normals);
    epiworld_double v_exp = calc_variance(num_normals);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(moreless(m_exp, 1.0, 0.025));
    REQUIRE_FALSE(moreless(v_exp, 1.0, 0.025));
    #endif

    // Now with lognormal
    model.set_rand_lognormal(0.0, 1.0);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rlognormal();
    
    // Computing the mean and variance
    epiworld_double m_lognormal = calc_mean(num_normals);
    epiworld_double v_lognormal = calc_variance(num_normals);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(moreless(m_lognormal, std::exp(0.5), 0.025));
    REQUIRE_FALSE(moreless(v_lognormal/((std::exp(1.0) - 1.0)*std::exp(1.0)), 1.0, 0.25));
    #endif

    // Now with binomial
    model.set_rand_binom(10, 0.5);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rbinom();
    
    // Computing the mean and variance
    epiworld_double m_binom = calc_mean(num_normals);
    epiworld_double v_binom = calc_variance(num_normals);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(moreless(m_binom, 5.0, 0.025));
    REQUIRE_FALSE(moreless(v_binom, 2.5, 0.025));
    #endif

    // Now with negative binomial
    model.set_rand_nbinom(10, 0.5);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rnbinom();

    // Computing the mean and variance
    epiworld_double m_nbinom = calc_mean(num_normals);
    epiworld_double v_nbinom = calc_variance(num_normals);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(moreless(m_nbinom/10.0, 1.0, 0.025));
    REQUIRE_FALSE(moreless(v_nbinom/(10.0*0.5/(0.5*0.5)), 1.0, 0.025));
    #endif

    // Now with geometric
    model.set_rand_geom(0.8);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rgeom();

    // Computing the mean and variance
    epiworld_double m_geom = calc_mean(num_normals);
    epiworld_double v_geom = calc_variance(num_normals);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(moreless(m_geom, 0.2/0.8, 0.025));
    REQUIRE_FALSE(moreless(v_geom - (1.0 - .8) /(.8 * .8), 0.0, 0.025));
    #endif

    // Now with poisson
    model.set_rand_poiss(1.0);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rpoiss();

    // Computing the mean and variance
    epiworld_double m_poiss = calc_mean(num_normals);
    epiworld_double v_poiss = calc_variance(num_normals);

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE_FALSE(moreless(m_poiss, 1.0, 0.025));
    REQUIRE_FALSE(moreless(v_poiss, 1.0, 0.025));
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}