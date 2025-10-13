#ifndef EPIWORLD_CONTACT_TRACING_H
#define EPIWORLD_CONTACT_TRACING_H

class ContactTracing
{
private:

    std::vector< size_t > contact_matrix;
    std::vector< size_t > contacts_per_agent;
    std::vector< size_t > contact_date;

    size_t n_agents;
    size_t max_contacts;

    size_t get_location(size_t row, size_t col) const;

public:

    ContactTracing(size_t n_agents, size_t max_contacts);
    void add_contact(size_t agent_a, size_t agent_b, size_t day);
    size_t get_n_contacts(size_t agent);
    std::pair<size_t, size_t> get_contact(size_t agent, size_t idx);

    void reset();
};

inline size_t ContactTracing::get_location(size_t row, size_t col) const
{
    return col * n_agents + row;
}

inline ContactTracing::ContactTracing(size_t n_agents, size_t max_contacts)
{
    this->n_agents = n_agents;
    this->max_contacts = max_contacts;

    contact_matrix.resize(n_agents * max_contacts, 0u);
    contacts_per_agent.resize(n_agents, 0);
    contact_date.resize(n_agents * max_contacts, 0);
}

inline void ContactTracing::add_contact(size_t agent_a, size_t agent_b, size_t day)
{
    
    // Checking overflow
    size_t col_location = contacts_per_agent[agent_a] % max_contacts;
    size_t array_location = get_location(agent_a, col_location);

    contact_matrix[array_location] = agent_b;
    contact_date[array_location] = day;

    contacts_per_agent[agent_a] += 1;

}

inline size_t ContactTracing::get_n_contacts(size_t agent)
{
    return contacts_per_agent[agent];
}

inline std::pair< size_t, size_t> ContactTracing::get_contact(size_t agent, size_t idx)
{
    if (
        (idx >= contacts_per_agent[agent]) ||
        (idx >= max_contacts)
    )
        throw std::out_of_range("Index out of range in get_contact");

    size_t col_location = idx % max_contacts;
    size_t array_location = get_location(agent, col_location);

    return { contact_matrix[array_location], contact_date[array_location] };
}

inline void ContactTracing::reset()
{
    contact_matrix.assign(n_agents * max_contacts, 0u);
    contacts_per_agent.assign(n_agents, 0u);
    contact_date.assign(n_agents * max_contacts, 0u);
}

#endif
