#ifndef EPIWORLD_MODELS_MEASLESMIXINGRISKQUARANTINE_HPP
#define EPIWORLD_MODELS_MEASLESMIXINGRISKQUARANTINE_HPP

using namespace epiworld;

#define MM(i, j, n) \
    j * n + i

#if __cplusplus >= 202302L
    // C++23 or later
    #define GET_MODEL(model, output) \
        auto * output = dynamic_cast< ModelMeaslesMixingRiskQuarantine<TSeq> * >( (model) ); \
        /*Using the [[assume(...)]] to avoid the compiler warning \
        if the standard is C++23 or later */ \
        [[assume((output) != nullptr)]]
#else
    // C++17 or C++20
    #define GET_MODEL(model, output) \
        auto * output = dynamic_cast< ModelMeaslesMixingRiskQuarantine<TSeq> * >( (model) ); \
        assert((output) != nullptr); // Use assert for runtime checks
#endif

#define SAMPLE_FROM_PROBS(n, ans) \
    size_t ans; \
    epiworld_double p_total = m->runif(); \
    for (ans = 0u; ans < n; ++ans) \
    { \
        if (p_total < m->array_double_tmp[ans]) \
            break; \
        m->array_double_tmp[ans + 1] += m->array_double_tmp[ans]; \
    }

/**
 * @file measlesmixingriskquarantine.hpp
 * @brief Template for a Measles model with population mixing and risk-based quarantine
 */

