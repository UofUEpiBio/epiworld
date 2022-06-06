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

    std::vector< Agent<TSeq>* > * agents    = nullptr;
    bool delete_agents = false;

    size_t * n_agents = nullptr;

    std::vector< size_t > sampled;
    Model<TSeq> * model = nullptr;


public:

    Agents() = delete;
    Agents(Model<TSeq> & model_);
    Agents(Entity<TSeq> & entity_);

    ~Agents();

    typename std::vector< Agent<TSeq> * >::iterator begin();
    typename std::vector< Agent<TSeq> * >::iterator end();

    Agent<TSeq> * operator[](size_t n);
    Agent<TSeq> * operator()(size_t n);
    const size_t size() const noexcept;

};

template<typename TSeq>
inline Agents<TSeq>::Agents(Model<TSeq> & model_) {

    model = &model_;
    
    agents = new std::vector< Agent<TSeq> * >(model_.size());
    for (auto & a: model_.population)
        agents->push_back(&a);

    delete_agents = true;

    n_agents = agents->size();

    return; 

}

template<typename TSeq>
inline Agents<TSeq>::Agents(Entity<TSeq> & entity_) {

    model    = &entity_.model;
    agents   = &entity_.agents;
    n_agents = &entity_.n_agents;

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
    return *n_agents;
}

template<typename TSeq>
inline Agent<TSeq> * Agents<TSeq>::operator[](size_t i)
{
    return &agents->operator[](i);
}

template<typename TSeq>
inline Agent<TSeq> * Agents<TSeq>::operator()(size_t i)
{

    if (i >= *n_agents)
        throw std::range_error("The requested agent is out of range.");

    return &agents->operator[](i);
}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator Agents<TSeq>::begin()
{
    if (*n_agents > 0u)
        return agents->begin();
    else
        return agents->end();
}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator Agents<TSeq>::end()
{
    return agents->begin() + *n_agents;
}


#endif