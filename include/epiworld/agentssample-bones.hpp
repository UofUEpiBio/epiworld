#ifndef EPIWORLD_AGENTS_BONES_HPP
#define EPIWORLD_AGENTS_BONES_HPP

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
    
    bool is_model = false;

    void sample_n(size_t n); ///< Backbone function for sampling


public:

    // Not available (for now)
    AgentsSample() = delete;                       ///< Default constructor
    AgentsSample(const AgentsSample<TSeq> & a) = delete; ///< Copy constructor
    AgentsSample(AgentsSample<TSeq> && a) = delete;      ///< Move constructor

    AgentsSample(Model<TSeq> & model_, size_t n);
    AgentsSample(Entity<TSeq> & entity_, size_t n);

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
    is_model    = true;
    
    model    = &model_;

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

    model    = &entity_.model;

    is_model = false;

    agents   = &entity_.sampled_agents;
    agents_n = &entity_.sampled_agents_n;

    agents_left   = &entity_.sampled_agents_left;
    agents_left_n = &entity_.sampled_agents_left_n;

    sample_n(n);

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
    if (is_model)
    {

        if (model->size() != agents_left->size())
        {
            agents_left->resize(model->size(), 0u);
            std::iota(agents_left->begin(), agents_left->end(), 0u);
        }


    } else {

        if (entity->size() != agents_left->size())
        {

            agents_left->resize(entity->size(), 0u);
            std::iota(agents_left->begin(), agents_left->end(), 0u);

        }

    }

    // Restart the counter of agents left
    *agents_left_n = agents_left->size();

    size_t idx = 0;
    if (agents->size() < sample_size)
        agents->resize(sample_size, nullptr);

    if (is_model)
    {

        for (size_t i = 0u; i < n; ++i)
        {

            size_t ith = agents_left->operator[](model->runif() * ((*agents_left_n)--));
            agents->operator[](i) = &model->population[ith];

            // Updating list
            std::swap(agents_left->operator[](ith), agents_left->operator[](*agents_left_n));

        }

    } else {

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