// #include "../../include/epiworld/epiworld.hpp"
#include "../../include/epiworld/models/sir.hpp"
#include "../../include/epiworld/math/lfmcmc.hpp"

using namespace epiworld;

Model<> model;

size_t niter = 0;

std::vector< int > simfun(
    std::vector< epiworld_double > params,
    LFMCMC<std::vector<int>> * m
) {

    model("Immune recovery") = params[0u];
    model("Infectiousness")  = params[1u];

    model.reset();
    model.run();
       
    std::vector< int > res;
    model.get_db().get_today_total(nullptr, &res);

    auto prev = m->get_params_prev();
    printf("Iteration N %3li: {%.2f, .%2f}\n", niter++, prev[0u], prev[1u]);

    return res;
};


std::vector< epiworld_double > sumfun(
    std::vector< int > & dat,
    LFMCMC< std::vector<int> > * m
) {

    std::vector< epiworld_double > res(0u);
    for (auto & v : dat)
        res.push_back(static_cast< epiworld_double >(v));

    return res;

};

// FUN<VEC( epiworld_double )(TData&, LFMCMC<TData>*)> summary_fun;

int main()
{
    
    set_up_sir(model, "covid", .1, .9, 0, .5, 1.0);

    // Creating a new LFMCMC model
    LFMCMC<std::vector< int >> lfmcmc;

    lfmcmc.set_simulation_fun(simfun);
    lfmcmc.set_summary_fun(sumfun);
    lfmcmc.set_proposal_fun(default_proposal_fun_unif<std::vector<int>>);

    // Simulating some data
    model.init(50, 122);
    model.set_backup();
    model.verbose_off();
    model.run();

    std::vector< int > obs_dat;
    model.get_db().get_today_total(nullptr, &obs_dat);

    lfmcmc.set_observed_data(obs_dat);

    std::vector< epiworld_double > par0 = {.5, .5};

    lfmcmc.run(par0, 1000, .01);


    printf("True: {%.2f, .%2f}\n", .9, .5);

  
}
