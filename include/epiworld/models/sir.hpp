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
template<typename TSeq = int>
inline void sir(
    epiworld::Model<TSeq> & model,
    std::string vname,
    epiworld_double prevalence,
    epiworld_double infectiousness,
    epiworld_double susceptibility_reduction,
    epiworld_double recovery,
    epiworld_double post_immunity
    )
{

    // Adding statuses
    model.add_status("Susceptible", epiworld::default_update_susceptible<TSeq>);
    model.add_status("Infected", epiworld::default_update_exposed<TSeq>);
    model.add_status("Recovered");
    model.add_status("Gained Immunity");

    // Setting up parameters
    model.add_param(post_immunity, "Post immunity");
    model.add_param(susceptibility_reduction, "Immune suscept. redux.");
    model.add_param(recovery, "Immune recovery");
    model.add_param(infectiousness, "Infectiousness");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname);
    virus.set_status(1,2,2);

    epiworld::Tool<TSeq> immune("Immunity (" + vname + ")");
    immune.set_susceptibility_reduction(&model("Post immunity"));

    model.add_tool_n(immune, 0u);
    
    EPI_NEW_POSTRECOVERYFUN_LAMBDA(add_immunity,TSeq) {

        if (m->runif() > .5)
            p->add_tool(m->get_tools()[0u], 3);

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
   
}

template<typename TSeq = int>
inline epiworld::Model<TSeq> sir(
    std::string vname,
    epiworld_double prevalence,
    epiworld_double infectiousness,
    epiworld_double susceptibility_reduction,
    epiworld_double recovery,
    epiworld_double post_immunity
    )
{

    epiworld::Model<TSeq> model;

    sir(
        model,
        vname,
        prevalence,
        infectiousness,
        susceptibility_reduction,
        recovery,
        post_immunity
        );



    return model;

}

#endif
