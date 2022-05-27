#ifndef EPIWORLD_PERSON_MEAT_HPP
#define EPIWORLD_PERSON_MEAT_HPP

// template<typename Ta>
// inline bool IN(Ta & a, std::vector< Ta > & b);

#define CHECK_COALESCE_(proposed_, virus_tool_, alt_) \
    if (static_cast<int>(proposed_) == -99) {\
        if (static_cast<int>(virus_tool_) == -99) \
            (proposed_) = (alt_);\
        else (proposed_) = (virus_tool_);}

template<typename TSeq>
inline void default_add_virus(Action<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p = a.agent;
    VirusPtr<TSeq> v = a.virus;

    CHECK_COALESCE_(a.new_status, v->status_init, p->get_status())
    CHECK_COALESCE_(a.queue, v->queue_init, 1)

    // Has a agent? If so, we need to register the transmission
    if (v->get_agent())
    {

        // ... only if not the same agent
        if (v->get_agent()->get_id() != v->get_id())
            m->get_db().record_transmission(
                v->get_agent()->get_id(), p->get_id(), v->get_id() 
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

    m->get_db().today_variant[v->get_id()][p->status]++;

}

template<typename TSeq>
inline void default_add_tool(Action<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p = a.agent;
    ToolPtr<TSeq> t = a.tool;

    CHECK_COALESCE_(a.new_status, t->status_init, p->get_status())
    CHECK_COALESCE_(a.queue, t->queue_init, 0)
    
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
inline void default_rm_virus(Action<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p   = a.agent;    
    VirusPtr<TSeq> & v = a.agent->viruses[a.virus->agent_idx];
    
    CHECK_COALESCE_(a.new_status, v->status_post, p->get_status())
    CHECK_COALESCE_(a.queue, v->queue_post, -1)

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
inline void default_rm_tool(Action<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p  = a.agent;    
    ToolPtr<TSeq> & t = a.agent->tools[a.tool->agent_idx];

    CHECK_COALESCE_(a.new_status, t->status_post, p->get_status())
    CHECK_COALESCE_(a.queue, t->queue_post, 0)

    if (--p->n_tools > 0)
    {
        p->tools[p->n_tools]->agent_idx = t->agent_idx;
        std::swap(t, p->tools[p->n_tools - 1]);
    }

    return;

}

template<typename TSeq>
inline Agent<TSeq>::Agent()
{
    
}

template<typename TSeq>
inline Agent<TSeq>::Agent(const Agent<TSeq> & p)
{

    model = p.model;
    
    // We can't do anything with the neighbors
    neighbors.reserve(p.neighbors.size());

    index  = p.index;
    status = p.status;
    id     = p.id;

    #ifdef EPI_DEBUG
    if (index < 0)
        throw std::logic_error("Index in agents cannot be negative.");
    #endif
    
    in_queue = p.in_queue;

    // Dealing with the virus
    viruses.reserve(p.n_viruses);
    for (auto & v : p.viruses)
    {
        // Will create a copy of the virus, with the exeption of
        // the virus code
        viruses.push_back(std::make_shared<Virus<TSeq>>(*v));
        viruses[n_viruses++]->agent = this;

    }

    n_viruses = p.n_viruses;

    tools.reserve(p.n_tools);
    for (auto & t : p.tools)
    {
        // Will create a copy of the virus, with the exeption of
        // the virus code
        tools.push_back(std::make_shared<Tool<TSeq>>(*t));

    }
    
    n_tools = p.n_tools;

    add_virus_ = p.add_virus_;
    add_tool_  = p.add_tool_;
    rm_virus_  = p.rm_virus_;
    rm_tool_   = p.rm_tool_;
    
}

template<typename TSeq>
inline void Agent<TSeq>::add_tool(
    ToolPtr<TSeq> tool,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
) {

    // Checking the virus exists
    if (tool->get_id() >= static_cast<int>(model->get_db().get_n_tools()))
        throw std::range_error("The tool with id: " + std::to_string(tool->get_id()) + 
            " has not been registered. There are only " + std::to_string(model->get_n_tools()) + 
            " included in the model.");
    

    model->actions_add(
        this, nullptr, tool, status_new, queue, add_tool_
        );

}

template<typename TSeq>
inline void Agent<TSeq>::add_tool(
    Tool<TSeq> tool,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{
    ToolPtr<TSeq> tool_ptr = std::make_shared< Tool<TSeq> >(tool);
    add_tool(tool_ptr, status_new, queue);
}

template<typename TSeq>
inline void Agent<TSeq>::add_virus(
    VirusPtr<TSeq> virus,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{

    // Checking the virus exists
    if (virus->get_id() >= static_cast<int>(model->get_db().get_n_variants()))
        throw std::range_error("The virus with id: " + std::to_string(virus->get_id()) + 
            " has not been registered. There are only " + std::to_string(model->get_n_variants()) + 
            " included in the model.");

    model->actions_add(
        this, virus, nullptr, status_new, queue, add_virus_
        );

}

template<typename TSeq>
inline void Agent<TSeq>::add_virus(
    Virus<TSeq> virus,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{
    VirusPtr<TSeq> virus_ptr = std::make_shared< Virus<TSeq> >(virus);
    add_virus(virus_ptr, status_new, queue);
}

template<typename TSeq>
inline void Agent<TSeq>::rm_tool(
    epiworld_fast_uint tool_idx,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{

    if (tool_idx >= n_tools)
        throw std::range_error(
            "The Tool you want to remove is out of range. This Agent only has " +
            std::to_string(n_tools) + " tools."
        );

    model->actions_add(
        this, nullptr, tools[tool_idx] , status_new, queue, rm_tool_
        );

}

template<typename TSeq>
inline void Agent<TSeq>::rm_tool(
    ToolPtr<TSeq> & tool,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{

    if (tool->agent != this)
        throw std::logic_error("Cannot remove a virus from another agent!");

    model->actions_add(
        this, tool, nullptr , status_new, queue, rm_tool_
        );

}

template<typename TSeq>
inline void Agent<TSeq>::rm_virus(
    epiworld_fast_uint virus_idx,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{
    if (virus_idx >= n_viruses)
        throw std::range_error(
            "The Virus you want to remove is out of range. This Agent only has " +
            std::to_string(n_viruses) + " viruses."
        );
    else if (n_viruses == 0u)
        throw std::logic_error(
            "There is no virus to remove here!"
        );


    model->actions_add(
        this, viruses[virus_idx], nullptr, status_new, queue, default_rm_virus<TSeq>
        );
    
}

template<typename TSeq>
inline void Agent<TSeq>::rm_virus(
    VirusPtr<TSeq> & virus,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{

    if (virus->agent != this)
        throw std::logic_error("Cannot remove a virus from another agent!");

    model->actions_add(
        this, virus, nullptr, status_new, queue, default_rm_virus<TSeq>
        );


}

template<typename TSeq>
inline epiworld_double Agent<TSeq>::get_susceptibility_reduction(
    VirusPtr<TSeq> v
) {

    return model->susceptibility_reduction_mixer(this, v, model);
}

template<typename TSeq>
inline epiworld_double Agent<TSeq>::get_transmission_reduction(
    VirusPtr<TSeq> v
) {
    return model->transmission_reduction_mixer(this, v, model);
}

template<typename TSeq>
inline epiworld_double Agent<TSeq>::get_recovery_enhancer(
    VirusPtr<TSeq> v
) {
    return model->recovery_enhancer_mixer(this, v, model);
}

template<typename TSeq>
inline epiworld_double Agent<TSeq>::get_death_reduction(
    VirusPtr<TSeq> v
) {
    return model->death_reduction_mixer(this, v, model);
}

template<typename TSeq>
inline int Agent<TSeq>::get_id() const
{
    return id;
}

template<typename TSeq>
inline unsigned int Agent<TSeq>::get_index() const
{
    return index;
}

template<typename TSeq>
inline std::mt19937 * Agent<TSeq>::get_rand_endgine() {
    return model->get_rand_endgine();
}

template<typename TSeq>
inline Model<TSeq> * Agent<TSeq>::get_model() {
    return model;
}

template<typename TSeq>
inline Viruses<TSeq> Agent<TSeq>::get_viruses() {

    return Viruses<TSeq>(*this);

}

template<typename TSeq>
inline const Viruses_const<TSeq> Agent<TSeq>::get_viruses() const {

    return Viruses_const<TSeq>(*this);
    
}

template<typename TSeq>
inline VirusPtr<TSeq> & Agent<TSeq>::get_virus(int i) {
    return viruses.at(i);
}

template<typename TSeq>
inline size_t Agent<TSeq>::get_n_viruses() const noexcept
{
    return n_viruses;
}

template<typename TSeq>
inline Tools<TSeq> Agent<TSeq>::get_tools() {
    return Tools<TSeq>(*this);
}

template<typename TSeq>
inline const Tools_const<TSeq> Agent<TSeq>::get_tools() const {
    return Tools_const<TSeq>(*this);
}

template<typename TSeq>
inline ToolPtr<TSeq> & Agent<TSeq>::get_tool(int i)
{
    return tools.at(i);
}

template<typename TSeq>
inline size_t Agent<TSeq>::get_n_tools() const noexcept
{
    return n_tools;
}

template<typename TSeq>
inline void Agent<TSeq>::mutate_variant()
{

    for (auto & v : viruses)
        v->mutate();

}

template<typename TSeq>
inline void Agent<TSeq>::add_neighbor(
    Agent<TSeq> * p,
    bool check_source,
    bool check_target
) {
    // Can we find the neighbor?
    if (check_source)
    {

        bool found = false;
        for (auto & n: neighbors)    
            if (n->get_id() == p->get_id())
            {
                found = true;
                break;
            }

        if (!found)
            neighbors.push_back(p);

    } else 
        neighbors.push_back(p);

    if (check_target)
    {

        bool found = false;
        for (auto & n: p->neighbors)
            if (n->get_id() == id)
            {
                found = true;
                break;
            }

        if (!found)
            p->neighbors.push_back(this);
    
    } else 
        p->neighbors.push_back(this);
    

}

template<typename TSeq>
inline std::vector< Agent<TSeq> *> & Agent<TSeq>::get_neighbors()
{
    return neighbors;
}

template<typename TSeq>
inline void Agent<TSeq>::change_status(
    epiworld_fast_uint new_status,
    epiworld_fast_int queue
    )
{

    model->actions_add(
        this, nullptr, nullptr, new_status, queue, nullptr
    );
    
    return;

}

template<typename TSeq>
inline const epiworld_fast_uint & Agent<TSeq>::get_status() const {
    return status;
}

template<typename TSeq>
inline void Agent<TSeq>::reset()
{

    this->viruses.clear();
    n_viruses = 0u;

    this->tools.clear();
    n_tools = 0u;

    this->status = 0u;
    
}

template<typename TSeq>
inline bool Agent<TSeq>::has_tool(unsigned int t) const
{

    for (auto & tool : tools)
        if (tool->get_id() == t)
            return true;

    return false;

}

template<typename TSeq>
inline bool Agent<TSeq>::has_tool(std::string name) const
{

    for (auto & tool : tools)
        if (tool->get_name() == name)
            return true;

    return false;

}

template<typename TSeq>
inline bool Agent<TSeq>::has_virus(unsigned int t) const
{
    for (auto & v : viruses)
        if (v->get_id() == t)
            return true;

    return false;
}

template<typename TSeq>
inline bool Agent<TSeq>::has_virus(std::string name) const
{
    
    for (auto & v : viruses)
        if (v->get_name() == name)
            return true;

    return false;

}

#undef CHECK_COALESCE_

#endif