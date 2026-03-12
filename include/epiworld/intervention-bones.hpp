#ifndef EPIWORLD_INTERVENTION_BONES_HPP
#define EPIWORLD_INTERVENTION_BONES_HPP

/**
 * @brief Base class for model interventions.
 *
 * @details Interventions provide a structured, reusable interface for plugging
 * public-health actions (quarantine, isolation, contact-tracing, vaccination
 * campaigns, etc.) into any @ref Model.  They complement the existing
 * @ref GlobalEvent mechanism by offering a richer lifecycle:
 *
 *  - **setup()** – called inside `Model::reset()` so that the intervention can
 *    resize tracking arrays, etc. before each simulation run.
 *  - **update()** – called once per simulated day (or on a specific day) inside
 *    `Model::run_globalevents()`, where it drives per-agent logic.
 *  - **applies_to() / apply()** – helpers that determine *who* is affected and
 *    *what* happens to them; called from the default `update()` loop.
 *  - **should_release() / release()** – companions to the above that handle the
 *    exit from an intervention state (e.g., end of quarantine).
 *
 * The default `update()` implementation iterates over every agent and applies
 * the following logic in order:
 * 1. If `should_release(agent)` returns `true`, call `release(agent)`.
 * 2. Else if `applies_to(agent)` returns `true`, call `apply(agent)`.
 *
 * Derived classes may override any subset of these virtual methods, or override
 * `update()` entirely for full control.
 *
 * @note
 * `setup()` is invoked on *every* call to `Model::reset()`.  One-time
 * operations (e.g., registering new disease states with `Model::add_state()`)
 * must therefore be guarded by the implementation to avoid duplicate
 * registration.  Array-resize operations using `std::vector::assign()` are
 * naturally idempotent and do not need a guard.
 *
 * @tparam TSeq Sequence type (should match `TSeq` across the model; default:
 *              EPI_DEFAULT_TSEQ).
 * @ingroup interventions
 */
template<typename TSeq>
class Intervention
{
private:
    std::string name_ = "An intervention"; ///< Descriptive name.
    int day_ = -99; ///< Execution day; negative means every day.

public:

    Intervention() = default;

    /**
     * @brief Construct a named intervention with an optional fixed execution day.
     *
     * @param name Descriptive name for the intervention.
     * @param day  Simulation day on which `update()` is triggered.
     *             Any negative value (the default is @c -99, matching the
     *             @ref GlobalEvent convention) means the intervention runs
     *             every day.
     */
    Intervention(std::string name, int day = -99);

    virtual ~Intervention() = default;

    /**
     * @brief Perform one-time and per-run initialisation.
     *
     * @details Called by `Model::reset()` before the first simulation step.
     * Override to resize per-agent tracking vectors, register compliance
     * flags, etc.
     *
     * @param m Pointer to the owning model.
     */
    virtual void setup(Model<TSeq>* m);

    /**
     * @brief Execute the intervention logic for the current day.
     *
     * @details Called by `Model::run_globalevents()` on every day (when
     * `get_day() < 0`) or only on the configured day.  The default
     * implementation loops over all agents and calls `should_release()` /
     * `release()` or `applies_to()` / `apply()` as appropriate.
     *
     * @param m Pointer to the owning model.
     */
    virtual void update(Model<TSeq>* m);

    /**
     * @brief Dispatch operator used by `Model::run_globalevents()`.
     *
     * @details Applies the day filter (`get_day()`) and then delegates to
     * `update()`.  You should not normally need to override this.
     *
     * @param m   Pointer to the owning model.
     * @param day The current simulation day (`Model::today()`).
     */
    void operator()(Model<TSeq>* m, int day);

    /**
     * @brief Predicate: should this intervention be applied to @p a today?
     *
     * @details Override to implement eligibility checks (state, compliance
     * flag, etc.).  The default returns `false`.
     *
     * @param a Pointer to the candidate agent.
     * @param m Pointer to the owning model.
     * @return `true` if `apply()` should be called for this agent.
     */
    virtual bool applies_to(Agent<TSeq>* a, Model<TSeq>* m);

    /**
     * @brief Apply the intervention to a single agent.
     *
     * @details Called by the default `update()` loop when `applies_to()`
     * returns `true`.  Override to move agents into intervention states,
     * assign tool modifiers, or record tracking data.
     *
     * @param a Pointer to the agent.
     * @param m Pointer to the owning model.
     */
    virtual void apply(Agent<TSeq>* a, Model<TSeq>* m);

    /**
     * @brief Predicate: should agent @p a be released from this intervention?
     *
     * @details Override to check time-based or state-based release conditions
     * (e.g., quarantine period elapsed, agent recovered).  The default returns
     * `false`.
     *
     * @param a Pointer to the agent.
     * @param m Pointer to the owning model.
     * @return `true` if `release()` should be called for this agent.
     */
    virtual bool should_release(Agent<TSeq>* a, Model<TSeq>* m);

    /**
     * @brief Release an agent from this intervention.
     *
     * @details Called by the default `update()` loop when `should_release()`
     * returns `true`.  Override to move agents out of intervention states,
     * clear tracking data, etc.  The default is a no-op.
     *
     * @param a Pointer to the agent.
     * @param m Pointer to the owning model.
     */
    virtual void release(Agent<TSeq>* a, Model<TSeq>* m);

    // -------------------------------------------------------------------------
    // Metadata
    // -------------------------------------------------------------------------

    /**
     * @brief Set the descriptive name.
     * @param name New name string.
     */
    void set_name(std::string name);

    /**
     * @brief Get the descriptive name.
     * @return The name string.
     */
    std::string get_name() const;

    /**
     * @brief Set the execution day.
     * @param day Day index; use a negative value to run every day.
     */
    void set_day(int day);

    /**
     * @brief Get the configured execution day.
     * @return Day index (any negative value means every day, following the
     *         same convention as @ref GlobalEvent).
     */
    int get_day() const;

    /**
     * @brief Print a one-line summary of this intervention to stdout.
     */
    void print() const;

    // -------------------------------------------------------------------------
    // Comparison operators
    // -------------------------------------------------------------------------

    /** @brief Equality: name and day must match. */
    bool operator==(const Intervention<TSeq>& other) const;
    /** @brief Inequality: complement of operator==. */
    bool operator!=(const Intervention<TSeq>& other) const;

    // -------------------------------------------------------------------------
    // Polymorphic clone
    // -------------------------------------------------------------------------

    /**
     * @brief Create a heap-allocated clone of this intervention.
     *
     * @details Used internally by `Model::add_intervention(Intervention&)` so
     * that the model stores an independent copy.  Override in derived classes
     * to ensure the correct dynamic type is preserved.
     *
     * @return A `std::unique_ptr` to a copy of this intervention.
     */
    virtual std::unique_ptr<Intervention<TSeq>> clone_ptr() const;
};

#endif
