#ifndef EPIWORLD_ENTITY_BONES_HPP
#define EPIWORLD_ENTITY_BONES_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Agent;

template<typename TSeq>
class Agents;

template<typename TSeq>
class Entity {
    friend class Agent<TSeq>;
    friend class Agents<TSeq>;
    friend class Model<TSeq>;
private:
    
    std::vector< Agent<TSeq> * > agents;
    size_t n_agents = 0u;

    int max_capacity = -1;
    std::string entity_name = "Unknown entity";

    std::vector< epiworld_double > location = {0.0}; ///< An arbitrary vector for location
    Model<TSeq> * model = nullptr;

public:

    Entity() {};

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

    typename std::vector< Agent<TSeq> * >::iterator begin_sample

};

template<typename TSeq>
inline void Entity<TSeq>::add_agent(Agent<TSeq> & p)
{
    if (++n_agents <= agents.size())
        agents.push_back(&p);
    else
        agents[n_agents - 1] = &p;
}

template<typename TSeq>
inline void Entity<TSeq>::add_agent(Agent<TSeq> * p)
{
    if (++n_agents <= agents.size())
        agents.push_back(p);
    else
        agents[n_agents - 1] = p;
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


#endif
