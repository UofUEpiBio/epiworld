#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

#include "../model-bones.hpp"

/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * ![Model Diagram](../assets/img/sir.png)
 * 
 * @ingroup basic_compartmental
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param n Number of agents.
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery_rate rate of the immune system
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSIR : public Model<TSeq>
{
public:

    static const int SUSCEPTIBLE = 0;
    static const int INFECTED    = 1;
    static const int RECOVERED   = 2;

    ModelSIR() = delete;

    ModelSIR(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double recovery_rate
    );

    /**
     * @brief Set the initial states of the model
     * @param proportions_ Double vector with a single element:
     * - The proportion of non-infected individuals who have recovered.
    */
    ModelSIR<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    ) override;
    

    /// States of any virus in this model (`Virus::set_state()`): the
    /// model's own, and those a user adds (`InfectionStates::apply()`)
    static constexpr InfectionStates infection_states{1, 2, 2};

    /// This model's entry in a `ModelRegistry`: arguments, states, builder
    static ModelInfo<TSeq> model_info();
};

template<typename TSeq>
inline ModelSIR<TSeq>::ModelSIR(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate
    )
{

    this->set_population_size(n);

    // Adding statuses
    this->add_state("Susceptible", default_update_susceptible<TSeq>);
    this->add_state("Infected", default_update_exposed<TSeq>);
    this->add_state("Recovered");

    // Setting up parameters
    this->add_param(recovery_rate, "Recovery rate");
    this->add_param(transmission_prob, "Transmission probability");

    // Preparing the virus -------------------------------------------
    Virus<TSeq> virus(vname, prevalence, true);
    infection_states.apply(virus);
    
    virus.set_prob_recovery("Recovery rate");
    virus.set_prob_infecting("Transmission probability");
    
    this->add_virus(virus);

    this->set_name("Susceptible-Infected-Recovered (SIR)");
   
}

template<typename TSeq>
inline ModelSIR<TSeq> & ModelSIR<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
) {

    Model<TSeq>::initial_states_fun =
        create_init_function_sir<TSeq>(proportions_)
        ;

    return *this;

}

template<typename TSeq>
inline ModelInfo<TSeq> ModelSIR<TSeq>::model_info()
{

    using uint_t = epiworld_fast_uint;

    return {"SIR", "Susceptible-Infected-Recovered",
        "Infected agents recover with lasting immunity.",
        "Disease",
        {
            ParamInfo::agents(10000),
            ParamInfo::prevalence(0.01),
            ParamInfo::probability("transmission_prob", "Transmission probability",
                0.05, "Probability that a contact with an infected agent infects."),
            ParamInfo::probability("recovery_rate", "Recovery rate", 0.14,
                "Daily probability that an infected agent recovers.")
        },
        infection_states,
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelSIR<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("recovery_rate")
            );
        }};
}

#endif
