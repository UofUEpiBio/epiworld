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

    NaturalHistory<TSeq> nh("SISD");
    nh.add_stage("Susceptible", default_update_susceptible<TSeq>);
    nh.add_stage("Infected", default_update_exposed<TSeq>);
    nh.add_stage("Deceased");
    nh.set_infection(1, 0, 2);
    this->add_natural_history(nh);

    // Setting up parameters
    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(recovery_rate, "Recovery rate");
    this->add_param(death_rate, "Death rate");

    // Preparing the virus -------------------------------------------
    Virus<TSeq> virus(vname, prevalence, true);
    virus.set_natural_history(0);
    
    virus.set_prob_infecting("Transmission probability");
    virus.set_prob_recovery("Recovery rate");
    virus.set_prob_death(0.01);
    
    this->add_virus(virus);

}

#endif
