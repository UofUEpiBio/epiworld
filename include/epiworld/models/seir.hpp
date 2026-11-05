#ifndef EPIWORLD_MODELS_SEIR_HPP
#define EPIWORLD_MODELS_SEIR_HPP

#include "../model-bones.hpp"

/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model
 * 
 * ![Model Diagram](../assets/img/seir.png)
 * 
 * @ingroup basic_compartmental
 *
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param n Number of agents.
 * @param prevalence epiworld_double Initial prevalence the immune system
 * @param transmission_prob epiworld_double Probability that a contact with an
 * infected agent infects.
 * @param latent_period epiworld_double Mean days in Exposed (carrying the
 * virus, not yet infectious).
 * @param recovery_rate epiworld_double Recovery rate of the virus.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSEIR : public Model<TSeq>
{

public:
    static const int SUSCEPTIBLE = 0;
    static const int EXPOSED     = 1;
    static const int INFECTED    = 2;
    static const int REMOVED     = 3;

    ModelSEIR() = delete;

    ModelSEIR(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double latent_period,
        epiworld_double recovery_rate
    );


};


template<typename TSeq>
inline ModelSEIR<TSeq>::ModelSEIR(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double latent_period,
    epiworld_double recovery_rate
    )
{

    this->set_population_size(n);

    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(latent_period, "Latent period");
    this->add_param(recovery_rate, "Recovery rate");

    Virus<TSeq> virus(vname);
    virus.add_stage("Susceptible");
    virus.add_stage("Exposed", stage::carrier);
    virus.add_stage("Infected", stage::infectious);
    virus.add_stage("Removed");
    virus.infection(SUSCEPTIBLE, EXPOSED, "Transmission probability");
    virus.after(EXPOSED, INFECTED, Duration<TSeq>::geometric("Latent period"));
    virus.rate(INFECTED, REMOVED, "Recovery rate").effect(Effect::recovery);

    this->add_virus(virus, assign::share<TSeq>(prevalence));

    this->set_name("Susceptible-Exposed-Infected-Removed (SEIR)");

}


#endif
