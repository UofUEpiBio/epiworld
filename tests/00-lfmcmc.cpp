#include "tests.hpp"
#include "../include/epiworld/math/lfmcmc.hpp"

typedef std::vector<epiworld_double> vec_double;

using namespace epiworld;

vec_double simfun(const vec_double & p, LFMCMC<vec_double> * m)
{

    vec_double res;
    for (size_t i = 0; i < 1000; ++i)
        res.push_back(m->rnorm(p[0], p[1]));

    return res;
}

void summary_fun(vec_double & res, const vec_double & p, LFMCMC<vec_double> * m)
{

    if (res.size() == 0u)
        res.resize(2u);

    epiworld_double * mean = &res[0u];
    epiworld_double * sd   = &res[1u];
    epiworld_double n      = static_cast<epiworld_double>(p.size());

    *mean = 0.0;
    for (auto & v : p)
        *mean += (v/n);

    *sd = 0.0;
    for (auto & v: p)
        *sd += (std::pow(*mean - v, 2.0)/(n - 1));

    *sd = std::sqrt(*sd);

    auto params = m->get_params_now();

    // printf("{mu, sigma} = {% 4.2f, % 4.2f}; ", params[0u], params[1u]);
    // printf("{mean, sd} =  {% 4.2f, % 4.2f}\n", *mean, *sd);

}

EPIWORLD_TEST_CASE("LFMCMC", "[Basic example]") {

    std::mt19937 rand;
    rand.seed(91231);
    std::normal_distribution<epiworld_double> rnorm(5, 1.5);

    vec_double obsdata;
    for (size_t i = 0u; i < 50000; ++i)
        obsdata.push_back(rnorm(rand));

    LFMCMC< vec_double > model(obsdata);
    model.set_rand_engine(rand);

    model.set_simulation_fun(simfun);
    model.set_summary_fun(summary_fun);
    model.set_proposal_fun(make_proposal_norm_reflective<vec_double>(.05, .0000001, 10));
    model.set_kernel_fun(kernel_fun_gaussian<vec_double>);

    model.run({1,1}, 100000, .125);

    
    model.print();

    auto params_means = model.get_params_mean();
    auto stats_means  = model.get_stats_mean();

    #ifdef CATCH_CONFIG_MAIN
    std::vector<epiworld_double> expected = {5.0, 1.5};
    REQUIRE_THAT(params_means, Catch::Approx(expected).margin(0.2));
    REQUIRE_THAT(stats_means, Catch::Approx(expected).margin(0.2));
    #endif 

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}