#ifndef EPIWORLD_NATURALHISTORY_MEAT_HPP
#define EPIWORLD_NATURALHISTORY_MEAT_HPP

template<typename TSeq>
inline int NaturalHistory<TSeq>::check_stage(int s) const
{
    if ((s < 0) || (s >= static_cast< int >(labels.size())))
        throw std::range_error(
            "Natural history \"" + name + "\" has no stage " +
            std::to_string(s) + "."
        );
    return s;
}

template<typename TSeq>
inline int NaturalHistory<TSeq>::add_stage(
    std::string label,
    UpdateFun<TSeq> fun
)
{

    for (const auto & l : labels)
        if (l == label)
            throw std::logic_error(
                "Natural history \"" + name + "\" already has a stage \"" +
                label + "\"."
            );

    labels.push_back(std::move(label));
    funs.push_back(std::move(fun));
    durations.push_back(nullptr);
    next_stage.push_back(-1);

    return static_cast< int >(labels.size()) - 1;

}

template<typename TSeq>
inline void NaturalHistory<TSeq>::set_duration(
    int s,
    StageDurationFun<TSeq> fun,
    int next
)
{
    durations[check_stage(s)] = std::move(fun);
    next_stage[s] = (next < 0) ? -1 : check_stage(next);
}

template<typename TSeq>
inline void NaturalHistory<TSeq>::set_infection(int start, int end)
{
    infection_stage = check_stage(start);
    recovery_stage  = check_stage(end);
}

template<typename TSeq>
inline int NaturalHistory<TSeq>::stage_of(std::string_view label) const
{
    for (size_t s = 0u; s < labels.size(); ++s)
        if (labels[s] == label)
            return static_cast< int >(s);

    throw std::logic_error(
        "Natural history \"" + name + "\" has no stage \"" +
        std::string(label) + "\"."
    );
}

template<typename TSeq>
inline void NaturalHistory<TSeq>::reset(size_t n)
{

    if ((infection_stage < 0) || (recovery_stage < 0))
        throw std::logic_error(
            "Natural history \"" + name + "\": set the infection and "
            "recovery stages with set_infection()."
        );

    stage.assign(n, 0);
    episode.assign(n, 0);
    until.assign(n, -1);
    entry.assign(n * labels.size(), -1);
    recorded.assign(n, 0);

}

template<typename TSeq>
inline void NaturalHistory<TSeq>::enter(
    Agent<TSeq> & p,
    int s,
    Model<TSeq> & m
)
{

    const size_t id = static_cast< size_t >(p.get_id());
    stage[id] = s;
    entry[id * labels.size() + static_cast< size_t >(s)] = m.today();
    until[id] = durations[s] ?
        m.today() + std::max(1, durations[s](&p, &m)) : -1;

}

template<typename TSeq>
inline void NaturalHistory<TSeq>::start_episode(
    Agent<TSeq> & p,
    Model<TSeq> & m
)
{

    const size_t id = static_cast< size_t >(p.get_id());
    episode[id]++;
    std::fill_n(entry.begin() + id * labels.size(), labels.size(), -1);
    enter(p, infection_stage, m);

}

template<typename TSeq>
inline void NaturalHistory<TSeq>::update(
    Agent<TSeq> & p,
    Model<TSeq> & m,
    size_t k
)
{

    const int s = stage[static_cast< size_t >(p.get_id())];
    if (funs[s])
    {
        funs[s](&p, &m);
        return;
    }

    const int next = next_stage[s];
    if ((next < 0) || !due(p, m))
        return;

    // Leaving for the recovery stage is losing the virus
    const auto & v = p.get_virus();
    if (
        (next == recovery_stage) && (v != nullptr) &&
        (v->get_natural_history() == static_cast< int >(k))
    )
        p.rm_virus(m);
    else
        p.change_stage(m, next, k);

}

template<typename TSeq>
inline int NaturalHistory<TSeq>::get_stage(const Agent<TSeq> & p) const
{
    return stage[static_cast< size_t >(p.get_id())];
}

template<typename TSeq>
inline int NaturalHistory<TSeq>::get_episode(const Agent<TSeq> & p) const
{
    return episode[static_cast< size_t >(p.get_id())];
}

template<typename TSeq>
inline int NaturalHistory<TSeq>::entered(const Agent<TSeq> & p, int s) const
{
    return entry[
        static_cast< size_t >(p.get_id()) * labels.size() +
        static_cast< size_t >(check_stage(s))
    ];
}

template<typename TSeq>
inline int NaturalHistory<TSeq>::get_until(const Agent<TSeq> & p) const
{
    return until[static_cast< size_t >(p.get_id())];
}

template<typename TSeq>
inline bool NaturalHistory<TSeq>::due(
    const Agent<TSeq> & p,
    const Model<TSeq> & m
) const
{
    const int u = until[static_cast< size_t >(p.get_id())];
    return (u >= 0) && (m.today() >= u);
}

#endif
