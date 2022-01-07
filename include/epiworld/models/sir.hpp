#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

#include "../epiworld.hpp"

/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param initial_prevalence double Initial prevalence
 * @param initial_efficacy double Initial efficacy of the immune system
 * @param initial_recovery double Initial recovery rate of the immune system
 */
template<typename TSeq>
inline void set_up_sir(
    epiworld::Model<TSeq> & model,
    std::string vname,
    double prevalence,
    double efficacy,
    double recovery,
    double post_immunity
    )
{

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(true, vname);

    EPI_NEW_POSTRECFUN_LAMBDA(add_immunity,TSeq) {

        EPI_NEW_TOOL_LAMBDA(virus_immune,TSeq) {return MPAR(0);};

        epiworld::Tool<TSeq> immune;
        immune.set_efficacy(virus_immune);
        immune.set_param("Post immunity", *m);
        p->add_tool(m->today(), immune);
        return;

    };
    
    virus.set_post_recovery(add_immunity);
    virus.add_param(post_immunity, "Post immunity", model);

    // Preparing the immune system -----------------------------------
    epiworld::Tool<TSeq> immune_sys(true, "Immune system");
    
    EPI_NEW_TOOL_LAMBDA(immune_efficacy,TSeq) {return MPAR(0);};
    EPI_NEW_TOOL_LAMBDA(immune_recovery,TSeq) {return MPAR(1);};
    
    immune_sys.set_efficacy(immune_efficacy);
    immune_sys.set_recovery(immune_recovery);

    immune_sys.add_param(efficacy, "Immune efficacy", model);
    immune_sys.add_param(recovery, "Immune recovery", model);

    // Adding the tool and the virus
    model.add_tool(immune_sys, 1.0);
    model.add_virus(virus, prevalence);

    return;

}

#endif
