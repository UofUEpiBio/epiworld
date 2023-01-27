#ifndef EPIWORLD_ENTITY_MEAT_HPP
#define EPIWORLD_ENTITY_MEAT_HPP

// template<typename TSeq>
// inline Entity<TSeq>::Entity(const Entity<TSeq> & e) :
//     model(e.model),
//     id(e.id),
//     agents(0u),
//     agents_location(0u),
//     n_agents(0),
//     sampled_agents(0u),
//     sampled_agents_n(0u),
//     sampled_agents_left(0u),
//     sampled_agents_left_n(0u),
//     max_capacity(e.max_capacity),
//     entity_name(e.entity_name),
//     location(e.location),
//     status_init(e.status_init),
//     status_post(e.status_post),
//     queue_init(e.queue_init),
//     queue_post(e.queue_post)
// {

// }

template<typename TSeq>
inline void Entity<TSeq>::add_agent(
    Agent<TSeq> & p,
    Model<TSeq> * model
    )
{

    // Need to add it to the actions, through the individual
    p.add_entity(*this, model);    

}

template<typename TSeq>
inline void Entity<TSeq>::add_agent(
    Agent<TSeq> * p,
    Model<TSeq> * model
    )
{
    p->add_entity(*this, model);
}

template<typename TSeq>
inline void Entity<TSeq>::rm_agent(size_t idx)
{
    if (idx >= n_agents)
        throw std::out_of_range(
            "Trying to remove agent "+ std::to_string(idx) +
            " out of " + std::to_string(n_agents)
            );

    model->population[idx].rm_entity(*this);

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
inline Agent<TSeq> * Entity<TSeq>::operator[](size_t i)
{
    if (n_agents <= i)
        throw std::logic_error("There are not that many agents in this entity.");

    return &model->get_agents()[i];
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