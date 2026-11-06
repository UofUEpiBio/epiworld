#ifndef EPIWORLD_NATURALHISTORY_MEAT_HPP
#define EPIWORLD_NATURALHISTORY_MEAT_HPP

// Value -----------------------------------------------------------------------

template<typename TSeq>
inline Value<TSeq> Value<TSeq>::inverse(std::string name)
{
    Value<TSeq> v(std::move(name));
    v.kind = Kind::inverse;
    return v;
}

template<typename TSeq>
inline Value<TSeq> Value<TSeq>::complement(std::string name)
{
    Value<TSeq> v(std::move(name));
    v.kind = Kind::complement;
    return v;
}

template<typename TSeq>
inline epiworld_double Value<TSeq>::eval(
    Agent<TSeq> & agent,
    const Virus<TSeq> * virus,
    Model<TSeq> & model
) const
{

    switch (kind)
    {
    case Kind::number:
        return number;
    case Kind::fun:
        return fun(agent, model);
    case Kind::none:
        throw std::logic_error("A value of a transition is not set.");
    default:
        break;
    }

    ParamId id = param.id(model);
    if (virus != nullptr)
        id = virus->param_redirect(id, model);

    const epiworld_double x = model.par_at(id);

    if (kind == Kind::inverse)
        return 1.0 / x;
    if (kind == Kind::complement)
        return 1.0 - x;

    return x;

}

template<typename TSeq>
inline epiworld_double Value<TSeq>::eval(
    Agent<TSeq> & agent,
    Model<TSeq> & model
) const
{
    const auto & v = agent.get_virus();
    return eval(agent, v ? v.get() : nullptr, model);
}

// Duration --------------------------------------------------------------------

template<typename TSeq>
inline Duration<TSeq> Duration<TSeq>::fixed(Value<TSeq> days)
{
    Duration<TSeq> d;
    d.kind = Kind::fixed;
    d.mean = std::move(days);
    return d;
}

template<typename TSeq>
inline Duration<TSeq> Duration<TSeq>::geometric(Value<TSeq> mean)
{
    Duration<TSeq> d;
    d.kind = Kind::geometric;
    d.mean = std::move(mean);
    return d;
}

template<typename TSeq>
inline Duration<TSeq> Duration<TSeq>::lognormal(Value<TSeq> mean, Value<TSeq> sd)
{
    Duration<TSeq> d;
    d.kind = Kind::lognormal;
    d.mean = std::move(mean);
    d.sd = std::move(sd);
    return d;
}

template<typename TSeq>
inline Duration<TSeq> Duration<TSeq>::gamma(Value<TSeq> mean, Value<TSeq> sd)
{
    Duration<TSeq> d;
    d.kind = Kind::gamma;
    d.mean = std::move(mean);
    d.sd = std::move(sd);
    return d;
}

template<typename TSeq>
inline Duration<TSeq> Duration<TSeq>::since(int s) const
{
    Duration<TSeq> d(*this);
    d.anchor = s;
    return d;
}

template<typename TSeq>
inline int Duration<TSeq>::draw(Agent<TSeq> & agent, Model<TSeq> & model) const
{

    auto whole = [](epiworld_double days) -> int {
        if (!(days < static_cast< epiworld_double >(INT_MAX / 2)))
            return INT_MAX / 2;
        return std::max(1, static_cast< int >(std::lround(days)));
    };

    switch (kind)
    {
    case Kind::fun:
        return std::max(1, fun(agent, model));

    case Kind::fixed:
        return whole(mean.eval(agent, model));

    case Kind::geometric:
    {
        // Days until the first success of a daily Bernoulli(1 / mean): the
        // same as leaving each day with that probability.
        const epiworld_double m = mean.eval(agent, model);
        if (!(m > 1.0))
            return 1;
        return 1 + model.rgeom(1.0 / m);
    }

    case Kind::lognormal:
    {
        const epiworld_double m = mean.eval(agent, model);
        const epiworld_double s = sd.eval(agent, model);
        if (!(s > 0.0) || !(m > 0.0))
            return whole(m);

        // A standard normal from two uniforms (Box-Muller), which keeps no
        // state between calls, so a run's draws depend only on its seed.
        const epiworld_double u1 = 1.0 - model.runif(); // in (0, 1]
        const epiworld_double u2 = model.runif();
        const epiworld_double z  = std::sqrt(-2.0 * std::log(u1)) *
            std::cos(2.0 * M_PI * u2);

        const epiworld_double s2 = std::log(1.0 + (s * s) / (m * m));
        return whole(std::exp(std::log(m) - s2 / 2.0 + std::sqrt(s2) * z));
    }

    case Kind::gamma:
    {
        const epiworld_double m = mean.eval(agent, model);
        const epiworld_double s = sd.eval(agent, model);
        if (!(s > 0.0) || !(m > 0.0))
            return whole(m);

        // Shape m^2 / s^2 and scale s^2 / m give mean m and sd s
        return whole(model.rgamma((m * m) / (s * s), (s * s) / m));
    }

    default:
        throw std::logic_error("A duration of a transition is not set.");
    }

}

