#include "tests.hpp"

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

    auto params = m->get_current_proposed_params();

    // printf("{mu, sigma} = {% 4.2f, % 4.2f}; ", params[0u], params[1u]);
    // printf("{mean, sd} =  {% 4.2f, % 4.2f}\n", *mean, *sd);

}

EPIWORLD_TEST_CASE("LFMCMC", "[Basic example]") {
    
    auto rand = std::make_shared<std::mt19937>();
    rand->seed(91231);
    std::normal_distribution<epiworld_double> rnorm(5, 1.5);

    vec_double obsdata;
    for (size_t i = 0u; i < 50000; ++i)
        obsdata.push_back(rnorm(*rand));

    LFMCMC< vec_double > model(obsdata);
    model.set_rand_engine(rand);

    model.set_simulation_fun(simfun);
    model.set_summary_fun(summary_fun);
    model.set_proposal_fun(make_proposal_norm_reflective<vec_double>(.05, .0000001, 10));
    model.set_kernel_fun(kernel_fun_gaussian<vec_double>);

    model.run({1,1}, 100000, .125);

    
    model.print();
    model.print(50000);

    auto params_means = model.get_mean_params();
    auto stats_means  = model.get_mean_stats();

    std::vector<epiworld_double> expected = {5.0, 1.5};
    REQUIRE_THAT(params_means, Catch::Approx(expected).margin(0.5));
    REQUIRE_THAT(stats_means, Catch::Approx(expected).margin(0.5));
    REQUIRE_THROWS(model.print(200000));
    REQUIRE_NOTHROW(model.print(50000));

    // Add test of summary stats between [0, 1]
    std::normal_distribution<epiworld_double> rnorm2(0.5, 0.5);

    vec_double obsdata2;
    for (size_t i = 0u; i < 50000; ++i)
        obsdata2.push_back(rnorm2(*rand));

    LFMCMC< vec_double > model2(obsdata2);
    model2.set_rand_engine(rand);

    model2.set_simulation_fun(simfun);
    model2.set_summary_fun(summary_fun);
    model2.set_proposal_fun(make_proposal_norm_reflective<vec_double>(.05, .0000001, 10));
    model2.set_kernel_fun(kernel_fun_gaussian<vec_double>);

    model2.run({1,1}, 100000, .125);
    model2.print();

    REQUIRE_NOTHROW(model2.print());




}