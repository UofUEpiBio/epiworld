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

    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int INFECTED    = 1;

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

    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(recovery_rate, "Recovery rate");

    Virus<TSeq> virus(vname);
    virus.add_stage("Susceptible");
    virus.add_stage("Infected", stage::infectious);
    virus.infection(SUSCEPTIBLE, INFECTED, "Transmission probability");
    virus.rate(INFECTED, SUSCEPTIBLE, "Recovery rate").effect(Effect::recovery);

    this->add_virus(virus, assign::share<TSeq>(prevalence));

}

#endif
