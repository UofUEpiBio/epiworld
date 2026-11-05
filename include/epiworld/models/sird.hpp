#ifndef EPIWORLD_SIRD_H 
#define EPIWORLD_SIRD_H

#include "../model-bones.hpp"

/**
 * @brief Template for a Susceptible-Infected-Removed-Deceased (SIRD) model
 * 
 * ![Model Diagram](../assets/img/sird.png)
 * 
 * @ingroup death_compartmental
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSIRD : public Model<TSeq>
{
public:

    static const int SUSCEPTIBLE = 0;
    static const int INFECTED    = 1;
    static const int RECOVERED   = 2;
    static const int DECEASED    = 3;

    ModelSIRD() = delete;

    /**
     * @brief Constructs a new SIRD model with the given parameters.
     * 
     * @param vname The name of the vertex associated with this model.
 * @param n Number of agents.
     * @param prevalence The initial prevalence of the disease in the population.
     * @param transmission_prob Probability that a contact with an infected agent infects.
     * @param recovery_rate The rate at which infected individuals recover and become immune.
     * @param death_rate The rate at which infected individuals die.
     */
    ///@{
    ModelSIRD(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double recovery_rate, 
        epiworld_double death_rate
    );
    ///@}

    /**
     * @brief Set the initial states of the model
     * @param proportions_ Double vector with two elements:
     * - The proportion of non-infected individuals who have recovered.
     * - The proportion of non-infected individuals who have died.
    */
    ModelSIRD<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    ) override;
    
};

template<typename TSeq>
inline ModelSIRD<TSeq>::ModelSIRD(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate, 
    epiworld_double death_rate
    )
{

    this->set_population_size(n);

    this->add_param(recovery_rate, "Recovery rate");
    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(death_rate, "Death rate");

    NaturalHistory<TSeq> nh(vname);
    nh.add_stage("Susceptible");
    nh.add_stage("Infected", stage::infectious);
    nh.add_stage("Recovered");
    nh.add_stage("Deceased");
    nh.set_contact(DECEASED, 0.0);
    nh.infection(SUSCEPTIBLE, INFECTED, "Transmission probability");
    nh.rate(INFECTED, RECOVERED, "Recovery rate").effect(Effect::recovery);
    nh.rate(INFECTED, DECEASED, "Death rate").effect(Effect::death);
    this->set_natural_history(std::move(nh));

    this->add_virus(Virus<TSeq>(vname), Seed::proportion(prevalence));

    this->set_name("Susceptible-Infected-Recovered-Deceased (SIRD)");

   
}

template<typename TSeq>
inline ModelSIRD<TSeq> & ModelSIRD<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /**/ 
) {

    if (proportions_.size() != 2u)
        throw std::invalid_argument(
            "The vector of proportions must have two elements."
            );

    if ((proportions_[0u] + proportions_[1u]) >= 1.0)
        throw std::invalid_argument(
            "The proportions must sum up to less than 1."
            );

    this->set_initial(RECOVERED, proportions_[0u]);
    this->set_initial(DECEASED, proportions_[1u]);

    return *this;

}

#endif
