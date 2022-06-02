#include "tests.hpp"
#include "../include/epiworld/math/lfmcmc.hpp"

typedef std::vector<float> vec_double;

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

    float * mean = &res[0u];
    float * sd   = &res[1u];
    float n      = static_cast<float>(p.size());

    *mean = 0.0;
    for (auto & v : p)
        *mean += (v/n);

    *sd = 0.0;
    for (auto & v: p)
        *sd += (std::pow(*mean - v, 2.0)/(n - 1));

    *sd = std::sqrt(*sd);

    auto params = m->get_params_now();

    printf("{mu, sigma} = {% 4.2f, % 4.2f}; ", params[0u], params[1u]);
    printf("{mean, sd} =  {% 4.2f, % 4.2f}\n", *mean, *sd);

}

EPIWORLD_TEST_CASE("LFMCMC", "[Basic example]") {

    std::mt19937 rand;
    rand.seed(91231);
    std::normal_distribution<float> rnorm(5, 1.5);

    vec_double obsdata;
    for (size_t i = 0u; i < 1000; ++i)
        obsdata.push_back(rnorm(rand));

    LFMCMC< vec_double > model(obsdata);
    model.set_rand_engine(rand);

    model.set_simulation_fun(simfun);
    model.set_summary_fun(summary_fun);
    model.set_proposal_fun(make_proposal_norm_reflective<vec_double>(.1, .0000001, 20));
    model.set_kernel_fun(kernel_fun_gaussian<vec_double>);

    model.run({1,1}, 10000, .5);

    model.print();

}