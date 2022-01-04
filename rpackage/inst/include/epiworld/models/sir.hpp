#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

#include "../epiworld.hpp"

EPI_NEW_TOOL(virus_immune,bool) {
    return 1.0;
}

EPI_POSTRECFUN(add_immunity,bool) {
    epiworld::Tool<bool> immune;
    immune.set_efficacy(virus_immune);
    p->add_tool(m->today(), immune);
    return;
}

EPI_NEW_TOOL(immune_efficacy,bool) {
    return *(t->p00);
}

EPI_NEW_TOOL(immune_recovery,bool) {
    return *(t->p01);
}

inline void set_up_sir(
    epiworld::Model<bool> & m,
    std::string vname,
    double initial_prevalence,
    unsigned int dur
    )
{

    // Preparing the virus
    epiworld::Virus<bool> virus(true, vname);
    virus.set_post_recovery(add_immunity);

    // Preparing the immune system
    epiworld::Tool<bool> immune_sys(true, "immune system");
    immune_sys.set_efficacy(immune_efficacy);
    immune_sys.set_recovery(immune_recovery);

    immune_sys.add_param(.5, "Immune efficacy", m);
    immune_sys.add_param(.5, "Immune recovery", m);

    // Adding the tool and the virus
    m.add_tool(immune_sys, 1.0);
    m.add_virus(virus, initial_prevalence);

    // Other parameters
    m.set_ndays(dur);

    return;

}

#endif
