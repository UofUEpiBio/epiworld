#ifndef EPIWORLD_MODELS_SIS_HPP 
#define EPIWORLD_MODELS_SIS_HPP

/**
 * @brief Template for a Susceptible-Infected-Susceptible (SIS) model
 * 
 * @param vname std::string Name of the virus
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery rate of the immune system
 */
template<typename TSeq = int>
class ModelSIS : public epiworld::Model<TSeq>
{

public:

    ModelSIS() {};

    ModelSIS(
        ModelSIS<TSeq> & model,
        std::string vname,
        epiworld_double prevalence,
        epiworld_double infectiousness,
        epiworld_double recovery
    );

    ModelSIS(
        std::string vname,
        epiworld_double prevalence,
        epiworld_double infectiousness,
        epiworld_double recovery
    );

};

template<typename TSeq>
inline ModelSIS<TSeq>::ModelSIS(
    ModelSIS<TSeq> & model,
    std::string vname,
    epiworld_double prevalence,
    epiworld_double infectiousness,
    epiworld_double recovery
    )
{

    // Adding statuses
    model.add_status("Susceptible", epiworld::default_update_susceptible<TSeq>);
    model.add_status("Infected", epiworld::default_update_exposed<TSeq>);

    // Setting up parameters
    model.add_param(infectiousness, "Infection rate");
    model.add_param(recovery, "Recovery rate");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname);
    virus.set_status(1,0,0);
    
    virus.set_prob_infecting(&model("Infection rate"));
    virus.set_prob_recovery(&model("Recovery rate"));
    virus.set_prob_death(0.0);
    
    model.add_virus(virus, prevalence);

    return;

}

template<typename TSeq>
inline ModelSIS<TSeq>::ModelSIS(
    std::string vname,
    epiworld_double prevalence,
    epiworld_double infectiousness,
    epiworld_double recovery
    )
{

    ModelSIS<TSeq>(
        *this,
        vname,
        prevalence,
        infectiousness,
        recovery
    );    

    return;

}

#endif
