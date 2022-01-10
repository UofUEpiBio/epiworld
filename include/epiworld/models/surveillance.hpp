#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

#include "../epiworld.hpp"

enum JAYSTATUS {
    SUSCEPTIBLE,
    LATENT,
    SYMPTOMATIC,
    SYMPTOMATIC_ISOLATED, // sampled and discovered
    ASYMPTOMATIC,
    ASYMPTOMATIC_ISOLATED,
    RECOVERED,
    REMOVED
};

enum TOOLS {
    IMMUNESYS,
    VACCINE
};

template<typename TSeq>
EPI_NEW_UPDATEFUN(surveillance_update_susceptible, TSeq) {

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
    p->get_virus(0).get_data().clear();

    return static_cast<unsigned int>(JAYSTATUS::LATENT); 

}

template<typename TSeq>
EPI_NEW_UPDATEFUN(surveillance_update_infected,TSeq)
{

    EPIWORLD_UPDATE_INFECTED_CALC_PROBS(p_rec, p_die)
    
    unsigned int days_since_infected = m->today() - v->get_date();
    unsigned int status = p->get_status();

    // Figuring out latent period
    if (v->get_data().size() == 0u)
    {
        epiworld_double latent_days = m->rgamma(MPAR(0), 1.0);
        v->get_data().push_back(latent_days);

        v->get_data().push_back(
            m->rgamma(MPAR(1), 1.0) + latent_days
        );
    }
    
    // If still latent, nothing happens
    if (days_since_infected <= v->get_data()[0u])
        return status;

    // If past days infected + latent, then bye.
    if (days_since_infected >= v->get_data()[1u])
        EPIWORLD_UPDATE_INFECTED_REMOVE(JAYSTATUS::RECOVERED);

    // If it is infected, then it can be asymptomatic or symptomatic
    if (status == JAYSTATUS::LATENT)
    {
        // Will be symptomatic?
        if (EPI_RUNIF() < MPAR(2))
        {
            // If you are symptomatic, then you may be catched
            EPIWORLD_UPDATE_INFECTED_REMOVE(JAYSTATUS::SYMPTOMATIC);

        }
        
        EPIWORLD_UPDATE_INFECTED_REMOVE(JAYSTATUS::ASYMPTOMATIC);

    }
    
    // Otherwise, it can be removed
    if (EPI_RUNIF() < p_die)
        EPIWORLD_UPDATE_INFECTED_REMOVE(JAYSTATUS::REMOVED);
    
    return p->get_status();

}

template<typename TSeq>
EPI_NEW_POSTRECOVERYFUN(post_covid, TSeq)
{

    epiworld::Tool<TSeq> immunity;

    immunity.set_contagion_reduction(1.0 - MPAR(8));

    p->add_tool(m->today(), immunity);

}

