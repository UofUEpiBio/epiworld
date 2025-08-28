#ifndef EPIWORLD_MODELS_MEASLESENTITYQUARANTINE_HPP
#define EPIWORLD_MODELS_MEASLESENTITYQUARANTINE_HPP

using namespace epiworld;

#define MM(i, j, n) \
    j * n + i

#if __cplusplus >= 202302L
    // C++23 or later
    #define GET_MODEL(model, output) \
        auto * output = dynamic_cast< ModelMeaslesEntityQuarantine<TSeq> * >( (model) ); \
        /*Using the [[assume(...)]] to avoid the compiler warning \
        if the standard is C++23 or later */ \
        [[assume((output) != nullptr)]]
#else
    // C++17 or C++20
    #define GET_MODEL(model, output) \
        auto * output = dynamic_cast< ModelMeaslesEntityQuarantine<TSeq> * >( (model) ); \
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
 * @file measlesentityquarantine.hpp
 * @brief Template for a Measles model with entity-level quarantine
 */

/**
 * @brief Measles model with entity-level quarantine
 * 
 * This class implements a Measles epidemiological model based on the SEIR framework
 * with entity-level quarantine features including:
 * - Population mixing based on contact matrices
 * - Measles-specific disease progression: Susceptible → Exposed → Prodromal → Rash
 * - Prodromal individuals are infectious (replace the "Infected" state in SEIR)
 * - Rash individuals are no longer infectious but can be detected for isolation
 * - Entity-level quarantine measures (no contact tracing)
 * - Quarantine duration varies based on vaccination prevalence within entities
 * - Isolation policies for detected individuals during the rash state
 * - Hospitalization of severe cases
 * - Individual willingness to comply with public health measures
 * 
 * The model supports 13 distinct states:
 * - Susceptible: Individuals who can become infected
 * - Exposed: Infected but not yet infectious (incubation period)  
 * - Prodromal: Infectious individuals in the community (replaces "Infected" in SEIR)
 * - Rash: Non-infectious individuals with visible symptoms (detection occurs here)
 * - Isolated: Detected individuals in self-isolation
 * - Isolated Recovered: Recovered individuals still in isolation
 * - Detected Hospitalized: Hospitalized individuals who were detected
 * - Quarantined Exposed: Exposed individuals in quarantine due to entity quarantine
 * - Quarantined Susceptible: Susceptible individuals in quarantine due to entity quarantine
 * - Quarantined Prodromal: Prodromal individuals in quarantine due to entity quarantine
 * - Quarantined Recovered: Recovered individuals in quarantine due to entity quarantine
 * - Hospitalized: Individuals requiring hospital care
 * - Recovered: Individuals who have recovered and gained immunity
 * 
 * @tparam TSeq Type for genetic sequences (default: EPI_DEFAULT_TSEQ)
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelMeaslesEntityQuarantine : public Model<TSeq> 
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
    std::vector< size_t > entity_quarantine_triggered; ///< Whether the quarantine process has started for an entity
    std::vector< int > day_flagged; ///< Either detected or started quarantine
    std::vector< int > day_rash_onset; ///< Day of rash onset
    std::vector< int > day_exposed; ///< Day of exposure

    // Entity-level quarantine based on vaccination prevalence
    std::vector< double > vaccination_prevalence_thresholds; ///< Vaccination prevalence thresholds
    std::vector< size_t > quarantine_durations; ///< Quarantine durations for each threshold

    void m_quarantine_process();
    static void m_update_model(Model<TSeq> * m);

public:

    void initialize_entity_data();
    
    /**
     * @brief Calculate vaccination prevalence within an entity
     * @param entity_id The ID of the entity
     * @return Vaccination prevalence (0.0 to 1.0)
     */
    double calculate_entity_vaccination_prevalence(size_t entity_id);
    
    /**
     * @brief Get quarantine duration for an entity based on vaccination prevalence
     * @param entity_id The ID of the entity
     * @return Quarantine duration in days
     */
    size_t get_quarantine_duration_for_entity(size_t entity_id);

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

    static constexpr size_t QUARANTINE_PROCESS_INACTIVE = 0u;
    static constexpr size_t QUARANTINE_PROCESS_ACTIVE   = 1u;
    static constexpr size_t QUARANTINE_PROCESS_DONE     = 2u;

    ModelMeaslesEntityQuarantine() {};
    
    /**
     * @brief Constructs a ModelMeaslesEntityQuarantine object.
     *
     * @param model A reference to an existing ModelMeaslesEntityQuarantine object.
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
     * @param quarantine_willingness The proportion of individuals willing to comply with quarantine measures.
     * @param isolation_willingness The proportion of individuals willing to self-isolate when detected.
     * @param isolation_period The duration of isolation in days for detected infected individuals.
     * @param prop_vaccinated The proportion of vaccinated agents.
     * @param vaccination_prevalence_thresholds The vaccination prevalence thresholds for quarantine duration.
     * @param quarantine_durations The quarantine durations corresponding to vaccination prevalence thresholds.
     */
    ModelMeaslesEntityQuarantine(
        ModelMeaslesEntityQuarantine<TSeq> & model,
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
        epiworld_double quarantine_willingness,
        epiworld_double isolation_willingness,
        epiworld_fast_int isolation_period,
        epiworld_double prop_vaccinated,
        std::vector< double > vaccination_prevalence_thresholds,
        std::vector< size_t > quarantine_durations
    );
    
    /**
     * @brief Constructs a ModelMeaslesEntityQuarantine object.
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
     * @param quarantine_willingness The proportion of individuals willing to comply with quarantine measures.
     * @param isolation_willingness The proportion of individuals willing to self-isolate when detected.
     * @param isolation_period The duration of isolation in days for detected infected individuals.
     * @param prop_vaccinated The proportion of vaccinated agents.
     * @param vaccination_prevalence_thresholds The vaccination prevalence thresholds for quarantine duration.
     * @param quarantine_durations The quarantine durations corresponding to vaccination prevalence thresholds.
     */
    ModelMeaslesEntityQuarantine(
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
        epiworld_double quarantine_willingness,
        epiworld_double isolation_willingness,
        epiworld_fast_int isolation_period,
        epiworld_double prop_vaccinated,
        std::vector< double > vaccination_prevalence_thresholds,
        std::vector< size_t > quarantine_durations
    );

    /**
     * @brief Run the model simulation
     * @param ndays Number of days to simulate
     * @param seed Random seed for reproducibility (default: -1 for random seed)
     * @return Reference to this model instance
     */
    ModelMeaslesEntityQuarantine<TSeq> & run(
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
    ModelMeaslesEntityQuarantine<TSeq> & initial_states(
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
     * @brief Get the entity quarantine trigger status
     * @return Vector indicating quarantine process status for each entity
     */
    std::vector< size_t > get_entity_quarantine_triggered() const
    {
        return entity_quarantine_triggered;
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
     * @brief Set vaccination prevalence thresholds and corresponding quarantine durations
     * @param thresholds Vector of vaccination prevalence thresholds (0.0 to 1.0)
     * @param durations Vector of quarantine durations (days) corresponding to thresholds
     */
    void set_quarantine_thresholds(
        std::vector< double > thresholds,
        std::vector< size_t > durations
    );

};

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_model(Model<TSeq> * m)
{
    GET_MODEL(m, model);
    model->m_quarantine_process();
    model->events_run();
    model->m_update_infectious_list();
    return;
}

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_infectious_list()
{

    auto & agents = Model<TSeq>::get_agents();

    std::fill(n_infectious_per_group.begin(), n_infectious_per_group.end(), 0u);
    
    for (const auto & a : agents)
    {

        if (a.get_state() == ModelMeaslesEntityQuarantine<TSeq>::PRODROMAL)
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
inline double ModelMeaslesEntityQuarantine<TSeq>::calculate_entity_vaccination_prevalence(size_t entity_id)
{
    size_t total_agents = 0;
    size_t vaccinated_agents = 0;
    
    auto & agents = Model<TSeq>::get_agents();
    
    for (const auto & agent : agents)
    {
        if (agent.get_n_entities() > 0u)
        {
            const auto & entity = agent.get_entity(0u);
            if (entity.get_id() == static_cast<int>(entity_id))
            {
                total_agents++;
                if (agent.get_n_tools() > 0u) // Vaccinated agents have tools
                {
                    vaccinated_agents++;
                }
            }
        }
    }
    
    if (total_agents == 0) return 0.0;
    return static_cast<double>(vaccinated_agents) / static_cast<double>(total_agents);
}

template<typename TSeq>
inline size_t ModelMeaslesEntityQuarantine<TSeq>::get_quarantine_duration_for_entity(size_t entity_id)
{
    double vaccination_prevalence = calculate_entity_vaccination_prevalence(entity_id);
    
    // Default quarantine duration if no thresholds are set
    if (vaccination_prevalence_thresholds.empty() || quarantine_durations.empty())
    {
        return 21; // Default 21 days
    }
    
    // Find the appropriate quarantine duration based on vaccination prevalence
    for (size_t i = 0; i < vaccination_prevalence_thresholds.size(); ++i)
    {
        if (vaccination_prevalence >= vaccination_prevalence_thresholds[i])
        {
            if (i < quarantine_durations.size())
                return quarantine_durations[i];
        }
    }
    
    // Return the last duration if we don't find a matching threshold
    return quarantine_durations.back();
}

template<typename TSeq>
inline size_t ModelMeaslesEntityQuarantine<TSeq>::sample_agents(
    Agent<TSeq> * agent,
    std::vector< size_t > & sampled_agents
)
{

    // We are assuming one agent per entity
    size_t agent_group_id = agent->get_entity(0u).get_id();

    #ifdef EPI_DEBUG
    if (sampled_sizes.size() <= agent_group_id)
        sampled_sizes.resize(agent_group_id + 1u, 0);
    #endif
    
    // Adjusting the contact rate based on the number of agents per entity
    const epiworld_double & contact_rate_adjusted = 
        adjusted_contact_rate[agent_group_id];

    // Is there an entity from which to take samples?
    // (at least self)
    for (size_t g = 0u; g < Model<TSeq>::get_entities().size(); ++g)
    {

        // How many from this entity?
        int n_contacts = std::min(
            static_cast<int>(
                static_cast<epiworld_double>(Model<TSeq>::runif()) * 
                    contact_rate_adjusted * contact_matrix[g * Model<TSeq>::get_entities().size() + agent_group_id] +
                    Model<TSeq>::runif()
            ),
            static_cast<int>(n_infectious_per_group[g])
        );

        if (n_contacts <= 0)
            continue;
            
        // Sampling from the entity
        for (int i = 0; i < n_contacts; ++i)
        {

            int which = static_cast<epiworld_fast_uint>(
                Model<TSeq>::runif() * n_infectious_per_group[g]
            );

            #ifdef EPI_DEBUG
            if (this->get_db().size() > 0u)
            {
                // auto & a = this->population.at(infectious.at(entity_indices[g] + which));
            } else {
                // auto & a = this->get_agent(infectious[entity_indices[g] + which]);
            }
            sampled_sizes[agent_group_id]++;
            #endif

            sampled_agents.push_back(infectious[entity_indices[g] + which]);

        }

    }

    return sampled_agents.size();

}

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_quarantine_process() {

    // Process entity-level quarantine
    for (size_t entity_i = 0u; entity_i < Model<TSeq>::get_entities().size(); ++entity_i)
    {

        // Checking if the quarantine in the entity was triggered
        // or not
        if (
            entity_quarantine_triggered[entity_i] != 
            ModelMeaslesEntityQuarantine<TSeq>::QUARANTINE_PROCESS_ACTIVE
        )
            continue;

        // Get quarantine duration for this entity based on vaccination prevalence
        size_t quarantine_duration = get_quarantine_duration_for_entity(entity_i);
        
        if (quarantine_duration == 0)
            continue;

        // Quarantine all eligible agents in this entity
        auto & agents = Model<TSeq>::get_agents();
        
        for (auto & agent : agents)
        {
            if (agent.get_n_entities() > 0u)
            {
                const auto & entity = agent.get_entity(0u);
                if (entity.get_id() == static_cast<int>(entity_i))
                {
                    // Check if agent is in a state that can be quarantined
                    if (agent.get_state() > RASH)
                        continue;

                    // Agents with some tool (vaccinated) won't be quarantined
                    if (agent.get_n_tools() != 0u)
                        continue;

                    if (quarantine_willingness[agent.get_id()])
                    {

                        switch (agent.get_state())
                        {
                            case SUSCEPTIBLE:
                                agent.change_state(this, QUARANTINED_SUSCEPTIBLE);
                                day_flagged[agent.get_id()] = Model<TSeq>::today();
                                break;
                            case EXPOSED:
                                agent.change_state(this, QUARANTINED_EXPOSED);
                                day_flagged[agent.get_id()] = Model<TSeq>::today();
                                break;
                            case PRODROMAL:
                                agent.change_state(this, QUARANTINED_PRODROMAL);
                                day_flagged[agent.get_id()] = Model<TSeq>::today();
                                break;
                            case RASH:
                                if (isolation_willingness[agent.get_id()])
                                {
                                    agent.change_state(this, ISOLATED);
                                    day_flagged[agent.get_id()] = Model<TSeq>::today();
                                }
                                break;
                            default:
                                throw std::logic_error(
                                    "The agent is not in a state that can be quarantined."
                                );
                        }

                    }
                }
            }
        }

        // Setting the quarantine process off
        entity_quarantine_triggered[entity_i] = 
            ModelMeaslesEntityQuarantine<TSeq>::QUARANTINE_PROCESS_DONE;
    }

    return;
}

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_susceptible(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Sampling how many contacts occur (this is for mixing)
    model->sampled_agents.clear();
    model->sample_agents(p, model->sampled_agents);

    // This computes the prob of getting any neighbor variant
    int nviruses_tmp = 0;

    for (auto & neighbor_id: model->sampled_agents) 
    {
        
        auto & neighbor = m->get_agent(neighbor_id);
        auto & v = neighbor.get_virus();

        /* \
         * For each virus, we compute the probability of contagion
         * and sample accordingly
         */
        if (v != nullptr)
        {
            
            #ifdef EPI_DEBUG
            if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
                throw std::logic_error(
                    "Trying to add an extra element to a temporal array outside of the range."
                );
            #endif
                
            /* And it is a function of susceptibility_reduction as well */ 
            m->array_double_tmp[nviruses_tmp] =
                (1.0 - p->get_susceptibility_reduction(v, m)) * 
                v->get_prob_infecting(m) * 
                (1.0 - neighbor.get_transmission_reduction(v, m)) 
                ; 
        
            m->array_virus_tmp[nviruses_tmp++] = v.get();

        }

    }

    // If there are no viruses, then no need to check
    if (nviruses_tmp == 0u)
        return;

    // Running the roulette
    SAMPLE_FROM_PROBS(nviruses_tmp, which);

    // Did the individual get infected?
    if (m->runif() < m->array_double_tmp[which])
    {

        p->set_virus(
            *m->array_virus_tmp[which], 
            m,
            ModelMeaslesEntityQuarantine<TSeq>::EXPOSED
            );

        model->day_exposed[p->get_id()] = m->today();

    }

    return;
}

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_exposed(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    if (m->runif() < 1.0/(p->get_virus()->get_incubation(m)))
    {
        p->change_state(
            m, ModelMeaslesEntityQuarantine<TSeq>::PRODROMAL
        );
    }

};

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_prodromal(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Sampling whether the agent is detected or not
    bool detected = m->runif() < 1.0/m->par("Days undetected");

    // If detected and the entity can quarantine, we start
    // the quarantine process for the entire entity
    if (detected && p->get_n_entities() > 0u)
    {
        size_t entity_id = p->get_entity(0u).get_id();
        model->entity_quarantine_triggered[entity_id] = 
            ModelMeaslesEntityQuarantine<TSeq>::QUARANTINE_PROCESS_ACTIVE;
    }

    // Computing probabilities for state change
    const auto & v = p->get_virus();
    m->array_double_tmp[0] = 1.0 - (1.0 - v->get_prob_recovery(m)) *
        (1.0 - p->get_recovery_enhancer(v, m));
    m->array_double_tmp[1] = m->par("Hospitalization rate");
        
    SAMPLE_FROM_PROBS(2, which);
    
    if (which == 0) // Recovers
    {
        
        p->rm_virus(m, ModelMeaslesEntityQuarantine<TSeq>::RECOVERED);
        
    }
    else if (which == 1) // Hospitalized
    {
        
        p->change_state(m, ModelMeaslesEntityQuarantine<TSeq>::HOSPITALIZED);
        
    }
    else // Develops rash
    {
        
        model->day_rash_onset[p->get_id()] = m->today();
        p->change_state(m, ModelMeaslesEntityQuarantine<TSeq>::RASH);
        
    }

};

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_rash(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Checking if the agent is willing to isolate individually
    bool isolation_detected = (m->par("Isolation period") >= 0) &&
        (model->isolation_willingness[p->get_id()]);

    // Recording the date of detection
    if (isolation_detected)
        model->day_flagged[p->get_id()] = m->today();

    // Computing probabilities for state change
    const auto & v = p->get_virus();
    m->array_double_tmp[0] = 1.0 - (1.0 - v->get_prob_recovery(m)) *
        (1.0 - p->get_recovery_enhancer(v, m));
    m->array_double_tmp[1] = m->par("Hospitalization rate");
        
    SAMPLE_FROM_PROBS(2, which);
    
    if (which == 0) // Recovers
    {
        if (isolation_detected)
        {
            p->change_state(
                m, ModelMeaslesEntityQuarantine<TSeq>::ISOLATED_RECOVERED
            );
        }
        else
        {
            p->rm_virus(
                m, ModelMeaslesEntityQuarantine<TSeq>::RECOVERED
            );
        }
    }
    else if (which == 1) // Hospitalized
    {
        if (isolation_detected)
        {
            p->change_state(m, ModelMeaslesEntityQuarantine<TSeq>::DETECTED_HOSPITALIZED);
        }
        else
        {
            p->change_state(m, ModelMeaslesEntityQuarantine<TSeq>::HOSPITALIZED);
        }
    }
    else // Stays in rash
    {
        if (isolation_detected)
        {
            p->change_state(m, ModelMeaslesEntityQuarantine<TSeq>::ISOLATED);
        }
    }

};

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_isolated(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Computing probabilities for state change
    const auto & v = p->get_virus();
    m->array_double_tmp[0] = 1.0 - (1.0 - v->get_prob_recovery(m)) *
        (1.0 - p->get_recovery_enhancer(v, m));
    m->array_double_tmp[1] = m->par("Hospitalization rate");
        
    SAMPLE_FROM_PROBS(2, which);
    
    if (which == 0) // Recovers
    {
        p->change_state(
            m, ModelMeaslesEntityQuarantine<TSeq>::ISOLATED_RECOVERED
        );
    }
    else if (which == 1) // Hospitalized
    {
        p->change_state(m, ModelMeaslesEntityQuarantine<TSeq>::DETECTED_HOSPITALIZED);
    }

};

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_quarantine_suscep(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);
    
    // Getting the quarantine duration based on entity vaccination prevalence
    size_t quarantine_duration = model->get_quarantine_duration_for_entity(
        p->get_entity(0u).get_id()
    );
    
    int days_since = m->today() - model->day_flagged[p->get_id()];

    bool unquarantine =
        (quarantine_duration <= static_cast<size_t>(days_since)) ?
        true: false;

    if (unquarantine)
        p->change_state(m, ModelMeaslesEntityQuarantine<TSeq>::SUSCEPTIBLE);

};

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_quarantine_exposed(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);
    
    // Getting the quarantine duration based on entity vaccination prevalence
    size_t quarantine_duration = model->get_quarantine_duration_for_entity(
        p->get_entity(0u).get_id()
    );
    
    int days_since = m->today() - model->day_flagged[p->get_id()];

    bool unquarantine =
        (quarantine_duration <= static_cast<size_t>(days_since)) ?
        true: false;

    if (m->runif() < 1.0/(p->get_virus()->get_incubation(m)))
    {

        // If the agent is unquarantined, it becomes prodromal
        if (unquarantine)
        {
            p->change_state(
                m, ModelMeaslesEntityQuarantine<TSeq>::PRODROMAL
            );
        }
        else
        {
            p->change_state(
                m, ModelMeaslesEntityQuarantine<TSeq>::QUARANTINED_PRODROMAL
            );
        }

    }
    else if (unquarantine)
    {
        p->change_state(
            m, ModelMeaslesEntityQuarantine<TSeq>::EXPOSED
        );
    }

};

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_quarantine_prodromal(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);
    
    // Getting the quarantine duration based on entity vaccination prevalence
    size_t quarantine_duration = model->get_quarantine_duration_for_entity(
        p->get_entity(0u).get_id()
    );

    int days_since = m->today() - model->day_flagged[p->get_id()];

    bool unquarantine =
        (quarantine_duration <= static_cast<size_t>(days_since)) ?
        true: false;
    
    // Develops rash?
    if (m->runif() < (1.0/m->par("Prodromal period")))
    {
        model->day_rash_onset[p->get_id()] = m->today();
        p->change_state(m, ModelMeaslesEntityQuarantine<TSeq>::ISOLATED);
    }
    else
    {
        
        if (unquarantine)
            p->change_state(m, ModelMeaslesEntityQuarantine<TSeq>::PRODROMAL);

    }

};

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_quarantine_recovered(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);
    
    // Getting the quarantine duration based on entity vaccination prevalence
    size_t quarantine_duration = model->get_quarantine_duration_for_entity(
        p->get_entity(0u).get_id()
    );
    
    int days_since = m->today() - model->day_flagged[p->get_id()];
    
    if (static_cast<size_t>(days_since) >= quarantine_duration)
        p->change_state(m, ModelMeaslesEntityQuarantine<TSeq>::RECOVERED);

};

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_isolated_recovered(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Figuring out if the agent can be released from isolation
    // if the isolation period is over.
    int days_since = m->today() - model->day_rash_onset[p->get_id()];

    bool unisolate =
        (m->par("Isolation period") <= days_since) ?
        true: false;

    if (unisolate)
    {
        p->change_state(
            m, ModelMeaslesEntityQuarantine<TSeq>::RECOVERED
        );
    }

};

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::m_update_hospitalized(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // The agent is removed from the system
    if (m->runif() < 1.0/m->par("Hospitalization period"))
        p->rm_virus(m, ModelMeaslesEntityQuarantine<TSeq>::RECOVERED);

};

