#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

enum SURVSTATUS {
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

    p->add_virus(variants[which]); 
    return m->get_default_exposed();

}

template<typename TSeq>
EPI_NEW_UPDATEFUN(surveillance_update_exposed,TSeq)
{

    EPIWORLD_UPDATE_EXPOSED_CALC_PROBS(p_rec, p_die)
    
    unsigned int days_since_exposed = m->today() - v->get_date();
    epiworld_fast_uint status = p->get_status();

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
    if (days_since_exposed <= v->get_data()[0u])
        return status;

    // If past days infected + latent, then bye.
    if (days_since_exposed >= v->get_data()[1u])
    {
        p->rm_virus(v);
        return SURVSTATUS::RECOVERED;
    }

    // If it is infected, then it can be asymptomatic or symptomatic
    if (status == SURVSTATUS::LATENT)
    {

        // Will be symptomatic?
        if (EPI_RUNIF() < MPAR(2))
        {
            // If you are symptomatic, then you may be catched
            return static_cast<epiworld_fast_uint>(SURVSTATUS::SYMPTOMATIC);

        }
        
        return static_cast<epiworld_fast_uint>(SURVSTATUS::ASYMPTOMATIC);

    }
    
    // Otherwise, it can be removed
    if (EPI_RUNIF() < p_die)
    {
        p->rm_virus(v);
        return SURVSTATUS::REMOVED;
    }
    
    return p->get_status();

}

template<typename TSeq>
EPI_NEW_POSTRECOVERYFUN(post_covid, TSeq)
{

    epiworld::Tool<TSeq> immunity;

    immunity.set_susceptibility_reduction(1.0 - MPAR(8));

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
    std::vector< bool > sampled(m->size(), false);
    
    while (to_go-- > 0)
    {

        // Who is the lucky one
        unsigned int i = static_cast<unsigned int>(std::floor(EPI_RUNIF() * m->size()));

        if (sampled[i])
            continue;

        sampled[i] = true;
        epiworld::Person<TSeq> * p = &pop[i];
        
        // If still exposed for the next term
        if (epiworld::IN(p->get_status_next(), m->get_status_exposed()))
        {

            ndetected += 1.0;
            if (p->get_status() == SURVSTATUS::ASYMPTOMATIC)
            {
                ndetected_asympt += 1.0;
                p->update_status(SURVSTATUS::ASYMPTOMATIC_ISOLATED);
            }
            else 
            {
                p->update_status(SURVSTATUS::SYMPTOMATIC_ISOLATED);
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
            static_cast<epiworld_double>(totals[SURVSTATUS::ASYMPTOMATIC])
        }
        );


}


/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_susceptibility_reduction epiworld_double Initial susceptibility_reduction of the immune system
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
    model.add_param(prop_vax_redux_transm, "Vax redux transmission");
    model.add_param(prob_transmission, "Prob of transmission");
    model.add_param(prob_death, "Prob. death");
    model.add_param(prob_reinfect, "Prob. reinfect");

    // Virus ------------------------------------------------------------------
    epiworld::Virus<TSeq> covid("Covid19");
    covid.set_post_recovery(post_covid<TSeq>);
    covid.set_prob_death(&model("Prob. death"));

    EPI_NEW_VIRUSFUN_LAMBDA(ptransmitfun, TSeq)
    {
        // No chance of infecting
        epiworld_fast_uint  s = p->get_status();
        if (s == SURVSTATUS::LATENT)
            return static_cast<epiworld_double>(0.0);
        else if (s == SURVSTATUS::SYMPTOMATIC_ISOLATED)
            return static_cast<epiworld_double>(0.0);
        else if (s == SURVSTATUS::ASYMPTOMATIC_ISOLATED)
            return static_cast<epiworld_double>(0.0);

        // Otherwise
        return MPAR(6);
    };

    covid.set_prob_infecting_fun(ptransmitfun);
    
    model.add_virus_n(covid, prevalence);

    model.set_user_data({"nsampled", "ndetected", "ndetected_asympt", "nasymptomatic"});
    model.add_global_action(surveilance<TSeq>,-1);
   
    // Vaccine tool -----------------------------------------------------------
    epiworld::Tool<TSeq> vax("Vaccine");
    vax.set_susceptibility_reduction(&model("Vax efficacy"));
    vax.set_transmission_reduction(&model("Vax redux transmission"));
    
    model.add_tool(vax, prop_vaccinated);

    std::vector< epiworld_fast_uint > new_status =
        {
            SURVSTATUS::SUSCEPTIBLE, SURVSTATUS::LATENT, SURVSTATUS::RECOVERED
        };

    model.reset_status_codes(
        new_status,
        {"susceptible", "latent", "recovered"},
        false
    );

    model.add_status_exposed(SURVSTATUS::SYMPTOMATIC, "symptomatic");
    model.add_status_exposed(SURVSTATUS::SYMPTOMATIC_ISOLATED, "symptomatic isolated");
    model.add_status_exposed(SURVSTATUS::ASYMPTOMATIC, "asymptomatic");
    model.add_status_exposed(SURVSTATUS::ASYMPTOMATIC_ISOLATED, "asymptomatic isolated");
    model.add_status_removed(SURVSTATUS::REMOVED, "removed");

    model.set_update_exposed(surveillance_update_exposed<TSeq>);
    model.set_update_susceptible(surveillance_update_susceptible<TSeq>);

    return;

}

#endif