/**
 * @brief Measles model with population mixing and risk-based quarantine strategies
 * 
 * This class extends the Measles epidemiological model to support different
 * quarantine strategies based on exposure risk levels:
 * 
 * - **High Risk**: Unvaccinated agents who share entity membership with the case
 * - **Medium Risk**: Unvaccinated agents who contacted an infected individual but don't share entity membership  
 * - **Low Risk**: Other unvaccinated agents
 * 
 * Each risk level can have different quarantine durations, allowing for targeted 
 * public health interventions. The model also includes enhanced detection during
 * active quarantine periods.
 * 
 * Disease progression follows the same states as ModelMeaslesMixing:
 * Susceptible → Exposed → Prodromal → Rash → Recovered
 * 
 * @tparam TSeq Type for genetic sequences (default: EPI_DEFAULT_TSEQ)
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelMeaslesMixingRiskQuarantine : public Model<TSeq> 
{
private:
    
    // Vector of vectors of infected agents (prodromal agents are infectious)     
    std::vector< size_t > infectious;

    // Number of infectious agents in each group
    std::vector< size_t > n_infectious_per_group;
    
    // Where the agents start in the `infectious` vector
    std::vector< size_t > entity_indices;

    void m_update_infectious_list();
    std::vector< size_t > sampled_agents;
    size_t sample_agents(
        Agent<TSeq> * agent,
        std::vector< size_t > & sampled_agents
        );
    std::vector< double > adjusted_contact_rate;
    std::vector< double > contact_matrix;

    #ifdef EPI_DEBUG
    std::vector< int > sampled_sizes;
    #endif

    // Update functions
    static void m_update_susceptible(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_exposed(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_prodromal(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_rash(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_isolated(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_isolated_recovered(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_quarantine_suscep(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_quarantine_exposed(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_quarantine_prodromal(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_quarantine_recovered(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_hospitalized(Agent<TSeq> * p, Model<TSeq> * m);

    // Data about the quarantine process
    std::vector< bool > quarantine_willingness; ///< Indicator for quarantine willingness
    std::vector< bool > isolation_willingness; ///< Indicator for isolation willingness
    std::vector< size_t > agent_quarantine_triggered; ///< Whether the quarantine process has started
    std::vector< int > day_flagged; ///< Either detected or started quarantine
    std::vector< int > day_rash_onset; ///< Day of rash onset
    std::vector< int > quarantine_risk_level; ///< Risk level assigned to each agent (0=low, 1=medium, 2=high)

    void m_quarantine_process();
    static void m_update_model(Model<TSeq> * m);

    // We will limit tracking to up to EPI_MAX_TRACKING
    std::vector< size_t > tracking_matrix; ///< Tracking matrix for agent interactions
    std::vector< size_t > tracking_matrix_size; ///< Number of current interactions for each agent
    std::vector< size_t > tracking_matrix_date; ///< Date of each interaction

    void m_add_tracking(size_t infectious_id, size_t agent_id);

public:

    static const int SUSCEPTIBLE              = 0;
    static const int EXPOSED                  = 1;
    static const int PRODROMAL                = 2;
    static const int RASH                     = 3;
    static const int ISOLATED                 = 4;
    static const int ISOLATED_RECOVERED       = 5;
    static const int DETECTED_HOSPITALIZED    = 6;
    static const int QUARANTINED_EXPOSED      = 7;
    static const int QUARANTINED_SUSCEPTIBLE  = 8;
    static const int QUARANTINED_PRODROMAL    = 9;
    static const int QUARANTINED_RECOVERED    = 10;
    static const int HOSPITALIZED             = 11;
    static const int RECOVERED                = 12;

    static const size_t QUARANTINE_PROCESS_INACTIVE = 0u;
    static const size_t QUARANTINE_PROCESS_ACTIVE   = 1u;
    static const size_t QUARANTINE_PROCESS_DONE     = 2u;

    // Risk levels for quarantine
    static const int RISK_LOW                 = 0;
    static const int RISK_MEDIUM              = 1;
    static const int RISK_HIGH                = 2;

    ModelMeaslesMixingRiskQuarantine() {};
    
    /**
     * @brief Constructs a ModelMeaslesMixingRiskQuarantine object.
     *
     * @param model A reference to an existing ModelMeaslesMixingRiskQuarantine object.
     * @param n The number of entities in the model.
     * @param prevalence The initial prevalence of the disease in the model.
     * @param contact_rate The contact rate between entities in the model.
     * @param transmission_rate The transmission rate of the disease in the model.
     * @param vax_efficacy The efficacy of the vaccine.
     * @param vax_reduction_recovery_rate The reduction in recovery rate due to the vaccine.
     * @param incubation_period The incubation period of the disease in the model.
     * @param prodromal_period The prodromal period of the disease in the model.
     * @param rash_period The rash period of the disease in the model.
     * @param contact_matrix The contact matrix between entities in the model. Specified in
     * column-major order.
     * @param hospitalization_rate The rate at which infected individuals are hospitalized.
     * @param hospitalization_period The average duration of hospitalization in days.
     * @param days_undetected The average number of days an infected individual remains undetected.
     * @param quarantine_period_high The duration of quarantine in days for high-risk contacts.
     * @param quarantine_period_medium The duration of quarantine in days for medium-risk contacts.
     * @param quarantine_period_low The duration of quarantine in days for low-risk contacts.
     * @param quarantine_willingness The proportion of individuals willing to comply with quarantine measures.
     * @param isolation_willingness The proportion of individuals willing to self-isolate when detected.
     * @param isolation_period The duration of isolation in days for detected infected individuals.
     * @param prop_vaccinated The proportion of vaccinated agents.
     * @param detection_rate_quarantine The detection rate during active quarantine periods.
     * @param contact_tracing_success_rate The probability of successfully identifying and tracing contacts (default: 1.0).
     * @param contact_tracing_days_prior The number of days prior to detection for which contacts are traced (default: 4).
     */
    ModelMeaslesMixingRiskQuarantine(
        ModelMeaslesMixingRiskQuarantine<TSeq> & model,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double vax_efficacy,
        epiworld_double vax_reduction_recovery_rate,
        epiworld_double incubation_period,
        epiworld_double prodromal_period,
        epiworld_double rash_period,
        std::vector< double > contact_matrix,
        epiworld_double hospitalization_rate,
        epiworld_double hospitalization_period,
        // Policy parameters
        epiworld_double days_undetected,
        epiworld_fast_int quarantine_period_high,
        epiworld_fast_int quarantine_period_medium,
        epiworld_fast_int quarantine_period_low,
        epiworld_double quarantine_willingness,
        epiworld_double isolation_willingness,
        epiworld_fast_int isolation_period,
        epiworld_double prop_vaccinated,
        epiworld_double detection_rate_quarantine,
        epiworld_double contact_tracing_success_rate = 1.0,
        epiworld_fast_uint contact_tracing_days_prior = 4u
    );
    
    /**
     * @brief Constructs a ModelMeaslesMixingRiskQuarantine object.
     *
     * @param n The number of entities in the model.
     * @param prevalence The initial prevalence of the disease in the model.
     * @param contact_rate The contact rate between entities in the model.
     * @param transmission_rate The transmission rate of the disease in the model.
     * @param vax_efficacy The efficacy of the vaccine.
     * @param vax_reduction_recovery_rate The reduction in recovery rate due to the vaccine.
     * @param incubation_period The incubation period of the disease in the model.
     * @param prodromal_period The prodromal period of the disease in the model.
     * @param rash_period The rash period of the disease in the model.
     * @param contact_matrix The contact matrix between entities in the model.
     * @param hospitalization_rate The rate at which infected individuals are hospitalized.
     * @param hospitalization_period The average duration of hospitalization in days.
     * @param days_undetected The average number of days an infected individual remains undetected.
     * @param quarantine_period_high The duration of quarantine in days for high-risk contacts.
     * @param quarantine_period_medium The duration of quarantine in days for medium-risk contacts.
     * @param quarantine_period_low The duration of quarantine in days for low-risk contacts.
     * @param quarantine_willingness The proportion of individuals willing to comply with quarantine measures.
     * @param isolation_willingness The proportion of individuals willing to self-isolate when detected.
     * @param isolation_period The duration of isolation in days for detected infected individuals.
     * @param prop_vaccinated The proportion of vaccinated agents.
     * @param detection_rate_quarantine The detection rate during active quarantine periods.
     * @param contact_tracing_success_rate The probability of successfully identifying and tracing contacts (default: 1.0).
     * @param contact_tracing_days_prior The number of days prior to detection for which contacts are traced (default: 4).
     */
    ModelMeaslesMixingRiskQuarantine(
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double vax_efficacy,
        epiworld_double vax_reduction_recovery_rate,
        epiworld_double incubation_period,
        epiworld_double prodromal_period,
        epiworld_double rash_period,
        std::vector< double > contact_matrix,
        epiworld_double hospitalization_rate,
        epiworld_double hospitalization_period,
        // Policy parameters
        epiworld_double days_undetected,
        epiworld_fast_int quarantine_period_high,
        epiworld_fast_int quarantine_period_medium,
        epiworld_fast_int quarantine_period_low,
        epiworld_double quarantine_willingness,
        epiworld_double isolation_willingness,
        epiworld_fast_int isolation_period,
        epiworld_double prop_vaccinated,
        epiworld_double detection_rate_quarantine,
        epiworld_double contact_tracing_success_rate = 1.0,
        epiworld_fast_uint contact_tracing_days_prior = 4u
    );

    /**
     * @brief Run the model simulation
     * @param ndays Number of days to simulate
     * @param seed Random seed for reproducibility (default: -1 for random seed)
     * @return Reference to this model instance
     */
    ModelMeaslesMixingRiskQuarantine<TSeq> & run(
        epiworld_fast_uint ndays,
        int seed = -1
    );

    /**
     * @brief Reset the model to initial state
     */
    void reset();

    /**
     * @brief Create a clone of this model
     * @return Pointer to a new model instance with the same configuration
     */
    Model<TSeq> * clone_ptr();

    /**
     * @brief Set the initial states of the model
     * @param proportions_ Double vector with two elements:
     * - [0]: The proportion of initially infected individuals who start in the exposed state.
     * - [1]: The proportion of initially non-infected individuals who have recovered (immune).
     * @param queue_ Optional vector for queuing specifications (default: empty).
     */
    ModelMeaslesMixingRiskQuarantine<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    );

    /**
     * @brief Set the contact matrix for population mixing
     * @param cmat Contact matrix specifying interaction rates between groups
     */
    void set_contact_matrix(std::vector< double > cmat)
    {
        contact_matrix = cmat;
        return;
    };

    /**
     * @brief Get the current contact matrix
     * @return Vector representing the contact matrix
     */
    std::vector< double > get_contact_matrix() const
    {
        return contact_matrix;
    };

    /**
     * @brief Get the quarantine trigger status for all agents
     * @return Vector indicating quarantine process status for each agent
     */
    std::vector< size_t > get_agent_quarantine_triggered() const
    {
        return agent_quarantine_triggered;
    };

    /**
     * @brief Get the quarantine willingness for all agents
     * @return Vector of boolean values indicating each agent's willingness to quarantine
     */
    std::vector< bool > get_quarantine_willingness() const
    {
        return quarantine_willingness;
    };

    /**
     * @brief Get the isolation willingness for all agents
     * @return Vector of boolean values indicating each agent's willingness to self-isolate
     */
    std::vector< bool > get_isolation_willingness() const
    {
        return isolation_willingness;
    };

    /**
     * @brief Get the risk levels assigned to all agents
     * @return Vector of integers indicating each agent's risk level (0=low, 1=medium, 2=high)
     */
    std::vector< int > get_quarantine_risk_levels() const
    {
        return quarantine_risk_level;
    };

};

