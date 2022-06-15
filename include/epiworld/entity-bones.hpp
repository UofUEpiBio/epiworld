#ifndef EPIWORLD_ENTITY_BONES_HPP
#define EPIWORLD_ENTITY_BONES_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Agent;

template<typename TSeq>
class AgentsSample;

template<typename TSeq>
inline void default_add_entity(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_entity(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
class Entity {
    friend class Agent<TSeq>;
    friend class AgentsSample<TSeq>;
    friend class Model<TSeq>;
    friend void default_add_entity<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_entity<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
private:
    
    int id = -1;
    std::vector< Agent<TSeq> * > agents;   ///< Vector of agents
    std::vector< size_t > agents_location; ///< Location where the entity is stored in the agent
    size_t n_agents = 0u;

    /**
     * @name Auxiliary variables for AgentsSample<TSeq> iterators
     * 
     * @details These variables+objects are used by the AgentsSample<TSeq>
     * class for building efficient iterators over agents. The idea is to
     * reduce the memory allocation, so only during the first call of
     * AgentsSample<TSeq>::AgentsSample(Entity<TSeq>) these vectors are allocated.
     */
    ///@{
    std::vector< Agent<TSeq> * > sampled_agents;
    size_t sampled_agents_n = 0u;
    std::vector< size_t > sampled_agents_left;
    size_t sampled_agents_left_n = 0u;
    ///@}

    int max_capacity = -1;
    std::string entity_name = "Unknown entity";

    std::vector< epiworld_double > location = {0.0}; ///< An arbitrary vector for location
    Model<TSeq> * model = nullptr;

    epiworld_fast_int status_init = -99;
    epiworld_fast_int status_post = -99;

    epiworld_fast_int queue_init = 0; ///< Change of status when added to agent.
    epiworld_fast_int queue_post = 0; ///< Change of status when removed from agent.

public:

    Entity() = delete;
    // Entity(const Entity & e) = delete;
    // Entity(Entity && e);
    Entity(std::string name) : entity_name(name) {};

    void add_agent(Agent<TSeq> & p);
    void add_agent(Agent<TSeq> * p);
    void rm_agent(size_t idx);
    size_t size() const noexcept;
    void set_location(std::vector< epiworld_double > loc);
    std::vector< epiworld_double > & get_location();

    typename std::vector< Agent<TSeq> * >::iterator begin();
    typename std::vector< Agent<TSeq> * >::iterator end();

    typename std::vector< Agent<TSeq> * >::const_iterator begin() const;
    typename std::vector< Agent<TSeq> * >::const_iterator end() const;

    int get_id() const noexcept;
    const std::string & get_name() const noexcept;

    void set_status(epiworld_fast_int init, epiworld_fast_int post);
    void set_queue(epiworld_fast_int init, epiworld_fast_int post);
    void get_status(epiworld_fast_int * init, epiworld_fast_int * post);
    void get_queue(epiworld_fast_int * init, epiworld_fast_int * post);

};

template<typename TSeq>
inline void Entity<TSeq>::add_agent(Agent<TSeq> & p)
{

    // Need to add it to the actions, through the individual
    p.add_entity(*this);    

}

template<typename TSeq>
inline void Entity<TSeq>::add_agent(Agent<TSeq> * p)
{
    p->add_entity(*this);
}

template<typename TSeq>
inline void Entity<TSeq>::rm_agent(size_t idx)
{
    if (idx >= n_agents)
        throw std::out_of_range(
            "Trying to remove agent "+ std::to_string(idx) +
            " out of " + std::to_string(n_agents)
            );

    if (--n_agents > 0)
        std::swap(agents[idx], agents[n_agents]);

    return;
}

template<typename TSeq>
inline size_t Entity<TSeq>::size() const noexcept
{
    return n_agents;
}

template<typename TSeq>
inline void Entity<TSeq>::set_location(std::vector< epiworld_double > loc)
{
    location = loc;
}

template<typename TSeq>
inline std::vector< epiworld_double > & Entity<TSeq>::get_location()
{
    return location;
}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator Entity<TSeq>::begin()
{

    if (n_agents == 0)
        return agents.end();

    return agents.begin();

}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator Entity<TSeq>::end()
{
    return agents.begin() + n_agents;
}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::const_iterator Entity<TSeq>::begin() const
{

    if (n_agents == 0)
        return agents.end();

    return agents.begin();

}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::const_iterator Entity<TSeq>::end() const
{
    return agents.begin() + n_agents;
}

template<typename TSeq>
inline int Entity<TSeq>::get_id() const noexcept
{
    return id;
}

template<typename TSeq>
inline const std::string & Entity<TSeq>::get_name() const noexcept
{
    return entity_name;
}

template<typename TSeq>
inline void Entity<TSeq>::set_status(
    epiworld_fast_int init,
    epiworld_fast_int end
)
{
    status_init = init;
    status_post = end;
}

template<typename TSeq>
inline void Entity<TSeq>::set_queue(
    epiworld_fast_int init,
    epiworld_fast_int end
)
{
    queue_init = init;
    queue_post = end;
}

template<typename TSeq>
inline void Entity<TSeq>::get_status(
    epiworld_fast_int * init,
    epiworld_fast_int * post
)
{
    if (init != nullptr)
        *init = status_init;

    if (post != nullptr)
        *post = status_post;

}

template<typename TSeq>
inline void Entity<TSeq>::get_queue(
    epiworld_fast_int * init,
    epiworld_fast_int * post
)
{
    if (init != nullptr)
        *init = queue_init;

    if (post != nullptr)
        *post = queue_post;

}

#endif