// TransitionRef ---------------------------------------------------------------

template<typename TSeq>
inline TransitionRef<TSeq> & TransitionRef<TSeq>::name(std::string n)
{
    for (auto & t : nh->transitions)
        if ((t.name == n) && (&t != &nh->transitions[id]))
            throw std::logic_error(
                "The natural history \"" + nh->name +
                "\" already has a transition named \"" + n + "\"."
            );

    nh->transitions[id].name = std::move(n);
    return *this;
}

template<typename TSeq>
inline TransitionRef<TSeq> & TransitionRef<TSeq>::effect(Effect e)
{
    auto & t = nh->transitions[id];
    if ((e != Effect::none) && (t.kind != TransitionKind::rate))
        throw std::logic_error(
            "Only a rate takes a tool effect (transition \"" + t.name + "\")."
        );

    t.effect = e;
    return *this;
}

template<typename TSeq>
inline TransitionRef<TSeq> & TransitionRef<TSeq>::to(
    std::vector< std::pair< int, Value<TSeq> > > targets
)
{
    auto & t = nh->transitions[id];
    if (targets.size() < 2u)
        throw std::logic_error(
            "A branch needs two or more targets (transition \"" + t.name +
            "\")."
        );

    t.branch_to.clear();
    t.branch_prob.clear();
    for (auto & target : targets)
    {
        nh->check_stage(target.first, "target");
        t.branch_to.push_back(target.first);
        t.branch_prob.push_back(std::move(target.second));
    }

    t.to = t.branch_to[0u];
    return *this;
}

template<typename TSeq>
inline TransitionRef<TSeq> & TransitionRef<TSeq>::sampler(InfectionSampler<TSeq> f)
{
    auto & t = nh->transitions[id];
    if (t.kind != TransitionKind::infection)
        throw std::logic_error(
            "Only an infection takes a sampler (transition \"" + t.name + "\")."
        );

    t.sampler = std::move(f);
    return *this;
}

template<typename TSeq>
inline TransitionRef<TSeq> & TransitionRef<TSeq>::when(Nodes stages)
{
    auto & t = nh->transitions[id];
    if (!nh->status_chart)
        throw std::logic_error(
            "Only a status transition takes when() (transition \"" + t.name +
            "\")."
        );
    t.when = std::move(stages.ids);
    return *this;
}

// NaturalHistory --------------------------------------------------------------

template<typename TSeq>
inline NaturalHistory<TSeq>::NaturalHistory(std::string name_) :
    name(std::move(name_)) {}

template<typename TSeq>
inline void NaturalHistory<TSeq>::check_stage(int s, const char * what) const
{
    if ((s < 0) || (static_cast< size_t >(s) >= stage_labels.size()))
        throw std::range_error(
            std::string("The ") + what + " stage " + std::to_string(s) +
            " is not a stage of the natural history \"" + name + "\" (it has " +
            std::to_string(stage_labels.size()) + ")."
        );
}

template<typename TSeq>
inline int NaturalHistory<TSeq>::add_stage(std::string label, unsigned int flags)
{

    for (auto & l : stage_labels)
        if (l == label)
            throw std::logic_error(
                "The natural history \"" + name + "\" already has a stage \"" +
                label + "\"."
            );

    if ((flags & ~stage::infectious) != 0u)
        throw std::invalid_argument(
            "Unknown stage flags for \"" + label +
            "\": use stage::carrier and stage::infectious."
        );

    stage_labels.push_back(std::move(label));
    stage_flags.push_back(flags);
    stage_contact.push_back(1.0);
    finalized = false;

    return static_cast< int >(stage_labels.size() - 1u);

}