// Static member definitions
template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::SUSCEPTIBLE;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::EXPOSED;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::PRODROMAL;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::RASH;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::ISOLATED;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::ISOLATED_RECOVERED;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::DETECTED_HOSPITALIZED;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINED_EXPOSED;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINED_SUSCEPTIBLE;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINED_PRODROMAL;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINED_RECOVERED;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::HOSPITALIZED;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::RECOVERED;

template<typename TSeq>
const size_t ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINE_PROCESS_INACTIVE;

template<typename TSeq>
const size_t ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINE_PROCESS_ACTIVE;

template<typename TSeq>
const size_t ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINE_PROCESS_DONE;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::RISK_LOW;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::RISK_MEDIUM;

template<typename TSeq>
const int ModelMeaslesMixingRiskQuarantine<TSeq>::RISK_HIGH;

// Implementation starts here

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_model(Model<TSeq> * m)
{
    GET_MODEL(m, model);
    model->m_quarantine_process();
    model->events_run();
    model->m_update_infectious_list();
    return;
}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_add_tracking(
    size_t infectious_id,
    size_t agent_id
)
{

    // We avoid the math if there's no point in tracking anymore
    if (
        agent_quarantine_triggered[infectious_id] >= 
        ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINE_PROCESS_DONE
    )
        return;    

    // If we are overflow, we start from the beginning
    size_t loc = tracking_matrix_size[infectious_id] % EPI_MAX_TRACKING;
    loc = MM(infectious_id, loc, Model<TSeq>::size());
    tracking_matrix[loc] = agent_id;
    tracking_matrix_date[loc] = Model<TSeq>::today();

    // We increase the size of the tracking matrix
    tracking_matrix_size[infectious_id]++;

    return;
}


template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_infectious_list()
{

    auto & agents = Model<TSeq>::get_agents();

    std::fill(n_infectious_per_group.begin(), n_infectious_per_group.end(), 0u);
    
    for (const auto & a : agents)
    {

        if (a.get_state() == ModelMeaslesMixingRiskQuarantine<TSeq>::PRODROMAL)
        {
            if (a.get_n_entities() > 0u)
            {
                const auto & entity = a.get_entity(0u);
                infectious[
                    // Position of the group in the `infectious` vector
                    entity_indices[entity.get_id()] +
                    // Position of the agent in the group
                    n_infectious_per_group[entity.get_id()]++
                ] = a.get_id();

            }
        }

    }

    return;

}

