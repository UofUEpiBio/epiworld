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

    /// States of any virus in this model (`Virus::set_state()`): the
    /// model's own, and those a user adds (`InfectionStates::apply()`)
    static constexpr InfectionStates infection_states{1, 0, 2};

    /// This model's entry in a `ModelRegistry`: arguments, states, builder
    static ModelInfo<TSeq> model_info();
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

    // Adding statuses
    this->add_state("Susceptible", default_update_susceptible<TSeq>);
    this->add_state("Infected", default_update_exposed<TSeq>);
    this->add_state("Deceased");

    // Setting up parameters
    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(recovery_rate, "Recovery rate");
    this->add_param(death_rate, "Death rate");

    // Preparing the virus -------------------------------------------
    Virus<TSeq> virus(vname, prevalence, true);
    infection_states.apply(virus);
    
    virus.set_prob_infecting("Transmission probability");
    virus.set_prob_recovery("Recovery rate");
    virus.set_prob_death(0.01);
    
    this->add_virus(virus);

}

template<typename TSeq>
inline ModelInfo<TSeq> ModelSISD<TSeq>::model_info()
{

    using uint_t = epiworld_fast_uint;

    return {"SISD", "Susceptible-Infected-Susceptible-Deceased",
        "SIS, where infected agents may also die.",
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
            return std::make_unique< ModelSISD<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("recovery_rate"), a("death_rate")
            );
        }};
}

#endif