/**
 * @brief Template for a Measles model with entity-level quarantine
 * 
 * @param model A ModelMeaslesEntityQuarantine<TSeq> object where to set up the model.
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
 * @param quarantine_willingness Proportion of individuals willing to comply with quarantine
 * @param isolation_willingness Proportion of individuals willing to self-isolate when detected
 * @param isolation_period Duration of isolation in days for detected infected individuals
 * @param prop_vaccinated Proportion of vaccinated agents
 * @param vaccination_prevalence_thresholds Vaccination prevalence thresholds for quarantine duration
 * @param quarantine_durations Quarantine durations corresponding to vaccination prevalence thresholds
 */
template<typename TSeq>
inline ModelMeaslesEntityQuarantine<TSeq>::ModelMeaslesEntityQuarantine(
    ModelMeaslesEntityQuarantine<TSeq> & model,
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
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    epiworld_fast_int isolation_period,
    epiworld_double prop_vaccinated,
    std::vector< double > vaccination_prevalence_thresholds,
    std::vector< size_t > quarantine_durations
    )
{

    // Setting up the contact matrix
    this->contact_matrix = contact_matrix;

    // Setting up vaccination prevalence thresholds and quarantine durations
    this->vaccination_prevalence_thresholds = vaccination_prevalence_thresholds;
    this->quarantine_durations = quarantine_durations;

    // Setting up parameters
    model.add_param(contact_rate, "Contact rate");
    model.add_param(transmission_rate, "Transmission rate");
    model.add_param(incubation_period, "Incubation period");
    model.add_param(prodromal_period, "Prodromal period");
    model.add_param(rash_period, "Rash period");
    model.add_param(hospitalization_rate, "Hospitalization rate");
    model.add_param(hospitalization_period, "Hospitalization period");
    model.add_param(days_undetected, "Days undetected");
    model.add_param(
        quarantine_willingness, "Quarantine willingness"
    );
    model.add_param(
        isolation_willingness, "Isolation willingness"
    );
    model.add_param(isolation_period, "Isolation period");
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
        ModelMeaslesEntityQuarantine<TSeq>::EXPOSED,
        ModelMeaslesEntityQuarantine<TSeq>::RECOVERED,
        ModelMeaslesEntityQuarantine<TSeq>::RECOVERED
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

    // Initialize data structures after agents are created
    model.quarantine_willingness.resize(n);
    model.isolation_willingness.resize(n);
    model.entity_quarantine_triggered.resize(model.get_entities().size(), QUARANTINE_PROCESS_INACTIVE);
    model.day_flagged.resize(n, -1);
    model.day_rash_onset.resize(n, -1);
    model.day_exposed.resize(n, -1);

    // Initialize willingness based on model parameters
    for (size_t i = 0u; i < n; ++i) {
        model.quarantine_willingness[i] = (model.runif() < quarantine_willingness);
        model.isolation_willingness[i] = (model.runif() < isolation_willingness);
    }

    // Initialize mixing infrastructure after entities are added
    model.infectious.resize(n);
    model.sampled_agents.resize(0);

    #ifdef EPI_DEBUG
    model.sampled_sizes.resize(0);
    #endif

    // These will be initialized when entities are added
    // model.n_infectious_per_group will be resized when entities are added
    // model.entity_indices will be resized when entities are added
    // model.adjusted_contact_rate will be resized when entities are added

    model.set_name("Measles with Entity-level Quarantine");

    return;

}

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::initialize_entity_data() {
    
    size_t n_entities = this->get_entities().size();
    if (n_entities == 0) return;
    
    // Initialize entity-related data structures
    n_infectious_per_group.resize(n_entities, 0u);
    entity_indices.resize(n_entities);
    adjusted_contact_rate.resize(n_entities);
    entity_quarantine_triggered.resize(n_entities, ModelMeaslesEntityQuarantine<TSeq>::QUARANTINE_PROCESS_INACTIVE);
    
    // Computing the cumulative number of agents per entity
    size_t cumsum = 0u;
    for (size_t i = 0u; i < n_entities; ++i) {
        entity_indices[i] = cumsum;
        cumsum += this->get_entity(i).size();
        
        // Computing the adjustment to the contact rate
        adjusted_contact_rate[i] = static_cast<double>(this->get_entity(i).size()) /
            static_cast<double>(this->size());
    }
    
}

