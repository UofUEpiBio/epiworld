
#ifndef EPIWORLD_MODELS_MEASLESMIXINGRISKQUARANTINE_HPP
#define EPIWORLD_MODELS_MEASLESMIXINGRISKQUARANTINE_HPP

#include "samplermixing.hpp"
#include "stages.hpp"

using namespace epiworld;

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
 * - **High Risk**: Unvaccinated agents in the same group as the case
 * - **Medium Risk**: Unvaccinated agents who contacted an infected individual but are not in its group
 * - **Low Risk**: Other unvaccinated agents
 * 
 * Each risk level can have different quarantine durations, allowing for targeted 
 * public health interventions. The model also includes enhanced detection during
 * active quarantine periods.
 * 
 * Disease progression follows the same states as ModelMeaslesMixing:
 * Susceptible → Latent → Prodromal → Rash → Recovered
 *
 * The latent, prodromal and rash stages last a number of days drawn from a
 * lognormal distribution when the agent enters them (`measles_stage_days()`),
 * whatever the agent's status.
 * 
 * The contacts come from the model's mixing layer, which the user adds, as for
 * ModelMeaslesMixing: a `layer::Mixing` with the groups and a contact matrix.
 * The model samples from it with its own sampler and logs the contacts in it.
 *
 * ![Model Diagram](../assets/img/measlesmixingriskquarantine.png)
 * 
 * @tparam TSeq Type for genetic sequences (default: EPI_DEFAULT_TSEQ)
 * @ingroup disease_specific
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelMeaslesMixingRiskQuarantine final : public Model<TSeq>
{
private:
    // The model's mixing layer and its contact rate (_update_infectious_list())
    layer::Mixing<TSeq> * mixing = nullptr;
    double contact_rate = 1.0;

    // Vector of vectors of infected agents (prodromal agents are infectious)
    std::vector< size_t > infectious;

    // Number of infectious agents in each group
    std::vector< size_t > n_infectious_per_group;
    
    // Where the groups start in the `infectious` vector
    std::vector< size_t > group_start;

    double m_get_risk_period(size_t agent_id);

    // Update the list of infectious agents
    void _update_infectious_list();

    // Vector to hold sampled agents temporarily
    std::vector< size_t > sampled_agents;

    /**
     * @brief Sample infected agents based on contact matrix
     * @param agent Pointer to the agent for whom contacts are being sampled.
     * @param sampled_agents Vector to store the IDs of sampled agents.
     * @return The number of sampled agents.
     */
    size_t sample_infectious_agents(
        Agent<TSeq> * agent,
        std::vector< size_t > & sampled_agents
        );

    std::vector< double > adjusted_contact_rate;

    #ifdef EPI_DEBUG
    std::vector< int > sampled_sizes;
    #endif

    // Update functions
    // Statuses
    static void _update_free(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_quarantined(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_isolated(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_hospitalized(Agent<TSeq> * p, Model<TSeq> * m);

    // Stages (the others last their drawn duration; the statuses decide
    // what happens to agents with rash)
    static void _update_susceptible(Agent<TSeq> * p, Model<TSeq> * m);

    static void _on_stage_change(
        Agent<TSeq> * p, size_t k, int from, int to, Model<TSeq> * m
    );

    /// Hospitalizes the agent with the daily probability (agents with rash)
    static bool _hospitalize(Agent<TSeq> * p, Model<TSeq> * m);

    // Data about the quarantine process
    std::vector< bool > quarantine_willingness; ///< Indicator for quarantine willingness
    std::vector< bool > isolation_willingness;  ///< Indicator for isolation willingness
    std::vector< int > day_flagged;             ///< Either detected or started quarantine

    std::vector< size_t > agents_triggered_contact_tracing; ///< List of agents that triggered contact tracing
    size_t agents_triggered_contact_tracing_size = 0u;      ///< Number of agents that triggered contact tracing
    std::vector< bool > agents_checked_contact_tracing;     ///< if an agent has been checked for contact tracing

    // Add an agent to the list of those that triggered contact tracing
    void m_add_contact_tracing(size_t agent_id);
    
    std::vector< int > quarantine_risk_level; ///< Risk level assigned to each agent (0=low, 1=medium, 2=high)
    
    static void _quarantine_process(Model<TSeq> * m); ///< Overall quarantine process

    std::vector< size_t > days_quarantine_triggered; ///< Days when quarantine was triggered

public:

    /// @name Statuses
    ///@{
    static constexpr int FREE         = 0;
    static constexpr int QUARANTINED  = 1;
    static constexpr int ISOLATED     = 2;
    static constexpr int HOSPITALIZED = 3;
    ///@}

    /// @name Stages of the natural history (`get_natural_history(0)`)
    ///@{
    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int LATENT      = 1;
    static constexpr int PRODROMAL   = 2;
    static constexpr int RASH        = 3;
    static constexpr int RECOVERED   = 4;
    ///@}

    /// The model's state with this status and stage.
    static constexpr epiworld_fast_uint state(int status, int stage)
    {
        return static_cast< epiworld_fast_uint >(status + 4 * stage);
    }

    static constexpr size_t QUARANTINE_PROCESS_INACTIVE = 0u;
    static constexpr size_t QUARANTINE_PROCESS_ACTIVE   = 1u;
    static constexpr size_t QUARANTINE_PROCESS_DONE     = 2u;

    // Risk levels for quarantine
    static constexpr int RISK_LOW    = 0;
    static constexpr int RISK_MEDIUM = 1;
    static constexpr int RISK_HIGH   = 2;

    ModelMeaslesMixingRiskQuarantine() = delete;

    /**
     * @brief Constructs a ModelMeaslesMixingRiskQuarantine object.
     *
     * @param n The number of agents in the model.
     * @param prevalence The initial prevalence of the disease in the model.
     * @param transmission_rate The transmission rate of the disease in the model.
     * @param vax_efficacy The efficacy of the vaccine.
     * @param incubation_period The incubation period of the disease in the model.
     * @param prodromal_period The prodromal period of the disease in the model.
     * @param rash_period The rash period of the disease in the model.
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
     * @param contact_tracing_days_window The number of days prior or after
     * rash onset for which contacts are traced (default: 4).
     * @param incubation_period_sd Standard deviation of the incubation
     * period in days (default: 2.5).
     * @param prodromal_period_sd Standard deviation of the prodromal period
     * in days (default: 1.7).
     * @param rash_period_sd Standard deviation of the rash period in days
     * (default: 1.0).
     */
    ModelMeaslesMixingRiskQuarantine(
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double vax_efficacy,
        epiworld_double incubation_period,
        epiworld_double prodromal_period,
        epiworld_double rash_period,
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
        epiworld_fast_uint contact_tracing_days_window = 4u,
        epiworld_double incubation_period_sd = 2.5,
        epiworld_double prodromal_period_sd = 1.7,
        epiworld_double rash_period_sd = 1.0
    );

    /**
     * @brief Reset the model to initial state
     */
    void reset() override;

    /**
     * @brief Create a clone of this model
     * @return Pointer to a new model instance with the same configuration
     */
    std::unique_ptr< Model<TSeq> > clone_ptr() override;

    /**
     * @brief Set the initial states of the model
     * @param proportions_ Double vector with two elements:
     * - [0]: The proportion of initially infected individuals who start in the latent state.
     * - [1]: The proportion of initially non-infected individuals who have recovered (immune).
     * @param queue_ Optional vector for queuing specifications (default: empty).
     */
    ModelMeaslesMixingRiskQuarantine<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    ) override;

    /**
     * @brief Get the quarantine willingness for all agents
     * @return Vector of boolean values indicating each agent's willingness to quarantine
     */
    auto get_quarantine_willingness() const
    {
        return quarantine_willingness;
    };

    /**
     * @brief Get the isolation willingness for all agents
     * @return Vector of boolean values indicating each agent's willingness to self-isolate
     */
    auto get_isolation_willingness() const
    {
        return isolation_willingness;
    };

    /**
     * @brief Get the risk level assigned to each agent for quarantine purposes
     * @return Vector of integers representing risk levels (0=low, 1=medium, 2=high)
     */
    const auto & get_quarantine_risk_level() const
    {
        return quarantine_risk_level;
    };


    /**
     * @brief Get the total number of quarantines that have occurred
     * @return The number of quarantines
     */
    auto get_days_quarantine_triggered() const
    {
        return days_quarantine_triggered;
    };

    void next() override;

};

// Implementation starts here

template<typename TSeq>
inline double ModelMeaslesMixingRiskQuarantine<TSeq>::m_get_risk_period(size_t agent_id)
{

    double risk_period = 0.0;

    // Getting the risk level
    int risk_level = quarantine_risk_level[agent_id];

    if (risk_level == RISK_HIGH)
        risk_period = EPI_PAR(this, "Quarantine period high");
    else if (risk_level == RISK_MEDIUM)
        risk_period = EPI_PAR(this, "Quarantine period medium");
    else
        risk_period = EPI_PAR(this, "Quarantine period low");

    return risk_period;

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::_update_infectious_list()
{

    // The groups (which may have changed) and the contact rate
    mixing = &measles_mixing_layer(*this);
    contact_rate = mixing->get_contact_rate(*this);

    const size_t ngroups = mixing->get_n_groups();
    constexpr size_t none = layer::Mixing<TSeq>::NO_GROUP;

    // Resetting the infectious list (no infected agents)
    n_infectious_per_group.assign(ngroups, 0u);

    // Resetting the number of available contacts
    adjusted_contact_rate.assign(ngroups, 0.0);

    // A group takes as many slots in `infectious` as it has members
    group_start.assign(ngroups, 0u);
    for (size_t g = 1u; g < ngroups; ++g)
        group_start[g] = group_start[g - 1] + mixing->members_of(g - 1).size();

    // Looping over agents to find the infectious ones
    for (const auto & a : this->get_agents())
    {

        const size_t g = mixing->get_group(a.get_id());
        if (g == none)
            continue;

        // Free prodromal agents are infectious; free agents without rash
        // are in contact
        if (a.get_status(*this) != FREE)
            continue;
        const int stage = a.get_stage(*this);

        if (stage == PRODROMAL)
            infectious[group_start[g] + n_infectious_per_group[g]++] =
                a.get_id();

        // Setting how many agents are available for contact
        if (stage != RASH)
            adjusted_contact_rate[g] += 1.0;

    }

    // This simplifies calculations later
    for (auto & rate: adjusted_contact_rate)
    {
        if (rate > 0.0)
            rate = 1.0 / rate;
        else
            rate = 0.0;  // No available contacts in this group

        if (rate > 1.0)
            rate = 1.0;
    }

    return;

}

template<typename TSeq>
inline size_t ModelMeaslesMixingRiskQuarantine<TSeq>::sample_infectious_agents(
    Agent<TSeq> * agent,
    std::vector< size_t > & sampled_agents
    )
{

    // An agent in no group meets nobody
    const size_t agent_group_id = mixing->get_group(agent->get_id());
    if (agent_group_id == layer::Mixing<TSeq>::NO_GROUP)
        return 0u;

    const size_t ngroups = mixing->get_n_groups();
    const auto & contact_matrix = mixing->get_contact_matrix();

    int samp_id = 0;
    for (size_t g = 0; g < ngroups; ++g)
    {

        size_t group_size = n_infectious_per_group[g];

        if (group_size == 0u)
            continue;

        // How many from this group? (contacts per day of the agent's group
        // with group g; column-major) The probability is capped at 1, since
        // the contacts can outnumber the agents available for contact
        int nsamples = this->rbinom(
            group_size,
            std::min(
                1.0,
                adjusted_contact_rate[g] * contact_rate *
                    contact_matrix[g * ngroups + agent_group_id]
            )
        );

        if (nsamples == 0)
            continue;

        // Sampling from the group
        for (int s = 0; s < nsamples; ++s)
        {

            // Randomly selecting an agent
            int which = this->runif() * group_size;

            // Correcting overflow error
            if (which >= static_cast<int>(group_size))
                which = static_cast<int>(group_size) - 1;

            #ifdef EPI_DEBUG
            auto & a = this->population.at(
                infectious.at(group_start[g] + which)
            );
            #else
            const auto & a = this->get_agent(
                infectious[group_start[g] + which]
            );
            #endif

            #ifdef EPI_DEBUG
            if (a.get_stage(*this) != PRODROMAL)
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

    // The contacts go to the layer's log, and to the post-sampling callback
    mixing->trace(sampled_agents.data(), samp_id, agent->get_id());
    if (this->has_post_sampling())
        this->register_sampled_contacts(
            sampled_agents.data(), samp_id, agent->get_id()
        );
    
    return samp_id;

}

template<typename TSeq>
inline bool ModelMeaslesMixingRiskQuarantine<TSeq>::_hospitalize(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    if (m->runif() >= EPI_PAR(m, "Hospitalization rate"))
        return false;

    m->record_hospitalization(*p);
    p->change_status(*m, HOSPITALIZED);
    return true;

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::_update_susceptible(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // Only free agents are in contact
    if (p->get_status(*m) != FREE)
        return;

    auto * m_down = model_cast<ModelMeaslesMixingRiskQuarantine<TSeq>, TSeq>(m);

    size_t ndraws = m_down->sample_infectious_agents(p, m_down->sampled_agents);

    #ifdef EPI_DEBUG
    m_down->sampled_sizes.push_back(static_cast<int>(ndraws));
    #endif

    if (ndraws == 0u)
        return;

    Virus<TSeq> * virus = m->sample_virus(
        p, m_down->sampled_agents.data(), ndraws
    );
    if (virus != nullptr)
        p->set_virus(*m, *virus);

    return;

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::_update_free(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    if (p->get_stage(*m) != RASH)
        return;

    auto* model = model_cast<ModelMeaslesMixingRiskQuarantine<TSeq>,TSeq>(m);

    // Detection starts contact tracing and isolates the agent, unless it is
    // hospitalized
    bool detected = false;
    if (
        (EPI_PAR(m, "Isolation period") >= 0) &&
        (m->runif() < 1.0/EPI_PAR(m, "Days undetected"))
    )
    {
        model->m_add_contact_tracing(p->get_id());
        detected = true;
        model->day_flagged[p->get_id()] = m->today();
    }

    // Each day with rash, the agent may be hospitalized; otherwise, it
    // recovers at the end of the rash period (in isolation, if detected)
    if (_hospitalize(p, m))
        return;

    if (m->get_natural_history().due(*p, *m))
        p->rm_virus(*m);

    if (detected)
        p->change_status(*m, ISOLATED);

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::_update_quarantined(
    Agent<TSeq> * p,
    Model<TSeq> * m
) {

    // Quarantine lasts the period of the agent's risk level
    auto* model = model_cast<ModelMeaslesMixingRiskQuarantine<TSeq>,TSeq>(m);
    auto quarantine_period = model->m_get_risk_period(p->get_id());
    int days_since = m->today() - model->day_flagged[p->get_id()];
    if (quarantine_period <= days_since)
        p->change_status(*m, FREE);

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::_update_isolated(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // With rash, the agent may be hospitalized, or recover at the end of the
    // rash period. Isolation lasts from rash onset, also for agents who
    // recovered.
    const auto & nh = m->get_natural_history();
    if (p->get_stage(*m) == RASH)
    {
        if (_hospitalize(p, m))
            return;

        if (nh.due(*p, *m))
            p->rm_virus(*m);
    }

    int days_since = m->today() - nh.entered(*p, RASH);
    if (EPI_PAR(m, "Isolation period") <= days_since)
        p->change_status(*m, FREE);

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::_update_hospitalized(
    Agent<TSeq> * p,
    Model<TSeq> * m
) {

    // Discharged free; agents still with rash recover then
    if (m->runif() < 1.0/EPI_PAR(m, "Hospitalization period"))
    {
        p->change_status(*m, FREE);
        if (p->get_stage(*m) == RASH)
            p->rm_virus(*m);
    }

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::_on_stage_change(
    Agent<TSeq> * p, size_t, int, int to, Model<TSeq> * m
) {

    if (to != RASH)
        return;

    // At rash onset, a quarantined agent is isolated and its contacts
    // traced. Once quarantine has been triggered, a free agent is detected
    // (and so isolated and traced) with probability "Detection rate
    // quarantine".
    auto* model = model_cast<ModelMeaslesMixingRiskQuarantine<TSeq>,TSeq>(m);
    const int status = p->get_status(*m);
    bool isolate = (status == QUARANTINED);
    if (status == FREE)
        isolate = (model->get_days_quarantine_triggered().size() > 0u) &&
            (m->runif() < EPI_PAR(m, "Detection rate quarantine"));

    if (isolate)
    {
        model->m_add_contact_tracing(p->get_id());
        p->change_status(*m, ISOLATED);
    }

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::m_add_contact_tracing(size_t agent_id) {

    this->agents_triggered_contact_tracing[
        this->agents_triggered_contact_tracing_size++
    ] = agent_id;

    return;

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::_quarantine_process(Model<TSeq> * m) {

    auto* model = model_cast<ModelMeaslesMixingRiskQuarantine<TSeq>,TSeq>(m);

    // If no agents triggered contact tracing, then we skip
    if (model->agents_triggered_contact_tracing_size == 0u)
        return;

    // We increase the number of quarantines
    model->days_quarantine_triggered.push_back(m->today());

    // This vector indicates whether an agent can be quarantined
    // (i.e., is not already in quarantine or isolation)
    std::vector< bool > can_quarantine(
        m->size(), false
    );

    // Assigning all individuals who are not currently in quarantine/isolation
    // a low level
    for (auto & agent: m->get_agents())
    {

                // Free agents who have not recovered
        if (
            (agent.get_status(*m) == FREE) &&
            (agent.get_stage(*m) != RECOVERED)
        )
        {
            model->quarantine_risk_level[agent.get_id()] = RISK_LOW;
            can_quarantine[agent.get_id()] = true;
        }

    }

    #ifdef EPI_DEBUG
    std::map< std::pair<size_t, size_t>, bool> success_contact;
    #endif

    // Checking the risk levels
    double param_days_prior = EPI_PAR(m, "Contact tracing days window");
    for (size_t i = 0u; i < model->agents_triggered_contact_tracing_size; ++i)
    {

        size_t agent_i_idx = model->agents_triggered_contact_tracing[i];
        auto & agent_i = m->get_agent(agent_i_idx);

        // (A) Checking the group first
        // It doesn't matter if the agent doesn't have a tool; we will
        // check that later
        const size_t group_i = model->mixing->get_group(agent_i_idx);
        if (group_i != layer::Mixing<TSeq>::NO_GROUP)
        {
            for (size_t agent_j_idx: model->mixing->members_of(group_i))
            {

                // Only if not already checked we set to high risk
                if (can_quarantine[agent_j_idx])
                    model->quarantine_risk_level[agent_j_idx] = RISK_HIGH;

            }
        }

        // (B) Checking the contacts
        auto & ct = model->mixing->get_tracing();
        size_t n_contacts = ct.get_n_contacts(agent_i_idx);
        if (n_contacts > ct.get_max_contacts())
            n_contacts = ct.get_max_contacts();

        for (size_t contact_j_idx = 0u; contact_j_idx < n_contacts; ++contact_j_idx)
        {

            // Getting the location in the matrix
            auto [contact_id, contact_date] = ct.get_contact(
                agent_i_idx, contact_j_idx
            );

            // Checked agents should not be considered
            if (!can_quarantine[contact_id])
                continue;

            // Will we detect the contact?
            if (m->runif() > EPI_PAR(m, "Contact tracing success rate"))
                continue;

            // How many days since they contacted relative to the rash onset?
            // (recall that, in this model, the rash period is not infectious)
            double days_since_contact =
                static_cast<double>(
                    m->get_natural_history().entered(agent_i, RASH)
                ) -
                static_cast<double>(contact_date);

            // If the contact is outside of the tracing window, we skip it
            if (std::abs(days_since_contact) > param_days_prior)
                continue;

            #ifdef EPI_DEBUG
            auto pair = std::make_pair(agent_i_idx, contact_id);
            success_contact[pair] = true;
            #endif

            // Setting the risk level to medium if not already high
            if (model->quarantine_risk_level[contact_id] < RISK_HIGH)
                model->quarantine_risk_level[contact_id] = RISK_MEDIUM;
            
        }

    }

    // Proceeding to quarantine/isolating agents
    for (auto & agent: m->get_agents())
    {

        // Only if not already checked
        if (!can_quarantine[agent.get_id()])
            continue;

        // If has a tool, then skip (is vaxxed)
        if (agent.get_n_tools() != 0u)
            continue;

        if (model->quarantine_willingness[agent.get_id()] == false)
            continue;

        // Setting the day flagged to today
        model->day_flagged[agent.get_id()] = m->today();

                // Agents with rash are isolated instead, if willing
        if (agent.get_stage(*m) != RASH)
            agent.change_status(*m, QUARANTINED);
        else if (model->isolation_willingness[agent.get_id()])
            agent.change_status(*m, ISOLATED);
    }

    #ifdef EPI_DEBUG
    // Agents in groups that triggered the quarantine level, who are not
    // quarantined should be in RISK_HIGH
    std::set< size_t > groups_ids;
    std::set< size_t > contacted_agents;
    for (size_t i = 0u; i < model->agents_triggered_contact_tracing_size; ++i)
    {

        auto agent_i_idx = model->agents_triggered_contact_tracing[i];

        // We also check who are the contacted agents
        auto & ct = model->mixing->get_tracing();
        size_t n_contacts = ct.get_n_contacts(agent_i_idx);
        if (n_contacts > ct.get_max_contacts())
            n_contacts = ct.get_max_contacts();

        for (size_t contact_j_idx = 0u; contact_j_idx < n_contacts; ++contact_j_idx)
        {

            // Getting the location in the matrix
            auto [contact_id, contact_date] = ct.get_contact(
                agent_i_idx, contact_j_idx
            );

            // Was the contact successful?
            auto pair = std::make_pair(agent_i_idx, contact_id);
            if (!success_contact[pair])
                continue;

            contacted_agents.insert(contact_id);
        }

        if (model->mixing->get_group(agent_i_idx) == layer::Mixing<TSeq>::NO_GROUP)
            continue;

        groups_ids.insert(model->mixing->get_group(agent_i_idx));
        
    }

    // Iterating over the agents in those groups, if they in the states
    // SUSCEPTIBLE, LATENT, PRODROMAL, or RASH, they should be in
    // RISK_HIGH
    for (auto & group: groups_ids)
    {
        for (auto & agent_i_idx: model->mixing->members_of(group))
        {

            auto & agent_i = m->get_agent(agent_i_idx);
                        // Only free agents who have not recovered
            if (
                (agent_i.get_status(*m) != FREE) ||
                (agent_i.get_stage(*m) == RECOVERED)
            )
                continue;

            // If has a tool, then skip (is vaxxed)
            if (agent_i.get_n_tools() != 0u)
                continue;

            if (model->quarantine_risk_level[agent_i_idx] != RISK_HIGH)
                throw std::logic_error(
                    "An agent in a group that triggered contact tracing is not in RISK_HIGH."
                );

        }
    }

    // Now, agents who were not members of the groups that triggered
    // contact tracing, but were contacted by them should be at least
    // in RISK_MEDIUM
    for (auto & agent_i_idx: contacted_agents)
    {
        auto & agent_i = m->get_agent(agent_i_idx);

        // If has a tool, then skip (is vaxxed)
        if (agent_i.get_n_tools() != 0u)
            continue;

        // Checking the agent is not in a group that triggered
        // contact tracing
        if (groups_ids.count(model->mixing->get_group(agent_i_idx)) != 0u)
            continue;

        if (model->quarantine_risk_level[agent_i_idx] != RISK_MEDIUM)
            throw std::logic_error(
                "An agent who was contacted by an infectious agent is not in at least RISK_MEDIUM."
            );
    }

    // Tabulating the number of agents per group
    std::vector< size_t > n_agents_per_group(3u, 0u);
    for (auto i: model->quarantine_risk_level)
        n_agents_per_group[i]++;

    #endif
    
    model->agents_triggered_contact_tracing_size = 0u; 

    return;
}

template<typename TSeq>
inline ModelMeaslesMixingRiskQuarantine<TSeq>::ModelMeaslesMixingRiskQuarantine(
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double vax_efficacy,
    epiworld_double incubation_period,
    epiworld_double prodromal_period,
    epiworld_double rash_period,
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
    epiworld_fast_uint contact_tracing_days_window,
    epiworld_double incubation_period_sd,
    epiworld_double prodromal_period_sd,
    epiworld_double rash_period_sd
    )
{

    measles_check_stage_sds(
        incubation_period_sd, prodromal_period_sd, rash_period_sd,
        "ModelMeaslesMixingRiskQuarantine"
    );

    // Setting up parameters
    this->add_param(transmission_rate, "Transmission rate");
    this->add_param(incubation_period, "Incubation period");
    this->add_param(prodromal_period, "Prodromal period");
    this->add_param(rash_period, "Rash period");
    this->add_param(hospitalization_rate, "Hospitalization rate");
    this->add_param(hospitalization_period, "Hospitalization period");
    this->add_param(days_undetected, "Days undetected");
    this->add_param(quarantine_period_high, "Quarantine period high");
    this->add_param(quarantine_period_medium, "Quarantine period medium");
    this->add_param(quarantine_period_low, "Quarantine period low");
    this->add_param(
        quarantine_willingness, "Quarantine willingness"
    );
    this->add_param(
        isolation_willingness, "Isolation willingness"
    );
    this->add_param(isolation_period, "Isolation period");
    this->add_param(detection_rate_quarantine, "Detection rate quarantine");
    this->add_param(
        contact_tracing_success_rate, "Contact tracing success rate"
    );
    this->add_param(
        contact_tracing_days_window, "Contact tracing days window"
    );
    this->add_param(prop_vaccinated, "Vaccination rate");
    this->add_param(vax_efficacy, "Vax efficacy");
    this->add_param(incubation_period_sd, "Incubation period sd");
    this->add_param(prodromal_period_sd, "Prodromal period sd");
    this->add_param(rash_period_sd, "Rash period sd");
    
    // state
    // Statuses
    this->add_state("Free", _update_free);
    this->add_state("Quarantined", _update_quarantined);
    this->add_state("Isolated", _update_isolated);
    this->add_state("Hospitalized", _update_hospitalized);

    // The natural history
    NaturalHistory<TSeq> nh("Measles");
    nh.add_stage("Susceptible", _update_susceptible);
    nh.add_stage("Latent");
    nh.add_stage("Prodromal");
    nh.add_stage("Rash");
    nh.add_stage("Recovered");
    nh.set_duration(
        LATENT, measles_stage_days<TSeq>("Incubation period", "Incubation period sd"),
        PRODROMAL
    );
    nh.set_duration(
        PRODROMAL, measles_stage_days<TSeq>("Prodromal period", "Prodromal period sd"),
        RASH
    );
    nh.set_duration(
        RASH, measles_stage_days<TSeq>("Rash period", "Rash period sd")
    );
    nh.set_infection(LATENT, RECOVERED);
    nh.set_infectious(PRODROMAL);
    this->add_natural_history(nh);
    this->set_stage_hook(_on_stage_change);

    // Global function
    this->add_globalevent(
        this->_quarantine_process,
        "Update infected individuals"
    );

    this->queuing_off();

    // Preparing the virus -------------------------------------------
    Virus<TSeq> virus("Measles", prevalence, true);
        virus.set_natural_history(0);
    virus.set_prob_infecting("Transmission rate");

    this->add_virus(virus);

    // Designing the vaccine
    ToolVaccine<TSeq> vaccine("MMR");
    
    vaccine.set_susceptibility_reduction(this->get_param("Vax efficacy"));

    vaccine.set_distribution(
        distribute_tool_randomly(prop_vaccinated, true)
    );

    this->add_tool(vaccine);

    // Adding the empty population
    this->set_population_size(n);

    this->set_name("Measles with Mixing and Risk-based Quarantine");

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::reset()
{

    Model<TSeq>::reset();

    // The quarantine process reads the contacts in the layer's log
    auto & mix = measles_mixing_layer(*this);
    if (!mix.is_tracing())
        mix.enable_tracing();

    sampled_agents.resize(this->size());

    // At most one slot per agent
    infectious.assign(this->size(), 0u);

    this->_update_infectious_list();

    // Setting up the quarantine parameters
    quarantine_willingness.resize(this->size(), false);
    isolation_willingness.resize(this->size(), false);
    for (size_t idx = 0; idx < quarantine_willingness.size(); ++idx)
    {
        quarantine_willingness[idx] =
            this->runif() < EPI_PAR(this, "Quarantine willingness");
        isolation_willingness[idx] =
            this->runif() < EPI_PAR(this, "Isolation willingness");
    }

    day_flagged.assign(this->size(), 0);
    

    // Variables about contact tracing
    agents_triggered_contact_tracing.assign(this->size(), 0u);
    agents_triggered_contact_tracing_size = 0u;
    agents_checked_contact_tracing.assign(this->size(), false);

    quarantine_risk_level.assign(this->size(), RISK_LOW);

    // Resetting the number of quarantines
    days_quarantine_triggered.clear();

    

    return;

}

template<typename TSeq>
inline std::unique_ptr<Model<TSeq>> ModelMeaslesMixingRiskQuarantine<TSeq>::clone_ptr()
{
    
    return std::make_unique<ModelMeaslesMixingRiskQuarantine<TSeq>>(*this);

}

template<typename TSeq>
inline ModelMeaslesMixingRiskQuarantine<TSeq> & ModelMeaslesMixingRiskQuarantine<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
)
{

        Model<TSeq>::initial_states_fun =
        epimodels::create_init_function_stages<TSeq>(
            proportions_, PRODROMAL, RECOVERED
        );

    return *this;

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::next()
{
    this->_update_infectious_list();
    Model<TSeq>::next();
}

#endif
