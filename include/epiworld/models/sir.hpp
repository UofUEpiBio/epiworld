#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

#include "../epiworld.hpp"

/**
 * @brief Immunity to the virus
 * 
 */
EPI_NEW_TOOL(virus_immune,bool) {
    return *(t->p00);
}

/**
 * @brief Adds immunity after recovery
 * 
 */
EPI_POSTRECFUN(add_immunity,bool) {
    epiworld::Tool<bool> immune;
    immune.set_efficacy(virus_immune);
    immune.set_param("post immunity", *m);
    p->add_tool(m->today(), immune);
    return;
}

/**
 * @brief Efficacy of the immune system
 * 
 */
EPI_NEW_TOOL(immune_efficacy,bool) {
    return *(t->p00);
}

/**
 * @brief Recovery rate of the immune system
 * 
 */
EPI_NEW_TOOL(immune_recovery,bool) {
    return *(t->p01);
}

EPI_NEW_TOOL(immune_transmision,bool) {
    return *(t->p01);
}

/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param initial_prevalence double Initial prevalence
 * @param initial_efficacy double Initial efficacy of the immune system
 * @param initial_recovery double Initial recovery rate of the immune system
 */
inline void set_up_sir(
    epiworld::Model<bool> & model,
    std::string vname,
    double initial_prevalence,
    double initial_efficacy,
    double initial_recovery
    )
{

    // Preparing the virus
    epiworld::Virus<bool> virus(true, vname);
    virus.set_post_recovery(add_immunity);

    // Preparing the immune system
    epiworld::Tool<bool> immune_sys(true, "immune system");
    immune_sys.set_efficacy(immune_efficacy);
    immune_sys.set_recovery(immune_recovery);

    immune_sys.add_param(initial_efficacy, "Immune efficacy", model);
    immune_sys.add_param(initial_recovery, "Immune recovery", model);

    // Adding the tool and the virus
    model.add_tool(immune_sys, 1.0);
    model.add_virus(virus, initial_prevalence);

    return;

}

#endif
