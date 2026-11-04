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

    UpdateFun<TSeq> update_exposed_seir = [](
        Agent<TSeq> * p,
        Model<TSeq> * m
    ) -> void {

        // Getting the virus
        auto v = p->get_virus();

        // Does the agent become infected?
        if (m->runif() < 1.0/(v->get_incubation(m)))
            p->change_state(*m, ModelSEIR<TSeq>::INFECTED);

        return;
    };

    /**
     * @brief Set up the initial states of the model.
     * @param proportions_ Double vector with the following values:
     * - 0: Proportion of non-infected agents who are removed.
     * - 1: Proportion of exposed agents to be set as infected.
    */
    ModelSEIR<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    ) override;

    /// States of any virus in this model (`Virus::set_state()`): the
    /// model's own, and those a user adds (`InfectionStates::apply()`)
    static constexpr InfectionStates infection_states{EXPOSED, REMOVED, REMOVED};

    /// This model's entry in a `ModelRegistry`: arguments, states, builder
    static ModelInfo<TSeq> model_info();
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

    // Adding statuses
    this->add_state("Susceptible", default_update_susceptible<TSeq>);
    this->add_state("Exposed", this->update_exposed_seir);
    this->add_state("Infected", default_update_exposed<TSeq>);
    this->add_state("Removed");

    // Exposed agents carry the virus but are not infectious yet
    this->set_infectious_states({INFECTED});

    // Setting up parameters
    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(latent_period, "Latent period");
    this->add_param(recovery_rate, "Recovery rate");

    // Preparing the virus -------------------------------------------
    Virus<TSeq> virus(vname, prevalence, true);
    infection_states.apply(virus);

    virus.set_prob_infecting("Transmission probability");
    virus.set_incubation("Latent period");
    virus.set_prob_recovery("Recovery rate");

    // Adding the tool and the virus
    this->add_virus(virus);

    this->set_name("Susceptible-Exposed-Infected-Removed (SEIR)");

}

template<typename TSeq>
inline ModelSEIR<TSeq> & ModelSEIR<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /**/
) {

    Model<TSeq>::initial_states_fun =
        create_init_function_seir<TSeq>(proportions_)
        ;

    return *this;

}

template<typename TSeq>
inline ModelInfo<TSeq> ModelSEIR<TSeq>::model_info()
{

    using uint_t = epiworld_fast_uint;

    return {"SEIR", "Susceptible-Exposed-Infected-Recovered",
        "Infected agents go through a latent (exposed, not infectious) period "
        "before they become infectious.",
        "Disease",
        {
            ParamInfo::agents(10000),
            ParamInfo::prevalence(0.01),
            ParamInfo::probability("transmission_prob", "Transmission probability",
                0.05, "Probability that a contact with an infected agent infects."),
            ParamInfo::duration("latent_period", "Latent period", 7.0,
                "Mean days in the exposed state (infected, not yet infectious)."),
            ParamInfo::probability("recovery_rate", "Recovery rate", 0.14,
                "Daily probability that an infected agent recovers.")
        },
        infection_states,
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelSEIR<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("latent_period"), a("recovery_rate")
            );
        }};
}

#endif
