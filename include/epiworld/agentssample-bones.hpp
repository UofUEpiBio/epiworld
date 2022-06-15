#ifndef EPIWORLD_AGENTS_BONES_HPP
#define EPIWORLD_AGENTS_BONES_HPP

enum SAMPLETYPE {
    MODEL,
    ENTITY,
    AGENT
};

template<typename TSeq>
class Agent;

template<typename TSeq>
class Model;

template<typename TSeq>
class Entity;

/**
 * @brief Sample of agents
 * 
 * This class allows sampling agents from Entity<TSeq> and Model<TSeq>.
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class AgentsSample {
private:

    size_t sample_size = 0u;

    std::vector< Agent<TSeq>* > * agents = nullptr; ///< Pointer to sample of agents
    size_t * agents_n = nullptr;                    ///< Size of sample of agents
    
    std::vector< size_t > * agents_left = nullptr;  ///< Pointer to agents left (iota)
    size_t * agents_left_n = nullptr;               ///< Size of agents left

    Model<TSeq> * model = nullptr;   ///< Extracts runif() and (if the case) population.
    Entity<TSeq> * entity = nullptr; ///
    Agent<TSeq> * agent = nullptr;
    
    int sample_type = SAMPLETYPE::AGENT;

    void sample_n(size_t n); ///< Backbone function for sampling


public:

    // Not available (for now)
    AgentsSample() = delete;                       ///< Default constructor
    AgentsSample(const AgentsSample<TSeq> & a) = delete; ///< Copy constructor
    AgentsSample(AgentsSample<TSeq> && a) = delete;      ///< Move constructor

    AgentsSample(Model<TSeq> & model_, size_t n);
    AgentsSample(Entity<TSeq> & entity_, size_t n);
    AgentsSample(Agent<TSeq> & agent_, size_t n);

    ~AgentsSample();

    typename std::vector< Agent<TSeq> * >::iterator begin();
    typename std::vector< Agent<TSeq> * >::iterator end();

    Agent<TSeq> * operator[](size_t n);
    Agent<TSeq> * operator()(size_t n);
    const size_t size() const noexcept;

};

template<typename TSeq>
inline AgentsSample<TSeq>::AgentsSample(Model<TSeq> & model_, size_t n) {

    if (n > model_.size())
        throw std::logic_error(
            "There are only " + std::to_string(model_.size()) + " agents. You cannot " +
            "sample " + std::to_string(n));

    sample_size = n;
    model       = &model_;
    sample_type = SAMPLETYPE::MODEL;

    agents   = &model_.sampled_population;
    agents_n = &model_.sampled_population_n;

    agents_left   = &model_.population_left;
    agents_left_n = &model_.population_left_n;

    sample_n(n);
    
    return; 

}

template<typename TSeq>
inline AgentsSample<TSeq>::AgentsSample(Entity<TSeq> & entity_, size_t n) {

    if (n > entity_.size())
        throw std::logic_error(
            "There are only " + std::to_string(entity_.size()) + " agents. You cannot " +
            "sample " + std::to_string(n));

    sample_size = n;
    model       = &entity_.model;
    sample_type = SAMPLETYPE::ENTITY;

    agents   = &entity_.sampled_agents;
    agents_n = &entity_.sampled_agents_n;

    agents_left   = &entity_.sampled_agents_left;
    agents_left_n = &entity_.sampled_agents_left_n;

    sample_n(n);

    return; 

}

template<typename TSeq>
inline AgentsSample<TSeq>::AgentsSample(Agent<TSeq> & agent_, size_t n)
{

    sample_size = n;
    model       = &agent_.model;
    sample_type = SAMPLETYPE::AGENT;
    
    agent = &agent;

    agents   = &agent_.sampled_agents;
    agents_n = &agent_.sampled_agents_n;

    agents_left   = &agent_.sampled_agents_left;
    agents_left_n = &agent_.sampled_agents_left_n;

    bool up_to_date = true;
    size_t agents_in_entities = 0;

    for (const auto & e : agent->get_entities())
    {
        agents_in_entities += (e->size() - 1u);
        if (e->date_last_add_or_remove >= agent->date_last_build_sample)
            up_to_date = false;

    }

    if (n > agents_in_entities)
        throw std::logic_error(
            "There are only " + std::to_string(agents_in_entities) + " agents. You cannot " +
            "sample " + std::to_string(n));

    // Checking if we need to redo the list, listing the agents that
    // user can access
    if (!up_to_date)
    {
        if (agents->size() < agents_in_entities)
            agents->resize(agents_in_entities);

        size_t locator = 0u;
        for (auto & e: agent->get_entities())
        {
            for (auto & a_e : *e)
            {
                // Adding agent to the queue of available agents
                if (a_e->get_id() != agent->get_id())
                    agents->operator[](locator++) = a_e;
            }
        }

        // We modify the iota right away
        agents_left->resize(agents_in_entities, 0);
        std::iota(agents_left->begin(), agents_left->end(), 0);

        // So in the future we may not need to rebuild the list
        agent->date_last_build_sample = model->today();
        
    }

    // We now have full space
    *agents_left_n = agents_in_entities;

    for (size_t i = 0u; i < n; ++i)
    {

        size_t ith = agents_left->operator[](model->runif() * ((*agents_left_n)--)) + i;

        // Only make the change if we are dealing with the same
        // The move is:
        // i <-> j : j in (i + 1, nleft)
        std::swap(agents->operator[](i), agents->operator[](ith));

    }

    *agents_n = n;

    return; 

}

template<typename TSeq>
inline AgentsSample<TSeq>::~AgentsSample() {}

template<typename TSeq>
inline const size_t AgentsSample<TSeq>::size() const noexcept
{ 
    return this->sample_size;
}

template<typename TSeq>
inline Agent<TSeq> * AgentsSample<TSeq>::operator[](size_t i)
{

    return agents->operator[](i);

}

template<typename TSeq>
inline Agent<TSeq> * AgentsSample<TSeq>::operator()(size_t i)
{

    if (i >= this->sample_size)
        throw std::range_error("The requested agent is out of range.");

    return agents->operator[](i);

}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator AgentsSample<TSeq>::begin()
{

    if (sample_size > 0u)
        return agents->begin();
    else
        return agents->end();

}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator AgentsSample<TSeq>::end()
{

    return agents->begin() + sample_size;

}

template<typename TSeq>
inline void AgentsSample<TSeq>::sample_n(size_t n)
{

    // Checking if the size of the entity has changed (or hasn't been initialized)
    if (sample_type == SAMPLETYPE::MODEL)
    {

        if (model->size() != agents_left->size())
        {
            agents_left->resize(model->size(), 0u);
            std::iota(agents_left->begin(), agents_left->end(), 0u);
        }


    } else if (sample_type == SAMPLETYPE::ENTITY) {

        if (entity->size() != agents_left->size())
        {

            agents_left->resize(entity->size(), 0u);
            std::iota(agents_left->begin(), agents_left->end(), 0u);

        }

    } 

    // Restart the counter of agents left
    *agents_left_n = agents_left->size();

    if (agents->size() < sample_size)
        agents->resize(sample_size, nullptr);

    if (sample_type == SAMPLETYPE::MODEL)
    {

        for (size_t i = 0u; i < n; ++i)
        {

            size_t ith = agents_left->operator[](model->runif() * ((*agents_left_n)--));
            agents->operator[](i) = &model->population[ith];

            // Updating list
            std::swap(agents_left->operator[](ith), agents_left->operator[](*agents_left_n));

        }

    } else if (sample_type == SAMPLETYPE::ENTITY) {

        for (size_t i = 0u; i < n; ++i)
        {

            size_t ith = agents_left->operator[](model->runif() * (--(*agents_left_n)));
            agents->operator[](i) = entity->agents[ith];

            // Updating list
            std::swap(agents_left->operator[](ith), agents_left->operator[](*agents_left_n));

        }

    }

    return;

}

#endif