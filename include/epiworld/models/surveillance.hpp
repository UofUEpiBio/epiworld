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
    // state
    static constexpr int SUSCEPTIBLE           = 0;
    static constexpr int LATENT                = 1;
    static constexpr int SYMPTOMATIC           = 2;
    static constexpr int SYMPTOMATIC_ISOLATED  = 3; // sampled and discovered
    static constexpr int ASYMPTOMATIC          = 4;
    static constexpr int ASYMPTOMATIC_ISOLATED = 5;
    static constexpr int RECOVERED             = 6;
    static constexpr int REMOVED               = 7;

    /**
     * @brief Vector of days spent in latent and infectious states
     * A row-major matrix
     */
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
     * This model features the following states:
     * 
     * - Susceptible
     * - Latent
     * - Symptomatic
     * - Symptomatic isolated
     * - Asymptomatic
     * - Asymptomatic isolated
     * - Recovered
     * - Removed    
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

};

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

    std::vector< unsigned int > exposed_state = {
        SYMPTOMATIC,
        SYMPTOMATIC_ISOLATED,
        ASYMPTOMATIC,
        ASYMPTOMATIC_ISOLATED,
        LATENT
    };

    GlobalFun<TSeq> surveillance_program = 
    [exposed_state](
        Model<TSeq>* m
        ) -> void
    {

        const auto * nh = m->get_natural_history();
        const int detected = nh->transition_id("Detected");
        const int detected_asympt = nh->transition_id("Detected asymptomatic");

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
            if (IN(p->get_state(), exposed_state ))
            {

                // The ones already isolated stay so
                ndetected += 1.0;
                if (p->get_state() == ModelSURV<TSeq>::ASYMPTOMATIC)
                {
                    ndetected_asympt += 1.0;
                    m->fire(detected_asympt, *p);
                }
                else
                    m->fire(detected, *p);

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
                static_cast<epiworld_double>(totals[ModelSURV<TSeq>::ASYMPTOMATIC])
            }
            );


    };

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

    // The latent and infectious periods are gamma with shape the parameter
    // and scale 1 (mean the parameter, variance the same)
    auto gamma_days = [](const char * pname) -> DurationFun<TSeq> {
        const std::string name(pname);
        return [name](Agent<TSeq> &, Model<TSeq> & m) -> int {
            return static_cast< int >(std::lround(m.rgamma(m.par(name), 1.0)));
        };
    };

    Virus<TSeq> covid(vname);
    covid.add_stage("Susceptible");
    covid.add_stage("Latent", stage::carrier);
    covid.add_stage("Symptomatic", stage::infectious);
    covid.add_stage("Symptomatic isolated", stage::carrier);
    covid.add_stage("Asymptomatic", stage::infectious);
    covid.add_stage("Asymptomatic isolated", stage::carrier);
    covid.add_stage("Recovered");
    covid.add_stage("Removed");
    covid.set_contact(REMOVED, 0.0);

    covid.infection(SUSCEPTIBLE, LATENT, "Prob of transmission");

    // After the latent period, symptoms or not
    covid.after(LATENT, SYMPTOMATIC, Duration<TSeq>(gamma_days("Latent period")))
      .name("Latent -> Symptomatic or Asymptomatic")
      .to({
        {SYMPTOMATIC, Value<TSeq>("Prob of symptoms")},
        {ASYMPTOMATIC, Value<TSeq>::complement("Prob of symptoms")}
      });

    // The infectious period counts from the end of the latent one; isolation
    // keeps counting from there
    const Duration<TSeq> infectious(gamma_days("Infect period"));
    covid.after(SYMPTOMATIC, RECOVERED, infectious);
    covid.after(SYMPTOMATIC_ISOLATED, RECOVERED, infectious.since(SYMPTOMATIC));
    covid.after(ASYMPTOMATIC, RECOVERED, infectious);
    covid.after(ASYMPTOMATIC_ISOLATED, RECOVERED, infectious.since(ASYMPTOMATIC));

    // Each infectious day, a risk of death (competing with the recovery)
    covid.rate(
        {SYMPTOMATIC, SYMPTOMATIC_ISOLATED, ASYMPTOMATIC, ASYMPTOMATIC_ISOLATED},
        REMOVED,
        "Prob. death"
    ).name("Death").effect(Effect::death);

    // Surveillance finds infected agents
    covid.external({LATENT, SYMPTOMATIC}, SYMPTOMATIC_ISOLATED, "Detected");
    covid.external(ASYMPTOMATIC, ASYMPTOMATIC_ISOLATED, "Detected asymptomatic");


    // Virus ------------------------------------------------------------------
    covid.set_post_immunity("Prob. no reinfect");
    this->add_virus(covid, assign::count<TSeq>(prevalence));

    this->set_user_data({"nsampled", "ndetected", "ndetected_asympt", "nasymptomatic"});
    this->add_globalevent(surveillance_program, "Surveilance program", -1);
   
    // Vaccine tool -----------------------------------------------------------
    Tool<TSeq> vax("Vaccine");
    vax.set_susceptibility_reduction("Vax efficacy");
    vax.set_transmission_reduction("Vax redux transmission");

    this->add_tool(vax, assign::share<TSeq>(prop_vaccinated));

    this->set_name("Surveillance");


}

#endif
