#ifndef EPIWORLD_QUEUE_BONES_HPP
#define EPIWORLD_QUEUE_BONES_HPP

/**
 * @brief Controls which agents are verified at each step
 * 
 * @details The idea is that only agents who are either in
 * an infected state or have an infected neighbor should be
 * checked. Otherwise it makes no sense (no chance to recover
 * or capture the disease).
 * 
 * **Implementation details:**
 * <a href="../impl/queueing-system.md">Queueing System</a>
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class Queue
{
    friend class Model<TSeq>;

private:

    /**
     * @brief Count of ego's neighbors in queue (including ego)
     */
    std::vector< epiworld_fast_int > active;

    /**
     * @brief Outstanding `Everyone` registrations per agent.
     *
     * @details `active[i]` is the sum of `everyone[j]` over `j` in `i`'s
     * neighborhood plus `i` itself, so this is what a tie is worth to the far
     * end of it: adding an edge next to a registered agent is a `+everyone[]`
     * over there, and removing one is the mirror (see `notify_edge_added`).
     *
     * Keeping the count explicit -- rather than inferring "is this agent
     * contributing?" from whether it carries a virus -- means the bookkeeping
     * stays right no matter what registered the agent: a virus, a tool, or an
     * explicit queue argument to `Agent::change_state`.
     */
    std::vector< epiworld_fast_int > everyone;

    Model<TSeq> * model = nullptr;
    int n_in_queue = 0;

    /// @brief Adds `n` to `active[id]`, keeping `n_in_queue` in step.
    void shift(size_t id, epiworld_fast_int n);

    /// @brief Whether the counters have been sized to cover these two agents.
    bool tracks(Agent<TSeq> * a, Agent<TSeq> * b) const;

    // Auxiliary variable that checks how many steps
    // left are there
    // int n_steps_left;
    // bool queuing_started   = false;

public:

    void operator+=(Agent<TSeq> * p);
    void operator-=(Agent<TSeq> * p);
    epiworld_fast_int & operator[](epiworld_fast_uint i);

    /**
     * @name Keep the queue in step with a change to the contact network
     *
     * @details The queue counts, for every agent, how many of its neighbors are
     * registered as active. That count is built when an agent is registered
     * (`operator+=`) and unwound when it is deregistered (`operator-=`), both
     * walking the agent's neighbors *as they are at that moment*. Changing the
     * network in between would leave the two walks disagreeing, and an agent
     * whose count drifted to zero is silently skipped by
     * `Model::update_state()`.
     *
     * These keep the counts exact as the change happens, in constant time: a new
     * tie hands each end whatever the other end contributes, and a removed tie
     * takes it back.
     *
     * @param a,b The two ends of the tie that was just added or removed.
     */
    ///@{
    void notify_edge_added(Agent<TSeq> * a, Agent<TSeq> * b);
    void notify_edge_removed(Agent<TSeq> * a, Agent<TSeq> * b);
    ///@}

    // void initialize(Model<TSeq> * m, Agent<TSeq> * p);
    void reset();

    bool operator==(const Queue<TSeq> & other) const;
    bool operator!=(const Queue<TSeq> & other) const {return !operator==(other);};

    static const int NoOne    = 0;
    static const int OnlySelf = 1;
    static const int Everyone = 2;

};

template<typename TSeq>
inline void Queue<TSeq>::shift(size_t id, epiworld_fast_int n)
{

    if (n == 0)
        return;

    epiworld_fast_int before = active[id];
    active[id] += n;

    if ((before == 0) && (active[id] != 0))
        n_in_queue++;
    else if ((before != 0) && (active[id] == 0))
        n_in_queue--;

}

template<typename TSeq>
inline void Queue<TSeq>::operator+=(Agent<TSeq> * p)
{

    everyone[p->id]++;

    if (++active[p->id] == 1)
        n_in_queue++;

    if (p->get_n_neighbors() == 0u)
        return; // No neighbors, no need to add them

    for (auto n : (*p->neighbors))
    {

        if (++active[n] == 1)
            n_in_queue++;

    }

}

template<typename TSeq>
inline void Queue<TSeq>::operator-=(Agent<TSeq> * p)
{

    everyone[p->id]--;

    if (--active[p->id] == 0)
        n_in_queue--;

    if (p->get_n_neighbors() == 0u)
        return; // No neighbors, no need to add them

    for (auto n : (*p->neighbors))
    {
        if (--active[n] == 0)
            n_in_queue--;
    }

}

template<typename TSeq>
inline bool Queue<TSeq>::tracks(Agent<TSeq> * a, Agent<TSeq> * b) const
{

    // The counters are sized by reset(), i.e. when a run starts. Editing the
    // network before that -- while the model is still being set up -- has no
    // queue to keep in step, and the counts are built from the finished network
    // anyway.
    size_t hi = static_cast< size_t >(a->id > b->id ? a->id : b->id);
    return everyone.size() > hi;

}

template<typename TSeq>
inline void Queue<TSeq>::notify_edge_added(Agent<TSeq> * a, Agent<TSeq> * b)
{

    if (!tracks(a, b))
        return;

    shift(static_cast< size_t >(b->id), everyone[a->id]);
    shift(static_cast< size_t >(a->id), everyone[b->id]);

}

template<typename TSeq>
inline void Queue<TSeq>::notify_edge_removed(Agent<TSeq> * a, Agent<TSeq> * b)
{

    if (!tracks(a, b))
        return;

    shift(static_cast< size_t >(b->id), -everyone[a->id]);
    shift(static_cast< size_t >(a->id), -everyone[b->id]);

}

template<typename TSeq>
inline epiworld_fast_int & Queue<TSeq>::operator[](epiworld_fast_uint i)
{
    return active[i];
}

template<typename TSeq>
inline void Queue<TSeq>::reset()
{

    if (n_in_queue)
    {

        for (auto & q : this->active)
            q = 0;

        n_in_queue = 0;
        
    }

    for (auto & e : this->everyone)
        e = 0;

    active.resize(model->size(), 0);
    everyone.resize(model->size(), 0);

}

template<typename TSeq>
inline bool Queue<TSeq>::operator==(const Queue<TSeq> & other) const 
{
    if (active.size() != other.active.size())
        return false;

    for (size_t i = 0u; i < active.size(); ++i)
    {
        if (active[i] != other.active[i])
            return false;
    }

    if (everyone.size() != other.everyone.size())
        return false;

    for (size_t i = 0u; i < everyone.size(); ++i)
    {
        if (everyone[i] != other.everyone[i])
            return false;
    }

    return true;
}

#endif