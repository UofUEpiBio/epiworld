#ifndef EPIWORLD_VIRUS_MEAT_HPP
#define EPIWORLD_VIRUS_MEAT_HPP

#include "config.hpp"

template<typename TSeq>
inline Virus<TSeq>::Virus()
{

    #ifdef EPI_DEBUG_VIRUS
    counter_construct++;
    #endif

}

template<typename TSeq>
inline Virus<TSeq>::Virus(
    std::string name
    ) {

    #ifdef EPI_DEBUG_VIRUS
    counter_construct++;
    #endif
    
    set_name(name);
}

#ifdef EPI_DEBUG_VIRUS
template<typename TSeq>
std::atomic<int> Virus<TSeq>::counter_construct = 0;

template<typename TSeq>
std::atomic<int> Virus<TSeq>::counter_copy_construct = 0;

template<typename TSeq>
std::atomic<int> Virus<TSeq>::counter_move_construct = 0;

template<typename TSeq>
std::atomic<int> Virus<TSeq>::counter_copy_assign = 0;

template<typename TSeq>
std::atomic<int> Virus<TSeq>::counter_move_assign = 0;

template<typename TSeq>
std::atomic<int> Virus<TSeq>::counter_destruct = 0;

template<typename TSeq>
inline Virus<TSeq>::~Virus()
{
    counter_destruct++;
}

// Copy constructor
template<typename TSeq>
inline Virus<TSeq>::Virus(const Virus<TSeq>& other)
    : agent(other.agent),
      virus_name(other.virus_name),
      date(other.date),
      id(other.id),
      lineage_id(other.lineage_id),
      lineage_bit(other.lineage_bit),
      param_redirects(other.param_redirects),
      nh_def(other.nh_def)
{
    counter_copy_construct++;
}

// Move constructor
template<typename TSeq>
inline Virus<TSeq>::Virus(Virus<TSeq>&& other) noexcept
    : agent(other.agent),
      virus_name(std::move(other.virus_name)),
      date(other.date),
      id(other.id),
      lineage_id(other.lineage_id),
      lineage_bit(other.lineage_bit),
      param_redirects(std::move(other.param_redirects)),
      nh_def(std::move(other.nh_def))
{
    counter_move_construct++;
    // other.agent = nullptr;
}

// Copy assignment
template<typename TSeq>
inline Virus<TSeq>& Virus<TSeq>::operator=(const Virus<TSeq>& other)
{
    if (this != &other) {
        agent = other.agent;
        virus_name = other.virus_name;
        date = other.date;
        id = other.id;
        lineage_id = other.lineage_id;
        lineage_bit = other.lineage_bit;
        param_redirects = other.param_redirects;
        nh_def = other.nh_def;
        counter_copy_assign++;
    }
    return *this;
}

// Move assignment
template<typename TSeq>
inline Virus<TSeq>& Virus<TSeq>::operator=(Virus<TSeq>&& other) noexcept
{
    if (this != &other) {
        agent = other.agent;
        virus_name = std::move(other.virus_name);
        date = other.date;
        id = other.id;
        lineage_id = other.lineage_id;
        lineage_bit = other.lineage_bit;
        param_redirects = std::move(other.param_redirects);
        nh_def = std::move(other.nh_def);
        other.agent = nullptr;
        counter_move_assign++;
    }
    return *this;
}
#endif

template<typename TSeq>
inline void Virus<TSeq>::mutate(
    Model<TSeq> * model
) {

    if (mutation)
        if (mutation(agent, *this, model))
        {
            check_params(*model);
            model->get_db().record_virus(*this);
        }

    return;
    
}

template<typename TSeq>
inline void Virus<TSeq>::set_mutation(
    MutFun<TSeq> fun
) {
    mutation = MutFun<TSeq>(fun);
}


template<typename TSeq>
inline Agent<TSeq> * Virus<TSeq>::get_agent()
{

    return agent;

}

template<typename TSeq>
inline void Virus<TSeq>::set_agent(Agent<TSeq> * p)
{
    agent = p;
}

template<typename TSeq>
inline void Virus<TSeq>::set_id(int idx)
{

    id = idx;
    return;

}

template<typename TSeq>
inline int Virus<TSeq>::get_id() const
{
    
    return id;

}

template<typename TSeq>
inline int Virus<TSeq>::get_lineage_id() const
{
    return lineage_id;
}

