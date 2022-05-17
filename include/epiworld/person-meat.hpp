#ifndef EPIWORLD_PERSON_MEAT_HPP
#define EPIWORLD_PERSON_MEAT_HPP

// template<typename Ta>
// inline bool IN(Ta & a, std::vector< Ta > & b);

template<typename TSeq>
inline void default_add_virus(Action<TSeq> & a, Model<TSeq> * m)
{

    Person<TSeq> * p = a.person;
    std::shared_ptr< Virus<TSeq> > v = p->virus_tmp;

    // Has a host? If so, we need to register the transmission
    if (v->get_host())
    {

        // ... only if not the same person
        if (v->get_host()->get_id() != v->get_id())
            m->get_db().record_transmission(
                v->get_host()->get_id(), p->get_id(), v->get_id() 
            );

    }
    
    // Update virus accounting
    p->n_viruses++;
    size_t n_viruses = p->n_viruses;

    if (n_viruses-- >= p->viruses.size())
        p->viruses[n_viruses] = std::make_shared< Virus<TSeq> >(*v);
    else
        p->viruses.push_back(std::make_shared< Virus<TSeq> >(*v));

    // Notice that both host and date can be changed in this case
    // as only the sequence is a shared_ptr itself.
    p->viruses[n_viruses].host = p;
    p->viruses[n_viruses].date = m->today();


}

template<typename TSeq>
inline void default_add_tool(Action<TSeq> & a, Model<TSeq> * m)
{

    Person<TSeq> * p = a.person;
    std::shared_ptr< Tool<TSeq> > t = p->tool_tmp;
    
    // Update tool accounting
    p->n_tools++;
    size_t n_tools = p->n_tools;

    if (n_tools-- >= p->tools.size())
        p->tools[n_tools] = std::make_shared< Tool<TSeq> >(*t);
    else
        p->tools.push_back(std::make_shared< Tool<TSeq> >(*t));

    p->tools[n_tools].date = m->today();

}

template<typename TSeq>
inline void default_rm_virus(Action<TSeq> & a, Model<TSeq> * m)
{

    Person<TSeq> * p = a.person;    

    if (--p->n_viruses > 0)
        std::swap(p->viruses[p->virus_to_remove_idx], p->viruses[p->n_viruses - 1]);

    return;

}

template<typename TSeq>
inline void default_rm_tool(Action<TSeq> & a, Model<TSeq> * m)
{

    Person<TSeq> * p = a.person;    

    if (--p->n_viruses > 0)
        std::swap(p->viruses[p->virus_to_remove_idx], p->viruses[p->n_viruses - 1]);

    return;

}

template<typename TSeq>
inline Person<TSeq>::Person()
{
    
}

template<typename TSeq>
inline void Person<TSeq>::init(epiworld_fast_uint baseline_status)
{
    tools.person = this;
    viruses.host = this;
    status       = baseline_status;
    status_next  = baseline_status;
}
    

template<typename TSeq>
inline void Person<TSeq>::add_tool(
    std::shared_ptr< Tool<TSeq> > tool,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
) {
    
    epiworld_fast_uint status_new_ =
        ((status_new < 0) ? status : static_cast<epiworld_fast_uint>(status_new));

    tool_tmp = tool;

    model->actions_add(this, status_new_, add_tool_, queue_);

}

template<typename TSeq>
inline void Person<TSeq>::add_virus(
    std::shared_ptr< Virus<TSeq> > virus,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{

    epiworld_fast_uint status_new_ =
        ((status_new < 0) ? status : static_cast<epiworld_fast_uint>(status_new));

    virus_tmp = virus;

    model->actions_add(this, status_new_, add_virus_, queue);

}

template<typename TSeq>
inline epiworld_double Person<TSeq>::get_susceptibility_reduction(
    Virus<TSeq> * v
) {
    return tools.get_susceptibility_reduction(v);
}

template<typename TSeq>
inline epiworld_double Person<TSeq>::get_transmission_reduction(
    Virus<TSeq> * v
) {
    return tools.get_transmission_reduction(v);
}

template<typename TSeq>
inline epiworld_double Person<TSeq>::get_recovery_enhancer(
    Virus<TSeq> * v
) {
    return tools.get_recovery_enhancer(v);
}

template<typename TSeq>
inline epiworld_double Person<TSeq>::get_death_reduction(
    Virus<TSeq> * v
) {
    return tools.get_death_reduction(v);
}

template<typename TSeq>
inline int Person<TSeq>::get_id() const
{
    return id;
}

template<typename TSeq>
inline unsigned int Person<TSeq>::get_index() const
{
    return index;
}

template<typename TSeq>
inline std::mt19937 * Person<TSeq>::get_rand_endgine() {
    return model->get_rand_endgine();
}

template<typename TSeq>
inline Model<TSeq> * Person<TSeq>::get_model() {
    return model;
}

template<typename TSeq>
inline PersonViruses<TSeq> & Person<TSeq>::get_viruses() {
    return viruses;
}

template<typename TSeq>
inline Virus<TSeq> & Person<TSeq>::get_virus(int i) {
    return viruses(i);
}

template<typename TSeq>
inline PersonTools<TSeq> & Person<TSeq>::get_tools() {
    return tools;
}

template<typename TSeq>
inline Tool<TSeq> & Person<TSeq>::get_tool(int i) {
    return tools(i);
}

template<typename TSeq>
inline void Person<TSeq>::mutate_variant() {
    viruses.mutate();
}

template<typename TSeq>
inline void Person<TSeq>::add_neighbor(
    Person<TSeq> * p,
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
inline std::vector< Person<TSeq> *> & Person<TSeq>::get_neighbors()
{
    return neighbors;
}

template<typename TSeq>
inline void Person<TSeq>::change_status(
    epiworld_fast_uint new_status,
    epiworld_fast_int queue
    )
{

    model->actions_add(
        this, new_status, nullptr, queue
    );
    
    return;

}

template<typename TSeq>
inline const epiworld_fast_uint & Person<TSeq>::get_status() const {
    return status;
}

template<typename TSeq>
inline void Person<TSeq>::reset() {

    this->viruses.reset();
    this->tools.reset();
    this->status      = model->baseline_status_susceptible;
    this->status_next = model->baseline_status_susceptible;

}

template<typename TSeq>
inline bool Person<TSeq>::has_tool(unsigned int t) const {
    return tools.has_tool(t);
}

template<typename TSeq>
inline bool Person<TSeq>::has_tool(std::string name) const {
    return tools.has_tool(name);
}

template<typename TSeq>
inline bool Person<TSeq>::has_virus(unsigned int t) const {
    return viruses.has_virus(t);
}

template<typename TSeq>
inline bool Person<TSeq>::has_virus(std::string name) const {
    return viruses.has_virus(name);
}

#endif