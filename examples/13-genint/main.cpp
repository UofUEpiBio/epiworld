#include <iostream>
#include <vector>
#include <numeric>
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

// Everyone meets "Contact rate" others a day, at random: one group, a 1 x 1
// contact matrix of 1, scaled by the parameter.
void add_random_mixing(Model<> & model, double contact_rate)
{
    model.add_param(contact_rate, "Contact rate");
    auto mix = layer::mixing<>("random mixing");
    mix->set_groups(std::vector< size_t >(model.size(), 0u), 1u);
    mix->set_contact_matrix({1.0});
    mix->set_contact_rate("Contact rate");
    model.add_layer(mix);
}

// Expected generation interval on each of the first `ndays` days, from that
// day's susceptible agents (gen_int_mean()); the latent period, if any, adds
// to it.
std::vector< double > expected_gentime(
    Model<> & model, size_t ndays, double latent_period
)
{

    std::vector< int > date, counts;
    std::vector< std::string > state;
    model.get_db().get_hist_total(&date, &state, &counts);

    std::vector< double > S(ndays, 0.0);
    for (size_t i = 0u; i < date.size(); ++i)
        if ((state[i] == "Susceptible") && (date[i] < static_cast< int >(ndays)))
            S[date[i]] += counts[i];

    const double p_c = model("Contact rate") / static_cast< double >(model.size());
    std::vector< double > res(ndays);
    for (size_t d = 0u; d < ndays; ++d)
        res[d] = latent_period + gen_int_mean(
            S[d], p_c, model("Transmission probability"), model("Recovery rate")
        );

    return res;

}

// Mean observed generation interval of the infections in the first `ndays`
// days (the first three quarters of the records)
double observed_gentime(Model<> & model, size_t ndays)
{

    std::vector< int > agent, virus, time, gentime;
    model.get_db().get_generation_time(agent, virus, time, gentime);

    double mean = 0.0;
    size_t n = 0u;
    for (size_t i = 0u; i < gentime.size() * 3u / 4u; ++i)
    {
        if ((time[i] >= static_cast< int >(ndays)) || (gentime[i] < 0))
            continue;

        mean += gentime[i];
        ++n;
    }

    return mean / static_cast< double >(n);

}

int main()
{

    // Model parameters
    const double r          = 10.0;     // Contact rate
    const size_t popsize    = 5000u;
    const double prevalence = 0.0005;
    const double p_i        = .1;       // Transmission probability
    const double p_r        = 1.0/7.0;  // Recovery rate
    const double latent     = 2.0;      // Latent period

    const size_t nsteps = 200;    // Number of days
    const size_t ndays  = 20;     // Days used in the comparison

    epimodels::ModelSIR<> model("avirus", popsize, prevalence, p_i, p_r);
    add_random_mixing(model, r);
    model.run(nsteps, 554);
    model.print();

    auto sir = expected_gentime(model, ndays, 0.0);

    epimodels::ModelSEIR<> model2(
        "avirus", popsize, prevalence, p_i, latent, p_r
    );
    add_random_mixing(model2, r);
    model2.run(nsteps, 554);
    model2.print();

    auto seir = expected_gentime(model2, ndays, latent);

    std::cout << "SIR Gen. Int. (obs)       : " << observed_gentime(model, ndays) << std::endl;
    std::cout << "SIR Gen. Int. (expected)  : " <<
        std::accumulate(sir.begin(), sir.end(), 0.0) / ndays << std::endl;

    std::cout << "SEIR Gen. Int. (obs)      : " << observed_gentime(model2, ndays) << std::endl;
    std::cout << "SEIR Gen. Int. (expected) : " <<
        std::accumulate(seir.begin(), seir.end(), 0.0) / ndays << std::endl;

    return 0;

}
