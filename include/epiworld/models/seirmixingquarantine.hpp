#ifndef EPIWORLD_MODELS_SEIRMIXINGQUARANTINE_HPP
#define EPIWORLD_MODELS_SEIRMIXINGQUARANTINE_HPP

using namespace epiworld;

#define MM(i, j, n) \
    j * n + i

#if __cplusplus >= 202302L
    // C++23 or later
    #define GET_MODEL(model, output) \
        auto * output = dynamic_cast< ModelSEIRMixingQuarantine<TSeq> * >( (model) ); \
        /*Using the [[assume(...)]] to avoid the compiler warning \
        if the standard is C++23 or later */ \
        [[assume((output) != nullptr)]]
#else
    // C++17 or C++20
    #define GET_MODEL(model, output) \
        auto * output = dynamic_cast< ModelSEIRMixingQuarantine<TSeq> * >( (model) ); \
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
 * @file seirmixingquarantine.hpp
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model with mixing, quarantine, and contact tracing
 */

/**
 * @brief SEIR model with mixing, quarantine, and contact tracing
 * 
 * This class implements a Susceptible-Exposed-Infected-Removed (SEIR) epidemiological model
 * with additional features including:
 * - Population mixing based on contact matrices
 * - Quarantine measures for exposed contacts
 * - Isolation policies for detected infected individuals
 * - Contact tracing with configurable success rates
 * - Hospitalization of severe cases
 * - Individual willingness to comply with public health measures
 * 
 * The model supports 10 distinct states:
 * - Susceptible: Individuals who can become infected
 * - Exposed: Infected but not yet infectious (incubation period)
 * - Infected: Infectious individuals in the community
 * - Isolated: Detected infected individuals in self-isolation
 * - Detected Hospitalized: Hospitalized individuals who were contact-traced
 * - Quarantined Susceptible: Susceptible individuals in quarantine due to contact tracing
 * - Quarantined Exposed: Exposed individuals in quarantine due to contact tracing
 * - Isolated Recovered: Recovered individuals still in isolation
 * - Hospitalized: Individuals requiring hospital care
 * - Recovered: Individuals who have recovered and gained immunity
 * 
 * @tparam TSeq Type for genetic sequences (default: EPI_DEFAULT_TSEQ)
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSEIRMixingQuarantine : public Model<TSeq> 
{
private:
    
    // Vector of vectors of infected agents     
    std::vector< size_t > infected;

    // Number of infected agents in each group
    std::vector< size_t > n_infected_per_group;
    
    // Where the agents start in the `infected` vector
    std::vector< size_t > entity_indices;

    void m_update_infected_list();
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
    static void m_update_infected(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_isolated(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_quarantine_suscep(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_quarantine_exposed(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_hospitalized(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_isolated_recovered(Agent<TSeq> * p, Model<TSeq> * m);

    // Data about the quarantine process
    std::vector< bool > quarantine_willingness; ///< Indicator
    std::vector< bool > isolation_willingness; ///< Indicator for isolation willingness
    std::vector< size_t > agent_quarantine_triggered; ///< Whether the quarantine process has started
    std::vector< int > day_flagged; ///< Either detected or started quarantine
    std::vector< int > day_onset; ///< Day of onset of the disease
    std::vector< int > day_exposed; ///< Day of exposure

    void m_quarantine_process();
    static void m_update_model(Model<TSeq> * m);

    // We will limit tracking to up to EPI_MAX_TRACKING
    std::vector< size_t > tracking_matrix; ///< Tracking matrix for agent interactions
    std::vector< size_t > tracking_matrix_size; ///< Number of current interactions for each agent

    void m_add_tracking(size_t infected_id, size_t agent_id);

public:

    static const int SUSCEPTIBLE             = 0;
    static const int EXPOSED                 = 1;
    static const int INFECTED                = 2;
    static const int ISOLATED                = 3;
    static const int DETECTED_HOSPITALIZED   = 4;
    static const int QUARANTINED_SUSCEPTIBLE = 5;
    static const int QUARANTINED_EXPOSED     = 6;
    static const int ISOLATED_RECOVERED      = 7;
    static const int HOSPITALIZED            = 8;
    static const int RECOVERED               = 9;

    static const size_t QUARANTINE_PROCESS_INACTIVE = 0u;
    static const size_t QUARANTINE_PROCESS_ACTIVE   = 1u;
    static const size_t QUARANTINE_PROCESS_DONE     = 2u;

    ModelSEIRMixingQuarantine() {};
    
    /**
     * @brief Constructs a ModelSEIRMixingQuarantine object.
     *
     * @param model A reference to an existing ModelSEIRMixingQuarantine object.
     * @param vname The name of the ModelSEIRMixingQuarantine object.
     * @param n The number of entities in the model.
     * @param prevalence The initial prevalence of the disease in the model.
     * @param contact_rate The contact rate between entities in the model.
     * @param transmission_rate The transmission rate of the disease in the model.
     * @param avg_incubation_days The average incubation period of the disease in the model.
     * @param recovery_rate The recovery rate of the disease in the model.
     * @param contact_matrix The contact matrix between entities in the model. Specified in
     * column-major order.
     * @param hospitalization_rate The rate at which infected individuals are hospitalized.
     * @param hospitalization_period The average duration of hospitalization in days.
     * @param days_undetected The average number of days an infected individual remains undetected.
     * @param quarantine_period The duration of quarantine in days for exposed contacts.
     * @param quarantine_willingness The proportion of individuals willing to comply with quarantine measures.
     * @param isolation_willingness The proportion of individuals willing to self-isolate when detected.
     * @param isolation_period The duration of isolation in days for detected infected individuals.
     * @param contact_tracing_success_rate The probability of successfully identifying and tracing contacts (default: 1.0).
     * @param contact_tracing_days_prior The number of days prior to detection for which contacts are traced (default: 4).
     */
    ModelSEIRMixingQuarantine(
        ModelSEIRMixingQuarantine<TSeq> & model,
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate,
        std::vector< double > contact_matrix,
        epiworld_double hospitalization_rate,
        epiworld_double hospitalization_period,
        // Policy parameters
        epiworld_double days_undetected,
        epiworld_fast_int quarantine_period,
        epiworld_double quarantine_willingness,
        epiworld_double isolation_willingness,
        epiworld_fast_int isolation_period,
        epiworld_double contact_tracing_success_rate = 1.0,
        epiworld_fast_uint contact_tracing_days_prior = 4u
    );
    
    /**
     * @brief Constructs a ModelSEIRMixingQuarantine object.
     *
     * @param vname The name of the ModelSEIRMixingQuarantine object.
     * @param n The number of entities in the model.
     * @param prevalence The initial prevalence of the disease in the model.
     * @param contact_rate The contact rate between entities in the model.
     * @param transmission_rate The transmission rate of the disease in the model.
     * @param avg_incubation_days The average incubation period of the disease in the model.
     * @param recovery_rate The recovery rate of the disease in the model.
     * @param contact_matrix The contact matrix between entities in the model.
     * @param hospitalization_rate The rate at which infected individuals are hospitalized.
     * @param hospitalization_period The average duration of hospitalization in days.
     * @param days_undetected The average number of days an infected individual remains undetected.
     * @param quarantine_period The duration of quarantine in days for exposed contacts.
     * @param quarantine_willingness The proportion of individuals willing to comply with quarantine measures.
     * @param isolation_willingness The proportion of individuals willing to self-isolate when detected.
     * @param isolation_period The duration of isolation in days for detected infected individuals.
     * @param contact_tracing_success_rate The probability of successfully identifying and tracing contacts (default: 1.0).
     * @param contact_tracing_days_prior The number of days prior to detection for which contacts are traced (default: 4).
     */
    ModelSEIRMixingQuarantine(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate,
        std::vector< double > contact_matrix,
        epiworld_double hospitalization_rate,
        epiworld_double hospitalization_period,
        // Policy parameters
        epiworld_double days_undetected,
        epiworld_fast_int quarantine_period,
        epiworld_double quarantine_willingness,
        epiworld_double isolation_willingness,
        epiworld_fast_int isolation_period,
        epiworld_double contact_tracing_success_rate = 1.0,
        epiworld_fast_uint contact_tracing_days_prior = 4u
    );

    /**
     * @brief Run the model simulation
     * @param ndays Number of days to simulate
     * @param seed Random seed for reproducibility (default: -1 for random seed)
     * @return Reference to this model instance
     */
    ModelSEIRMixingQuarantine<TSeq> & run(
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
    ModelSEIRMixingQuarantine<TSeq> & initial_states(
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

};

template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::m_update_model(Model<TSeq> * m)
{
    GET_MODEL(m, model);
    model->m_quarantine_process();
    model->events_run();
    model->m_update_infected_list();
    return;
}

template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::m_add_tracking(
    size_t infected_id,
    size_t agent_id
)
{

    // We avoid the math if there's no point in tracking anymore
    if (
        agent_quarantine_triggered[infected_id] >= 
        ModelSEIRMixingQuarantine<TSeq>::QUARANTINE_PROCESS_DONE
    )
        return;

    // We avoid the math if the contact happened before
    // the lower bound of the contact tracing
    size_t days_since_onset = Model<TSeq>::today() - day_onset[infected_id];
    if (days_since_onset > 
        Model<TSeq>::par("Contact tracing days prior")
    )
        return;
    

    // If we are overflow, we start from the beginning
    size_t loc = tracking_matrix_size[infected_id] % EPI_MAX_TRACKING;
    tracking_matrix[MM(infected_id, loc, Model<TSeq>::size())] = agent_id;

    // We increase the size of the tracking matrix
    tracking_matrix_size[infected_id]++;

    return;
}


template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::m_update_infected_list()
{

    auto & agents = Model<TSeq>::get_agents();

    std::fill(n_infected_per_group.begin(), n_infected_per_group.end(), 0u);
    
    for (auto & a : agents)
    {

        if (a.get_state() == ModelSEIRMixingQuarantine<TSeq>::INFECTED)
        {
            if (a.get_n_entities() > 0u)
            {
                const auto & entity = a.get_entity(0u);
                infected[
                    // Position of the group in the `infected` vector
                    entity_indices[entity.get_id()] +
                    // Position of the agent in the group
                    n_infected_per_group[entity.get_id()]++
                ] = a.get_id();

            }
        }

    }

    return;

}

template<typename TSeq>
inline size_t ModelSEIRMixingQuarantine<TSeq>::sample_agents(
    epiworld::Agent<TSeq> * agent,
    std::vector< size_t > & sampled_agents
    )
{

    size_t agent_group_id = agent->get_entity(0u).get_id();
    size_t ngroups = this->entities.size();

    int samp_id = 0;
    for (size_t g = 0; g < ngroups; ++g)
    {

        size_t group_size = n_infected_per_group[g];

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
            auto & a = this->population.at(infected.at(entity_indices[g] + which));
            #else
            auto & a = this->get_agent(infected[entity_indices[g] + which]);
            #endif

            #ifdef EPI_DEBUG
            if (a.get_state() != ModelSEIRMixingQuarantine<TSeq>::INFECTED)
                throw std::logic_error(
                    "The agent is not infected, but it should be."
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
inline ModelSEIRMixingQuarantine<TSeq> & ModelSEIRMixingQuarantine<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{

    Model<TSeq>::run(ndays, seed);

    return *this;

}

template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::reset()
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
    n_infected_per_group.resize(this->entities.size(), 0u);
    std::fill(n_infected_per_group.begin(), n_infected_per_group.end(), 0u);

    // We are assuming one agent per entity
    infected.resize(Model<TSeq>::size());
    std::fill(infected.begin(), infected.end(), 0u);

    // This will say when do the groups start in the `infected` vector
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

    this->m_update_infected_list();

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

    day_onset.resize(this->size(), 0);
    std::fill(
        day_onset.begin(),
        day_onset.end(),
        0
    );

    day_exposed.resize(this->size(), 0);
    std::fill(
        day_exposed.begin(),
        day_exposed.end(),
        0
    );

    // Tracking matrix
    tracking_matrix.resize(EPI_MAX_TRACKING * Model<TSeq>::size(), 0u);
    std::fill(tracking_matrix.begin(), tracking_matrix.end(), 0u);

    tracking_matrix_size.resize(Model<TSeq>::size(), 0u);
    std::fill(tracking_matrix_size.begin(), tracking_matrix_size.end(), 0u);

    return;

}

template<typename TSeq>
inline Model<TSeq> * ModelSEIRMixingQuarantine<TSeq>::clone_ptr()
{
    
    ModelSEIRMixingQuarantine<TSeq> * ptr = new ModelSEIRMixingQuarantine<TSeq>(
        *dynamic_cast<const ModelSEIRMixingQuarantine<TSeq>*>(this)
        );

    #if __cplusplus >= 202302L
        // C++23 or later
        [[assume(ptr != nullptr)]]
    #else
        // C++17 or C++20
        assert(ptr != nullptr); // Use assert for runtime checks
    #endif

    return dynamic_cast< Model<TSeq> *>(ptr);

}

template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::m_update_susceptible(
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
        ModelSEIRMixingQuarantine<TSeq>::EXPOSED
        );

    return; 

};

template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::m_update_exposed(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // Getting the virus
    auto & v = p->get_virus();

    // Does the agent become infected?
    if (m->runif() < 1.0/(v->get_incubation(m)))
    {

        p->change_state(m, ModelSEIRMixingQuarantine<TSeq>::INFECTED);

        GET_MODEL(m, model);
        model->day_onset[p->get_id()] = m->today();

        return;

    }

    return;

};

template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::m_update_infected(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Sampling whether the agent is detected or not
    bool detected = m->runif() < 1.0/m->par("Days undetected");

    // If detected and the entity can quarantine, we start
    // the quarantine process
    if (detected)
    {
        model->agent_quarantine_triggered[p->get_id()] = 
            ModelSEIRMixingQuarantine<TSeq>::QUARANTINE_PROCESS_ACTIVE;
    }

    // Checking if the agent is willing to isolate individually
    // This is separate from quarantine and can happen even if agent cannot quarantine
    bool isolation_detected = (m->par("Isolation period") >= 0) &&
        detected &&
        (model->isolation_willingness[p->get_id()])
    ;

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
                m, ModelSEIRMixingQuarantine<TSeq>::ISOLATED_RECOVERED
            );
        }
        else
        {
            p->rm_virus(
                m, ModelSEIRMixingQuarantine<TSeq>::RECOVERED
            );
        }

        return;
    }
    else if (which == 1) // Hospitalized
    {

        if (detected)
        {
            p->change_state(
                m, ModelSEIRMixingQuarantine<TSeq>::DETECTED_HOSPITALIZED
            );
        }
        else
        {
            p->change_state(
                m, ModelSEIRMixingQuarantine<TSeq>::HOSPITALIZED
            );
        }

    }
    else if ((which == 2) && isolation_detected) // Nothing, but detected
    {
        // If the agent is detected, it goes to isolation
        p->change_state(
            m, ModelSEIRMixingQuarantine<TSeq>::ISOLATED
        );

    }
    
    return ;

};