template<typename TSeq>
inline void initialize_entity_data(ModelMeaslesEntityQuarantine<TSeq> & model) {
    model.initialize_entity_data();
}

template<typename TSeq>
inline ModelMeaslesEntityQuarantine<TSeq>::ModelMeaslesEntityQuarantine(
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
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    epiworld_fast_int isolation_period,
    epiworld_double prop_vaccinated,
    std::vector< double > vaccination_prevalence_thresholds,
    std::vector< size_t > quarantine_durations
    )
{   

    this->contact_matrix = contact_matrix;

    ModelMeaslesEntityQuarantine(
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
        quarantine_willingness,
        isolation_willingness,
        isolation_period,
        prop_vaccinated,
        vaccination_prevalence_thresholds,
        quarantine_durations
    );

    // Initialize entity data structures
    this->initialize_entity_data();

    return;

}

template<typename TSeq>
inline ModelMeaslesEntityQuarantine<TSeq> & ModelMeaslesEntityQuarantine<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
)
{

    Model<TSeq>::initial_states_fun =
        create_init_function_seir<TSeq>(proportions_)
        ;

    return *this;

}

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::set_quarantine_thresholds(
    std::vector< double > thresholds,
    std::vector< size_t > durations
) {
    
    if (thresholds.size() != durations.size()) {
        throw std::invalid_argument(
            "Vaccination prevalence thresholds and quarantine durations must have the same size."
        );
    }
    
    this->vaccination_prevalence_thresholds = thresholds;
    this->quarantine_durations = durations;
}

template<typename TSeq>
inline ModelMeaslesEntityQuarantine<TSeq> & ModelMeaslesEntityQuarantine<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
) {
    Model<TSeq>::run(ndays, seed);
    return *this;
}

template<typename TSeq>
inline void ModelMeaslesEntityQuarantine<TSeq>::reset() {
    Model<TSeq>::reset();
    
    // Reset quarantine tracking
    std::fill(entity_quarantine_triggered.begin(), entity_quarantine_triggered.end(), QUARANTINE_PROCESS_INACTIVE);
    std::fill(day_flagged.begin(), day_flagged.end(), -1);
    std::fill(day_rash_onset.begin(), day_rash_onset.end(), -1);
    std::fill(day_exposed.begin(), day_exposed.end(), -1);
}

template<typename TSeq>
inline Model<TSeq> * ModelMeaslesEntityQuarantine<TSeq>::clone_ptr() {
    
    ModelMeaslesEntityQuarantine<TSeq> * ptr = new ModelMeaslesEntityQuarantine<TSeq>(
        *this
    );
    
    return dynamic_cast< Model<TSeq> *>(ptr);
}

#undef MM
#undef GET_MODEL
#endif