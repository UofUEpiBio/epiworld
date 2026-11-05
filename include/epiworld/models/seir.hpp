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

    ModelSEIR<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    ) override;

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

    NaturalHistory<TSeq> nh(vname);
    nh.add_stage("Susceptible");
    nh.add_stage("Exposed", stage::carrier);
    nh.add_stage("Infected", stage::infectious);
    nh.add_stage("Removed");
    nh.infection(SUSCEPTIBLE, EXPOSED, "Transmission probability");
    nh.after(EXPOSED, INFECTED, Duration<TSeq>::geometric("Latent period"));
    nh.rate(INFECTED, REMOVED, "Recovery rate").effect(Effect::recovery);
    this->set_natural_history(std::move(nh));

    this->add_virus(Virus<TSeq>(vname), Seed::proportion(prevalence));

    this->set_name("Susceptible-Exposed-Infected-Removed (SEIR)");

}

template<typename TSeq>
inline ModelSEIR<TSeq> & ModelSEIR<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /**/
) {

    if (proportions_.size() != 2u)
        throw std::invalid_argument("-proportions_- must have two entries.");

    for (auto & v : proportions_)
        if ((v < 0.0) || (v > 1.0))
            throw std::invalid_argument(
                "-proportions_- must have values between 0 and 1."
                );

    // Of the initial cases, a share starts Infected; of the others, a share
    // starts Removed
    this->nh_seeds[0u].split = {
        {EXPOSED, 1.0 - proportions_[0u]},
        {INFECTED, proportions_[0u]}
    };
    this->set_initial(REMOVED, proportions_[1u]);

    return *this;

}

#endif