template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::m_update_isolated(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Figuring out if the agent can be released from isolation
    // if the quarantine period is over.
    int days_since = m->today() - model->day_onset[p->get_id()];

    bool unisolate =
        (m->par("Isolation period") <= days_since) ?
        true: false;

    // Sampling from the probabilities of recovery   
    m->array_double_tmp[0] = 1.0 -
        (1.0 - p->get_virus()->get_prob_recovery(m)) *
        (1.0 - p->get_recovery_enhancer(p->get_virus(), m));

    // And hospitalization
    m->array_double_tmp[1] = m->par("Hospitalization rate");

    SAMPLE_FROM_PROBS(2, which);

    // Recovers
    if (which == 0)
    {
        if (unisolate)
        {
            p->rm_virus(
                m,
                ModelSEIRMixingQuarantine<TSeq>::RECOVERED
            );
        }
        else
            p->rm_virus(
                m, ModelSEIRMixingQuarantine<TSeq>::ISOLATED_RECOVERED
            );
    }
    else if (which == 1)
    {

        if (unisolate)
        {
            p->change_state(
                m, ModelSEIRMixingQuarantine<TSeq>::HOSPITALIZED
            );
        }
        else
        {
            p->change_state(
                m, ModelSEIRMixingQuarantine<TSeq>::DETECTED_HOSPITALIZED
            );
        }
    }
    else if ((which == 2) && unisolate)
    {
        p->change_state(
            m, ModelSEIRMixingQuarantine<TSeq>::INFECTED
        );
    }


};