template<typename TSeq>
inline size_t ModelMeaslesMixingRiskQuarantine<TSeq>::sample_agents(
    Agent<TSeq> * agent,
    std::vector< size_t > & sampled_agents
    )
{

    // If agent has no entities, then we return 0
    if (agent->get_n_entities() == 0u)
        return 0u;

    size_t agent_group_id = agent->get_entity(0u).get_id();
    size_t ngroups = this->entities.size();

    int samp_id = 0;
    for (size_t g = 0; g < ngroups; ++g)
    {

        size_t group_size = n_infectious_per_group[g];

        if (group_size == 0u)
            continue;

        // How many from this entity?
        int nsamples = epiworld::Model<TSeq>::rbinom(
            group_size,
            adjusted_contact_rate[g] * contact_matrix[
                MM(agent_group_id, g, ngroups)
            ]
        );

        if (nsamples == 0)
            continue;

        // Sampling from the entity
        for (int s = 0; s < nsamples; ++s)
        {

            // Randomly selecting an agent
            int which = epiworld::Model<TSeq>::runif() * group_size;

            // Correcting overflow error
            if (which >= static_cast<int>(group_size))
                which = static_cast<int>(group_size) - 1;

            #ifdef EPI_DEBUG
            auto & a = this->population.at(infectious.at(entity_indices[g] + which));
            #else
            const auto & a = this->get_agent(infectious[entity_indices[g] + which]);
            #endif

            #ifdef EPI_DEBUG
            if (a.get_state() != ModelMeaslesMixingRiskQuarantine<TSeq>::PRODROMAL)
                throw std::logic_error(
                    "The agent is not in prodromal state, but it should be."
                );
            #endif

            // Can't sample itself
            if (a.get_id() == agent->get_id())
                continue;

            sampled_agents[samp_id++] = a.get_id();
            
        }

    }
    
    return samp_id;

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_susceptible(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    if (p->get_n_entities() == 0)
        return;

    // Downcasting to retrieve the sampler attached to the
    // class
    GET_MODEL(m, m_down);
    
    size_t ndraws = m_down->sample_agents(p, m_down->sampled_agents);

    #ifdef EPI_DEBUG
    m_down->sampled_sizes.push_back(static_cast<int>(ndraws));
    #endif

    if (ndraws == 0u)
        return;
    
    // Drawing from the set
    int nviruses_tmp = 0;
    for (size_t n = 0u; n < ndraws; ++n)
    {

        auto & neighbor = m->get_agent(m_down->sampled_agents[n]);

        auto & v = neighbor.get_virus();

        #ifdef EPI_DEBUG
        if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
            throw std::logic_error(
                "Trying to add an extra element to a temporal array outside of the range."
            );
        #endif

        // Adding the current agent to the tracked interactions
        m_down->m_add_tracking(neighbor.get_id(), p->get_id());
            
        /* And it is a function of susceptibility_reduction as well */ 
        m->array_double_tmp[nviruses_tmp] =
            (1.0 - p->get_susceptibility_reduction(v, m)) * 
            v->get_prob_infecting(m) * 
            (1.0 - neighbor.get_transmission_reduction(v, m)) 
            ; 
    
        m->array_virus_tmp[nviruses_tmp++] = &(*v);

    }

    // Running the roulette
    int which = roulette(nviruses_tmp, m);

    if (which < 0)
        return;

    p->set_virus(
        *m->array_virus_tmp[which],
        m,
        ModelMeaslesMixingRiskQuarantine<TSeq>::EXPOSED
        );

    return; 

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_exposed(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // Getting the virus
    auto & v = p->get_virus();

    // Does the agent become prodromal (infectious)?
    if (m->runif() < 1.0/(v->get_incubation(m)))
    {
        p->change_state(m, ModelMeaslesMixingRiskQuarantine<TSeq>::PRODROMAL);
        return;
    }

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_prodromal(
    Agent<TSeq> * p, Model<TSeq> * m
) {
    
    GET_MODEL(m, model);

    // Enhanced detection during active quarantine
    bool quarantine_active = false;
    for (size_t i = 0u; i < model->agent_quarantine_triggered.size(); ++i) {
        if (model->agent_quarantine_triggered[i] == ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINE_PROCESS_ACTIVE) {
            quarantine_active = true;
            break;
        }
    }
    
    // Check for detection during active quarantine
    bool detect_it = (
        quarantine_active &&
        (m->runif() < m->par("Detection rate quarantine"))
    );

    // Does the agent transition to rash?
    if (m->runif() < 1.0/m->par("Prodromal period"))
    {
        model->day_rash_onset[p->get_id()] = m->today();
        if (detect_it)
        {
            p->change_state(m, ISOLATED);
        } else {
            p->change_state(m, RASH);
        }
    } else if (detect_it)
    {
        p->change_state(m, QUARANTINED_PRODROMAL);
    }

    return ;

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_rash(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Checking if the agent will be detected or not
    bool detected = false;
    if (
        (m->par("Isolation period") >= 0) &&
        (m->runif() < 1.0/m->par("Days undetected"))
    )
    {
        model->agent_quarantine_triggered[p->get_id()] = 
            ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINE_PROCESS_ACTIVE;
        detected = true;
    }

    // Computing probabilities for state change
    m->array_double_tmp[0] = 1.0/m->par("Rash period"); // Recovery
    m->array_double_tmp[1] = m->par("Hospitalization rate"); // Hospitalization
        
    SAMPLE_FROM_PROBS(2, which);
    
    if (which == 2) // Recovers
    {
        p->rm_virus(
            m,
            detected ?
                ModelMeaslesMixingRiskQuarantine<TSeq>::ISOLATED_RECOVERED:
                ModelMeaslesMixingRiskQuarantine<TSeq>::RECOVERED
        );
    }
    else if (which == 1) // Hospitalized
    {
        p->change_state(
            m,
            detected ?
                ModelMeaslesMixingRiskQuarantine<TSeq>::DETECTED_HOSPITALIZED :
                ModelMeaslesMixingRiskQuarantine<TSeq>::HOSPITALIZED
        );
    }
    else if (which != 0)
    {
        throw std::logic_error("The roulette returned an unexpected value.");
    }
    else if ((which == 0u) && detected)
    {
        // If the agent is not hospitalized or recovered, then it is moved to
        // isolation.
        p->change_state(m, ModelMeaslesMixingRiskQuarantine<TSeq>::ISOLATED);
        model->day_flagged[p->get_id()] = m->today();
    }
    
    return ;

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_isolated(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Figuring out if the agent can be released from isolation
    // if the isolation period is over.
    int days_since = m->today() - model->day_rash_onset[p->get_id()];

    bool unisolate =
        (m->par("Isolation period") <= days_since) ?
        true: false;

    // Probability of staying in the rash period vs becoming
    // hospitalized
    m->array_double_tmp[0] = 1.0/m->par("Rash period");
    m->array_double_tmp[1] = m->par("Hospitalization rate");

    // Sampling from the probabilities
    SAMPLE_FROM_PROBS(2, which);

    // Recovers
    if (which == 2u)
    {
        if (unisolate)
        {
            if (p->get_virus() != nullptr)
                p->rm_virus(
                    m,
                    ModelMeaslesMixingRiskQuarantine<TSeq>::RECOVERED
                );
        }
        else
            if (p->get_virus() != nullptr)
                p->rm_virus(
                    m,
                    ModelMeaslesMixingRiskQuarantine<TSeq>::ISOLATED_RECOVERED
                );
    }
    // Moves to hospitalized
    else 
    {
        if (p->get_virus() != nullptr)
            p->change_state(
                m,
                ModelMeaslesMixingRiskQuarantine<TSeq>::DETECTED_HOSPITALIZED
            );
    }

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_isolated_recovered(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Figuring out if the agent can be released from isolation
    // if the isolation period is over.
    int days_since = m->today() - model->day_rash_onset[p->get_id()];

    if (m->par("Isolation period") <= days_since)
        p->change_state(
            m,
            ModelMeaslesMixingRiskQuarantine<TSeq>::RECOVERED
        );

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_quarantine_suscep(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Get the appropriate quarantine period based on risk level
    epiworld_double quarantine_period;
    int risk_level = model->quarantine_risk_level[p->get_id()];
    
    if (risk_level == RISK_HIGH) {
        quarantine_period = m->par("Quarantine period high");
    } else if (risk_level == RISK_MEDIUM) {
        quarantine_period = m->par("Quarantine period medium");
    } else {
        quarantine_period = m->par("Quarantine period low");
    }

    // Figuring out if the agent can be released from quarantine
    int days_since = m->today() - model->day_flagged[p->get_id()];

    if (quarantine_period <= days_since)
        p->change_state(
            m,
            ModelMeaslesMixingRiskQuarantine<TSeq>::SUSCEPTIBLE
        );

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_quarantine_exposed(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Get the appropriate quarantine period based on risk level
    epiworld_double quarantine_period;
    int risk_level = model->quarantine_risk_level[p->get_id()];
    
    if (risk_level == RISK_HIGH) {
        quarantine_period = m->par("Quarantine period high");
    } else if (risk_level == RISK_MEDIUM) {
        quarantine_period = m->par("Quarantine period medium");
    } else {
        quarantine_period = m->par("Quarantine period low");
    }

    // Figuring out if the agent can be released from quarantine
    int days_since = m->today() - model->day_flagged[p->get_id()];

    // Probability of moving to prodromal
    if (m->runif() < 1.0/(p->get_virus()->get_incubation(m)))
    {

        // If the agent is unquarantined, it becomes prodromal
        if (quarantine_period <= days_since)
        {
            p->change_state(
                m, ModelMeaslesMixingRiskQuarantine<TSeq>::PRODROMAL
            );
        }
        else
        {
            p->change_state(
                m, ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINED_PRODROMAL
            );
        }

    }
    else if (quarantine_period <= days_since)
    {
        p->change_state(
            m, ModelMeaslesMixingRiskQuarantine<TSeq>::EXPOSED
        );
    }

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_quarantine_prodromal(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Get the appropriate quarantine period based on risk level
    epiworld_double quarantine_period;
    int risk_level = model->quarantine_risk_level[p->get_id()];
    
    if (risk_level == RISK_HIGH) {
        quarantine_period = m->par("Quarantine period high");
    } else if (risk_level == RISK_MEDIUM) {
        quarantine_period = m->par("Quarantine period medium");
    } else {
        quarantine_period = m->par("Quarantine period low");
    }

    // Figuring out if the agent can be released from quarantine
    int days_since = m->today() - model->day_flagged[p->get_id()];

    // Develops rash?
    if (m->runif() < (1.0/m->par("Prodromal period")))
    {
        model->day_rash_onset[p->get_id()] = m->today();
        p->change_state(m, ModelMeaslesMixingRiskQuarantine<TSeq>::ISOLATED);
    }
    else
    {
        
        if (quarantine_period <= days_since)
            p->change_state(m, ModelMeaslesMixingRiskQuarantine<TSeq>::PRODROMAL);

    }

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_quarantine_recovered(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Get the appropriate quarantine period based on risk level
    epiworld_double quarantine_period;
    int risk_level = model->quarantine_risk_level[p->get_id()];
    
    if (risk_level == RISK_HIGH) {
        quarantine_period = m->par("Quarantine period high");
    } else if (risk_level == RISK_MEDIUM) {
        quarantine_period = m->par("Quarantine period medium");
    } else {
        quarantine_period = m->par("Quarantine period low");
    }

    // Figuring out if the agent can be released from quarantine
    int days_since = m->today() - model->day_flagged[p->get_id()];

    if (quarantine_period <= days_since)
        p->change_state(
            m,
            ModelMeaslesMixingRiskQuarantine<TSeq>::RECOVERED
        );

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_update_hospitalized(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // The agent is removed from the system
    if (m->runif() < 1.0/m->par("Hospitalization period"))
        p->rm_virus(m, ModelMeaslesMixingRiskQuarantine<TSeq>::RECOVERED);

};

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_quarantine_process() {

    // Process entity-level quarantine
    for (size_t agent_i = 0u; agent_i < Model<TSeq>::size(); ++agent_i)
    {

        // Checking if the quarantine in the agent was triggered
        // or not
        if (
            agent_quarantine_triggered[agent_i] != 
            ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINE_PROCESS_ACTIVE
        )
            continue;

        // Getting the number of contacts, if it is greater
        // than the maximum, it means that we overflowed, so 
        // we will only quarantine the first EPI_MAX_TRACKING
        size_t n_contacts = this->tracking_matrix_size[agent_i];
        if (n_contacts >= EPI_MAX_TRACKING)
            n_contacts = EPI_MAX_TRACKING;

        // When the rash onset started (this is for contact tracing)
        size_t day_rash_onset_agent_i = this->day_rash_onset[agent_i];

        // Get the entity of the triggering agent for risk classification
        auto & triggering_agent = Model<TSeq>::get_agent(agent_i);
        size_t triggering_entity_id = (triggering_agent.get_n_entities() > 0) ? 
            triggering_agent.get_entity(0).get_id() : SIZE_MAX;

        for (size_t contact_i = 0u; contact_i < n_contacts; ++contact_i)
        {

            // Checking if the contact is within the contact tracing days prior
            size_t loc = MM(agent_i, contact_i, Model<TSeq>::size());
            bool within_days_prior =
                (day_rash_onset_agent_i - tracking_matrix_date[loc]) <=
                Model<TSeq>::par("Contact tracing days prior");
            if (!within_days_prior)
                continue;

            // Checking if we will detect the contact
            if (Model<TSeq>::runif() > Model<TSeq>::par("Contact tracing success rate"))
                continue;

            size_t contact_id = this->tracking_matrix[loc];

            auto & agent = Model<TSeq>::get_agent(contact_id);

            if (agent.get_state() > RASH)
                continue;

            // Agents with some tool won't be quarantined
            if (agent.get_n_tools() != 0u)
                continue;

            // Determine risk level based on entity membership and vaccination status
            int risk_level = RISK_LOW; // Default to low risk
            
            // Only unvaccinated agents are subject to risk-based quarantine
            if (agent.get_n_tools() == 0u) {
                // Check if agent shares entity membership with triggering case
                bool shares_entity = false;
                if (triggering_entity_id != SIZE_MAX && agent.get_n_entities() > 0) {
                    for (size_t e = 0; e < agent.get_n_entities(); ++e) {
                        if (static_cast<size_t>(agent.get_entity(e).get_id()) == triggering_entity_id) {
                            shares_entity = true;
                            break;
                        }
                    }
                }
                
                if (shares_entity) {
                    risk_level = RISK_HIGH;
                } else {
                    // Contact with infected but no shared entity
                    risk_level = RISK_MEDIUM;
                }
            }
            
            // Store the risk level
            quarantine_risk_level[contact_id] = risk_level;

            // Check if any quarantine period is enabled for this risk level
            bool quarantine_enabled = false;
            if (risk_level == RISK_HIGH && Model<TSeq>::par("Quarantine period high") >= 0) {
                quarantine_enabled = true;
            } else if (risk_level == RISK_MEDIUM && Model<TSeq>::par("Quarantine period medium") >= 0) {
                quarantine_enabled = true;
            } else if (risk_level == RISK_LOW && Model<TSeq>::par("Quarantine period low") >= 0) {
                quarantine_enabled = true;
            }

            if (quarantine_willingness[contact_id] && quarantine_enabled)
            {

                switch (agent.get_state())
                {
                    case SUSCEPTIBLE:
                        agent.change_state(this, QUARANTINED_SUSCEPTIBLE);
                        day_flagged[contact_id] = Model<TSeq>::today();
                        break;
                    case EXPOSED:
                        agent.change_state(this, QUARANTINED_EXPOSED);
                        day_flagged[contact_id] = Model<TSeq>::today();
                        break;
                    case PRODROMAL:
                        agent.change_state(this, QUARANTINED_PRODROMAL);
                        day_flagged[contact_id] = Model<TSeq>::today();
                        break;
                    case RASH:
                        if (isolation_willingness[contact_id])
                        {
                            agent.change_state(this, ISOLATED);
                            day_flagged[contact_id] = Model<TSeq>::today();
                        }
                        break;
                    default:
                        throw std::logic_error(
                            "The agent is not in a state that can be quarantined."
                        );
                }

            }
        }

        // Setting the quarantine process off
        agent_quarantine_triggered[agent_i] = 
            ModelMeaslesMixingRiskQuarantine<TSeq>::QUARANTINE_PROCESS_DONE;
    }

    return;
}

/**
 * @brief Template for a Measles model with population mixing and risk-based quarantine
 * 
 * @param model A ModelMeaslesMixingRiskQuarantine<TSeq> object where to set up the model.
 * @param n Number of agents in the population
 * @param prevalence Initial prevalence (proportion of infected individuals)
 * @param contact_rate Average number of contacts (interactions) per step
 * @param transmission_rate Probability of transmission per contact
 * @param vax_efficacy The efficacy of the vaccine
 * @param vax_reduction_recovery_rate The reduction in recovery rate due to the vaccine
 * @param incubation_period Average incubation period in days
 * @param prodromal_period Average prodromal period in days
 * @param rash_period Average rash period in days
 * @param contact_matrix Contact matrix specifying mixing patterns between population groups
 * @param hospitalization_rate Rate at which infected individuals are hospitalized
 * @param hospitalization_period Average duration of hospitalization in days
 * @param days_undetected Average number of days an infected individual remains undetected
 * @param quarantine_period_high Duration of quarantine in days for high-risk contacts
 * @param quarantine_period_medium Duration of quarantine in days for medium-risk contacts
 * @param quarantine_period_low Duration of quarantine in days for low-risk contacts
 * @param quarantine_willingness Proportion of individuals willing to comply with quarantine
 * @param isolation_willingness Proportion of individuals willing to self-isolate when detected
 * @param isolation_period Duration of isolation in days for detected infected individuals
 * @param prop_vaccinated Proportion of vaccinated agents
 * @param detection_rate_quarantine Detection rate during active quarantine periods
 * @param contact_tracing_success_rate Probability of successfully identifying contacts during tracing
 * @param contact_tracing_days_prior Number of days prior to detection for contact tracing
 */
template<typename TSeq>
inline ModelMeaslesMixingRiskQuarantine<TSeq>::ModelMeaslesMixingRiskQuarantine(
    ModelMeaslesMixingRiskQuarantine<TSeq> & model,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double vax_efficacy,
    epiworld_double vax_reduction_recovery_rate,
    epiworld_double incubation_period,
    epiworld_double prodromal_period,
    epiworld_double rash_period,
    std::vector< double > contact_matrix,
    epiworld_double hospitalization_rate,
    epiworld_double hospitalization_period,
    // Policy parameters
    epiworld_double days_undetected,
    epiworld_fast_int quarantine_period_high,
    epiworld_fast_int quarantine_period_medium,
    epiworld_fast_int quarantine_period_low,
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    epiworld_fast_int isolation_period,
    epiworld_double prop_vaccinated,
    epiworld_double detection_rate_quarantine,
    epiworld_double contact_tracing_success_rate,
    epiworld_fast_uint contact_tracing_days_prior
    )
{

    // Setting up the contact matrix
    this->contact_matrix = contact_matrix;

    // Setting up parameters
    model.add_param(contact_rate, "Contact rate");
    model.add_param(transmission_rate, "Transmission rate");
    model.add_param(incubation_period, "Incubation period");
    model.add_param(prodromal_period, "Prodromal period");
    model.add_param(rash_period, "Rash period");
    model.add_param(hospitalization_rate, "Hospitalization rate");
    model.add_param(hospitalization_period, "Hospitalization period");
    model.add_param(days_undetected, "Days undetected");
    model.add_param(quarantine_period_high, "Quarantine period high");
    model.add_param(quarantine_period_medium, "Quarantine period medium");
    model.add_param(quarantine_period_low, "Quarantine period low");
    model.add_param(
        quarantine_willingness, "Quarantine willingness"
    );
    model.add_param(
        isolation_willingness, "Isolation willingness"
    );
    model.add_param(isolation_period, "Isolation period");
    model.add_param(detection_rate_quarantine, "Detection rate quarantine");
    model.add_param(
        contact_tracing_success_rate, "Contact tracing success rate"
    );
    model.add_param(
        contact_tracing_days_prior, "Contact tracing days prior"
    );
    model.add_param(prop_vaccinated, "Vaccination rate");
    model.add_param(vax_efficacy, "Vax efficacy");
    model.add_param(vax_reduction_recovery_rate, "Vax improved recovery");
    
    // state
    model.add_state("Susceptible", m_update_susceptible);
    model.add_state("Exposed", m_update_exposed);
    model.add_state("Prodromal", m_update_prodromal);
    model.add_state("Rash", m_update_rash);
    model.add_state("Isolated", m_update_isolated);
    model.add_state("Isolated Recovered", m_update_isolated_recovered);
    model.add_state("Detected Hospitalized", m_update_hospitalized);
    model.add_state("Quarantined Exposed", m_update_quarantine_exposed);
    model.add_state("Quarantined Susceptible", m_update_quarantine_suscep);
    model.add_state("Quarantined Prodromal", m_update_quarantine_prodromal);
    model.add_state("Quarantined Recovered", m_update_quarantine_recovered);
    model.add_state("Hospitalized", m_update_hospitalized);
    model.add_state("Recovered");

    // Global function
    model.add_globalevent(this->m_update_model, "Update infected individuals");
    model.queuing_off();

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus("Measles", prevalence, true);
    virus.set_state(
        ModelMeaslesMixingRiskQuarantine<TSeq>::EXPOSED,
        ModelMeaslesMixingRiskQuarantine<TSeq>::RECOVERED,
        ModelMeaslesMixingRiskQuarantine<TSeq>::RECOVERED
        );

    virus.set_prob_infecting(&model("Transmission rate"));
    virus.set_prob_recovery(&model("Rash period"));
    virus.set_incubation(&model("Incubation period"));

    model.add_virus(virus);

    // Designing the vaccine
    Tool<> vaccine("Vaccine");
    vaccine.set_susceptibility_reduction(&model("Vax efficacy"));
    vaccine.set_recovery_enhancer(&model("Vax improved recovery"));
    vaccine.set_distribution(
        distribute_tool_randomly(prop_vaccinated, true)
    );

    model.add_tool(vaccine);

    model.queuing_off(); // No queuing need

    // Adding the empty population
    model.agents_empty_graph(n);

    model.set_name("Measles with Mixing and Risk-based Quarantine");

    return;

}

template<typename TSeq>
inline ModelMeaslesMixingRiskQuarantine<TSeq>::ModelMeaslesMixingRiskQuarantine(
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double vax_efficacy,
    epiworld_double vax_reduction_recovery_rate,
    epiworld_double incubation_period,
    epiworld_double prodromal_period,
    epiworld_double rash_period,
    std::vector< double > contact_matrix,
    epiworld_double hospitalization_rate,
    epiworld_double hospitalization_period,
    // Policy parameters
    epiworld_double days_undetected,
    epiworld_fast_int quarantine_period_high,
    epiworld_fast_int quarantine_period_medium,
    epiworld_fast_int quarantine_period_low,
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    epiworld_fast_int isolation_period,
    epiworld_double prop_vaccinated,
    epiworld_double detection_rate_quarantine,
    epiworld_double contact_tracing_success_rate,
    epiworld_fast_uint contact_tracing_days_prior
    )
{   

    this->contact_matrix = contact_matrix;

    ModelMeaslesMixingRiskQuarantine(
        *this,
        n,
        prevalence,
        contact_rate,
        transmission_rate,
        vax_efficacy,
        vax_reduction_recovery_rate,
        incubation_period,
        prodromal_period,
        rash_period,
        contact_matrix,
        hospitalization_rate,
        hospitalization_period,
        // Policy parameters
        days_undetected,
        quarantine_period_high,
        quarantine_period_medium,
        quarantine_period_low,
        quarantine_willingness,
        isolation_willingness,
        isolation_period,
        prop_vaccinated,
        detection_rate_quarantine,
        contact_tracing_success_rate,
        contact_tracing_days_prior
    );

    return;

}

template<typename TSeq>
inline ModelMeaslesMixingRiskQuarantine<TSeq> & ModelMeaslesMixingRiskQuarantine<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{
    Model<TSeq>::run(ndays, seed);
    return *this;
}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::reset()
{

    Model<TSeq>::reset();   

    // Checking contact matrix's rows add to one
    size_t nentities = this->entities.size();
    if (this->contact_matrix.size() !=  nentities*nentities)
        throw std::length_error(
            std::string("The contact matrix must be a square matrix of size ") +
            std::string("nentities x nentities. ") +
            std::to_string(this->contact_matrix.size()) +
            std::string(" != ") + std::to_string(nentities*nentities) +
            std::string(".")
            );

    for (size_t i = 0u; i < this->entities.size(); ++i)
    {
        double sum = 0.0;
        for (size_t j = 0u; j < this->entities.size(); ++j)
        {
            if (this->contact_matrix[MM(i, j, nentities)] < 0.0)
                throw std::range_error(
                    std::string("The contact matrix must be non-negative. ") +
                    std::to_string(this->contact_matrix[MM(i, j, nentities)]) +
                    std::string(" < 0.")
                    );
            sum += this->contact_matrix[MM(i, j, nentities)];
        }
        if (sum < 0.999 || sum > 1.001)
            throw std::range_error(
                std::string("The contact matrix must have rows that add to one. ") +
                std::to_string(sum) +
                std::string(" != 1.")
                );
    }

    // Do it the first time only
    sampled_agents.resize(Model<TSeq>::size());

    // We only do it once
    n_infectious_per_group.resize(this->entities.size(), 0u);
    std::fill(n_infectious_per_group.begin(), n_infectious_per_group.end(), 0u);

    // We are assuming one agent per entity
    infectious.resize(Model<TSeq>::size());
    std::fill(infectious.begin(), infectious.end(), 0u);

    // This will say when do the groups start in the `infectious` vector
    entity_indices.resize(this->entities.size(), 0u);
    std::fill(entity_indices.begin(), entity_indices.end(), 0u);
    for (size_t i = 1u; i < this->entities.size(); ++i)
    {

        entity_indices[i] +=
            this->entities[i - 1].size() +
            entity_indices[i - 1]
            ;

    }
    
    // Adjusting contact rate
    adjusted_contact_rate.clear();
    adjusted_contact_rate.resize(this->entities.size(), 0.0);

    for (size_t i = 0u; i < this->entities.size(); ++i)
    {
                
        adjusted_contact_rate[i] = 
            Model<TSeq>::get_param("Contact rate") /
                static_cast< epiworld_double > (this->get_entity(i).size());


        // Possibly correcting for a small number of agents
        if (adjusted_contact_rate[i] > 1.0)
            adjusted_contact_rate[i] = 1.0;

    }

    this->m_update_infectious_list();

    // Setting up the quarantine parameters
    quarantine_willingness.resize(this->size(), false);
    isolation_willingness.resize(this->size(), false);
    for (size_t idx = 0; idx < quarantine_willingness.size(); ++idx)
    {
        quarantine_willingness[idx] =
            Model<TSeq>::runif() < this->par("Quarantine willingness");
        isolation_willingness[idx] =
            Model<TSeq>::runif() < this->par("Isolation willingness");
    }

    agent_quarantine_triggered.resize(this->size(), 0u);
    std::fill(
        agent_quarantine_triggered.begin(),
        agent_quarantine_triggered.end(),
        0u
    );

    day_flagged.resize(this->size(), 0);
    std::fill(
        day_flagged.begin(),
        day_flagged.end(),
        0
    );

    day_rash_onset.resize(this->size(), 0);
    std::fill(
        day_rash_onset.begin(),
        day_rash_onset.end(),
        0
    );

    quarantine_risk_level.resize(this->size(), RISK_LOW);
    std::fill(
        quarantine_risk_level.begin(),
        quarantine_risk_level.end(),
        RISK_LOW
    );

    // Setting up contact tracking matrix
    tracking_matrix.resize(this->size() * EPI_MAX_TRACKING, 0u);
    std::fill(
        tracking_matrix.begin(),
        tracking_matrix.end(),
        0u
    );

    tracking_matrix_size.resize(this->size(), 0u);
    std::fill(
        tracking_matrix_size.begin(),
        tracking_matrix_size.end(),
        0u
    );

    tracking_matrix_date.resize(this->size() * EPI_MAX_TRACKING, 0u);
    std::fill(
        tracking_matrix_date.begin(),
        tracking_matrix_date.end(),
        0u
    );

    return;

}

template<typename TSeq>
inline Model<TSeq> * ModelMeaslesMixingRiskQuarantine<TSeq>::clone_ptr()
{
    
    ModelMeaslesMixingRiskQuarantine<TSeq> * ptr = new ModelMeaslesMixingRiskQuarantine<TSeq>(
        *dynamic_cast<const ModelMeaslesMixingRiskQuarantine<TSeq>*>(this)
        );

    return dynamic_cast< Model<TSeq> *>(ptr);

}

template<typename TSeq>
inline ModelMeaslesMixingRiskQuarantine<TSeq> & ModelMeaslesMixingRiskQuarantine<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
)
{

    Model<TSeq>::initial_states_fun =
        create_init_function_seir<TSeq>(proportions_)
        ;

    return *this;

}

#endif