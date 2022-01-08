#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

#include "../epiworld.hpp"

/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param initial_prevalence double Initial prevalence
 * @param initial_efficacy double Initial contagion_reduction of the immune system
 * @param initial_recovery double Initial recovery rate of the immune system
 */
template<typename TSeq>
inline void set_up_sir(
    epiworld::Model<TSeq> & model,
    std::string vname,
    double prevalence,
    double contagion_reduction,
    double recovery,
    double post_immunity
    )
{

    // Setting up parameters
    model.add_param(post_immunity, "Post immunity");
    model.add_param(contagion_reduction, "Immune contagion_reduction");
    model.add_param(recovery, "Immune recovery");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(true, vname);

    EPI_NEW_VIRUSFUN_LAMBDA(add_immunity,TSeq) {

        epiworld::Tool<TSeq> immune;
        immune.set_contagion_reduction(&MPAR(0));
        p->add_tool(m->today(), immune);
        return;

    };
    
    virus.set_post_recovery(add_immunity);
    

    // Preparing the immune system -----------------------------------
    epiworld::Tool<TSeq> immune_sys(true, "Immune system");
    
    immune_sys.set_contagion_reduction(&model("Immune contagion_reduction"));
    immune_sys.set_recovery(&model("Immune recovery"));
   
    // Adding the tool and the virus
    model.add_tool(immune_sys, 1.0);
    model.add_virus(virus, prevalence);

    return;

}

#endif