template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::m_update_quarantine_suscep(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Figuring out if the agent can be released from quarantine
    // if the quarantine period is over.
    int days_since = m->today() - model->day_flagged[p->get_id()];

    bool unquarantine =
        (m->par("Quarantine period") <= days_since) ?
        true: false;

    if (unquarantine)
    {
        p->change_state(
            m, ModelSEIRMixingQuarantine<TSeq>::SUSCEPTIBLE
        );
    }

};

template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::m_update_quarantine_exposed(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Figuring out if the agent can be released from quarantine
    // if the quarantine period is over.
    int days_since = m->today() - model->day_flagged[p->get_id()];

    bool unquarantine =
        (m->par("Quarantine period") <= days_since) ?
        true: false;

    if (m->runif() < 1.0/(p->get_virus()->get_incubation(m)))
    {

        // Recording the day of onset
        model->day_onset[p->get_id()] = m->today();

        // If the agent is unquarantined, it becomes infected
        if (unquarantine)
        {
            p->change_state(
                m, ModelSEIRMixingQuarantine<TSeq>::INFECTED
            );
        }
        else
        {
            p->change_state(
                m, ModelSEIRMixingQuarantine<TSeq>::ISOLATED
            );
        }

    }
    else if (unquarantine)
    {
        p->change_state(
            m, ModelSEIRMixingQuarantine<TSeq>::EXPOSED
        );
    }

};

