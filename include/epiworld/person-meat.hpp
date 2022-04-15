#ifndef EPIWORLD_PERSON_MEAT_HPP
#define EPIWORLD_PERSON_MEAT_HPP

// template<typename Ta>
// inline bool IN(Ta & a, std::vector< Ta > & b);

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
    int d,
    Tool<TSeq> tool
) {
    tools.add_tool(d, tool);
}

template<typename TSeq>
inline void Person<TSeq>::add_virus(
    Virus<TSeq> * virus
)
{

    model->virus_to_add.push_back(virus);
    model->virus_to_add_person.push_back(this);

    if (model->is_queuing_on())
        model->get_queue() += this;

}

template<typename TSeq>
inline void Person<TSeq>::rm_virus(
    Virus<TSeq> * virus
)
{

    model->virus_to_remove.push_back(virus);

    if (model->is_queuing_on())
        model->get_queue() -= this;

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
    if (IN(status, model->status_removed))
    {
        if (update_removed)
            status_next = update_removed(this, model);

    } else if (IN(status, model->status_susceptible)) {
        
        if (!update_susceptible)
            throw std::logic_error("No update_susceptible function?!");

        if (update_susceptible)
            status_next = update_susceptible(this, model);

    } else if (IN(status, model->status_exposed)) {

        if (update_exposed)
            status_next = update_exposed(this, model);

    } else
        throw std::range_error(
            "The reported status " + std::to_string(status) + " is not valid.");

    return;

}

template<typename TSeq>
inline void Person<TSeq>::update_status(epiworld_fast_uint new_status)
{

    if (new_status == status)
        return;

    // No change if removed
    bool status_ok = 
        IN(new_status, model->status_removed) |
        IN(new_status, model->status_susceptible) |
        IN(new_status, model->status_exposed);

    if (!status_ok)
        throw std::range_error(
            "The reported status " + std::to_string(new_status) + " is not valid.");

    status_next = new_status;
    
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

template<typename TSeq>
inline bool Person<TSeq>::visited() const
{
    
    return visited_person == model->visited_model;

}

template<typename TSeq>
inline void Person<TSeq>::toggle_visited()
{
    visited_person = !visited_person;
}

#endif