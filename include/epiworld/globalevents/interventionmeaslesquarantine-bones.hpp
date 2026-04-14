#ifndef EPIWORLD_INTERVENTIONMEASLESQUARANTINE_BONES_HPP
#define EPIWORLD_INTERVENTIONMEASLESQUARANTINE_BONES_HPP

#include "../config.hpp"
#include "../model-bones.hpp"
#include "../agent-bones.hpp"

/**
 * @brief Global event implementing quarantine and isolation through contact
 * tracing.
 *
 * @details
 * This intervention generalizes the quarantine and isolation process for
 * measles (and similar) epidemiological models. It runs as an end-of-day
 * global event and performs the following steps each day:
 *
 * 1. **Detect newly triggered agents**: Scans for agents that have entered
 *    one of the configured `trigger_states` (e.g., Isolated) since the
 *    last check. An agent is processed at most once per simulation.
 *
 * 2. **Contact tracing**: For each newly triggered agent, retrieves their
 *    recorded contacts from the model's `ContactTracing` object. Only
 *    contacts made within `contact_tracing_days_prior` of today are
 *    considered. Each contact is successfully traced with probability
 *    `contact_tracing_success_rate`.
 *
 * 3. **Quarantine contacts**: Contacts that are in one of the configured
 *    `quarantine_source_states` are moved to the corresponding
 *    `quarantine_target_states` if the agent is willing (based on
 *    `quarantine_willingness`). Agents that already have a tool
 *    (e.g., vaccinated) are skipped.
 *
 * 4. **Isolate contacts**: Contacts that are in one of the configured
 *    `isolation_source_states` are moved to the corresponding
 *    `isolation_target_states` if the agent satisfies **both** quarantine
 *    willingness and isolation willingness.
 *
 * **Requirements:**
 * - Contact tracing must be enabled on the model via
 *   `Model::contact_tracing_on()`. A runtime error is thrown otherwise.
 * - All state codes provided must be valid for the model.
 * - The model's susceptible/infectious update functions should record
 *   contacts using `Model::get_contact_tracing().add_contact(...)`.
 *
 * Per-agent data managed by this intervention:
 * - **quarantine_willingness**: Randomized at setup using the
 *   `"Quarantine willingness"` parameter.
 * - **isolation_willingness**: Randomized at setup using the
 *   `"Isolation willingness"` parameter.
 * - **day_flagged**: The day when an agent was quarantined. Used by
 *   model update functions to determine quarantine release.
 *
 * @tparam TSeq Type for genetic sequences (default: EPI_DEFAULT_TSEQ)
 * @ingroup model_utilities
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class InterventionMeaslesQuarantine final : public GlobalEvent<TSeq> {

private:

    /// @name Parameter names registered in the model
    ///@{
    static inline const std::string _par_quarantine_willingness{
        "Quarantine willingness"};
    static inline const std::string _par_isolation_willingness{
        "Isolation willingness"};
    static inline const std::string _par_ct_success_rate{
        "Contact tracing success rate"};
    static inline const std::string _par_ct_days_prior{
        "Contact tracing days prior"};
    ///@}

    /// @name Stored parameter values (used at setup)
    ///@{
    epiworld_double _quarantine_willingness;
    epiworld_double _isolation_willingness;
    epiworld_double _ct_success_rate;
    epiworld_fast_uint _ct_days_prior;
    ///@}

    /// @name Per-agent willingness (randomized during _setup)
    ///@{
    std::vector< bool > _willing_to_quarantine;
    std::vector< bool > _willing_to_isolate;
    ///@}

    /// Tracks which agents have already triggered quarantine in this sim.
    std::vector< bool > _processed;

    /// Day each agent was flagged for quarantine (initialized to -1).
    std::vector< int > _day_flagged;

    /// @name State mappings
    ///@{
    std::vector< int > _trigger_states;
    std::vector< int > _quarantine_source_states;
    std::vector< int > _quarantine_target_states;
    std::vector< int > _isolation_source_states;
    std::vector< int > _isolation_target_states;
    ///@}

    /// Model simulation id, used to detect new runs.
    int _model_id = -1;

    /**
     * @brief Set up the intervention at the start of a simulation.
     *
     * Registers model parameters, randomizes per-agent willingness, and
     * resets internal tracking arrays.
     *
     * @param model Pointer to the running model.
     */
    void _setup(Model<TSeq> * model);

public:

    /**
     * @brief Construct a new Intervention Measles Quarantine object.
     *
     * @param name Descriptive name for this global event.
     * @param quarantine_willingness Probability (0–1) that an agent is
     *        willing to comply with quarantine.
     * @param isolation_willingness Probability (0–1) that an agent is
     *        willing to comply with isolation (checked in addition to
     *        quarantine willingness for isolation-mapped states).
     * @param contact_tracing_success_rate Probability (0–1) of
     *        successfully tracing each contact.
     * @param contact_tracing_days_prior Number of days prior to today
     *        within which contacts are traced.
     * @param trigger_states States that trigger contact tracing when an
     *        agent is found in them (e.g., Isolated).
     * @param quarantine_source_states Current states of contacts eligible
     *        for quarantine (e.g., Susceptible, Latent, Prodromal).
     * @param quarantine_target_states Target states for quarantined
     *        contacts. Must be the same length as
     *        `quarantine_source_states`.
     * @param isolation_source_states Current states of contacts eligible
     *        for isolation (e.g., Rash). Requires both quarantine and
     *        isolation willingness.
     * @param isolation_target_states Target states for isolated contacts.
     *        Must be the same length as `isolation_source_states`.
     */
    InterventionMeaslesQuarantine(
        std::string name,
        epiworld_double quarantine_willingness,
        epiworld_double isolation_willingness,
        epiworld_double contact_tracing_success_rate,
        epiworld_fast_uint contact_tracing_days_prior,
        std::vector< int > trigger_states,
        std::vector< int > quarantine_source_states,
        std::vector< int > quarantine_target_states,
        std::vector< int > isolation_source_states,
        std::vector< int > isolation_target_states
    );

    /**
     * @brief Execute the quarantine intervention for the current day.
     *
     * @details
     * Called automatically by the model at the end of each simulation day.
     * The method:
     * 1. Sets up internal state on the first call of each simulation.
     * 2. Scans agents for those newly in a trigger state.
     * 3. Traces their contacts and quarantines/isolates eligible contacts.
     *
     * @param model Pointer to the running model.
     * @param day Current simulation day.
     */
    void operator()(Model<TSeq> * model, int day) override;

    /**
     * @brief Create a deep copy of this intervention.
     * @return A unique pointer to a cloned instance.
     */
    std::unique_ptr< GlobalEvent<TSeq> > clone_ptr() const override;

    /**
     * @brief Get the day each agent was flagged for quarantine.
     * @return Const reference to the day_flagged vector.
     */
    const std::vector< int > & get_day_flagged() const;

    /**
     * @brief Get the day a specific agent was flagged.
     * @param agent_id The agent's id.
     * @return The day the agent was flagged, or -1 if never flagged.
     */
    int get_day_flagged(size_t agent_id) const;

    /**
     * @brief Get per-agent quarantine willingness.
     * @return Const reference to the quarantine willingness vector.
     */
    const std::vector< bool > & get_quarantine_willingness() const;

    /**
     * @brief Get per-agent isolation willingness.
     * @return Const reference to the isolation willingness vector.
     */
    const std::vector< bool > & get_isolation_willingness() const;

};

#endif