template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::m_update_isolated_recovered(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    GET_MODEL(m, model);

    // Figuring out if the agent can be released from isolation
    // if the quarantine period is over.
    int days_since = m->today() - model->day_onset[p->get_id()];

    bool unisolate =
        (m->par("Isolation period") <= days_since) ?
        true: false;

    if (unisolate)
    {
        p->change_state(
            m, ModelSEIRMixingQuarantine<TSeq>::RECOVERED
        );
    }

};

template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::m_update_hospitalized(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // The agent is removed from the system
    if (m->runif() < 1.0/m->par("Hospitalization period"))
        p->rm_virus(m, ModelSEIRMixingQuarantine<TSeq>::RECOVERED);

};

template<typename TSeq>
inline void ModelSEIRMixingQuarantine<TSeq>::m_quarantine_process() {

    // Process entity-level quarantine
    for (size_t agent_i = 0u; agent_i < Model<TSeq>::size(); ++agent_i)
    {

        // Checking if the quarantine in the agent was triggered
        // or not
        if (
            agent_quarantine_triggered[agent_i] != 
            ModelSEIRMixingQuarantine<TSeq>::QUARANTINE_PROCESS_ACTIVE
        )
            continue;

        if (this->par("Quarantine period") < 0)
            continue;

        // Getting the number of contacts, if it is greater
        // than the maximum, it means that we overflowed, so 
        // we will only quarantine the first EPI_MAX_TRACKING
        size_t n_contacts = this->tracking_matrix_size[agent_i];
        if (n_contacts >= EPI_MAX_TRACKING)
            n_contacts = EPI_MAX_TRACKING;

        for (size_t contact_i = 0u; contact_i < n_contacts; ++contact_i)
        {

            // Checking if we will detect the contact
            if (Model<TSeq>::runif() > Model<TSeq>::par("Contact tracing success rate"))
                continue;

            size_t contact_id = this->tracking_matrix[
                MM(agent_i, contact_i, Model<TSeq>::size())
            ];

            auto & agent = Model<TSeq>::get_agent(contact_id);

            if (agent.get_state() > INFECTED)
                continue;

            // Agents with some tool won't be quarantined
            if (agent.get_n_tools() != 0u)
                continue;

            if (
                quarantine_willingness[contact_id] &&
                (Model<TSeq>::par("Quarantine period") >= 0))
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
                    case INFECTED:
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
            ModelSEIRMixingQuarantine<TSeq>::QUARANTINE_PROCESS_DONE;
    }

    return;
}

/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model with mixing, quarantine, and contact tracing
 * 
 * @param model A ModelSEIRMixingQuarantine<TSeq> object where to set up the SEIR model.
 * @param vname Name of the virus
 * @param n Number of agents in the population
 * @param prevalence Initial prevalence (proportion of infected individuals)
 * @param contact_rate Average number of contacts (interactions) per step
 * @param transmission_rate Probability of transmission per contact
 * @param avg_incubation_days Average incubation period in days
 * @param recovery_rate Probability of recovery per day
 * @param contact_matrix Contact matrix specifying mixing patterns between population groups
 * @param hospitalization_rate Rate at which infected individuals are hospitalized
 * @param hospitalization_period Average duration of hospitalization in days
 * @param days_undetected Average number of days an infected individual remains undetected
 * @param quarantine_period Duration of quarantine in days for exposed contacts
 * @param quarantine_willingness Proportion of individuals willing to comply with quarantine
 * @param isolation_willingness Proportion of individuals willing to self-isolate when detected
 * @param isolation_period Duration of isolation in days for detected infected individuals
 * @param contact_tracing_success_rate Probability of successfully identifying contacts during tracing
 * @param contact_tracing_days_prior Number of days prior to detection for contact tracing
 */
template<typename TSeq>
inline ModelSEIRMixingQuarantine<TSeq>::ModelSEIRMixingQuarantine(
    ModelSEIRMixingQuarantine<TSeq> & model,
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    std::vector< double > contact_matrix,
    epiworld_double hospitalization_rate,
    epiworld_double hospitalization_period,
    // Policy parameters
    epiworld_double days_undetected,
    epiworld_fast_int quarantine_period,
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    epiworld_fast_int isolation_period,
    epiworld_double contact_tracing_success_rate,
    epiworld_fast_uint contact_tracing_days_prior
    )
{

    // Setting up the contact matrix
    this->contact_matrix = contact_matrix;

    // Setting up parameters
    model.add_param(contact_rate, "Contact rate");
    model.add_param(transmission_rate, "Prob. Transmission");
    model.add_param(recovery_rate, "Prob. Recovery");
    model.add_param(avg_incubation_days, "Avg. Incubation days");
    model.add_param(hospitalization_rate, "Hospitalization rate");
    model.add_param(hospitalization_period, "Hospitalization period");
    model.add_param(days_undetected, "Days undetected");
    model.add_param(quarantine_period, "Quarantine period");
    model.add_param(
        quarantine_willingness, "Quarantine willingness"
    );
    model.add_param(
        isolation_willingness, "Isolation willingness"
    );
    model.add_param(isolation_period, "Isolation period");
    model.add_param(
        contact_tracing_success_rate, "Contact tracing success rate"
    );
    model.add_param(
        contact_tracing_days_prior, "Contact tracing days prior"
    );
    
    // state
    model.add_state("Susceptible", m_update_susceptible);
    model.add_state("Exposed", m_update_exposed);
    model.add_state("Infected", m_update_infected);
    model.add_state("Isolated", m_update_isolated);
    model.add_state("Detected Hospitalized", m_update_hospitalized);
    model.add_state("Quarantined Susceptible", m_update_quarantine_suscep);
    model.add_state("Quarantined Exposed", m_update_quarantine_exposed);
    model.add_state("Isolated Recovered", m_update_isolated_recovered);
    model.add_state("Hospitalized", m_update_hospitalized);
    model.add_state("Recovered");

    // Global function
    model.add_globalevent(this->m_update_model, "Update infected individuals");
    model.queuing_off();

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(
        ModelSEIRMixingQuarantine<TSeq>::EXPOSED,
        ModelSEIRMixingQuarantine<TSeq>::RECOVERED,
        ModelSEIRMixingQuarantine<TSeq>::RECOVERED
        );

    virus.set_prob_infecting(&model("Prob. Transmission"));
    virus.set_prob_recovery(&model("Prob. Recovery"));
    virus.set_incubation(&model("Avg. Incubation days"));

    model.add_virus(virus);

    model.queuing_off(); // No queuing need

    // Adding the empty population
    model.agents_empty_graph(n);

    model.set_name("SEIR with Mixing and Quarantine");

    return;

}

template<typename TSeq>
inline ModelSEIRMixingQuarantine<TSeq>::ModelSEIRMixingQuarantine(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    std::vector< double > contact_matrix,
    epiworld_double hospitalization_rate,
    epiworld_double hospitalization_period,
    // Policy parameters
    epiworld_double days_undetected,
    epiworld_fast_int quarantine_period,
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    epiworld_fast_int isolation_period,
    epiworld_double contact_tracing_success_rate,
    epiworld_fast_uint contact_tracing_days_prior
    )
{   

    this->contact_matrix = contact_matrix;

    ModelSEIRMixingQuarantine(
        *this,
        vname,
        n,
        prevalence,
        contact_rate,
        transmission_rate,
        avg_incubation_days,
        recovery_rate,
        contact_matrix,
        hospitalization_rate,
        hospitalization_period,
        // Policy parameters
        days_undetected,
        quarantine_period,
        quarantine_willingness,
        isolation_willingness,
        isolation_period,
        contact_tracing_success_rate,
        contact_tracing_days_prior
    );

    return;

}

template<typename TSeq>
inline ModelSEIRMixingQuarantine<TSeq> & ModelSEIRMixingQuarantine<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
)
{

    Model<TSeq>::initial_states_fun =
        create_init_function_seir<TSeq>(proportions_)
        ;

    return *this;

}
#undef MM
#undef GET_MODEL
#endif
