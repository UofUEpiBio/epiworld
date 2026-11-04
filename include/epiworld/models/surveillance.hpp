#ifndef EPIWORLD_MODELS_SURVEILLANCE_HPP
#define EPIWORLD_MODELS_SURVEILLANCE_HPP

#include "../model-bones.hpp"

/**
 * @brief Template for a Surveillance model
 * @ingroup special_models
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSURV : public Model<TSeq> {

public:
        /// @name Statuses (isolated: sampled and discovered)
    ///@{
    static constexpr int FREE     = 0;
    static constexpr int ISOLATED = 1;
    ///@}

    /// @name Stages of the natural history (`get_natural_history(0)`)
    ///@{
    static constexpr int SUSCEPTIBLE  = 0;
    static constexpr int LATENT       = 1;
    static constexpr int SYMPTOMATIC  = 2;
    static constexpr int ASYMPTOMATIC = 3;
    static constexpr int RECOVERED    = 4;
    static constexpr int REMOVED      = 5;
    ///@}

    /// The model's state with this status and stage.
    static constexpr epiworld_fast_uint state(int status, int stage)
    {
        return static_cast< epiworld_fast_uint >(status + 2 * stage);
    }

    /**
     * @brief Vector of days spent in latent and infectious states
     * A row-major matrix
     */
    std::vector< epiworld_double > days_latent_and_infectious;

    /**
     * @name Construct a new ModelSURV object
     * 
     * The ModelSURV class simulates a survaillence model where agents can be
     * isolated, even if asyptomatic.
     * 
     * @param vname String. Name of the virus
 * @param n Number of agents.
     * @param prevalence Integer. Number of initial cases of the virus.
     * @param efficacy_vax Double. Efficacy of the vaccine (1 - P(acquire the disease)).
     * @param latent_period Double. Shape parameter of a `Gamma(latent_period, 1)`
     *   distribution. This coincides with the expected number of latent days.
     * @param infect_period Double. Shape parameter of a `Gamma(infected_period, 1)`
     *   distribution. This coincides with the expected number of infectious days.
     * @param prob_symptoms Double. Probability of generating symptoms.
     * @param prop_vaccinated Double. Probability of vaccination. Coincides with
     *   the initial prevalence of vaccinated individuals.
     * @param prop_vax_redux_transm Double. Factor by which the vaccine reduces
     *   transmissibility.
     * @param prop_vax_redux_infect Double. Factor by which the vaccine reduces
     *   the chances of becoming infected.
     * @param surveillance_prob Double. Probability of testing an agent.
     * @param prob_transmission Double. Raw transmission probability.
     * @param prob_death Double. Raw probability of death for symptomatic individuals.
     * @param prob_noreinfect Double. Probability of no re-infection.
     * 
     * @details
          * Each agent has a status, Free or Isolated (sampled and discovered,
     * for the rest of the run), and a stage: Susceptible, Latent,
     * Symptomatic, Asymptomatic, Recovered or Removed. Latent and isolated
     * agents do not transmit.    
     * 
     * @returns An object of class `epiworld_surv`
     * 
     */
    ///@{
    ModelSURV() = delete;

    ModelSURV(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_fast_uint prevalence         = 50,
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
    );
    ///@}

    void reset() override;

};

template<typename TSeq>
inline void ModelSURV<TSeq>::reset()
{
    Model<TSeq>::reset();

    days_latent_and_infectious.clear();
    days_latent_and_infectious.resize(
        2u * this->size(),
        -1.0
    );
}

