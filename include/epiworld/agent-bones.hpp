#ifndef EPIWORLD_PERSON_BONES_HPP
#define EPIWORLD_PERSON_BONES_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Virus;

template<typename TSeq>
class Viruses;

template<typename TSeq>
class Viruses_const;

template<typename TSeq>
class Tool;


template<typename TSeq>
class Queue;

template<typename TSeq>
struct Event;

template<typename TSeq>
class Entity;

template<typename TSeq>
class Entities;

template<typename TSeq>
class AgentsSample;

/**
 * @brief Non-allocating range over an agent's neighbors.
 *
 * @details `Agent::get_neighbors()` builds and returns a `std::vector` of
 * pointers, which costs a heap allocation on every call -- and it is called once
 * per susceptible agent per day, in the innermost loop of every state-update
 * function. This view iterates the agent's neighbor ids in place and resolves
 * each one against the model's population as it goes, so the same loop runs
 * without allocating:
 *
 * ```cpp
 * for (auto * neighbor : p->neighbors_view(*m))
 *     ...
 * ```
 *
 * The order is the agent's neighbor order, identical to `get_neighbors()`. The
 * view borrows from the agent and the model, so it must not outlive either, and
 * it is invalidated by anything that changes the agent's ties.
 *
 * @tparam TSeq Sequence type (should match `TSeq` across the model)
 */
template<typename TSeq>
class NeighborsView {
private:

    const size_t * first = nullptr;
    const size_t * last  = nullptr;
    std::vector< Agent<TSeq> > * pop = nullptr;

public:

    class iterator {

        friend class NeighborsView<TSeq>;

        const size_t * ptr = nullptr;
        std::vector< Agent<TSeq> > * pop = nullptr;

        iterator(const size_t * ptr, std::vector< Agent<TSeq> > * pop) :
            ptr(ptr), pop(pop) {}

    public:

        Agent<TSeq> * operator*() const { return &pop->operator[](*ptr); }
        iterator & operator++() { ++ptr; return *this; }
        bool operator!=(const iterator & other) const { return ptr != other.ptr; }
        bool operator==(const iterator & other) const { return ptr == other.ptr; }

    };

    NeighborsView() = default;

    NeighborsView(
        const size_t * first,
        const size_t * last,
        std::vector< Agent<TSeq> > * pop
    ) : first(first), last(last), pop(pop) {}

    iterator begin() const { return iterator(first, pop); }
    iterator end() const { return iterator(last, pop); }

    size_t size() const { return static_cast< size_t >(last - first); }
    bool empty() const { return first == last; }

};

/**
 * @brief Agent (agents)
 * 
 * @tparam TSeq Sequence type (should match `TSeq` across the model)
 */
template<typename TSeq>
class Agent {
    friend class Model<TSeq>;
    friend class Virus<TSeq>;
    friend class Tool<TSeq>;
    friend class Queue<TSeq>;
    friend class AgentsSample<TSeq>;
protected:

    /**
     * @brief The agent's ties.
     *
     * @details `neighbors` holds the ids of the agent's neighbors in the order
     * they were added, and that order is load-bearing: `roulette()` walks the
     * per-neighbor probabilities in this order and consumes a single uniform, so
     * whichever transmitter sits at a given index is what gets recorded. Adding
     * or removing a tie must therefore never permute the survivors.
     *
     * `neighbor_pos` maps neighbor id -> index in `neighbors`. It is built
     * lazily: it stays null until the agent's degree passes
     * `EPI_NEIGHBOR_INDEX_THRESHOLD`, below which a linear scan of the
     * contiguous id vector is both smaller and faster.
     */
    std::vector< size_t > * neighbors = nullptr;
    std::unordered_map< size_t, size_t > * neighbor_pos = nullptr;
    size_t n_neighbors = 0u;

    /// @brief Builds `neighbor_pos` from `neighbors` (no-op if it exists).
    void build_neighbor_index();

    /// @brief Index of `neighbor_id` in `neighbors`, or `n_neighbors` if absent.
    size_t find_neighbor(size_t neighbor_id) const;

    /// @brief Drops the neighbor at `pos`, keeping the survivors in order.
    void erase_neighbor_at(size_t pos);

    std::vector< size_t > entities; ///< Entity IDs (indices into Model::entities)

    unsigned int state = 0u;
    unsigned int state_prev = 0u; ///< For accounting, if need to undo a change.
    
    int state_last_changed = -1; ///< Last time the agent was updated.
    int id = -1;
    
    VirusPtr<TSeq> virus = nullptr;

    std::vector< ToolPtr<TSeq> > tools;

    void reset(); ///< Resets the agent to the initial state (no virus, no tools, no entities, state 0.)

public:

    Agent() = default;
    Agent(Agent<TSeq> && p);
    Agent(const Agent<TSeq> & p);
    Agent<TSeq> & operator=(const Agent<TSeq> & other_agent);
    ~Agent();

    /**
     * @name Add/Remove Virus/Tool
     * 
     * Any of these is ultimately reflected at the end of the iteration.
     * 
     * @param tool Tool to add
     * @param virus Virus to add
     * @param state_new state after the change
     * @param queue 
     */
    ///@{
    void add_tool(
        Model<TSeq> & model,
        const Tool<TSeq> & tool,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
        );

