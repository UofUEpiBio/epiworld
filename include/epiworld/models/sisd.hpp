#ifndef EPIWORLD_MODELS_SISD_HPP 
#define EPIWORLD_MODELS_SISD_HPP

#include "../model-bones.hpp"

/**
 * @brief Template for a Susceptible-Infected-Susceptible-Deceased (SISD) model
 * 
 * ![Model Diagram](../assets/img/sisd.png)
 * 
 * @ingroup death_compartmental
 * 
 * @param vname std::string Name of the virus
 * @param n Number of agents.
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery_rate rate of the immune system
 * @param inital_death epiworld_double Initial death_rate of the immune system
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSISD : public Model<TSeq>
{

public:

    static const int SUSCEPTIBLE = 0;
    static const int INFECTED    = 1;
    static const int DECEASED    = 2;

    ModelSISD() = delete;

    ModelSISD(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double recovery_rate,
        epiworld_double death_rate
    );

};

template<typename TSeq>
inline ModelSISD<TSeq>::ModelSISD(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate,
    epiworld_double death_rate
    )
{

    this->set_population_size(n);

    this->set_name("Susceptible-Infected-Susceptible-Deceased (SISD)");

    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(recovery_rate, "Recovery rate");
    this->add_param(death_rate, "Death rate");

    NaturalHistory<TSeq> nh(vname);
    nh.add_stage("Susceptible");
    nh.add_stage("Infected", stage::infectious);
    nh.add_stage("Deceased");
    nh.set_contact(DECEASED, 0.0);
    nh.infection(SUSCEPTIBLE, INFECTED, "Transmission probability");
    nh.rate(INFECTED, SUSCEPTIBLE, "Recovery rate").effect(Effect::recovery);
    nh.rate(INFECTED, DECEASED, "Death rate").effect(Effect::death);
    this->set_natural_history(std::move(nh));

    this->add_virus(Virus<TSeq>(vname), Seed::proportion(prevalence));

}

#endif
