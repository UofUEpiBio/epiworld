#ifndef EPIWORLD_INTERVENTION_MEAT_HPP
#define EPIWORLD_INTERVENTION_MEAT_HPP

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------

template<typename TSeq>
inline Intervention<TSeq>::Intervention(std::string name, int day)
{
    this->name_ = name;
    this->day_  = day;
}

// -----------------------------------------------------------------------------
// Lifecycle hooks
// -----------------------------------------------------------------------------

template<typename TSeq>
inline void Intervention<TSeq>::setup(Model<TSeq>* /* m */)
{
    // Default: no-op.
    // Override to resize per-agent arrays, register compliance flags, etc.
}

template<typename TSeq>
inline void Intervention<TSeq>::update(Model<TSeq>* m)
{
    // Default loop: check every agent and either release or apply the
    // intervention as appropriate.
    for (auto & agent : m->get_agents())
    {
        if (should_release(&agent, m))
            release(&agent, m);
        else if (applies_to(&agent, m))
            apply(&agent, m);
    }
}

template<typename TSeq>
inline void Intervention<TSeq>::operator()(Model<TSeq>* m, int day)
{
    // Apply the day filter: a negative day means "run every day".
    if (this->day_ >= 0 && this->day_ != day)
        return;

    this->update(m);
}

// -----------------------------------------------------------------------------
// Per-agent helpers (all default to no-op / false)
// -----------------------------------------------------------------------------

template<typename TSeq>
inline bool Intervention<TSeq>::applies_to(
    Agent<TSeq>* /* a */, Model<TSeq>* /* m */)
{
    return false;
}

template<typename TSeq>
inline void Intervention<TSeq>::apply(
    Agent<TSeq>* /* a */, Model<TSeq>* /* m */)
{
    // Default: no-op. Override to apply the intervention to an agent.
}

template<typename TSeq>
inline bool Intervention<TSeq>::should_release(
    Agent<TSeq>* /* a */, Model<TSeq>* /* m */)
{
    return false;
}

template<typename TSeq>
inline void Intervention<TSeq>::release(
    Agent<TSeq>* /* a */, Model<TSeq>* /* m */)
{
    // Default: no-op. Override to release an agent from the intervention.
}

// -----------------------------------------------------------------------------
// Metadata
// -----------------------------------------------------------------------------

template<typename TSeq>
inline void Intervention<TSeq>::set_name(std::string name)
{
    this->name_ = name;
}

template<typename TSeq>
inline std::string Intervention<TSeq>::get_name() const
{
    return this->name_;
}

template<typename TSeq>
inline void Intervention<TSeq>::set_day(int day)
{
    this->day_ = day;
}

template<typename TSeq>
inline int Intervention<TSeq>::get_day() const
{
    return this->day_;
}

template<typename TSeq>
inline void Intervention<TSeq>::print() const
{
    printf_epiworld(
        "Intervention: %s (day = %d)\n",
        name_.c_str(),
        day_
    );
}

// -----------------------------------------------------------------------------
// Comparison operators
// -----------------------------------------------------------------------------

template<typename TSeq>
inline bool Intervention<TSeq>::operator==(
    const Intervention<TSeq>& other) const
{
    return name_ == other.name_ && day_ == other.day_;
}

template<typename TSeq>
inline bool Intervention<TSeq>::operator!=(
    const Intervention<TSeq>& other) const
{
    return !(*this == other);
}

// -----------------------------------------------------------------------------
// Polymorphic clone
// -----------------------------------------------------------------------------

template<typename TSeq>
inline std::unique_ptr<Intervention<TSeq>>
Intervention<TSeq>::clone_ptr() const
{
    return std::make_unique<Intervention<TSeq>>(*this);
}

#endif
