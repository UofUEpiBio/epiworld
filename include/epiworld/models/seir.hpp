#ifndef EPIWORLD_MODELS_SEIR_HPP
#define EPIWORLD_MODELS_SEIR_HPP

/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery rate of the immune system
 */
template<typename TSeq = int>
class ModelSEIR : public epiworld::Model<TSeq>
{
private:
    static const int SUSCEPTIBLE = 0;
    static const int EXPOSED     = 1;
    static const int INFECTED    = 2;
    static const int REMOVED     = 3;

public:

    ModelSEIR() {};

    ModelSEIR(
        ModelSEIR<TSeq> & model,
        std::string vname,
        epiworld_double prevalence,
        epiworld_double infectiousness,
        epiworld_double incubation_days,
        epiworld_double recovery
    );

    ModelSEIR(
        std::string vname,
        epiworld_double prevalence,
        epiworld_double infectiousness,
        epiworld_double incubation_days,
        epiworld_double recovery
    );
    
    epiworld::UpdateFun<TSeq> update_exposed_seir = [](
        epiworld::Agent<TSeq> * p,
        epiworld::Model<TSeq> * m
    ) -> void {
        // Does the agent become infected?
        if (m->runif() < 1.0/(*m->p1))
            p->change_status(m, ModelSEIR<TSeq>::INFECTED);

        return;    
    };
      

    epiworld::UpdateFun<TSeq> update_infected_seir = [](
        epiworld::Agent<TSeq> * p,
        epiworld::Model<TSeq> * m
    ) -> void {
        // Does the agent recover?
        if (m->runif() < (*m->p2))
            p->rm_virus(0, m);

        return;    
    };

};


template<typename TSeq>
inline ModelSEIR<TSeq>::ModelSEIR(
    ModelSEIR<TSeq> & model,
    std::string vname,
    epiworld_double prevalence,
    epiworld_double infectiousness,
    epiworld_double incubation_days,
    epiworld_double recovery
    )
{

    // Adding statuses
    model.add_status("Susceptible", epiworld::default_update_susceptible<TSeq>);
    model.add_status("Exposed", model.update_exposed_seir);
    model.add_status("Infected", model.update_infected_seir);
    model.add_status("Removed");

    // Setting up parameters
    model.add_param(infectiousness, "Infectiousness");
    model.add_param(incubation_days, "Incubation days");
    model.add_param(recovery, "Immune recovery");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname);
    virus.set_status(ModelSEIR<TSeq>::EXPOSED, ModelSEIR<TSeq>::REMOVED, ModelSEIR<TSeq>::REMOVED);

    virus.set_prob_infecting(&model("Infectiousness"));
    
    // Adding the tool and the virus
    model.add_virus(virus, prevalence);
    
    model.set_name("Susceptible-Exposed-Infected-Removed (SEIR)");

    return;
   
}

template<typename TSeq>
inline ModelSEIR<TSeq>::ModelSEIR(
    std::string vname,
    epiworld_double prevalence,
    epiworld_double infectiousness,
    epiworld_double incubation_days,
    epiworld_double recovery
    )
{

    ModelSEIR<TSeq>(
        *this,
        vname,
        prevalence,
        infectiousness,
        incubation_days,
        recovery
        );

    return;

}



#endif