template<typename TSeq>
inline void NaturalHistory<TSeq>::set_contact(int s, epiworld_double weight)
{
    check_stage(s, "");

    // Fractional weights come with the statuses (NH-2)
    if ((weight != 0.0) && (weight != 1.0))
        throw std::invalid_argument(
            "For now, a stage's contact weight is 0 or 1 (stage \"" +
            stage_labels[s] + "\")."
        );

    stage_contact[s] = weight;
}

template<typename TSeq>
inline int NaturalHistory<TSeq>::add_transition(Transition<TSeq> t)
{

    if (t.from.empty())
        throw std::logic_error("A transition needs at least one source stage.");

    for (int s : t.from)
        check_stage(s, "source");

    check_stage(t.to, "target");

    // Two default names may coincide until one is renamed; finalize() checks
    if (t.name.empty())
        t.name = stage_labels[t.from[0u]] + " -> " + stage_labels[t.to];
    else
        for (auto & other : transitions)
            if (other.name == t.name)
                throw std::logic_error(
                    "The natural history \"" + name + "\" already has a "
                    "transition named \"" + t.name + "\"."
                );

    transitions.push_back(std::move(t));
    finalized = false;

    return static_cast< int >(transitions.size() - 1u);

}

template<typename TSeq>
inline TransitionRef<TSeq> NaturalHistory<TSeq>::infection(
    Nodes from, int to, Value<TSeq> per_contact
)
{
    Transition<TSeq> t;
    t.kind = TransitionKind::infection;
    t.from = std::move(from.ids);
    t.to = to;
    t.value = std::move(per_contact);
    return TransitionRef<TSeq>(this, add_transition(std::move(t)));
}

template<typename TSeq>
inline TransitionRef<TSeq> NaturalHistory<TSeq>::rate(
    Nodes from, int to, Value<TSeq> daily_probability
)
{
    Transition<TSeq> t;
    t.kind = TransitionKind::rate;
    t.from = std::move(from.ids);
    t.to = to;
    t.value = std::move(daily_probability);
    return TransitionRef<TSeq>(this, add_transition(std::move(t)));
}

template<typename TSeq>
inline TransitionRef<TSeq> NaturalHistory<TSeq>::after(
    int from, int to, Duration<TSeq> d
)
{
    if (d.get_kind() == Duration<TSeq>::Kind::none)
        throw std::logic_error("A duration transition needs a duration.");

    Transition<TSeq> t;
    t.kind = TransitionKind::after;
    t.from = {from};
    t.to = to;
    t.duration = std::move(d);
    return TransitionRef<TSeq>(this, add_transition(std::move(t)));
}

template<typename TSeq>
inline TransitionRef<TSeq> NaturalHistory<TSeq>::external(
    Nodes from, int to, std::string label
)
{
    Transition<TSeq> t;
    t.kind = TransitionKind::external;
    t.from = std::move(from.ids);
    t.to = to;
    t.name = std::move(label);
    return TransitionRef<TSeq>(this, add_transition(std::move(t)));
}

template<typename TSeq>
inline int NaturalHistory<TSeq>::stage_id(std::string_view label) const
{
    for (size_t s = 0u; s < stage_labels.size(); ++s)
        if (stage_labels[s] == label)
            return static_cast< int >(s);

    throw std::logic_error(
        "The natural history \"" + name + "\" has no stage \"" +
        std::string(label) + "\"."
    );
}

template<typename TSeq>
inline int NaturalHistory<TSeq>::transition_id(std::string_view label) const
{
    for (size_t t = 0u; t < transitions.size(); ++t)
        if (transitions[t].name == label)
            return status_chart ? (-2 - static_cast< int >(t)) : static_cast< int >(t);

    throw std::logic_error(
        "The natural history \"" + name + "\" has no transition \"" +
        std::string(label) + "\"."
    );
}

template<typename TSeq>
inline bool NaturalHistory<TSeq>::is_source(int transition, int s) const
{
    for (int f : transitions[transition].from)
        if (f == s)
            return true;
    return false;
}

template<typename TSeq>
inline bool NaturalHistory<TSeq>::same_structure(
    const NaturalHistory<TSeq> & o
) const
{

    if (
        (stage_labels != o.stage_labels) ||
        (stage_flags != o.stage_flags) ||
        (stage_contact != o.stage_contact) ||
        (transitions.size() != o.transitions.size())
    )
        return false;

    for (size_t k = 0u; k < transitions.size(); ++k)
    {
        const auto & a = transitions[k];
        const auto & b = o.transitions[k];
        if (
            (a.name != b.name) || (a.kind != b.kind) || (a.from != b.from) ||
            (a.to != b.to) || (a.branch_to != b.branch_to) ||
            (a.effect != b.effect) ||
            (static_cast< bool >(a.sampler) != static_cast< bool >(b.sampler))
        )
            return false;
    }

    return true;

}

