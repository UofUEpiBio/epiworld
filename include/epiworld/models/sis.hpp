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

    /// States of any virus in this model (`Virus::set_state()`): the
    /// model's own, and those a user adds (`InfectionStates::apply()`)
    static constexpr InfectionStates infection_states{INFECTED, SUSCEPTIBLE, SUSCEPTIBLE};

    /// This model's entry in a `ModelRegistry`: arguments, states, builder
    static ModelInfo<TSeq> model_info();
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

    // Adding statuses
    this->add_state("Susceptible", default_update_susceptible<TSeq>);
    this->add_state("Infected", default_update_exposed<TSeq>);

    // Setting up parameters
    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(recovery_rate, "Recovery rate");

    // Preparing the virus -------------------------------------------
    Virus<TSeq> virus(vname, prevalence, true);
    infection_states.apply(virus);
    
    virus.set_prob_infecting("Transmission probability");
    virus.set_prob_recovery("Recovery rate");
    virus.set_prob_death(0.0);
    
    this->add_virus(virus);

}

template<typename TSeq>
inline ModelInfo<TSeq> ModelSIS<TSeq>::model_info()
{

    using uint_t = epiworld_fast_uint;

    return {"SIS", "Susceptible-Infected-Susceptible",
        "Infected agents become susceptible again when they recover.",
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
            return std::make_unique< ModelSIS<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("recovery_rate")
            );
        }};
}

#endif
