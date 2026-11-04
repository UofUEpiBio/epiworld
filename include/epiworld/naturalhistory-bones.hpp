#ifndef EPIWORLD_NATURALHISTORY_BONES_HPP
#define EPIWORLD_NATURALHISTORY_BONES_HPP

template<typename TSeq>
class Agent;

template<typename TSeq>
class Model;

template<typename TSeq>
class DataBase;

/**
 * @brief The natural history of a disease: its stages and how agents move
 * through them.
 *
 * @details
 * A model's *statuses* (`Model::add_state()`) describe where an agent is
 * (free, quarantined, isolated, hospitalized). A natural history describes
 * what a disease does: **every agent has a stage in each natural history of
 * the model**, infected or not (e.g., Susceptible, Latent, Prodromal, Rash,
 * Recovered). The model's states are the pairs of a status and a stage,
 * which the model builds itself (see `Model::add_natural_history()`). A virus follows one natural history
 * (`Virus::set_natural_history()`): an agent who gets the virus starts an
 * *episode* in the infection stage, and one who loses it moves to the
 * recovery stage (`set_infection()`). Neither changes the agent's status.
 *
 * Each step, the model runs the update function of the agent's stage, then
 * the update function of its status. A stage can be infectious
 * (`set_infectious()`): only agents in an infectious stage transmit. A stage can instead declare a
 * duration and a next stage (`set_duration()`): the number of days is drawn
 * when the agent enters the stage, and the agent moves on at its first update
 * on or after that day. A stage with both an update function and a duration
 * leaves the decision to the function, which can read `due()`.
 *
 * Per agent, the natural history keeps the episode (the number of
 * infections so far), the day the agent last entered each stage in the
 * current episode (`entered()`) and the day the current stage's duration ends.
 * The model resets and copies them; starting an episode clears the entry
 * days. So a duration drawn on entry survives changes of status.
 *
 * Stage changes are events (`Agent::change_stage()`), applied at the end of
 * the step. An event whose agent has left the stage or the episode it was
 * scheduled from is dropped. See `Model::set_stage_hook()` to react to stage
 * changes.
 *
 * @tparam TSeq Type of the genetic sequence.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class NaturalHistory {
    friend class Model<TSeq>;
    friend class DataBase<TSeq>;
private:

    std::string name;
    std::vector< std::string > labels;
    std::vector< UpdateFun<TSeq> > funs;
    std::vector< StageDurationFun<TSeq> > durations;
    std::vector< int > next_stage;
    std::vector< bool > infectious;
    int infection_stage = -1;
    int recovery_stage  = -1;
    int death_stage     = -1;

    // Per agent
    std::vector< int > episode;
    std::vector< int > until;    ///< Day the stage's duration ends (-1: none)
    std::vector< int > entry;    ///< Agent-major, nstages per agent

    void reset(size_t n);
    void enter(Agent<TSeq> & p, int s, Model<TSeq> & m);
    void start_episode(Agent<TSeq> & p, int s, Model<TSeq> & m);

    /// Whether agents in stage `s` need an update each step.
    bool has_update(int s) const;

    /// The update of an agent in stage `s` (natural history `k`).
    void update(Agent<TSeq> & p, Model<TSeq> & m, size_t k, int s) const;
    int check_stage(int s) const;

public:

    NaturalHistory() = delete;
    explicit NaturalHistory(std::string name_) : name(std::move(name_)) {};

    /**
     * @brief Adds a stage.
     * @param label Name of the stage (unique within the natural history).
     * @param fun Update function, run each step for the agents in the stage.
     * @return The stage's index. The first stage added is everyone's stage
     * when the run starts.
     */
    int add_stage(std::string label, UpdateFun<TSeq> fun = nullptr);

    /**
     * @brief Gives a stage a duration drawn on entry.
     * @param s The stage.
     * @param fun Draws the number of days (at least one).
     * @param next The stage that follows when the duration is over (-1: none;
     * the stage's update function decides). If it is the recovery stage, the
     * agent loses the virus.
     */
    void set_duration(int s, StageDurationFun<TSeq> fun, int next = -1);

    /**
     * @brief Sets the stages where infections start and end.
     * @param start Stage of an agent who gets a virus following this
     * natural history.
     * @param end Stage of an agent who loses it (recovers).
     * @param death Stage of an agent who dies of it (-1: none; see
     * `default_update_exposed()`).
     */
    void set_infection(int start, int end, int death = -1);
    int get_infection_stage() const { return infection_stage; };
    int get_recovery_stage() const { return recovery_stage; };
    int get_death_stage() const { return death_stage; };

    /**
     * @brief Marks a stage as infectious (or not).
     * @details Only agents in an infectious stage transmit. If no stage is
     * marked, every agent carrying a virus transmits.
     */
    void set_infectious(int s, bool is_infectious = true);

    const std::string & get_name() const { return name; };
    const std::vector< std::string > & get_stages() const { return labels; };
    size_t size() const noexcept { return labels.size(); };

    /// Index of the stage with this label (throws if there is none).
    int stage_of(std::string_view label) const;

    /// @name Per-agent data (the stage itself: `Agent::get_stage()`)
    ///@{
    int get_episode(const Agent<TSeq> & p) const;  ///< Infections so far
    int entered(const Agent<TSeq> & p, int s) const; ///< Day of the last entry into `s` in this episode (-1: none)
    int get_until(const Agent<TSeq> & p) const;    ///< Day the current stage's duration ends (-1: none)
    bool due(const Agent<TSeq> & p, const Model<TSeq> & m) const; ///< The current stage's duration is over
    ///@}

};

#endif
