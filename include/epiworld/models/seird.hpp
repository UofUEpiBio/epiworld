#ifndef EPIWORLD_MODELS_SEIRD_HPP
#define EPIWORLD_MODELS_SEIRD_HPP

/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed-Deceased (SEIRD) model
*/
template<typename TSeq = int>
class ModelSEIRD : public epiworld::Model<TSeq>
{

protected:

    enum S {
        Susceptible,
        Exposed,
        Infected,
        Hospitalized,
        Recovered,
        Deceased
    };
    
    static void update_exposed(epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m);
    static void update_infected(epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m);
    static void update_hospitalized(epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m);
    static void contact(Model<TSeq> * m);

public:

    ModelSEIRD() {};

    ModelSEIRD(
        ModelSEIRD<TSeq> & model,
        std::string vname
    );

    ModelSEIRD(
        std::string vname,
        epiworld_double prevalence,
        epiworld_double incu_shape,
        epiworld_double incu_rate,
        epiworld_double infe_shape,
        epiworld_double infe_rate,
        epiworld_double hospitalization_rate,
        epiworld_double hospitalization_recover_rate,
        epiworld_double hospitalization_decease_rate,
        epiworld_double transmission_rate,
        epiworld_double transmission_entity_rate,
        epiworld_double death_rate,
        size_t n_entities,
        size_t n_interactions
    );

    ModelSEIRD(
        std::string fn,
        std::string vname
        );
    

};

template<typename TSeq>
inline ModelSEIRD<TSeq>::ModelSEIRD(
    std::string fn,
    std::string vname
    )
{

    std::vector<std::string> expected = {
        "Gamma shape (incubation)",
        "Gamma rate (incubation)",
        "Gamma shape (infected)",
        "Gamma rate (infected)",
        "Hospitalization rate",
        "Recovery rate (hosp.)",
        "Decease rate (hosp)",
        "Transmission rate",
        "Transmission rate (entity)",
        "Prevalence",
        "N entities",
        "N interactions"
        "Death Rate"
        };

    // Reading the parameters in
    this->read_params(fn);

    for (const auto & p : expected)
    {
        if (this->parameters.find(p) == this->parameters.end())
        {
            throw std::logic_error(
                std::string(
                    "The specified file doesn't have all the required parameters for the model"
                )
            );
        }
    }


    // Setting up the model
    ModelSEIRD(*this, vname);

    return;

}

template<typename TSeq>
inline ModelSEIRD<TSeq>::ModelSEIRD(
    ModelSEIRD<TSeq> & model,
    std::string vname
    )
{

    model.add_state(
        "Susceptible", 
        sampler::make_update_susceptible<TSeq>({S::Exposed, S::Hospitalized})
        );

    model.add_state("Exposed", this->update_exposed);
    model.add_state("Infected", this->update_infected);
    model.add_state("Hospitalized", this->update_hospitalized);
    model.add_state("Recovered");
    model.add_state("Deceased");

    // Creating the virus
    Virus<TSeq> virus(vname);
    virus.set_prob_infecting(&model("Transmission rate"));
    virus.set_prob_death(&model("Death rate"));
    virus.set_state(S::Exposed, S::Recovered, S::Deceased);
    virus.set_queue(Queue<TSeq>::OnlySelf, -99LL);
    virus.get_data() = {0.0, 0.0F};

    model.add_virus_n(virus, model("Prevalence"));
     
    // Adding randomly distributed entities, each one with capacity for entity_capacity
    for (size_t r = 0u; r < model("N entities"); ++r)
    {
        Entity<TSeq> e(std::string("Location ") + std::to_string(r));
        model.add_entity(e);
    }

    // This will act through the global
    model.add_global_action(contact, "Transmission (contact)", -99);

    model.set_name("Susceptible-Exposed-Infected-Recovered-Deceased (SEIRD)");

    return;
   
}

