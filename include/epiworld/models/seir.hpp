#ifndef EPIWORLD_MODELS_SEIR_HPP
#define EPIWORLD_MODELS_SEIR_HPP

/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence epiworld_double Initial prevalence the immune system
 * @param transmission_rate epiworld_double Transmission rate of the virus
 * @param avg_incubation_days epiworld_double Average incubation days of the virus.
 * @param recovery_rate epiworld_double Recovery rate of the virus.
 */
template<typename TSeq = int>
class ModelSEIR : public epiworld::Model<TSeq>
{

public:
    static const int SUSCEPTIBLE = 0;
    static const int EXPOSED     = 1;
    static const int INFECTED    = 2;
    static const int REMOVED     = 3;

    ModelSEIR() {};

    ModelSEIR(
        ModelSEIR<TSeq> & model,
        std::string vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate
    );

    ModelSEIR(
        std::string vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate
    );
    
    epiworld::UpdateFun<TSeq> update_exposed_seir = [](
        epiworld::Agent<TSeq> * p,
        epiworld::Model<TSeq> * m
    ) -> void {

        // Getting the virus
        auto v = p->get_virus();

        // Does the agent become infected?
        if (m->runif() < 1.0/(v->get_incubation(m)))
            p->change_state(m, ModelSEIR<TSeq>::INFECTED);

        return;    
    };
      

    epiworld::UpdateFun<TSeq> update_infected_seir = [](
        epiworld::Agent<TSeq> * p,
        epiworld::Model<TSeq> * m
    ) -> void {
        // Does the agent recover?
        if (m->runif() < (m->par("Recovery rate")))
            p->rm_virus(m);

        return;    
    };

    /**
     * @brief Set up the initial states of the model.
     * @param proportions_ Double vector with the following values:
     * - 0: Proportion of non-infected agents who are removed.
     * - 1: Proportion of exposed agents to be set as infected.
    */
    void initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    );

};


template<typename TSeq>
inline ModelSEIR<TSeq>::ModelSEIR(
    ModelSEIR<TSeq> & model,
    std::string vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate
    )
{

    // Adding statuses
    model.add_state("Susceptible", epiworld::default_update_susceptible<TSeq>);
    model.add_state("Exposed", model.update_exposed_seir);
    model.add_state("Infected", model.update_infected_seir);
    model.add_state("Removed");

    // Setting up parameters
    model.add_param(transmission_rate, "Transmission rate");
    model.add_param(avg_incubation_days, "Incubation days");
    model.add_param(recovery_rate, "Recovery rate");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname);
    virus.set_state(ModelSEIR<TSeq>::EXPOSED, ModelSEIR<TSeq>::REMOVED, ModelSEIR<TSeq>::REMOVED);

    virus.set_prob_infecting(&model("Transmission rate"));
    virus.set_incubation(&model("Incubation days"));
    virus.set_prob_recovery(&model("Recovery rate"));
    
    // Adding the tool and the virus
    model.add_virus(virus, prevalence);
    
    model.set_name("Susceptible-Exposed-Infected-Removed (SEIR)");

    return;
   
}

template<typename TSeq>
inline ModelSEIR<TSeq>::ModelSEIR(
    std::string vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate
    )
{

    ModelSEIR<TSeq>(
        *this,
        vname,
        prevalence,
        transmission_rate,
        avg_incubation_days,
        recovery_rate
        );

    return;

}

template<typename TSeq>
inline void ModelSEIR<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /**/
) {

    // Checking widths
    size_t nstates = this->get_states().size();
    if (proportions_.size() != 2u) {
        throw std::invalid_argument("-proportions_- must have two entries.");
    }

    // proportions_ are values between 0 and 1, otherwise error
    for (auto & v : proportions_)
        if ((v < 0.0) || (v > 1.0))
            throw std::invalid_argument(
                "-proportions_- must have values between 0 and 1."
                );

    // Creating function
    std::function<void(epiworld::Model<TSeq>*)> fun =
    [proportions_] (epiworld::Model<TSeq> * model) -> void {

        // Figuring out information about the viruses
        double tot = 0.0;
        const auto & vpreval = model->get_prevalence_virus();
        const auto & vprop   = model->get_prevalence_virus_as_proportion();
        double n   = static_cast<double>(model->size());
        for (size_t i = 0u; i < model->get_n_viruses(); ++i)
        {
            if (vprop[i])
                tot += vpreval[i];
            else
                tot += vpreval[i] / n;
        }

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        size_t nexposed   = proportions_[0u] * tot * n;
        size_t nrecovered = proportions_[1u] * tot_left * n;
        
        epiworld::AgentsSample<TSeq> sample_suscept(
            *model,
            nrecovered,
            {0u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_suscept)
            agent->change_state(model, 3, Queue<TSeq>::NoOne);

        epiworld::AgentsSample<TSeq> sample_exposed(
            *model,
            nexposed,
            {1u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_exposed)
            agent->change_state(model, 2, Queue<TSeq>::NoOne);
        
        // Running the actions
        model->actions_run();

        return;

    };

    Model<TSeq>::initial_states_fun = fun;

    return;

}

#endif