template<typename TSeq>
inline ModelSURV<TSeq>::ModelSURV(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_fast_uint prevalence,
    epiworld_double efficacy_vax,
    epiworld_double latent_period,
    epiworld_double infect_period,
    epiworld_double prob_symptoms,
    epiworld_double prop_vaccinated,
    epiworld_double prop_vax_redux_transm,
    epiworld_double,
    epiworld_double surveillance_prob,
    epiworld_double prob_transmission,
    epiworld_double prob_death,
    epiworld_double prob_noreinfect
    )
{

    this->set_population_size(n);

    EPI_NEW_UPDATEFUN_LAMBDA(surveillance_update_susceptible, TSeq) {

        // This computes the prob of getting any neighbor variant
        epiworld_fast_uint nviruses_tmp = 0u;
        auto & m_ref = *m;
        // Every network layer, in the order the layers were added
        for (auto * net : m->get_network_layers())
        for (auto * neighbor : net->neighbors_view(static_cast< size_t >(p->get_id())))
        {
                    
            auto & v = neighbor->get_virus();

            if (v == nullptr)
                continue;
                
            /* And it is a function of susceptibility_reduction as well */ 
            epiworld_double tmp_transmission = 
                (1.0 - p->get_susceptibility_reduction(v, m_ref)) *
                v->get_prob_infecting(m) *
                (1.0 - neighbor->get_transmission_reduction(v, m_ref)) 
                ; 
        
            m->reserve_tmp(nviruses_tmp + 1u);
            m->array_double_tmp[nviruses_tmp]  = tmp_transmission;
            m->array_virus_tmp[nviruses_tmp++] = &(*v);
        }

        // No virus to compute on
        if (nviruses_tmp == 0)
            return;

        // Running the roulette
        int which = roulette(nviruses_tmp, m);

        if (which < 0)
            return;

        p->set_virus(*m, *m->array_virus_tmp[which]);
        return;

    };


    UpdateFun<TSeq> surveillance_update_exposed = 
    [](Agent<TSeq> * p, Model<TSeq> * m) -> void
    {

        // Dynamically getting the ModelSURV
        ModelSURV<TSeq> * model_surv = model_cast<ModelSURV<TSeq>,TSeq>(m);

        VirusPtr<TSeq> & v = p->get_virus(); 
        epiworld_double p_die = v->get_prob_death(m) * (1.0 - p->get_death_reduction(v, *m));
        
        epiworld_fast_uint days_since_exposed = m->today() - v->get_date();
                const int stage = p->get_stage(*m);

        // Figuring out latent period
        auto & dat = model_surv->days_latent_and_infectious;
                if (dat[p->get_id() * 2u] < 0)
        {
            epiworld_double latent_days = m->rgamma(
                EPI_PAR(m, "Latent period"), 1.0
            );

            dat[p->get_id() * 2u] = latent_days;

            dat[p->get_id() * 2u + 1u] = 
                m->rgamma(EPI_PAR(m, "Infect period"), 1.0) +
                latent_days;
        }
        
        // If still latent, nothing happens
        if (days_since_exposed <= dat[p->get_id() * 2u])
            return;

        // If past days infected + latent, then bye.
        if (days_since_exposed >= dat[p->get_id() * 2u + 1u])
        {
            p->rm_virus(*m);
            return;
        }

        // If it is infected, then it can be asymptomatic or symptomatic
                if (stage == ModelSURV<TSeq>::LATENT)
        {

            // Will be symptomatic?
            if (EPI_RUNIF() < EPI_PAR(m, "Prob of symptoms"))
                p->change_stage(*m, ModelSURV<TSeq>::SYMPTOMATIC);
            else
                p->change_stage(*m, ModelSURV<TSeq>::ASYMPTOMATIC);
            
            return;

        }
        
        // Otherwise, it can be removed
        if (EPI_RUNIF() < p_die)
        {
                        p->rm_virus(
                *m,
                m->state_with(p->get_state(), -1, 0u, ModelSURV<TSeq>::REMOVED),
                -1
            );
            return;
        }
        
        return;

    };

    

    GlobalFun<TSeq> surveillance_program = 
    [](
        Model<TSeq>* m
        ) -> void
    {

        // How many will we find
        std::binomial_distribution<> bdist(m->size(), EPI_PAR(m, "Surveilance prob."));
        int nsampled = bdist(*m->get_rand_endgine());

        int to_go = nsampled + 1;

        epiworld_double ndetected        = 0.0;
        epiworld_double ndetected_asympt = 0.0;
        
        auto & pop = m->get_agents();
        std::vector< bool > sampled(m->size(), false);
        
        while (to_go-- > 0)
        {

            // Who is the lucky one
            epiworld_fast_uint i = m->runif_index(m->size());

            if (sampled[i])
                continue;

            sampled[i] = true;
            Agent<TSeq> * p = &pop[i];
            
            // If still exposed for the next term
                        const int stage = p->get_stage(*m);
            if (
                (stage == ModelSURV<TSeq>::LATENT) ||
                (stage == ModelSURV<TSeq>::SYMPTOMATIC) ||
                (stage == ModelSURV<TSeq>::ASYMPTOMATIC)
            )
            {
                ndetected += 1.0;
                if (stage == ModelSURV<TSeq>::ASYMPTOMATIC)
                    ndetected_asympt += 1.0;
                p->change_status(*m, ModelSURV<TSeq>::ISOLATED);
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
                static_cast<epiworld_double>(totals[
                    ModelSURV<TSeq>::state(
                        ModelSURV<TSeq>::FREE, ModelSURV<TSeq>::ASYMPTOMATIC
                    )
                ])
            }
            );


    };

        // Statuses and the natural history
    this->add_state("Free");
    this->add_state("Isolated");

    NaturalHistory<TSeq> nh("Surveillance");
    nh.add_stage("Susceptible", surveillance_update_susceptible);
    nh.add_stage("Latent", surveillance_update_exposed);
    nh.add_stage("Symptomatic", surveillance_update_exposed);
    nh.add_stage("Asymptomatic", surveillance_update_exposed);
    nh.add_stage("Recovered");
    nh.add_stage("Removed");
    nh.set_infection(LATENT, RECOVERED, REMOVED);
    this->add_natural_history(nh);

    // General model parameters
    this->add_param(latent_period, "Latent period");
    this->add_param(infect_period, "Infect period");
    this->add_param(prob_symptoms, "Prob of symptoms");
    this->add_param(surveillance_prob, "Surveilance prob.");
    this->add_param(efficacy_vax, "Vax efficacy");
    this->add_param(prop_vax_redux_transm, "Vax redux transmission");
    this->add_param(prob_transmission, "Prob of transmission");
    this->add_param(prob_death, "Prob. death");
    this->add_param(prob_noreinfect, "Prob. no reinfect");

    // Virus ------------------------------------------------------------------
    Virus<TSeq> covid(vname, prevalence, false);
        covid.set_natural_history(0);
    covid.set_post_immunity("Prob. no reinfect");
    covid.set_prob_death("Prob. death");

    VirusFun<TSeq> ptransmitfun = [](
        Agent<TSeq> * p,
        Virus<TSeq> &,
        Model<TSeq> * m
        ) -> epiworld_double
    {
        // No chance of infecting
                if (
            (p->get_stage(*m) == ModelSURV<TSeq>::LATENT) ||
            (p->get_status(*m) == ModelSURV<TSeq>::ISOLATED)
        )
            return static_cast<epiworld_double>(0.0);

        // Otherwise
        return EPI_PAR(m, "Prob of transmission");
    };

    covid.set_prob_infecting_fun(ptransmitfun);
    
    this->add_virus(covid);

    this->set_user_data({"nsampled", "ndetected", "ndetected_asympt", "nasymptomatic"});
    this->add_globalevent(surveillance_program, "Surveilance program", -1);
   
    // Vaccine tool -----------------------------------------------------------
    Tool<TSeq> vax("Vaccine", prop_vaccinated, true);
    vax.set_susceptibility_reduction("Vax efficacy");
    vax.set_transmission_reduction("Vax redux transmission");
    
    this->add_tool(vax);

    this->set_name("Surveillance");


}

#endif
