#ifndef EPIWORLD_LOCATION_BONES_HPP
#define EPIWORLD_LOCATION_BONES_HPP

template<typename TSeq>
class Agent;

template<typename TSeq>
class Location {
private:
    
    int capacity = 0;
    std::string location_name = "Unknown Location";

    std::vector< Agent<TSeq> * > agents;
    size_t n_agents = 0u;

    /**
     * @brief Spatial location parameters
     * 
     */
    ///@{
    epiworld_double longitude = 0.0;
    epiworld_double latitude  = 0.0;
    epiworld_double altitude  = 0.0;
    ///@}

public:

    Location() {};

    void add_agent(Agent<TSeq> & p);
    void add_agent(Agent<TSeq> * p);
    void rm_agent(size_t idx);
    size_t size() const noexcept;
    void set_location(int lon, int lat, int alt = 0);

    typename std::vector< Agent<TSeq> * >::iterator begin();
    typename std::vector< Agent<TSeq> * >::iterator end();

};

template<typename TSeq>
inline void Location<TSeq>::add_agent(Agent<TSeq> & p)
{
    if (++n_agents <= agents.size())
        agents.push_back(&p);
    else
        agents[n_agents - 1] = &p;
}

template<typename TSeq>
inline void Location<TSeq>::add_agent(Agent<TSeq> * p)
{
    if (++n_agents <= agents.size())
        agents.push_back(p);
    else
        agents[n_agents - 1] = p;
}

template<typename TSeq>
inline void Location<TSeq>::rm_agent(size_t idx)
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
inline size_t Location<TSeq>::size() const noexcept
{
    return n_agents;
}

template<typename TSeq>
inline void Location<TSeq>::set_location(int lon, int lat, int alt)
{
    longitude = lon;
    latitude  = lat;
    altitude  = alt;
}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator Location<TSeq>::begin()
{

    if (n_agents == 0)
        return agents.end();

    return agents.begin();

}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator Location<TSeq>::end()
{
    return agents.begin() + n_agents;
}


#endif
