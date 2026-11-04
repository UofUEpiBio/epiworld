#ifndef EPIWORLD_MODELS_SIS_HPP 
#define EPIWORLD_MODELS_SIS_HPP

#include "../model-bones.hpp"

/**
 * @brief Template for a Susceptible-Infected-Susceptible (SIS) model
 * 
 * ![Model Diagram](../assets/img/sis.png)
 * 
 * @ingroup basic_compartmental
 * 
 * @param vname std::string Name of the virus
 * @param n Number of agents.
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery_rate rate of the immune system
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSIS : public Model<TSeq>
{

public:

    static const int SUSCEPTIBLE = 0;
    static const int INFECTED    = 1;

    ModelSIS() = delete;

    ModelSIS(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double recovery_rate
    );

};

template<typename TSeq>
inline ModelSIS<TSeq>::ModelSIS(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate
    )
{

    this->set_population_size(n);

    this->set_name("Susceptible-Infected-Susceptible (SIS)");

    NaturalHistory<TSeq> nh("SIS");
    nh.add_stage("Susceptible", default_update_susceptible<TSeq>);
    nh.add_stage("Infected", default_update_exposed<TSeq>);
    nh.set_infection(INFECTED, SUSCEPTIBLE);
    this->add_natural_history(nh);

    // Setting up parameters
    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(recovery_rate, "Recovery rate");

    // Preparing the virus -------------------------------------------
    Virus<TSeq> virus(vname, prevalence, true);
    virus.set_natural_history(0);
    
    virus.set_prob_infecting("Transmission probability");
    virus.set_prob_recovery("Recovery rate");
    virus.set_prob_death(0.0);
    
    this->add_virus(virus);

}

#endif
