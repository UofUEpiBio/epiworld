#ifndef EPIWORLD_PERSON_MEAT_HPP
#define EPIWORLD_PERSON_MEAT_HPP

// template<typename Ta>
// inline bool IN(Ta & a, std::vector< Ta > & b);

template<typename TSeq>
inline void default_add_virus(Person<TSeq> * p, Model<TSeq> * m)
{

    Virus<TSeq> * v = virus_tmp;

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
        p->viruses[n_viruses] = *(p->virus_tmp);
    else
        p->viruses.push_back(*(p->virus_tmp));

    p->viruses[n_viruses].host = p;
    p->viruses[n_viruses].date = m->today();


}

template<typename TSeq>
inline void default_add_tool(Person<TSeq> * p, Model<TSeq> * m)
{

    // Update tool accounting
    // m->get_db().state_change()
    
    // Adding the tool to the sequence
    p->tools.add_tool(m->today(), *p->tool_tmp);

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
    Tool<TSeq> tool,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
) {
    
    epiworld_fast_uint status_new_ =
        ((status_new < 0) ? status : static_cast<epiworld_fast_uint>(status_new));

    model->actions_add(this, status_new_, add_tool_, queue_);

}

template<typename TSeq>
inline void Person<TSeq>::add_virus(
    Virus<TSeq> * virus,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{

    epiworld_fast_uint status_new_ =
        ((status_new < 0) ? status : static_cast<epiworld_fast_uint>(status_new));

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
inline void Person<TSeq>::update_status()
{

    // No change if removed
    if (model->status_fun[status])
        model->status_fun[status](this, model);

    return;

}

template<typename TSeq>
inline void Person<TSeq>::update_status(
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
inline const epiworld_fast_uint & Person<TSeq>::get_status_next() const {
    return status_next;
}

template<typename TSeq>
inline void Person<TSeq>::reset() {

    this->viruses.reset();
    this->tools.reset();
    this->status      = model->baseline_status_susceptible;
    this->status_next = model->baseline_status_susceptible;

}

template<typename TSeq>
inline void Person<TSeq>::set_update_susceptible(UpdateFun<TSeq> fun) {
    update_susceptible = fun;
}

template<typename TSeq>
inline void Person<TSeq>::set_update_exposed(UpdateFun<TSeq> fun) {
    update_exposed = fun;
}

template<typename TSeq>
inline void Person<TSeq>::set_update_removed(UpdateFun<TSeq> fun) {
    update_removed = fun;
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