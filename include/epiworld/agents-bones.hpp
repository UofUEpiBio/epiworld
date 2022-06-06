#ifndef EPIWORLD_AGENTS_BONES_HPP
#define EPIWORLD_AGENTS_BONES_HPP

template<typename TSeq>
class Agent;

template<typename TSeq>
class Model;

template<typename TSeq>
class Entity;

template<typename TSeq>
class Agents {
private:

    std::vector< Agent<TSeq>* > * agents = nullptr;
    bool delete_agents = false;

    size_t n_agents = 0;

    std::vector< Agent<TSeq>* > sampled_agents;
    bool sampled = false;

    Model<TSeq> * model = nullptr;


public:

    // Not available (for now)
    Agents() = delete;                       ///< Default constructor
    Agents(const Agents<TSeq> & a) = delete; ///< Copy constructor
    Agents(Agents<TSeq> && a) = delete;      ///< Move constructor

    Agents(Model<TSeq> & model_);
    Agents(Entity<TSeq> & entity_);

    ~Agents();

    typename std::vector< Agent<TSeq> * >::iterator begin();
    typename std::vector< Agent<TSeq> * >::iterator end();

    Agent<TSeq> * operator[](size_t n);
    Agent<TSeq> * operator()(size_t n);
    const size_t size() const noexcept;

    void sample_n(size_t n);
    void sample(epiworld_double n);

};

template<typename TSeq>
inline Agents<TSeq>::Agents(Model<TSeq> & model_) {

    model = &model_;
    
    agents = new std::vector< Agent<TSeq> * >(0);
    agents->reserve(model->size());
    for (auto & a: model_.population)
        agents->push_back(&a);

    delete_agents = true;

    n_agents = model_.size();

    return; 

}

template<typename TSeq>
inline Agents<TSeq>::Agents(Entity<TSeq> & entity_) {

    model    = &entity_.model;
    agents   = &entity_.agents;
    n_agents = entity_.n_agents;

    return; 

}

template<typename TSeq>
inline Agents<TSeq>::~Agents()
{
    if (delete_agents)
        delete agents;
}

template<typename TSeq>
inline const size_t Agents<TSeq>::size() const noexcept
{
    if (sampled)
        return sampled_agents.size();
    
    return n_agents;
}

template<typename TSeq>
inline Agent<TSeq> * Agents<TSeq>::operator[](size_t i)
{

    if (sampled)
        return sampled_agents[i];

    return agents->operator[](i);
}

template<typename TSeq>
inline Agent<TSeq> * Agents<TSeq>::operator()(size_t i)
{

    if (sampled)
    {
        if (i >= sampled_agents.size())
            throw std::range_error("The requested agent is out of range.");    

        return sampled_agents[i];
    }

    if (i >= n_agents)
        throw std::range_error("The requested agent is out of range.");

    return agents->operator[](i);
}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator Agents<TSeq>::begin()
{

    if (sampled)
        return sampled_agents.begin();

    if (n_agents > 0u)
        return agents->begin();
    else
        return agents->end();
}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator Agents<TSeq>::end()
{
    if (sampled)
        return sampled_agents.end();

    return agents->begin() + n_agents;
}

template<typename TSeq>
inline void Agents<TSeq>::sample(epiworld_double p)
{
    if (p > 1)
        throw std::logic_error("Cannot sample more than 100%! (for now...)");

    if (p < 0)
        throw std::logic_error("Cannot sample a negative proportion!");

    sample_n(static_cast<size_t>(std::floor(p * n_agents)));
}

template<typename TSeq>
inline void Agents<TSeq>::sample_n(size_t n)
{

    if (n > n_agents)
        throw std::logic_error(
            "There are only " + std::to_string(n_agents) + " agents. You cannot " +
            "sample " + std::to_string(n));

    sampled_agents.resize(n);
    sampled = true;

    int n_left = n_agents;
    std::vector< size_t > agents_left(n_agents, 0);
    std::iota(agents_left.begin(), agents_left.end(), 0);

    size_t idx = 0;
    for (size_t i = 0u; i < n; ++i)
    {
        size_t ith = agents_left[model->runif() * (--n_left)];
        sampled_agents[i] = agents->operator[](ith);

        // Updating list
        std::swap(agents_left[ith], agents_left[n_left]);
    }

    return;


}

#endif