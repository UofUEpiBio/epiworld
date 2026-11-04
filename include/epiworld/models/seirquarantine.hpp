#ifndef EPIWORLD_MODELS_SEIRQUARANTINE_HPP
#define EPIWORLD_MODELS_SEIRQUARANTINE_HPP

#include "../model-bones.hpp"

/**
 * @file seirquarantine.hpp
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model
 *        with quarantine, isolation, and contact tracing
 */

/**
 * @brief SEIR model with quarantine, isolation, and contact tracing
 *
 * @details An SEIR model whose contacts come from its layers, of any kind
 * (see `Model::add_layer()`): networks, mixing by group, or both. Features:
 *
 * - Detection of infected agents, who isolate if willing.
 * - Contact tracing from the contact logs of the model's layers: at reset the
 *   model turns tracing on for each layer that does not have it yet (see
 *   `ContactLayer::enable_tracing()`), and a detected agent's contacts in every
 *   log are traced.
 * - Quarantine of the traced contacts who are willing to comply.
 * - Hospitalization of severe cases.
 * - Individual willingness to quarantine and to isolate (drawn once per agent
 *   and run).
 *
 * Each agent has a **status** and a **stage** of the SEIR natural history
 * (see `NaturalHistory`):
 *
 * | Statuses | Stages |
 * |---|---|
 * | Free         | Susceptible |
 * | Quarantined  | Exposed (infected, not infectious) |
 * | Isolated     | Infected (infectious) |
 * | Hospitalized | Recovered |
 *
 * Only agents in the Infected stage transmit. Quarantined, isolated and
 * hospitalized agents are out of the mixing layers altogether
 * (`Model::set_excluded_states()`): they are not met, and their group's
 * contacts go to the others.
 *
 * - A free infected agent may be detected each day (which starts contact
 *   tracing and, if willing, isolates it). Each day, free and isolated
 *   infected agents recover or are hospitalized (one draw: the two compete).
 * - Quarantine lasts "Quarantine period" days from the day the agent was
 *   flagged; a quarantined agent who becomes infected (onset) is isolated,
 *   unless its quarantine is over.
 * - Isolation lasts "Isolation period" days from onset, also for agents who
 *   recover in isolation.
 * - Hospitalized agents are discharged, free and recovered, with daily
 *   probability 1 / "Hospitalization period".
 *
 * **Implementation details:**
 * <a href="../impl/quarantine-isolation-and-contact-tracing.md">Quarantine, Isolation, and Contact Tracing</a>
 *
 * @tparam TSeq Type for genetic sequences (default: EPI_DEFAULT_TSEQ)
 * @ingroup basic_compartmental
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSEIRQuarantine : public Model<TSeq>
{
private:

    // Stages
    static void _update_exposed(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_infected(Agent<TSeq> * p, Model<TSeq> * m);

    // Statuses
    static void _update_quarantined(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_isolated(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_hospitalized(Agent<TSeq> * p, Model<TSeq> * m);

    static void _on_stage_change(
        Agent<TSeq> * p, size_t k, int from, int to, Model<TSeq> * m
    );

    // Data about the quarantine process
    std::vector< bool > quarantine_willingness; ///< Indicator
    std::vector< bool > isolation_willingness; ///< Indicator for isolation willingness
    std::vector< size_t > agent_quarantine_triggered; ///< Whether the quarantine process has started
    std::vector< int > day_flagged; ///< Day the agent was quarantined or isolated

    static void _quarantine_process(Model<TSeq> * m);

public:

    /// @name Statuses (the model's states are statuses × stages)
    ///@{
    static constexpr int FREE         = 0;
    static constexpr int QUARANTINED  = 1;
    static constexpr int ISOLATED     = 2;
    static constexpr int HOSPITALIZED = 3;
    ///@}

    /// @name Stages of the natural history (`get_natural_history(0)`)
    ///@{
    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int EXPOSED     = 1;
    static constexpr int INFECTED    = 2;
    static constexpr int RECOVERED   = 3;
    ///@}

    /// The model's state with this status and stage.
    static constexpr epiworld_fast_uint state(int status, int stage)
    {
        return static_cast< epiworld_fast_uint >(status + 4 * stage);
    }

    static const size_t QUARANTINE_PROCESS_INACTIVE = 0u;
    static const size_t QUARANTINE_PROCESS_ACTIVE   = 1u;
    static const size_t QUARANTINE_PROCESS_DONE     = 2u;

    ModelSEIRQuarantine() = delete;

    /**
     * @brief Constructs a ModelSEIRQuarantine object.
     *
     * @param vname The name of the virus.
     * @param n The number of agents in the model.
     * @param prevalence The initial prevalence of the disease.
     * @param transmission_prob Probability that a contact with an infected
     * agent infects ("Transmission probability").
     * @param latent_period Mean days in Exposed ("Latent period").
     * @param recovery_rate Daily probability of recovering ("Recovery rate").
     * @param hospitalization_rate Daily probability of being hospitalized
     * while infected ("Hospitalization rate").
     * @param hospitalization_period Mean days in hospital ("Hospitalization
     * period").
     * @param detection_delay Mean days infected before detection ("Detection
     * delay"; 0 detects at once, a negative value never).
     * @param quarantine_period Days in quarantine ("Quarantine period"; a
     * negative value turns quarantine off).
     * @param quarantine_willingness Probability that an agent complies with
     * quarantine ("Quarantine willingness").
     * @param isolation_willingness Probability that an agent isolates when
     * detected ("Isolation willingness").
     * @param isolation_period Days in isolation, from onset ("Isolation
     * period"; a negative value turns isolation off).
     * @param contact_tracing_prob Probability that a contact is traced
     * ("Contact tracing probability", default 1).
     * @param contact_tracing_window Days before detection whose contacts are
     * traced ("Contact tracing window", default 4).
     */
    ModelSEIRQuarantine(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double latent_period,
        epiworld_double recovery_rate,
        epiworld_double hospitalization_rate,
        epiworld_double hospitalization_period,
        // Policy parameters
        epiworld_double detection_delay,
        epiworld_fast_int quarantine_period,
        epiworld_double quarantine_willingness,
        epiworld_double isolation_willingness,
        epiworld_fast_int isolation_period,
        epiworld_double contact_tracing_prob = 1.0,
        epiworld_fast_uint contact_tracing_window = 4u
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
     * - [0]: The proportion of the initial cases who start infected (the
     *   rest start exposed).
     * - [1]: The proportion of the other agents who start recovered (immune).
     * @param queue_ Ignored.
     */
    ModelSEIRQuarantine<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    ) override;

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

