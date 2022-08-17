#ifndef EPIWORLD_MODELS_SURVEILLANCE_HPP
#define EPIWORLD_MODELS_SURVEILLANCE_HPP

template<typename TSeq>
class ModelSURV : public epiworld::Model<TSeq> {

private:
    // Status
    static const int SUSCEPTIBLE           = 0;
    static const int LATENT                = 1;
    static const int SYMPTOMATIC           = 2;
    static const int SYMPTOMATIC_ISOLATED  = 3; // sampled and discovered
    static const int ASYMPTOMATIC          = 4;
    static const int ASYMPTOMATIC_ISOLATED = 5;
    static const int RECOVERED             = 6;
    static const int REMOVED               = 7;

public:

};

namespace SURVSTATUS {



    template<typename TSeq>
    EPI_NEW_UPDATEFUN(surveillance_update_susceptible, TSeq) {

        // This computes the prob of getting any neighbor variant
        unsigned int nvariants_tmp = 0u;
        for (auto & neighbor: p->get_neighbors()) 
        {
                    
            for (size_t i = 0u; i < neighbor->get_n_viruses(); ++i) 
            { 

                auto & v = neighbor->get_virus(i);
                    
                /* And it is a function of susceptibility_reduction as well */ 
                epiworld_double tmp_transmission = 
                    (1.0 - p->get_susceptibility_reduction(v)) * 
                    v->get_prob_infecting() * 
                    (1.0 - neighbor->get_transmission_reduction(v)) 
                    ; 
            
                m->array_double_tmp[nvariants_tmp]  = tmp_transmission;
                m->array_virus_tmp[nvariants_tmp++] = &(*v);
                
            } 
        }

        // No virus to compute on
        if (nvariants_tmp == 0)
            return;

        // Running the roulette
        int which = roulette(nvariants_tmp, m);

        if (which < 0)
            return;

        p->add_virus(*m->array_virus_tmp[which]); 
        return;

    }

    template<typename TSeq>
    EPI_NEW_UPDATEFUN(surveillance_update_exposed,TSeq)
    {

        epiworld::VirusPtr<TSeq> & v = p->get_virus(0u); 
        epiworld_double p_die = v->get_prob_death() * (1.0 - p->get_death_reduction(v)); 
        
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
            return;

        // If past days infected + latent, then bye.
        if (days_since_exposed >= v->get_data()[1u])
        {
            p->rm_virus(0);
            return;
        }

        // If it is infected, then it can be asymptomatic or symptomatic
        if (status == SURVSTATUS::LATENT)
        {

            // Will be symptomatic?
            if (EPI_RUNIF() < MPAR(2))
                p->change_status(SURVSTATUS::SYMPTOMATIC);
            else
                p->change_status(SURVSTATUS::ASYMPTOMATIC);
            
            return;

        }
        
        // Otherwise, it can be removed
        if (EPI_RUNIF() < p_die)
        {
            p->change_status(SURVSTATUS::REMOVED, -1);
            return;
        }
        
        return;

    }

    std::vector< epiworld_fast_uint > exposed_status = {
        SYMPTOMATIC,
        SYMPTOMATIC_ISOLATED,
        ASYMPTOMATIC,
        ASYMPTOMATIC_ISOLATED,
        LATENT
    };


}



// Surveilance function
template<typename TSeq>
EPI_NEW_GLOBALFUN(surveillance_program, TSeq)
{

    // How many will we find
    std::binomial_distribution<> bdist(m->size(), m->par("Surveilance prob."));
    int nsampled = bdist(*m->get_rand_endgine());

    int to_go = nsampled + 1;

    epiworld_double ndetected        = 0.0;
    epiworld_double ndetected_asympt = 0.0;
    
    auto & pop = *(m->get_agents());
    std::vector< bool > sampled(m->size(), false);
    
    while (to_go-- > 0)
    {

        // Who is the lucky one
        unsigned int i = static_cast<unsigned int>(std::floor(EPI_RUNIF() * m->size()));

        if (sampled[i])
            continue;

        sampled[i] = true;
        epiworld::Agent<TSeq> * p = &pop[i];
        
        // If still exposed for the next term
        if (epiworld::IN(p->get_status(), SURVSTATUS::exposed_status ))
        {

            ndetected += 1.0;
            if (p->get_status() == SURVSTATUS::ASYMPTOMATIC)
            {
                ndetected_asympt += 1.0;
                p->change_status(SURVSTATUS::ASYMPTOMATIC_ISOLATED);
            }
            else 
            {
                p->change_status(SURVSTATUS::SYMPTOMATIC_ISOLATED);
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
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline void surveillance(
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
    epiworld_double prob_noreinfect       = 0.9
    )
{

    model.add_status("Susceptible", SURVSTATUS::surveillance_update_susceptible<TSeq>);
    model.add_status("Latent", SURVSTATUS::surveillance_update_exposed<TSeq>);
    model.add_status("Symptomatic", SURVSTATUS::surveillance_update_exposed<TSeq>);
    model.add_status("Symptomatic isolated", SURVSTATUS::surveillance_update_exposed<TSeq>);
    model.add_status("Asymptomatic", SURVSTATUS::surveillance_update_exposed<TSeq>);
    model.add_status("Asymptomatic isolated", SURVSTATUS::surveillance_update_exposed<TSeq>);
    model.add_status("Recovered");
    model.add_status("Removed");

    // General model parameters
    model.add_param(latent_period, "Latent period");
    model.add_param(infect_period, "Infect period");
    model.add_param(prob_symptoms, "Prob of symptoms");
    model.add_param(surveillance_prob, "Surveilance prob.");
    model.add_param(efficacy_vax, "Vax efficacy");
    model.add_param(prop_vax_redux_transm, "Vax redux transmission");
    model.add_param(prob_transmission, "Prob of transmission");
    model.add_param(prob_death, "Prob. death");
    model.add_param(prob_noreinfect, "Prob. no reinfect");

    // Virus ------------------------------------------------------------------
    epiworld::Virus<TSeq> covid("Covid19");
    covid.set_status(SURVSTATUS::LATENT, SURVSTATUS::RECOVERED, SURVSTATUS::REMOVED);
    covid.set_post_immunity(&model("Prob. no reinfect"));
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
    model.add_global_action(surveillance_program<TSeq>,-1);
   
    // Vaccine tool -----------------------------------------------------------
    epiworld::Tool<TSeq> vax("Vaccine");
    vax.set_susceptibility_reduction(&model("Vax efficacy"));
    vax.set_transmission_reduction(&model("Vax redux transmission"));
    
    model.add_tool(vax, prop_vaccinated);

    return;

}

template<typename TSeq = EPI_DEFAULT_TSEQ>
inline epiworld::Model<TSeq> surveillance(
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
    epiworld_double prob_noreinfect       = 0.9
    )
{

    epiworld::Model<TSeq> model;

    surveillance<TSeq>(
        model,
        vname,
        prevalence,
        efficacy_vax,
        latent_period,
        infect_period,
        prob_symptoms,
        prop_vaccinated,
        prop_vax_redux_transm,
        prop_vax_redux_infect,
        surveillance_prob,
        prob_transmission,
        prob_death,
        prob_noreinfect
    );

    return model;

}

#endif