    void set_virus(
        Model<TSeq> & model,
        const Virus<TSeq> & virus,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
        );

    void add_entity(
        Model<TSeq> & model,
        Entity<TSeq> & entity,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
        );

    void rm_tool(
        Model<TSeq> & model,
        epiworld_fast_uint tool_idx,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_tool(
        Model<TSeq> & model,
        ToolPtr<TSeq> & tool,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_virus(
        Model<TSeq> & model,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_entity(
        Model<TSeq> & model,
        epiworld_fast_uint entity_idx,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_entity(
        Model<TSeq> & model,
        Entity<TSeq> & entity,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_agent_by_virus(Model<TSeq> & model) = delete; ///< Agent removed by virus
    ///@}
    
    /**
     * @name Get the rates (multipliers) for the agent
     * 
     * @param v A pointer to a virus.
     * @return epiworld_double 
     */
    ///@{
    epiworld_double get_susceptibility_reduction(VirusPtr<TSeq> & v, Model<TSeq> & model);
    epiworld_double get_transmission_reduction(VirusPtr<TSeq> & v, Model<TSeq> & model);
    epiworld_double get_recovery_enhancer(VirusPtr<TSeq> & v, Model<TSeq> & model);
    epiworld_double get_death_reduction(VirusPtr<TSeq> & v, Model<TSeq> & model);
    ///@}

    int get_id() const; ///< Id of the individual

    VirusPtr<TSeq> & get_virus();
    const VirusPtr<TSeq> & get_virus() const;

    ToolPtr<TSeq> & get_tool(int i);
    ToolPtr<TSeq> & get_tool(std::string name);

    std::vector<ToolPtr<TSeq>> get_tools();
    const std::vector<ToolPtr<TSeq>> get_tools() const;
    size_t get_n_tools() const noexcept;

    void mutate_virus();

    /**
     * @brief Ties this agent to `p`, in both directions.
     *
     * @param p The agent to tie to.
     * @param check_source Whether to check that `p` is not already a neighbor of
     *        this agent before adding it.
     * @param check_target Whether to check that this agent is not already a
     *        neighbor of `p`.
     * @return `true` if a new tie was created, `false` if the two were already
     *         tied. Callers that must undo their own additions later -- a
     *         temporary-contact intervention, say -- use the return value to
     *         record only the ties they actually created.
     */
    bool add_neighbor(
        Agent<TSeq> & p,
        bool check_source = true,
        bool check_target = true
        );

    /**
     * @brief Removes the tie between this agent and `p`, in both directions.
     *
     * @details The surviving neighbors keep their relative order (see
     * `neighbors`), so removing a tie never changes which transmitter is
     * sampled among the others.
     *
     * @param p The agent to untie from.
     * @return `true` if a tie was removed, `false` if the two were not tied.
     */
    bool rm_neighbor(Agent<TSeq> & p);

    /// @brief Whether `neighbor_id` is one of this agent's neighbors.
    bool has_neighbor(size_t neighbor_id) const;

    /**
     * @brief Swaps neighbors between the current agent and agent `other`
     * 
     * @param other 
     * @param n_this 
     * @param n_other 
     */
    void swap_neighbors(
        Agent<TSeq> & other,
        size_t n_this,
        size_t n_other,
        Model<TSeq> & model
    );

    std::vector< Agent<TSeq> * > get_neighbors(Model<TSeq> & model);

    /**
     * @brief The agent's neighbors, without allocating.
     *
     * Same agents, same order as `get_neighbors()`, but as a borrowed range
     * rather than a freshly built vector -- see `NeighborsView`. Prefer it in
     * per-step loops.
     */
    NeighborsView<TSeq> neighbors_view(Model<TSeq> & model);

    size_t get_n_neighbors() const;

    void change_state(
        Model<TSeq> & model,
        epiworld_fast_uint new_state,
        epiworld_fast_int queue = 0
        );

    unsigned int get_state() const;
    unsigned int get_state_prev() const;
    int get_state_last_changed() const;


    bool has_tool(epiworld_fast_uint t) const;
    bool has_tool(std::string_view name) const;
    bool has_tool(const Tool<TSeq> & t) const;
    bool has_virus(epiworld_fast_uint t) const;
    bool has_virus(std::string_view name) const;
    bool has_virus(const Virus<TSeq> & v) const;
    bool has_entity(epiworld_fast_uint t) const;
    bool has_entity(std::string_view name, const Model<TSeq> & model) const;

    void print(Model<TSeq> & model, bool compressed = false) const;

    /**
     * @brief Access the j-th column of the agent
     * 
     * If an external array has been specified, then these two
     * functions can be used to access additional agent's features 
     * not included in the model.
     * 
     * @param j 
     * @param model Reference to the Model
     * @return double& 
     */
    ///@{
    double & operator()(size_t j, Model<TSeq> & model);
    double operator()(size_t j, const Model<TSeq> & model) const;
    ///@}

    const std::vector< size_t > & get_entities() const;

    const Entity<TSeq> & get_entity(size_t i, const Model<TSeq> & model) const;
    Entity<TSeq> & get_entity(size_t i, Model<TSeq> & model);

    size_t get_n_entities() const;

    bool operator==(const Agent<TSeq> & other) const;
    bool operator!=(const Agent<TSeq> & other) const {return !operator==(other);};

};



#endif