// -----------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------
template<typename TSeq>
inline void ModelSEIRQuarantine<TSeq>::reset()
{

    // The quarantine process reads the contacts of every layer
    for (size_t i = 0u; i < this->get_n_layers(); ++i)
        if (!this->get_layer(i).is_tracing())
            this->get_layer(i).enable_tracing();

    Model<TSeq>::reset();

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

    agent_quarantine_triggered.assign(this->size(), 0u);
    day_flagged.assign(this->size(), 0);

    return;

}

template<typename TSeq>
inline std::unique_ptr<Model<TSeq>> ModelSEIRQuarantine<TSeq>::clone_ptr()
{
    return std::make_unique<ModelSEIRQuarantine<TSeq>>(*this);
}

// -----------------------------------------------------------------------
// Stages
// -----------------------------------------------------------------------
template<typename TSeq>
inline void ModelSEIRQuarantine<TSeq>::_update_exposed(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // Onset (the day is the natural history's entry into Infected)
    if (m->runif() < 1.0/(p->get_virus()->get_incubation(m)))
        p->change_stage(*m, INFECTED);

};

template<typename TSeq>
inline void ModelSEIRQuarantine<TSeq>::_update_infected(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // In hospital, agents recover at discharge
    const int status = p->get_status(*m);
    if (status == HOSPITALIZED)
        return;

    auto * model = model_cast<ModelSEIRQuarantine<TSeq>, TSeq>(m);

    // Free agents may be detected. A negative detection delay disables
    // detection (never detected); 0 detects at once. Detection starts
    // contact tracing, and isolates the agent if willing.
    bool isolate = false;
    if (status == FREE)
    {

        epiworld_double detection_delay = EPI_PAR(m, "Detection delay");
        bool detected = (detection_delay < 0.0) ?
            false : ((detection_delay == 0.0) ?
                true : (m->runif() < 1.0 / detection_delay));

        if (detected)
            model->agent_quarantine_triggered[p->get_id()] =
                QUARANTINE_PROCESS_ACTIVE;

        isolate = detected && (EPI_PAR(m, "Isolation period") >= 0) &&
            model->isolation_willingness[p->get_id()];

        if (isolate)
            model->day_flagged[p->get_id()] = m->today();

    }

    // Recovery and hospitalization compete
    auto & v = p->get_virus();
    m->array_double_tmp[0] = 1.0 - (1.0 - v->get_prob_recovery(m)) *
        (1.0 - p->get_recovery_enhancer(v, *m));
    m->array_double_tmp[1] = EPI_PAR(m, "Hospitalization rate");

    auto which = m->sample_from_probs(2);
    if (which == 0)
        p->rm_virus(*m);
    else if (which == 1)
    {
        m->record_hospitalization(*p);
        p->change_status(*m, HOSPITALIZED);
        return;
    }

    if (isolate)
        p->change_status(*m, ISOLATED);

};

