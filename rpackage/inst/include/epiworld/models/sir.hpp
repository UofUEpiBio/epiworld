#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

#include "../epiworld.hpp"

/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_contagion_reduction epiworld_double Initial contagion_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery rate of the immune system
 */
template<typename TSeq>
inline void set_up_sir(
    epiworld::Model<TSeq> & model,
    std::string vname,
    epiworld_double prevalence,
    epiworld_double contagion_reduction,
    epiworld_double recovery,
    epiworld_double post_immunity
    )
{

    EPI_NEW_VIRUSFUN_LAMBDA(add_immunity,TSeq) {

        EPI_NEW_TOOL_LAMBDA(virus_immune,TSeq) {return MPAR(0);};

        epiworld::Tool<TSeq> immune;
        immune.set_contagion_reduction(virus_immune);
        immune.set_param("Post immunity", *m);
        p->add_tool(m->today(), immune);
        return;

    };

    EPI_NEW_TOOL_LAMBDA(immune_contagion_reduction,TSeq) {return MPAR(0);};
    EPI_NEW_TOOL_LAMBDA(immune_recovery,TSeq) {return MPAR(1);};
    EPI_NEW_TOOL_LAMBDA(immune_transmision,TSeq) {return MPAR(1);};

    // Preparing the virus
    epiworld::Virus<TSeq> virus(true, vname);
    virus.set_post_recovery(add_immunity);

    // Preparing the immune system
    epiworld::Tool<TSeq> immune_sys(true, "Immune system");
    immune_sys.set_contagion_reduction(immune_contagion_reduction);
    immune_sys.set_recovery(immune_recovery);

    immune_sys.add_param(contagion_reduction, "Immune contagion_reduction", model);
    immune_sys.add_param(recovery, "Immune recovery", model);
    immune_sys.add_param(post_immunity, "Post immunity", model);

    // Adding the tool and the virus
    model.add_tool(immune_sys, 1.0);
    model.add_virus(virus, prevalence);

    return;

}

#endif
