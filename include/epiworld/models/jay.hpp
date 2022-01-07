#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

#include "../epiworld.hpp"

enum JAYSTATUS {
    LATENT = 3u,
    SYMPTOMATIC,
    SYMPTOMATIC_ISOLATED, // sampled and discovered
    ASYMPTOMATIC
};

enum TOOLS {
    IMMUNESYS,
    VACCINE
};

template<typename TSeq>
EPI_NEW_UPDATEFUN(jay_update_susceptible, TSeq) {

    // This computes the prob of getting any neighbor variant
    EPIWORLD_UPDATE_SUSCEPTIBLE_CALC_PROBS(probs,variants)

    // No virus to compute on
    if (probs.size() == 0)
        return p->get_status();

    // Running the roulette
    int which = roulette(probs, m);

    if (which < 0)
        return p->get_status();

    EPIWORLD_ADD_VIRUS(variants[which], JAYSTATUS::LATENT)

    return static_cast<unsigned int>(JAYSTATUS::LATENT); 

}

template<typename TSeq>
EPI_NEW_UPDATEFUN(jay_update_infected,TSeq)
{

    EPIWORLD_UPDATE_INFECTED_CALC_PROBS(p_rec, p_die)
    
    unsigned int days_since_infected = m->today() - v->get_date();
    unsigned int status = p->get_status();
    
    // If still latent, nothing happens
    if (days_since_infected <= VPAR(00))
        return status;

    // If past days infected + latent, then bye.
    if (days_since_infected >= (VPAR(00) + VPAR(01)))
        EPIWORLD_UPDATE_INFECTED_REMOVE(STATUS::REMOVED);

    // If it is infected, then it can be asymptomatic or symptomatic
    if (status == JAYSTATUS::LATENT)
    {
        // Will be symptomatic?
        if (EPI_RUNIF() < VPAR(02))
        {
            // If you are symptomatic, then you may be catched
            if (EPI_RUNIF() < MPAR(00))
                EPIWORLD_UPDATE_INFECTED_REMOVE(JAYSTATUS::SYMPTOMATIC_ISOLATED);

            EPIWORLD_UPDATE_INFECTED_REMOVE(JAYSTATUS::SYMPTOMATIC);

        }
        
        EPIWORLD_UPDATE_INFECTED_REMOVE(JAYSTATUS::ASYMPTOMATIC);

    }
    
    // Otherwise, it can be removed
    double cumsum = p_die * (1 - p_rec) / (1.0 - p_die * p_rec); 
    if (EPI_RUNIF() < cumsum)
        EPIWORLD_UPDATE_INFECTED_REMOVE(STATUS::REMOVED);
    
    cumsum += p_rec * (1 - p_die) / (1.0 - p_die * p_rec);
    if (EPI_RUNIF() < cumsum)
        EPIWORLD_UPDATE_INFECTED_RECOVER(STATUS::RECOVERED)

    return p->get_status();

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
template<typename TSeq>
inline void set_up_jay(
    epiworld::Model<TSeq> & model,
    std::string vname,
    double prevalence            = 0.05,
    double efficacy_vax          = 0.9,
    unsigned int degree          = 3u,
    double latent_period         = 3u,
    double infect_period         = 6u,
    double prob_symptoms         = 0.6,
    double prop_vaccinated       = 0.25,
    double prop_vax_redux_transm = 0.5,
    double prop_vax_redux_infect = 0.5,
    double surveillance_prob     = 0.001,
    double prob_transmission     = 0.9
    )
{

    // General model parameters
    model.add_param(surveillance_prob, "Surveilance prob.");

    // Virus ------------------------------------------------------------------
    epiworld::Virus<TSeq> covid("Covid19");
    covid.add_param(latent_period, "Latent period", model);
    covid.add_param(infect_period, "Infect period", model);
    covid.add_param(prob_symptoms, "Prob of symptoms", model);
    model.add_virus(covid, prevalence);
   
    // Vaccine tool -----------------------------------------------------------
    epiworld::Tool<TSeq> vax("Vaccine");
    EPI_NEW_TOOL_LAMBDA(tool_vax_efficacy, TSeq) {return TPAR(00);};
    EPI_NEW_TOOL_LAMBDA(tool_vax_infect, TSeq) {return TPAR(01);};
        
    vax.set_efficacy(tool_vax_efficacy);
    vax.set_transmisibility(tool_vax_infect);
    vax.add_param(efficacy_vax, "Vax efficacy", model);
    vax.add_param(prop_vax_redux_transm, "Vax redux transmision", model);
    
    model.add_tool(vax, prop_vaccinated);

    // Immune system -----------------------------------------------------------
    epiworld::Tool<TSeq> immune("Immune sys.");
    EPI_NEW_TOOL_LAMBDA(tool_immune_infect, TSeq) {

        // No chance of infecting
        if (p->get_status() == JAYSTATUS::SYMPTOMATIC_ISOLATED)
            return 0.0;

        // Otherwise
        return TPAR(00);

    };

    immune.add_param(prob_transmission, "Prob of transmission", model);
    model.add_tool(immune, 1.0);


    model.add_status_infected("latent");
    model.add_status_infected("infectious");
    model.add_status_infected("symptomatic");
    model.add_status_infected("symptomatic isolated");
    model.add_status_infected("asymptomatic");

    model.set_update_infected(jay_update_infected<TSeq>);
    model.set_update_susceptible(jay_update_susceptible<TSeq>);

    return;

}

#endif