// -----------------------------------------------------------------------
// Statuses
// -----------------------------------------------------------------------
template<typename TSeq>
inline void ModelSEIRQuarantine<TSeq>::_update_quarantined(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    auto * model = model_cast<ModelSEIRQuarantine<TSeq>, TSeq>(m);
    int days_since = m->today() - model->day_flagged[p->get_id()];
    if (EPI_PAR(m, "Quarantine period") <= days_since)
        p->change_status(*m, FREE);

};

template<typename TSeq>
inline void ModelSEIRQuarantine<TSeq>::_update_isolated(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // Isolation lasts from onset, also for agents who recovered
    int onset = m->get_natural_history().entered(*p, INFECTED);
    if (EPI_PAR(m, "Isolation period") <= (m->today() - onset))
        p->change_status(*m, FREE);

};

template<typename TSeq>
inline void ModelSEIRQuarantine<TSeq>::_update_hospitalized(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // Discharged free; agents still infected recover then
    if (m->runif() < 1.0/EPI_PAR(m, "Hospitalization period"))
    {
        p->change_status(*m, FREE);
        if (p->get_virus() != nullptr)
            p->rm_virus(*m);
    }

};

template<typename TSeq>
inline void ModelSEIRQuarantine<TSeq>::_on_stage_change(
    Agent<TSeq> * p, size_t, int, int to, Model<TSeq> * m
) {

    // A quarantined agent with onset is isolated, unless its quarantine is
    // over
    if ((to != INFECTED) || (p->get_status(*m) != QUARANTINED))
        return;

    auto * model = model_cast<ModelSEIRQuarantine<TSeq>, TSeq>(m);
    int days_since = m->today() - model->day_flagged[p->get_id()];
    if (EPI_PAR(m, "Quarantine period") > days_since)
        p->change_status(*m, ISOLATED);

}

