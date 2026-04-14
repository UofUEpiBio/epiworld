#ifndef EPIWORLD_INTERVENTIONMEASLESQUARANTINE_BONES_HPP
#define EPIWORLD_INTERVENTIONMEASLESQUARANTINE_BONES_HPP

#include "../config.hpp"
#include "../model-bones.hpp"
#include "../agent-bones.hpp"
#include "../contacttracing-bones.hpp"

/**
 * @brief Global event implementing a generalized quarantine and isolation
 *        process driven by contact tracing.
 *
 * @details
 * This intervention runs at the end of each simulation day as a
 * @ref GlobalEvent.  It supports two complementary triggering modes that
 * the host model can activate independently:
 *
 * **Per-agent (contact-tracing) mode** – When a model's state-update
 * function detects an agent (e.g.\ moves it to the RASH state), it
 * calls @ref trigger_quarantine.  On the next invocation of
 * @c operator(), the intervention iterates over that agent's recorded
 * contacts (stored in its own @ref ContactTracing object), checks the
 * contact-tracing success rate and the days-prior window, and moves
 * eligible contacts to the appropriate quarantine / isolation state.
 *
 * **System-wide mode** – The model calls @ref trigger_system_quarantine,
 * which causes @c operator() to scan *every* agent for quarantine
 * eligibility (useful for school-wide or facility-wide responses).
 *
 * In both modes the same eligibility rules apply:
 *
 *  - Agents whose current state exceeds @c _max_base_state are already
 *    quarantined / isolated and are skipped.
 *  - Agents possessing any tool (e.g.\ a vaccine) are skipped.
 *  - Quarantine willingness and isolation willingness (per-agent
 *    Bernoulli draws made during @ref init) gate the state change.
 *  - The target state is determined by the mapping
 *    @c _quarantinable_states → @c _quarantine_targets (for quarantine)
 *    or @c _isolatable_state → @c _isolation_state (for isolation).
 *
 * The intervention also maintains per-agent *exposure dates* via
 * @ref set_exposure_date / @ref get_exposure_date, which other
 * interventions (e.g.\ post-exposure prophylaxis) can query.
 *
 * @tparam TSeq Sequence type forwarded from the model.
 *
 * @ingroup model_utilities
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class InterventionMeaslesQuarantine final : public GlobalEvent<TSeq> {

private:

    // ------------------------------------------------------------------ //
    // State mappings                                                      //
    // ------------------------------------------------------------------ //

    /**
     * @brief States eligible for quarantine (e.g.\ SUSCEPTIBLE, LATENT,
     *        PRODROMAL).
     */
    std::vector<int> _quarantinable_states;

    /**
     * @brief Corresponding quarantine-target states (same length as
     *        @ref _quarantinable_states).
     */
    std::vector<int> _quarantine_targets;

    /**
     * @brief The infectious state eligible for *isolation* (e.g.\ RASH or
     *        INFECTED).
     */
    int _isolatable_state;

    /** @brief Target state for isolated agents (e.g.\ ISOLATED). */
    int _isolation_state;

    /**
     * @brief Agents in states > @c _max_base_state are considered already
     *        processed (quarantined, isolated, hospitalised, …) and are
     *        skipped during the quarantine sweep.
     */
    int _max_base_state;

    // ------------------------------------------------------------------ //
    // Parameter values (registered with the model in _setup)              //
    // ------------------------------------------------------------------ //

    epiworld_double _quarantine_willingness;
    epiworld_double _isolation_willingness;

    static inline const std::string _par_quarantine_willingness{
        "Quarantine willingness"};
    static inline const std::string _par_isolation_willingness{
        "Isolation willingness"};

    // ------------------------------------------------------------------ //
    // Internal (mutable) data                                             //
    // ------------------------------------------------------------------ //

    ContactTracing _contact_tracing;

    std::vector<bool> _quarantine_willing;   ///< Per-agent quarantine draw
    std::vector<bool> _isolation_willing;    ///< Per-agent isolation draw

    /**
     * @brief Per-agent trigger status.
     *
     * Values use the public constants @ref QUARANTINE_INACTIVE,
     * @ref QUARANTINE_ACTIVE, and @ref QUARANTINE_DONE.
     */
    std::vector<size_t> _agent_quarantine_triggered;

    std::vector<int> _day_flagged;   ///< Day the agent was quarantined/isolated
    std::vector<int> _exposure_date; ///< Day the agent was first exposed

    /** @brief When @c true the next @c operator() sweep quarantines all
     *         eligible agents regardless of contact tracing. */
    bool _system_quarantine_triggered = false;

    int _model_id = -1;  ///< Tracks simulation id for lazy setup

    /**
     * @brief One-time per-simulation setup.
     * @details Registers the willingness parameters with the model (if not
     *          already present) and performs the per-agent Bernoulli draws.
     */
    void _setup(Model<TSeq> * model);

    /**
     * @brief Apply the quarantine / isolation action to a single agent.
     *
     * @param agent   Reference to the agent to process.
     * @param model   Reference to the model (needed for state changes).
     */
    void _apply_quarantine(Agent<TSeq> & agent, Model<TSeq> & model);