// Surveilance function
template<typename TSeq>
EPI_NEW_GLOBALFUN(surveilance, TSeq)
{

    // How many will we find
    std::binomial_distribution<> bdist(m->size(), m->par("Surveilance prob."));
    int nsampled = bdist(*m->get_rand_endgine());

    int to_go = nsampled + 1;

    epiworld_double ndetected        = 0.0;
    epiworld_double ndetected_asympt = 0.0;
    
    auto & pop = *(m->get_population());
    
    while (to_go-- > 0)
    {

        // Who is the lucky one
        unsigned int i = static_cast<unsigned int>(std::floor(EPI_RUNIF() * m->size()));
        epiworld::Person<TSeq> * p = &pop[i];
        if (epiworld::IN(p->get_status(), m->get_status_infected()))
        {

            ndetected += 1.0;
            if (p->get_status() == JAYSTATUS::ASYMPTOMATIC)
            {
                ndetected_asympt += 1.0;

                m->get_db().down_infected(
                    &p->get_virus(0u), p->get_status(), JAYSTATUS::ASYMPTOMATIC_ISOLATED
                    );
                
                p->get_status() = JAYSTATUS::ASYMPTOMATIC_ISOLATED;
            }
            else 
            {

                m->get_db().down_infected(
                    &p->get_virus(0u), p->get_status(), JAYSTATUS::SYMPTOMATIC_ISOLATED
                    );
                
                p->get_status() = JAYSTATUS::SYMPTOMATIC_ISOLATED;

            }

            

        }

    }

    // Writing the user data
    std::vector< int > totals;
    m->get_db().get_today_total(nullptr,&totals);
    m->add_user_data(
        {
            static_cast<epiworld_double>(nsampled),
            ndetected,
            ndetected_asympt,
            static_cast<epiworld_double>(totals[JAYSTATUS::ASYMPTOMATIC])
        }
        );


}


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
inline void set_up_surveillance(
    epiworld::Model<TSeq> & model,
    std::string vname,
    unsigned int prevalence               = 50,
    epiworld_double efficacy_vax          = 0.9,
    epiworld_double latent_period         = 3u,
    epiworld_double infect_period         = 6u,
    epiworld_double prob_symptoms         = 0.6,
    epiworld_double prop_vaccinated       = 0.25,
    epiworld_double prop_vax_redux_transm = 0.5,
    epiworld_double prop_vax_redux_infect = 0.5,
    epiworld_double surveillance_prob     = 0.001,
    epiworld_double prob_transmission     = 1.0,
    epiworld_double prob_death            = 0.001,
    epiworld_double prob_reinfect         = 0.1
    )
{

    // General model parameters
    model.add_param(latent_period, "Latent period");
    model.add_param(infect_period, "Infect period");
    model.add_param(prob_symptoms, "Prob of symptoms");
    model.add_param(surveillance_prob, "Surveilance prob.");
    model.add_param(efficacy_vax, "Vax efficacy");
    model.add_param(prop_vax_redux_transm, "Vax redux transmision");
    model.add_param(prob_transmission, "Prob of transmission");
    model.add_param(prob_death, "Prob. death");
    model.add_param(prob_reinfect, "Prob. reinfect");

    // Virus ------------------------------------------------------------------
    epiworld::Virus<TSeq> covid("Covid19");
    covid.set_post_recovery(post_covid<TSeq>);
    covid.set_death(&model("Prob. death"));
    covid.set_persistance(0.0);

    EPI_NEW_VIRUSFUN_LAMBDA(ptransmitfun, TSeq)
    {
        // No chance of infecting
        unsigned int  s = p->get_status();
        if (s == JAYSTATUS::LATENT)
            return static_cast<epiworld_double>(0.0);
        else if (s == JAYSTATUS::SYMPTOMATIC_ISOLATED)
            return static_cast<epiworld_double>(0.0);
        else if (s == JAYSTATUS::ASYMPTOMATIC_ISOLATED)
            return static_cast<epiworld_double>(0.0);

        // Otherwise
        return MPAR(6);
    };

    covid.set_infectiousness(ptransmitfun);
    
    model.add_virus_n(covid, prevalence);

    model.set_user_data({"nsampled", "ndetected", "ndetected_asympt", "nasymptomatic"});
    model.add_global_action(surveilance<TSeq>,-1);
   
    // Vaccine tool -----------------------------------------------------------
    epiworld::Tool<TSeq> vax("Vaccine");
    vax.set_contagion_reduction(&model("Vax efficacy"));
    vax.set_transmission_reduction(&model("Vax redux transmision"));
    
    model.add_tool(vax, prop_vaccinated);

    std::vector< unsigned int > new_status =
        {
            JAYSTATUS::SUSCEPTIBLE, JAYSTATUS::LATENT, JAYSTATUS::RECOVERED,
            JAYSTATUS::REMOVED
        };

    model.reset_status_codes(
        new_status,
        {"susceptible", "latent", "recovered", "removed"},
        false
    );

    model.add_status_infected(JAYSTATUS::SYMPTOMATIC, "symptomatic");
    model.add_status_infected(JAYSTATUS::SYMPTOMATIC_ISOLATED, "symptomatic isolated");
    model.add_status_infected(JAYSTATUS::ASYMPTOMATIC, "asymptomatic");
    model.add_status_infected(JAYSTATUS::ASYMPTOMATIC_ISOLATED, "asymptomatic isolated");

    model.set_update_infected(surveillance_update_infected<TSeq>);
    model.set_update_susceptible(surveillance_update_susceptible<TSeq>);

    return;

}

#endif
