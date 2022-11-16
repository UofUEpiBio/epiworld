#ifndef EPIWORLD_AGENT_ACTIONS_MEAT_HPP
#define EPIWORLD_AGENT_ACTIONS_MEAT_HPP

template<typename TSeq>
inline void default_add_virus(Action<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> *  p = a.agent;
    VirusPtr<TSeq> v = a.virus;

    CHECK_COALESCE_(a.new_status, v->status_init, p->get_status())
    CHECK_COALESCE_(a.queue, v->queue_init, 1)

    // Has a agent? If so, we need to register the transmission
    if (v->get_agent())
    {

        // ... only if not the same agent
        if (v->get_agent()->get_id() != p->get_id())
            m->get_db().record_transmission(
                v->get_agent()->get_id(),
                p->get_id(),
                v->get_id(),
                v->get_date() 
            );

    }
    
    // Update virus accounting
    p->n_viruses++;
    size_t n_viruses = p->n_viruses;

    if (n_viruses <= p->viruses.size())
        p->viruses[n_viruses - 1] = std::make_shared< Virus<TSeq> >(*v);
    else
        p->viruses.push_back(std::make_shared< Virus<TSeq> >(*v));

    n_viruses--;

    // Notice that both agent and date can be changed in this case
    // as only the sequence is a shared_ptr itself.
    p->viruses[n_viruses]->set_agent(p, n_viruses);
    p->viruses[n_viruses]->set_date(m->today());
    p->viruses[n_viruses]->agent_exposure_number = ++p->n_exposures;

    #ifdef EPI_DEBUG
    m->get_db().today_variant.at(v->get_id()).at(p->status)++;
    #else
    m->get_db().today_variant[v->get_id()][p->status]++;
    #endif

}

template<typename TSeq>
inline void default_add_tool(Action<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p = a.agent;
    ToolPtr<TSeq> t = a.tool;

    CHECK_COALESCE_(a.new_status, t->status_init, p->get_status())
    CHECK_COALESCE_(a.queue, t->queue_init, QueueValues::NoOne)
    
    // Update tool accounting
    p->n_tools++;
    size_t n_tools = p->n_tools;

    if (n_tools <= p->tools.size())
        p->tools[n_tools - 1] = std::make_shared< Tool<TSeq> >(*t);
    else
        p->tools.push_back(std::make_shared< Tool<TSeq> >(*t));

    n_tools--;

    p->tools[n_tools]->set_date(m->today());
    p->tools[n_tools]->set_agent(p, n_tools);

    m->get_db().today_tool[t->get_id()][p->status]++;

}

template<typename TSeq>
inline void default_rm_virus(Action<TSeq> & a, Model<TSeq> * /*m*/)
{

    Agent<TSeq> * p   = a.agent;    
    VirusPtr<TSeq> & v = a.agent->viruses[a.virus->agent_idx];
    
    CHECK_COALESCE_(a.new_status, v->status_post, p->get_status())
    CHECK_COALESCE_(a.queue, v->queue_post, -QueueValues::Everyone)

    if (--p->n_viruses > 0)
    {
        // The new virus will change positions
        p->viruses[p->n_viruses]->agent_idx = v->agent_idx;
        std::swap(v, p->viruses[p->n_viruses]);
    }
    
    // Calling the virus action over the removed virus
    v->post_recovery();

    return;

}

template<typename TSeq>
inline void default_rm_tool(Action<TSeq> & a, Model<TSeq> * /*m*/)
{

    Agent<TSeq> * p  = a.agent;    
    ToolPtr<TSeq> & t = a.agent->tools[a.tool->agent_idx];

    CHECK_COALESCE_(a.new_status, t->status_post, p->get_status())
    CHECK_COALESCE_(a.queue, t->queue_post, QueueValues::NoOne)

    if (--p->n_tools > 0)
    {
        p->tools[p->n_tools]->agent_idx = t->agent_idx;
        std::swap(t, p->tools[p->n_tools - 1]);
    }

    return;

}

template<typename TSeq>
inline void default_add_entity(Action<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p  = a.agent;
    Entity<TSeq> * e = a.entity;

    CHECK_COALESCE_(a.new_status, e->status_post, p->get_status())
    CHECK_COALESCE_(a.queue, e->queue_post, QueueValues::NoOne)

    // Adding the entity to the agent
    if (++p->n_entities <= p->entities.size())
    {

        p->entities[p->n_entities - 1]           = e;
        p->entities_locations[p->n_entities - 1] = e->n_agents;

    } else
    {
        p->entities.push_back(e);
        p->entities_locations.push_back(e->n_agents);
    }

    // Adding the agent to the entity
    // Adding the entity to the agent
    if (++e->n_agents <= e->agents.size())
    {

        e->agents[e->n_agents - 1]          = p;
        // Adjusted by '-1' since the list of entities in the agent just grew.
        e->agents_location[e->n_agents - 1] = p->n_entities - 1;

    } else
    {
        e->agents.push_back(p);
        e->agents_location.push_back(p->n_entities - 1);
    }

    // Today was the last modification
    e->date_last_add_or_remove = m->today();
    
}

template<typename TSeq>
inline void default_rm_entity(Action<TSeq> & a, Model<TSeq> * m)
{
    
    Agent<TSeq> *  p = a.agent;    
    Entity<TSeq> * e = a.entity;
    size_t idx_agent_in_entity = a.idx_agent;
    size_t idx_entity_in_agent = a.idx_object;

    CHECK_COALESCE_(a.new_status, e->status_post, p->get_status())
    CHECK_COALESCE_(a.queue, e->queue_post, QueueValues::NoOne)

    if (--p->n_entities > 0)
    {

        // When we move the end entity to the new location, the 
        // moved entity needs to reflect the change, i.e., where the
        // entity will now be located in the agent
        size_t agent_in_end_entity  = p->entities_locations[p->n_entities];
        Entity<TSeq> * moved_entity = p->entities[p->n_entities];

        // The end entity will be located where the removed was
        moved_entity->agents_location[agent_in_end_entity] = idx_entity_in_agent;

        // We now make the swap
        std::swap(
            p->entities[p->n_entities],
            p->entities[idx_entity_in_agent]
        );

    }

    if (--e->n_agents > 0)
    {

        // When we move the end entity to the new location, the 
        // moved entity needs to reflect the change, i.e., where the
        // entity will now be located in the agent
        size_t entity_in_end_agent = e->agents_location[e->n_agents];
        Agent<TSeq> * moved_agent  = e->agents[e->n_agents];

        // The end entity will be located where the removed was
        moved_agent->entities_locations[entity_in_end_agent] = idx_agent_in_entity;

        // We now make the swap
        std::swap(
            e->agents[e->n_agents],
            e->agents[idx_agent_in_entity]
        );

    }

    // Setting the date of the last removal
    e->date_last_add_or_remove = m->today();

    return;

}
#endif