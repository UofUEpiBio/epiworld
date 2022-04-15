#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery rate of the immune system
 */
template<typename TSeq = bool>
inline void set_up_sir(
    epiworld::Model<TSeq> & model,
    std::string vname,
    epiworld_double prevalence,
    epiworld_double infectiousness,
    epiworld_double susceptibility_reduction,
    epiworld_double recovery,
    epiworld_double post_immunity
    )
{

    // Setting up parameters
    model.add_param(post_immunity, "Post immunity");
    model.add_param(susceptibility_reduction, "Immune suscept. redux.");
    model.add_param(recovery, "Immune recovery");
    model.add_param(infectiousness, "Infectiousness");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname);

    EPI_NEW_POSTRECOVERYFUN_LAMBDA(add_immunity,TSeq) {

        epiworld::Tool<TSeq> immune;
        immune.set_susceptibility_reduction(&MPAR(0));
        p->add_tool(m->today(), immune);
        return;

    };
    virus.set_prob_infecting(&model("Infectiousness"));
    virus.set_post_recovery(add_immunity);
    
    // Preparing the immune system -----------------------------------
    epiworld::Tool<TSeq> immune_sys("Immune system");
    
    immune_sys.set_susceptibility_reduction(&model("Immune suscept. redux."));
    immune_sys.set_recovery_enhancer(&model("Immune recovery"));
   
    // Adding the tool and the virus
    model.add_tool(immune_sys, 1.0);
    model.add_virus(virus, prevalence);

    return;

}

#endif