template<typename TSeq>
inline void NaturalHistory<TSeq>::finalize(
    Model<TSeq> & model,
    const std::string & owner
)
{

    if (stage_labels.empty())
        throw std::logic_error(
            "The natural history \"" + name + "\" has no stages."
        );

    const size_t ns = stage_labels.size();
    stages.assign(ns, StageInfo());

    for (size_t a = 0u; a < transitions.size(); ++a)
        for (size_t b = a + 1u; b < transitions.size(); ++b)
            if (transitions[a].name == transitions[b].name)
                throw std::logic_error(
                    "The natural history \"" + name + "\" has two transitions "
                    "named \"" + transitions[a].name + "\"; name one of them "
                    "with .name()."
                );

    // A number becomes a model parameter, named after the transition
    auto as_param = [&model](Value<TSeq> & v, const std::string & pname) {
        if (v.kind != Value<TSeq>::Kind::number)
            return;
        model.add_param(v.number, pname, true);
        v = Value<TSeq>(pname);
    };

    for (size_t id = 0u; id < transitions.size(); ++id)
    {

        auto & t = transitions[id];
        const std::string where = "transition \"" + t.name + "\"";

        // The targets of a branch are all carriers or all not
        std::vector< int > targets = t.branch_to.empty() ?
            std::vector< int >({t.to}) : t.branch_to;

        for (int target : targets)
            if (is_carrier(target) != is_carrier(t.to))
                throw std::logic_error(
                    "The targets of the " + where + " must be all carrier "
                    "stages or all non-carrier stages."
                );

        for (int s : t.from)
        {

            if (is_carrier(s) != is_carrier(t.from[0u]))
                throw std::logic_error(
                    "The sources of the " + where + " must be all carrier "
                    "stages or all non-carrier stages."
                );

            if (t.kind == TransitionKind::infection)
            {

                if (is_carrier(s))
                    throw std::logic_error(
                        "The " + where + " leaves a carrier stage (\"" +
                        stage_labels[s] + "\"); an infection leaves a stage "
                        "without the virus."
                    );

                if (stages[s].infection >= 0)
                    throw std::logic_error(
                        "The stage \"" + stage_labels[s] + "\" has two "
                        "infection transitions."
                    );

                stages[s].infection = static_cast< int >(id);

            }
            else if (!is_carrier(s) && is_carrier(t.to))
                throw std::logic_error(
                    "The " + where + " gives the virus without an infection: "
                    "only an infection (or seeding) enters a carrier stage "
                    "from a stage without the virus."
                );

            if (t.kind == TransitionKind::rate)
                stages[s].rates.push_back(static_cast< int >(id));

            if (t.kind == TransitionKind::after)
            {
                if (stages[s].after >= 0)
                    throw std::logic_error(
                        "The stage \"" + stage_labels[s] + "\" has two "
                        "durations; a stage has at most one."
                    );
                stages[s].after = static_cast< int >(id);
            }

        }

        if ((t.kind == TransitionKind::infection) && !is_carrier(t.to))
            throw std::logic_error(
                "The " + where + " must lead to a carrier stage."
            );

        // An anchor is a disease stage: the model checks the statuses' own
        // against the virus (as it does `.when()`)
        if (
            !status_chart &&
            (t.kind == TransitionKind::after) &&
            (t.duration.get_anchor() >= 0)
        )
            check_stage(t.duration.get_anchor(), "anchor");

        // Numbers become parameters
        const std::string pname = owner + ": " + t.name;
        as_param(t.value, pname);
        as_param(t.duration.mean, pname + " (mean)");
        as_param(t.duration.sd, pname + " (sd)");
        for (size_t k = 0u; k < t.branch_prob.size(); ++k)
            as_param(
                t.branch_prob[k],
                pname + " (to " + stage_labels[t.branch_to[k]] + ")"
            );

    }

    for (auto & s : stages)
    {
        s.needs_visit = !s.rates.empty() || (s.after >= 0);
        s.pushable = (s.infection >= 0) && !s.needs_visit &&
            !transitions[s.infection].sampler;
    }

    finalized = true;

}

#endif