// -----------------------------------------------------------------------
// Quarantine process: trace contacts and quarantine them
// -----------------------------------------------------------------------
template<typename TSeq>
inline void ModelSEIRQuarantine<TSeq>::_quarantine_process(
    Model<TSeq> * m
) {

    auto * model = model_cast<ModelSEIRQuarantine<TSeq>, TSeq>(m);

    for (size_t agent_i = 0u; agent_i < m->size(); ++agent_i)
    {

        if (
            model->agent_quarantine_triggered[agent_i] !=
            QUARANTINE_PROCESS_ACTIVE
        )
            continue;

        if (EPI_PAR(m, "Quarantine period") < 0)
        {
            model->agent_quarantine_triggered[agent_i] = QUARANTINE_PROCESS_DONE;
            continue;
        }

        auto success_rate = EPI_PAR(m, "Contact tracing probability");
        auto days_prior = EPI_PAR(m, "Contact tracing window");

        // The contacts in the log of every traced layer, in the order the
        // layers were added (a log keeps each agent's last contacts only)
        for (size_t layer_i = 0u; layer_i < m->get_n_layers(); ++layer_i)
        {

            auto & layer = m->get_layer(layer_i);
            if (!layer.is_tracing())
                continue;

            auto & ct = layer.get_tracing();
            const size_t n_contacts = std::min(
                ct.get_n_contacts(agent_i), ct.get_max_contacts()
            );

            for (size_t contact_i = 0u; contact_i < n_contacts; ++contact_i)
            {

                // Checking if we will detect the contact
                if (m->runif() > success_rate)
                    continue;

                auto [contact_id, contact_date] = ct.get_contact(
                    agent_i, contact_i
                );

                // Skip contacts outside the tracing window
                if ((static_cast<double>(m->today()) -
                    static_cast<double>(contact_date)) > days_prior)
                    continue;

                // Only free agents who have not recovered
                auto & agent = m->get_agent(contact_id);
                const int stage = agent.get_stage(*m);
                if ((agent.get_status(*m) != FREE) || (stage == RECOVERED))
                    continue;

                // Agents with some tool won't be quarantined
                if (agent.get_n_tools() != 0u)
                    continue;

                if (!model->quarantine_willingness[contact_id])
                    continue;

                // Infected contacts are isolated instead, if willing
                if (stage != INFECTED)
                    agent.change_status(*m, QUARANTINED);
                else if (model->isolation_willingness[contact_id])
                    agent.change_status(*m, ISOLATED);
                else
                    continue;

                model->day_flagged[contact_id] = m->today();

            }

        }

        // Setting the quarantine process off
        model->agent_quarantine_triggered[agent_i] = QUARANTINE_PROCESS_DONE;

    }

    return;

}

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------
template<typename TSeq>
inline ModelSEIRQuarantine<TSeq>::ModelSEIRQuarantine(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double latent_period,
    epiworld_double recovery_rate,
    epiworld_double hospitalization_rate,
    epiworld_double hospitalization_period,
    // Policy parameters
    epiworld_double detection_delay,
    epiworld_fast_int quarantine_period,
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    epiworld_fast_int isolation_period,
    epiworld_double contact_tracing_prob,
    epiworld_fast_uint contact_tracing_window
    )
{

    this->set_population_size(n);

    // Setting up parameters
    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(recovery_rate, "Recovery rate");
    this->add_param(latent_period, "Latent period");
    this->add_param(hospitalization_rate, "Hospitalization rate");
    this->add_param(hospitalization_period, "Hospitalization period");
    this->add_param(detection_delay, "Detection delay");
    this->add_param(quarantine_period, "Quarantine period");
    this->add_param(
        quarantine_willingness, "Quarantine willingness"
    );
    this->add_param(
        isolation_willingness, "Isolation willingness"
    );
    this->add_param(isolation_period, "Isolation period");
    this->add_param(contact_tracing_prob, "Contact tracing probability");
    this->add_param(contact_tracing_window, "Contact tracing window");

    // Statuses (free agents have no update: their stage's is enough)
    this->add_state("Free");
    this->add_state("Quarantined", _update_quarantined);
    this->add_state("Isolated", _update_isolated);
    this->add_state("Hospitalized", _update_hospitalized);

    // The natural history; only infected agents transmit
    NaturalHistory<TSeq> nh("SEIR");
    nh.add_stage("Susceptible", default_update_susceptible<TSeq>);
    nh.add_stage("Exposed", _update_exposed);
    nh.add_stage("Infected", _update_infected);
    nh.add_stage("Recovered");
    nh.set_infection(EXPOSED, RECOVERED);
    nh.set_infectious(INFECTED);
    this->add_natural_history(nh);
    this->set_stage_hook(_on_stage_change);

    // Quarantined, isolated and hospitalized agents are out of the mixing
    this->set_excluded_states({QUARANTINED, ISOLATED, HOSPITALIZED});

    // Global function (quarantine process runs before state updates)
    this->add_globalevent(_quarantine_process, "Quarantine process");

    // Preparing the virus -------------------------------------------
    Virus<TSeq> virus(vname, prevalence, true);
    virus.set_natural_history(0);
    virus.set_prob_infecting("Transmission probability");
    virus.set_prob_recovery("Recovery rate");
    virus.set_incubation("Latent period");

    this->add_virus(virus);

    this->set_name("SEIR with Quarantine");

    return;

}

template<typename TSeq>
inline ModelSEIRQuarantine<TSeq> & ModelSEIRQuarantine<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
)
{

    Model<TSeq>::initial_states_fun =
        create_init_function_stages<TSeq>(proportions_, INFECTED, RECOVERED);

    return *this;

}

#endif
