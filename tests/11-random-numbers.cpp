#include "tests.hpp"

#include "../include/stats/stats.hpp"

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
    epiworld_double m_norm = stats::mean(num_normals);
    epiworld_double v_norm = stats::variance(num_normals);

    REQUIRE_FALSE(moreless(m_norm, 0.00, 0.025));
    REQUIRE_FALSE(moreless(v_norm, 1.00, 0.025));

    // Repeating with runif
    model.set_rand_unif(-1.0, 1.0);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.runif();

    // Computing the mean and variance
    epiworld_double m_unif = stats::mean(num_normals);
    epiworld_double v_unif = stats::variance(num_normals);

    REQUIRE_FALSE(moreless(m_unif, 0.00, 0.025));
    REQUIRE_FALSE(moreless(v_unif, 4.0/12.0, 0.025));

    // Now with gamma
    model.set_rand_gamma(1.5, 2.0);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rgamma();

    // Computing the mean and variance
    epiworld_double m_gamma = stats::mean(num_normals);
    epiworld_double v_gamma = stats::variance(num_normals);

    REQUIRE_FALSE(moreless(m_gamma, 1.5*2.0, 0.025));
    REQUIRE_FALSE(moreless(v_gamma, 1.5*2.0*2.0, 0.10));
    
    // Looking at the exponential
    model.set_rand_exp(1.0);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rexp();

    // Computing the mean and variance
    epiworld_double m_exp = stats::mean(num_normals);
    epiworld_double v_exp = stats::variance(num_normals);

    REQUIRE_FALSE(moreless(m_exp, 1.0, 0.025));
    REQUIRE_FALSE(moreless(v_exp, 1.0, 0.025));

    // Now with lognormal
    model.set_rand_lognormal(0.0, 1.0);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rlognormal();
    
    // Computing the mean and variance
    epiworld_double m_lognormal = stats::mean(num_normals);
    epiworld_double v_lognormal = stats::variance(num_normals);

    REQUIRE_FALSE(moreless(m_lognormal, std::exp(0.5), 0.025));
    REQUIRE_FALSE(moreless(v_lognormal/((std::exp(1.0) - 1.0)*std::exp(1.0)), 1.0, 0.25));

    // Now with binomial
    model.set_rand_binom(10, 0.5);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rbinom();
    
    // Computing the mean and variance
    epiworld_double m_binom = stats::mean(num_normals);
    epiworld_double v_binom = stats::variance(num_normals);

    REQUIRE_FALSE(moreless(m_binom, 5.0, 0.025));
    REQUIRE_FALSE(moreless(v_binom, 2.5, 0.025));

#ifdef EPI_FAST_BINOM
    {
        constexpr int fast_n = 999;
        constexpr epiworld_double fast_p = 0.01;
        constexpr epiworld_double fast_lambda =
            static_cast<epiworld_double>(fast_n) * fast_p;

        Model<> fast_binom_args;
        Model<> fast_poiss_args;
        fast_binom_args.seed(91823);
        fast_poiss_args.seed(91823);

        for (size_t i = 0u; i < 256u; ++i)
            REQUIRE(fast_binom_args.rbinom(fast_n, fast_p) == fast_poiss_args.rpoiss(fast_lambda));

        Model<> fast_binom_set;
        Model<> fast_poiss_set;
        fast_binom_set.seed(55123);
        fast_poiss_set.seed(55123);
        fast_binom_set.set_rand_binom(fast_n, fast_p);

        for (size_t i = 0u; i < 256u; ++i)
            REQUIRE(fast_binom_set.rbinom() == fast_poiss_set.rpoiss(fast_lambda));
    }
#endif

    // Now with negative binomial
    model.set_rand_nbinom(10, 0.5);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rnbinom();

    // Computing the mean and variance
    epiworld_double m_nbinom = stats::mean(num_normals);
    epiworld_double v_nbinom = stats::variance(num_normals);

    REQUIRE_FALSE(moreless(m_nbinom/10.0, 1.0, 0.025));
    REQUIRE_FALSE(moreless(v_nbinom/(10.0*0.5/(0.5*0.5)), 1.0, 0.025));

    // Now with geometric
    model.set_rand_geom(0.8);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rgeom();

    // Computing the mean and variance
    epiworld_double m_geom = stats::mean(num_normals);
    epiworld_double v_geom = stats::variance(num_normals);

    REQUIRE_FALSE(moreless(m_geom, 0.2/0.8, 0.025));
    REQUIRE_FALSE(moreless(v_geom - (1.0 - .8) /(.8 * .8), 0.0, 0.025));

    // Now with poisson
    model.set_rand_poiss(1.0);
    for (size_t i = 0u; i < n; ++i)
        num_normals[i] = model.rpoiss();

    // Computing the mean and variance
    epiworld_double m_poiss = stats::mean(num_normals);
    epiworld_double v_poiss = stats::variance(num_normals);

    REQUIRE_FALSE(moreless(m_poiss, 1.0, 0.025));
    REQUIRE_FALSE(moreless(v_poiss, 1.0, 0.025));



}
