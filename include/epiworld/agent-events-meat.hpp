#ifndef EPIWORLD_AGENT_EVENTS_MEAT_HPP
#define EPIWORLD_AGENT_EVENTS_MEAT_HPP


template<typename TSeq>
inline void default_add_virus(Event<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> *  p = a.agent;
    VirusPtr<TSeq> v = a.virus;
    
    m->get_db().record_transmission(
        v->get_agent() ? v->get_agent()->get_id() : -1,
        p->get_id(),
        v->get_id(),
        v->get_date() 
    );
    
    p->virus = std::make_shared< Virus<TSeq> >(*v);
    p->virus->set_date(m->today());
    p->virus->set_agent(p);

    // Change of state needs to be recorded and updated on the
    // tools.
    if ((a.new_state != -99) && (static_cast<int>(p->state) != a.new_state))
    {
        auto & db = m->get_db();
        db.update_state(p->state_prev, a.new_state);

        for (size_t i = 0u; i < p->n_tools; ++i)
            db.update_tool(
                p->tools[i]->get_id(),
                p->state_prev,
                a.new_state
            );
    }

    // Lastly, we increase the daily count of the virus
    #ifdef EPI_DEBUG
    m->get_db().today_virus.at(v->get_id()).at(
        a.new_state != -99 ? a.new_state : p->state
    )++;
    #else
    m->get_db().today_virus[v->get_id()][
        a.new_state != -99 ? a.new_state : p->state
    ]++;
    #endif

}

template<typename TSeq>
inline void default_add_tool(Event<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p = a.agent;
    ToolPtr<TSeq> t = a.tool;
    
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

    // Change of state needs to be recorded and updated on the
    // tools.
    if ((a.new_state != -99) && static_cast<int>(p->state) != a.new_state)
    {
        auto & db = m->get_db();
        db.update_state(p->state_prev, a.new_state);

        if (p->virus)
            db.update_virus(
                p->virus->get_id(),
                p->state_prev,
                a.new_state
            );
    }

    m->get_db().today_tool[t->get_id()][
        a.new_state != -99 ? a.new_state : p->state
    ]++;


}

template<typename TSeq>
inline void default_rm_virus(Event<TSeq> & a, Model<TSeq> * model)
{

    Agent<TSeq> * p    = a.agent;
    VirusPtr<TSeq> & v = a.virus;

    // Calling the virus action over the removed virus
    v->post_recovery(model);

    p->virus = nullptr;

    // Change of state needs to be recorded and updated on the
    // tools.
    if ((a.new_state != -99) && (static_cast<int>(p->state) != a.new_state))
    {
        auto & db = model->get_db();
        db.update_state(p->state_prev, a.new_state);

        for (size_t i = 0u; i < p->n_tools; ++i)
            db.update_tool(
                p->tools[i]->get_id(),
                p->state_prev,
                a.new_state
            );
    }

    // The counters of the virus only needs to decrease.
    // We use the previous state of the agent as that was
    // the state when the virus was added.
    #ifdef EPI_DEBUG
    model->get_db().today_virus.at(v->get_id()).at(p->state_prev)--;
    #else
    model->get_db().today_virus[v->get_id()][p->state_prev]--;
    #endif

    
    return;

}

template<typename TSeq>
inline void default_rm_tool(Event<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p   = a.agent;    
    ToolPtr<TSeq> & t = a.agent->tools[a.tool->pos_in_agent];

    if (--p->n_tools > 0)
    {
        p->tools[p->n_tools]->pos_in_agent = t->pos_in_agent;
        std::swap(
            p->tools[t->pos_in_agent],
            p->tools[p->n_tools]
            );
    }

    // Change of state needs to be recorded and updated on the
    // tools.
    if ((a.new_state != -99) && (static_cast<int>(p->state) != a.new_state))
    {
        auto & db = m->get_db();
        db.update_state(p->state_prev, a.new_state);

        if (p->virus)
            db.update_virus(
                p->virus->get_id(),
                p->state_prev,
                a.new_state
            );
    }

    // Lastly, we increase the daily count of the tool.
    // Like rm_virus, we use the previous state of the agent
    // as that was the state when the tool was added.
    #ifdef EPI_DEBUG
    m->get_db().today_tool.at(t->get_id()).at(p->state_prev)--;
    #else
    m->get_db().today_tool[t->get_id()][p->state_prev]--;
    #endif

    return;

}

template<typename TSeq>
inline void default_change_state(Event<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p = a.agent;

    if ((a.new_state != -99) && (static_cast<int>(p->state) != a.new_state))
    {
        auto & db = m->get_db();
        db.update_state(p->state_prev, a.new_state);

        if (p->virus)
            db.update_virus(
                p->virus->get_id(), p->state_prev, a.new_state
            );

        for (size_t i = 0u; i < p->n_tools; ++i)
            db.update_tool(
                p->tools[i]->get_id(),
                p->state_prev,
                a.new_state
            );

    }

}

template<typename TSeq>
inline void default_set_entity(Event<TSeq> & a, Model<TSeq> *)
{

    Agent<TSeq> *  p = a.agent;
    Entity<TSeq> * e = a.entity;

    // Checking if the agent already has an entity
    if (p->entity != -1)
        throw std::logic_error("An agent can only have one entity. Remove the current entity first.");

    // Setting the entity ID in the agent
    p->entity = e->get_id();

    // Adding the agent to the entity
    if (++e->n_agents <= e->agents.size())
    {
        e->agents[e->n_agents - 1] = p->get_id();
    } else
    {
        e->agents.push_back(p->get_id());
    }

    return;
    
}

template<typename TSeq>
inline void default_rm_entity(Event<TSeq> & a, Model<TSeq> * m)
{
    
    Agent<TSeq> *  p = a.agent;    
    Entity<TSeq> * e = a.entity;

    // Remove entity from agent
    p->entity = -1;

    // Find the agent in the entity's agents list and remove it
    size_t agent_idx_in_entity = 0;
    bool found = false;
    for (size_t i = 0; i < e->n_agents; ++i)
    {
        if (e->agents[i] == static_cast<size_t>(p->get_id()))
        {
            agent_idx_in_entity = i;
            found = true;
            break;
        }
    }

    if (!found)
        throw std::logic_error("Agent not found in entity's agents list.");

    // Remove agent from entity by moving the last agent to this position
    if (--e->n_agents > 0)
    {
        // Move the last agent to fill the gap
        e->agents[agent_idx_in_entity] = e->agents[e->n_agents];
    }

    return;

};

#endif