public:

    /** @name Quarantine trigger status constants */
    ///@{
    static constexpr size_t QUARANTINE_INACTIVE = 0u;
    static constexpr size_t QUARANTINE_ACTIVE   = 1u;
    static constexpr size_t QUARANTINE_DONE     = 2u;
    ///@}

    /**
     * @brief Construct a new InterventionMeaslesQuarantine.
     *
     * @param name                  Human-readable name for the global event.
     * @param quarantine_willingness Probability each agent is willing to
     *                               quarantine (drawn once per simulation).
     * @param isolation_willingness  Probability each agent is willing to
     *                               self-isolate (drawn once per simulation).
     * @param quarantinable_states   States eligible for quarantine
     *                               (e.g.\ {SUSCEPTIBLE, LATENT, PRODROMAL}).
     * @param quarantine_targets     Target quarantine states, same length as
     *                               @p quarantinable_states.
     * @param isolatable_state       State eligible for isolation (e.g.\ RASH).
     * @param isolation_state        Target isolation state (e.g.\ ISOLATED).
     * @param max_base_state         Agents in states > this are already
     *                               quarantined / isolated and skipped.
     */
    InterventionMeaslesQuarantine(
        std::string name,
        epiworld_double quarantine_willingness,
        epiworld_double isolation_willingness,
        std::vector<int> quarantinable_states,
        std::vector<int> quarantine_targets,
        int isolatable_state,
        int isolation_state,
        int max_base_state
    );

    /**
     * @brief Execute the quarantine process for the current day.
     *
     * @details
     * Called automatically by the model at the end of each simulated day.
     * The method:
     *
     *  1. Lazily calls @ref _setup on the first day of each simulation.
     *  2. If @ref trigger_system_quarantine was called, sweeps **all**
     *     agents and quarantines / isolates eligible ones.
     *  3. Otherwise, iterates over agents whose status is
     *     @ref QUARANTINE_ACTIVE, traces their contacts via the internal
     *     @ref ContactTracing, and quarantines / isolates eligible contacts.
     *
     * @param model Pointer to the running model.
     * @param day   Current simulation day.
     */
    void operator()(Model<TSeq> * model, int day) override;

    /** @brief Polymorphic clone (deep-copies the intervention). */
    std::unique_ptr<GlobalEvent<TSeq>> clone_ptr() const override;

    // ------------------------------------------------------------------ //
    // Initialisation (called by the host model's reset())                 //
    // ------------------------------------------------------------------ //

    /**
     * @brief (Re-)initialise internal vectors for a new simulation run.
     *
     * @param n_agents     Population size.
     * @param max_contacts Maximum contacts tracked per agent
     *                     (default @c EPI_MAX_TRACKING).
     */
    void init(size_t n_agents, size_t max_contacts = EPI_MAX_TRACKING);

    // ------------------------------------------------------------------ //
    // Contact-tracing interface                                           //
    // ------------------------------------------------------------------ //

    /**
     * @brief Record a contact between two agents.
     *
     * @param agent_a Source agent id (usually the infectious agent).
     * @param agent_b Target agent id (usually the susceptible contact).
     * @param day     Day of the contact.
     */
    void add_contact(size_t agent_a, size_t agent_b, size_t day);

    /** @brief Mutable access to the internal ContactTracing object. */
    ContactTracing & get_contact_tracing();

    /** @brief Const access to the internal ContactTracing object. */
    const ContactTracing & get_contact_tracing() const;

    // ------------------------------------------------------------------ //
    // Quarantine triggering                                               //
    // ------------------------------------------------------------------ //

    /**
     * @brief Flag an agent for contact-tracing-based quarantine.
     *
     * @details Marks the agent's trigger status as @ref QUARANTINE_ACTIVE.
     *          The next @c operator() call will process the agent's contacts.
     * @param agent_id Id of the detected agent.
     */
    void trigger_quarantine(size_t agent_id);

    /**
     * @brief Activate system-wide quarantine.
     *
     * @details On the next @c operator() call, **all** eligible agents will
     *          be quarantined / isolated, regardless of contact tracing.
     */
    void trigger_system_quarantine();

    /**
     * @brief Query the quarantine-trigger status of an agent.
     * @param agent_id Agent id.
     * @return One of @ref QUARANTINE_INACTIVE, @ref QUARANTINE_ACTIVE, or
     *         @ref QUARANTINE_DONE.
     */
    size_t get_quarantine_status(size_t agent_id) const;

    // ------------------------------------------------------------------ //
    // Exposure-date tracking                                              //
    // ------------------------------------------------------------------ //

    /**
     * @brief Set the exposure date for an agent.
     *
     * @details Typically called when the agent becomes infected/latent.
     *          The date is used by the contact-tracing window check
     *          (compared against "Contact tracing days prior" model
     *          parameter) and can also be queried by other interventions.
     *
     * @param agent_id Agent id.
     * @param day      Day of exposure.
     */
    void set_exposure_date(size_t agent_id, int day);

    /**
     * @brief Retrieve the exposure date for an agent.
     * @param agent_id Agent id.
     * @return The exposure day previously set via @ref set_exposure_date.
     */
    int get_exposure_date(size_t agent_id) const;

    // ------------------------------------------------------------------ //
    // Day-flagged tracking                                                //
    // ------------------------------------------------------------------ //

    /**
     * @brief Record the day an agent was quarantined or isolated.
     * @param agent_id Agent id.
     * @param day      Day of flagging.
     */
    void set_day_flagged(size_t agent_id, int day);

    /**
     * @brief Get the day an agent was flagged.
     * @param agent_id Agent id.
     * @return Day set via @ref set_day_flagged.
     */
    int get_day_flagged(size_t agent_id) const;

    // ------------------------------------------------------------------ //
    // Willingness access                                                  //
    // ------------------------------------------------------------------ //

    /**
     * @brief Set quarantine willingness for an agent.
     * @param agent_id Agent id.
     * @param willing  Whether the agent is willing.
     */
    void set_quarantine_willing(size_t agent_id, bool willing);

    /**
     * @brief Set isolation willingness for an agent.
     * @param agent_id Agent id.
     * @param willing  Whether the agent is willing.
     */
    void set_isolation_willing(size_t agent_id, bool willing);

    /** @brief Query quarantine willingness. */
    bool is_quarantine_willing(size_t agent_id) const;

    /** @brief Query isolation willingness. */
    bool is_isolation_willing(size_t agent_id) const;

};

#endif