template<typename TSeq>
inline void Virus<TSeq>::set_date(int d) 
{

    date = d;
    return;

}

template<typename TSeq>
inline int Virus<TSeq>::get_date() const
{
    
    return date;
    
}

template<typename TSeq>
inline void Virus<TSeq>::set_name(std::string name)
{

    virus_name = name;

}

template<typename TSeq>
inline std::string Virus<TSeq>::get_name() const
{

    return virus_name;

}

template<typename TSeq>
inline bool Virus<TSeq>::operator==(const Virus<TSeq> & other) const
{
    
    EPI_DEBUG_FAIL_AT_TRUE(
        virus_name != other.virus_name,
        "Virus:: virus_name don't match"
    )
    






    return true;

}

template<typename TSeq>
inline void Virus<TSeq>::print() const
{

    printf_epiworld("Virus         : %s\n", virus_name.c_str());
    printf_epiworld("Id            : %s\n", (id < 0)? std::string("(empty)").c_str() : std::to_string(id).c_str());

}

template<typename TSeq>
inline std::unique_ptr<Virus<TSeq>> Virus<TSeq>::clone_ptr() const
{
    auto cloned = std::make_unique<Virus<TSeq>>(*this);
    return cloned;
}

template<typename TSeq>
inline void Virus<TSeq>::use_param(std::string param, std::string replacement)
{
    for (auto & r : param_redirects)
        if (r.first.name() == param)
        {
            r.second = ParamRef(std::move(replacement));
            return;
        }

    param_redirects.emplace_back(
        ParamRef(std::move(param)), ParamRef(std::move(replacement))
    );
}

template<typename TSeq>
inline ParamId Virus<TSeq>::param_redirect(
    ParamId id,
    const Model<TSeq> & model
) const
{
    for (const auto & r : param_redirects)
        if (r.first.id(model).idx == id.idx)
            return r.second.id(model);

    return id;
}

template<typename TSeq>
inline NaturalHistory<TSeq> & Virus<TSeq>::nh_edit()
{
    // A virus in a model (its copy there, or an agent's) keeps the model's
    // stages and transitions; a strain changes parameters instead
    if (id >= 0)
        throw std::logic_error(
            "The virus \"" + virus_name + "\" is in a model: its natural "
            "history cannot change (a strain reads other parameters with "
            "use_param())."
        );

    // Copies share the natural history until one of them is edited
    if (!nh_def)
        nh_def = std::make_shared< NaturalHistory<TSeq> >(virus_name);
    else if (nh_def.use_count() > 1)
        nh_def = std::make_shared< NaturalHistory<TSeq> >(*nh_def);

    return *nh_def;
}

template<typename TSeq>
inline int Virus<TSeq>::add_stage(std::string label, unsigned int flags)
{
    return nh_edit().add_stage(std::move(label), flags);
}

template<typename TSeq>
inline void Virus<TSeq>::set_contact(int s, epiworld_double weight)
{
    nh_edit().set_contact(s, weight);
}

template<typename TSeq>
inline TransitionRef<TSeq> Virus<TSeq>::infection(
    Nodes from, int to, Value<TSeq> per_contact
)
{
    return nh_edit().infection(std::move(from), to, std::move(per_contact));
}

template<typename TSeq>
inline TransitionRef<TSeq> Virus<TSeq>::rate(
    Nodes from, int to, Value<TSeq> daily_probability
)
{
    return nh_edit().rate(std::move(from), to, std::move(daily_probability));
}

template<typename TSeq>
inline TransitionRef<TSeq> Virus<TSeq>::after(int from, int to, Duration<TSeq> d)
{
    return nh_edit().after(from, to, std::move(d));
}

template<typename TSeq>
inline TransitionRef<TSeq> Virus<TSeq>::external(
    Nodes from, int to, std::string label
)
{
    return nh_edit().external(std::move(from), to, std::move(label));
}

template<typename TSeq>
inline void Virus<TSeq>::check_params(const Model<TSeq> & model) const
{
    for (const auto & r : param_redirects)
        if (!model.has_param(r.second.name()))
            throw std::logic_error(
                "The virus \"" + virus_name + "\" reads the parameter \"" +
                r.second.name() + "\" in place of \"" + r.first.name() +
                "\", but the model has no such parameter (add it with "
                "add_param())."
            );
}

#endif
