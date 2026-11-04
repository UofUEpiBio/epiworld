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
    

    /// States of any virus in this model (`Virus::set_state()`): the
    /// model's own, and those a user adds (`InfectionStates::apply()`)
    static constexpr InfectionStates infection_states{1, 2, 3};

    /// This model's entry in a `ModelRegistry`: arguments, states, builder
    static ModelInfo<TSeq> model_info();
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

    // Adding statuses
    this->add_state("Susceptible", default_update_susceptible<TSeq>);
    this->add_state("Infected", default_update_exposed<TSeq>);
    this->add_state("Recovered");
    this->add_state("Deceased");

    // Setting up parameters
    this->add_param(recovery_rate, "Recovery rate");
    this->add_param(transmission_prob, "Transmission probability"),
    this->add_param(death_rate, "Death rate");

    // Preparing the virus -------------------------------------------
    Virus<TSeq> virus(vname, prevalence, true);
    infection_states.apply(virus);
    virus.set_prob_recovery("Recovery rate");
    virus.set_prob_infecting("Transmission probability");
    virus.set_prob_death("Death rate");
    
    this->add_virus(virus);

    this->set_name("Susceptible-Infected-Recovered-Deceased (SIRD)");

   
}

template<typename TSeq>
inline ModelSIRD<TSeq> & ModelSIRD<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /**/ 
) {

    Model<TSeq>::initial_states_fun = 
        create_init_function_sird<TSeq>(proportions_)
        ;

    return *this;

}

template<typename TSeq>
inline ModelInfo<TSeq> ModelSIRD<TSeq>::model_info()
{

    using uint_t = epiworld_fast_uint;

    return {"SIRD", "Susceptible-Infected-Recovered-Deceased",
        "SIR, where infected agents may also die.",
        "Disease",
        {
            ParamInfo::agents(10000),
            ParamInfo::prevalence(0.01),
            ParamInfo::probability("transmission_prob", "Transmission probability",
                0.05, "Probability that a contact with an infected agent infects."),
            ParamInfo::probability("recovery_rate", "Recovery rate", 0.14,
                "Daily probability that an infected agent recovers."),
            ParamInfo::probability("death_rate", "Death rate", 0.01,
                "Daily probability that an infected agent dies.")
        },
        infection_states,
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelSIRD<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("recovery_rate"), a("death_rate")
            );
        }};
}

#endif