template<typename TSeq>
inline ModelSEIRD<TSeq>::ModelSEIRD(
    std::string vname,
    epiworld_double prevalence,
    epiworld_double incu_shape,
    epiworld_double incu_rate,
    epiworld_double infe_shape,
    epiworld_double infe_rate,
    epiworld_double hospitalization_rate,
    epiworld_double hospitalization_recover_rate,
    epiworld_double hospitalization_decease_rate,
    epiworld_double transmission_rate,
    epiworld_double transmission_entity_rate,
    epiworld_double death_rate,
    size_t n_entities,
    size_t n_interactions
) {

    // Adding parameters
    this->add_param(incu_shape, "Gamma shape (incubation)");
    this->add_param(incu_rate, "Gamma rate (incubation) ");
    this->add_param(infe_shape, "Gamma shape (infected)");
    this->add_param(infe_rate, "Gamma rate (infected)");
    this->add_param(hospitalization_rate, "Hospitalization rate");
    this->add_param(hospitalization_recover_rate, "Recovery rate (hosp.)");
    this->add_param(hospitalization_recover_rate, "Decease rate (hosp)");
    this->add_param(transmission_rate, "Transmission rate");
    this->add_param(transmission_entity_rate, "Transmission rate (entity)");
    this->add_param(prevalence, "Prevalence");
    this->add_param(prevalence, "Death Rate");
    this->add_param(static_cast<epiworld_double>(n_entities), "N entities");
    this->add_param(static_cast<epiworld_double>(n_interactions), "N interactions");

    ModelSEIRD<TSeq>(
        *this,
        vname
        );

    return;

}




// Update dynamics for exposed individuals
template<typename TSeq>
inline void ModelSEIRD<TSeq>::update_exposed(
    epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
)
{
    // Checking if the data was assigned
    auto v = p->get_virus(0u);
    if (m->today() == (v->get_date() + 1))
    {

        // Checking the same
        if (v->get_data().size() != 3u)
            v->get_data().resize(3u);

        // Days of incubation
        v->get_data()[0u] = v->get_date() + m->rgamma(
            m->par("Gamma shape (incubation)"),
            m->par("Gamma rate (incubation)")
            );

        // Duration as Infected
        v->get_data()[1u] = v->get_date() + m->rgamma(
            m->par("Gamma shape (infected)"),
            m->par("Gamma rate (infected)")
            );

        // Prob of becoming hospitalized
        v->get_data()[2u] = (
            m->runif() < m->par("Hospitalization rate")
            ) ? 100.0 : -100.0;


    } 
    
    if (m->today() >= v->get_data()[0u])
    {
        p->change_state(m, S::Infected, epiworld::Queue<TSeq>::Everyone);
        return;
    }

    return;

}

// Update dynamics for infected individuals
template<typename TSeq>
inline void ModelSEIRD<TSeq>::update_infected(
    epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
)
{

    // Computing probability of recovery
    auto v = p->get_virus(0u);

    if (m->today() < v->get_data()[1u])
        return;

    if (v->get_data()[2u] < 0)
    {
        
        p->rm_virus(v, m, S::Recovered, -epiworld::Queue<TSeq>::Everyone);
        return;

    }
    else
    {

        // Individual goes hospitalized
        p->change_state(m, S::Hospitalized, -epiworld::Queue<TSeq>::Everyone);
        return;

    }

}

// Update dynamics for hospitalized individuals
template<typename TSeq>
inline void ModelSEIRD<TSeq>::update_hospitalized(
    epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
)
{

    // Computing the recovery probability
    auto v = p->get_virus(0u);
    auto probs = {
        m->par("Recovery rate (hosp.)"),
        m->par("Decease rate (hosp)")
        };

    int which = epiworld::roulette(probs, m);

    // Nothing happens
     if (which < 0)
        return;

    if (which == 0) // Then it recovered
    {
        p->rm_virus(v, m, S::Recovered, epiworld::Queue<TSeq>::NoOne);
        return;
    }

    // Individual dies
    p->rm_virus(v, m, S::Deceased, epiworld::Queue<TSeq>::NoOne);

    return;

}

/**
 * @brief Transmission by contact outside home
 */
template<typename TSeq>
inline void ModelSEIRD<TSeq>::contact(Model<TSeq> * m)
{
    for (auto & a : (m->get_agents()))
    {
        // Will it get it from the entities?
        if (a.get_state() == S::Susceptible)
        {

            AgentsSample<int> neighbors(m, a, m->par("N interactions"), true);

            int n_viruses = 0;
            for (auto n : neighbors) {
                if (n->get_state() == S::Infected)
                    m->array_virus_tmp[n_viruses++] = &(*n->get_virus(0u));
            }

            // Nothing to see here
            if (n_viruses == 0)
                continue;

            // Is the individual getting the infection?
            double p_infection = 1.0 - std::pow(
	        1.0 - m->par("Transmission rate (entity)"), n_viruses
		);

            if (m->runif() >= p_infection)
                continue;

            // Who infects the individual?
            int which = std::floor(m->runif() * n_viruses);

            a.add_virus(
                *(m->array_virus_tmp[which]), // Viruse.
                m, 
                S::Exposed,                   // New state.
                Queue<TSeq>::OnlySelf         // Change on the queue.
                ); 

        }


    }
}


#endif
