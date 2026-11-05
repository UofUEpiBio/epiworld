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
            return static_cast< int >(t);

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
inline void NaturalHistory<TSeq>::finalize(Model<TSeq> & model)
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

        if (
            (t.kind == TransitionKind::after) &&
            (t.duration.get_anchor() >= 0)
        )
            check_stage(t.duration.get_anchor(), "anchor");

        // Numbers become parameters
        const std::string pname = name + ": " + t.name;
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

// Seed ------------------------------------------------------------------------

inline Seed Seed::proportion(
    epiworld_double p,
    std::vector< std::pair< int, epiworld_double > > split
)
{
    if ((p < 0.0) || (p > 1.0))
        throw std::range_error("A seeding proportion must be within [0, 1].");

    Seed s;
    s.prevalence = p;
    s.as_proportion = true;
    s.split = std::move(split);
    return s;
}

inline Seed Seed::count(
    size_t n,
    std::vector< std::pair< int, epiworld_double > > split
)
{
    Seed s;
    s.prevalence = static_cast< epiworld_double >(n);
    s.as_proportion = false;
    s.split = std::move(split);
    return s;
}

// Model: the natural history ------------------------------------------------

template<typename TSeq>
inline void Model<TSeq>::set_natural_history(NaturalHistory<TSeq> natural_history)
{

    if (nh)
        throw std::logic_error(
            "The model already has a natural history (\"" + nh->get_name() +
            "\"); a model has one."
        );

    if (nstates != 0u)
        throw std::logic_error(
            "The model already has states. With a natural history, the stages "
            "are the model's states: set it before anything else."
        );

    natural_history.finalize(*this);

    if (natural_history.is_carrier(0))
        throw std::logic_error(
            "The first stage of \"" + natural_history.get_name() + "\" (\"" +
            natural_history.get_stages()[0] + "\") is where agents start, so "
            "it cannot be a carrier stage."
        );

    // Every stage with a drawn exit is updated by the natural history
    UpdateFun<TSeq> update = [](Agent<TSeq> * p, Model<TSeq> * m) -> void {
        m->nh_step(*p);
    };

    const int ns = static_cast< int >(natural_history.get_n_stages());
    std::vector< int > infectious;
    std::vector< int > excluded;
    nh_queue.assign(static_cast< size_t >(ns), 0);
    bool default_infection = false;
    for (int s = 0; s < ns; ++s)
    {

        const auto & info = natural_history.get_stage_info(s);
        const bool drawn = (info.infection >= 0) || info.needs_visit;
        add_state(natural_history.get_stages()[s], drawn ? update : nullptr);

        if (natural_history.get_contact(s) <= 0.0)
            excluded.push_back(s);
        else if (natural_history.is_infectious(s))
            infectious.push_back(s);

        if (
            (info.infection >= 0) &&
            !natural_history.get_transition(info.infection).sampler
        )
            default_infection = true;

        // Carriers queue their contacts; stages with a rate or a duration
        // queue themselves
        nh_queue[s] = natural_history.is_carrier(s) ? 2 :
            (info.needs_visit ? 1 : 0);

    }

    if (default_infection && infectious.empty())
        throw std::logic_error(
            "The natural history \"" + natural_history.get_name() + "\" has an "
            "infection by contact but no infectious stage (stage::infectious)."
        );

    infectious_states = std::move(infectious);
    excluded_states = std::move(excluded);

    nh = std::make_shared< const NaturalHistory<TSeq> >(
        std::move(natural_history)
    );

}

template<typename TSeq>
inline void Model<TSeq>::add_virus(Virus<TSeq> v, Seed seed)
{

    if (!seed.split.empty())
    {

        if (!nh)
            throw std::logic_error(
                "A seed split across stages needs a natural history."
            );

        epiworld_double total = 0.0;
        for (auto & s : seed.split)
        {
            nh->check_stage(s.first, "seed");
            if (!nh->is_carrier(s.first))
                throw std::logic_error(
                    "Seeds go to carrier stages; \"" + nh->get_stages()[s.first] +
                    "\" is not one."
                );
            total += s.second;
        }

        if (std::abs(total - 1.0) > 1e-9)
            throw std::range_error(
                "The shares of a seed split must add up to 1."
            );

    }

    // Where in nh_seeds this virus's seed lives (it can change later, e.g., by
    // a model's initial_states())
    const size_t slot = nh_seeds.size();
    nh_seeds.push_back(std::move(seed));

    v.set_distribution(
        [slot](Virus<TSeq> & virus, Model<TSeq> * model) -> void {

        const Seed & s = model->nh_seeds[slot];
        const auto * h = model->nh.get();

        std::vector< size_t > idx;
        for (auto & a : model->population)
            if (
                (a.virus == nullptr) &&
                ((h == nullptr) || (h->get_stage_info(a.state).infection >= 0))
            )
                idx.push_back(static_cast< size_t >(a.id));

        const size_t n = s.as_proportion ?
            static_cast< size_t >(std::floor(
                s.prevalence * static_cast< epiworld_double >(model->size())
            )) :
            static_cast< size_t >(s.prevalence);

        if (n > idx.size())
            throw std::range_error(
                "Only " + std::to_string(idx.size()) + " agents can be "
                "infected; cannot seed " + std::to_string(n) + "."
            );

        // How many go to each stage of the split (the first takes the rest)
        std::vector< size_t > counts(s.split.size(), 0u);
        size_t assigned = 0u;
        for (size_t k = 1u; k < s.split.size(); ++k)
        {
            counts[k] = static_cast< size_t >(std::floor(
                s.split[k].second * static_cast< epiworld_double >(n)
            ));
            assigned += counts[k];
        }
        if (!counts.empty())
            counts[0u] = n - std::min(n, assigned);

        size_t avail = idx.size();
        size_t k = 0u;
        for (size_t i = 0u; i < n; ++i)
        {

            while ((k < counts.size()) && (counts[k] == 0u))
                ++k;

            const size_t loc = model->runif_index(
                static_cast< uint32_t >(avail--)
            );
            Agent<TSeq> & agent = model->population[idx[loc]];

            model->nh_pending = NH_SEED;
            if (counts.empty())
                agent.set_virus(*model, virus);
            else
            {
                agent.set_virus(
                    *model, virus, s.split[k].first, Queue<TSeq>::NoOne
                );
                --counts[k];
            }
            model->nh_pending = -1;

            std::swap(idx[loc], idx[avail]);

        }

    });

    add_virus(v);

}

template<typename TSeq>
inline bool Model<TSeq>::fire(int transition, Agent<TSeq> & agent)
{

    if (!nh)
        throw std::logic_error("fire() needs a natural history.");

    if ((transition < 0) || (static_cast< size_t >(transition) >= nh->get_n_transitions()))
        throw std::range_error(
            "There is no transition " + std::to_string(transition) + "."
        );

    if (nh->get_transition(transition).kind == TransitionKind::infection)
        throw std::logic_error(
            "An infection is not fired; seed the virus instead."
        );

    if (!nh->is_source(transition, static_cast< int >(agent.state)))
        return false;

    nh_propose(transition, agent);
    return true;

}

template<typename TSeq>
inline void Model<TSeq>::set_initial(int stage, epiworld_double share)
{

    if (!nh)
        throw std::logic_error("set_initial() needs a natural history.");

    nh->check_stage(stage, "initial");

    if (nh->is_carrier(stage))
        throw std::logic_error(
            "set_initial() places agents without the virus; \"" +
            nh->get_stages()[stage] + "\" is a carrier stage (seed instead)."
        );

    if ((share < 0.0) || (share > 1.0))
        throw std::range_error("An initial share must be within [0, 1].");

    for (auto & init : nh_initial)
        if (init.first == stage)
        {
            init.second = share;
            return;
        }

    nh_initial.emplace_back(stage, share);

}

template<typename TSeq>
inline int Model<TSeq>::get_stage_entered(size_t id, int stage) const
{
    if (!nh)
        throw std::logic_error("The model has no natural history.");
    nh->check_stage(stage, "");
    return nh_entered.at(id * nh->get_n_stages() + static_cast< size_t >(stage));
}

template<typename TSeq>
inline int Model<TSeq>::get_stage_until(size_t id) const
{
    if (!nh)
        throw std::logic_error("The model has no natural history.");
    return nh_until.at(id);
}

template<typename TSeq>
inline int Model<TSeq>::get_episode(size_t id) const
{
    if (!nh)
        throw std::logic_error("The model has no natural history.");
    return nh_episode.at(id);
}

template<typename TSeq>
inline void Model<TSeq>::nh_reset()
{

    const size_t n  = population.size();
    const size_t ns = nh->get_n_stages();

    nh_until.assign(n, INT_MAX);
    nh_entered.assign(n * ns, -1);
    nh_episode.assign(n, 0);
    nh_pending = -1;

    // Everyone starts in the first stage, on day 0
    const int after = nh->stages[0u].after;
    for (auto & p : population)
    {
        const size_t id = static_cast< size_t >(p.id);
        nh_entered[id * ns] = 0;
        if (after >= 0)
            nh_until[id] = nh->transitions[after].duration.draw(p, *this);
    }

    if (use_queuing && (nh_queue[0u] == 1))
        for (size_t id = 0u; id < n; ++id)
            queue.shift(id, 1);

}

template<typename TSeq>
inline void Model<TSeq>::nh_step(Agent<TSeq> & p)
{

    const StageInfo & st = nh->stages[p.state];

    // 1. Infection by contact
    if ((st.infection >= 0) && (p.virus == nullptr))
    {

        const auto & tr = nh->transitions[st.infection];
        Virus<TSeq> * v = tr.sampler ? tr.sampler(p, *this) : sample_virus(&p);
        if (v != nullptr)
        {
            nh_propose(st.infection, p, v);
            return;
        }

    }

    // 2. The rates compete: one draw picks one, or none
    const size_t nr = st.rates.size();
    if (nr > 0u)
    {

        reserve_tmp(nr);
        for (size_t k = 0u; k < nr; ++k)
        {

            const auto & tr = nh->transitions[st.rates[k]];
            epiworld_double x = tr.value.eval(p, *this);

            if ((tr.effect != Effect::none) && (p.virus != nullptr))
            {
                if (tr.effect == Effect::recovery)
                    x = 1.0 - (1.0 - x) * (1.0 - p.get_recovery_enhancer(p.virus, *this));
                else
                    x = x * (1.0 - p.get_death_reduction(p.virus, *this));
            }

            array_double_tmp[k] = x;

        }

        const int which = roulette(static_cast< epiworld_fast_uint >(nr), this);
        if (which >= 0)
        {
            nh_propose(st.rates[static_cast< size_t >(which)], p);
            return;
        }

    }

    // 3. The duration, if it ended (a deadline: the rates went first)
    if ((st.after >= 0) && (nh_until[static_cast< size_t >(p.id)] <= today()))
        nh_propose(st.after, p);

}

template<typename TSeq>
inline void Model<TSeq>::nh_propose(
    int transition,
    Agent<TSeq> & p,
    Virus<TSeq> * virus
)
{

    const auto & tr = nh->transitions[transition];

    // A branch draws its target
    int to = tr.to;
    if (!tr.branch_to.empty())
    {

        const epiworld_double u = runif();
        epiworld_double cum = 0.0;
        for (size_t k = 0u; k < tr.branch_to.size(); ++k)
        {
            cum += tr.branch_prob[k].eval(p, *this);
            to = tr.branch_to[k];
            if (u < cum)
                break;
        }

    }

    nh_pending = transition;

    if (tr.kind == TransitionKind::infection)
        p.set_virus(*this, *virus, to, Queue<TSeq>::NoOne);
    else if (nh->is_carrier(static_cast< int >(p.state)) && !nh->is_carrier(to))
        p.rm_virus(*this, to, Queue<TSeq>::NoOne);
    else
        p.change_state(*this, static_cast< epiworld_fast_uint >(to), Queue<TSeq>::NoOne);

    nh_pending = -1;

}

template<typename TSeq>
inline void Model<TSeq>::nh_enter(
    Agent<TSeq> & p,
    unsigned int state_old,
    int transition
)
{

    const size_t id = static_cast< size_t >(p.id);
    const size_t ns = nh->get_n_stages();
    const int s = static_cast< int >(p.state);

    // A new infection starts a new episode: its entry days start afresh
    const bool infected =
        (transition == NH_SEED) ||
        (!nh->is_carrier(static_cast< int >(state_old)) && nh->is_carrier(s));

    int * entered = nh_entered.data() + id * ns;
    if (infected)
    {
        ++nh_episode[id];
        std::fill(entered, entered + ns, -1);
    }

    entered[s] = today();

    const int after = nh->stages[static_cast< size_t >(s)].after;
    if (after >= 0)
    {

        const auto & d = nh->transitions[after].duration;
        int base = today();
        if ((d.get_anchor() >= 0) && (entered[d.get_anchor()] >= 0))
            base = entered[d.get_anchor()];

        nh_until[id] = base + d.draw(p, *this);

    }
    else
        nh_until[id] = INT_MAX;

    if (transition >= 0)
        db.record_declared_transition(transition);

}

template<typename TSeq>
inline void Model<TSeq>::nh_queue_move(Agent<TSeq> & p, unsigned int state_old)
{

    const char from = nh_queue[state_old];
    const char to = nh_queue[p.state];
    if (from == to)
        return;

    const size_t id = static_cast< size_t >(p.id);

    if (from == 2)
        queue -= &p;
    else if (from == 1)
        queue.shift(id, -1);

    if (to == 2)
        queue += &p;
    else if (to == 1)
        queue.shift(id, 1);

}

template<typename TSeq>
inline void Model<TSeq>::nh_initial_place()
{

    if (nh_initial.empty())
        return;

    // The agents left without the virus, in the first stage
    std::vector< size_t > idx;
    for (auto & p : population)
        if ((p.virus == nullptr) && (p.state == 0u))
            idx.push_back(static_cast< size_t >(p.id));

    const epiworld_double n_free = static_cast< epiworld_double >(idx.size());
    size_t avail = idx.size();

    for (auto & init : nh_initial)
    {

        const size_t k = static_cast< size_t >(init.second * n_free);
        if (k > avail)
            throw std::range_error(
                "The initial shares add up to more than the agents without "
                "the virus."
            );

        for (size_t i = 0u; i < k; ++i)
        {
            const size_t loc = runif_index(static_cast< uint32_t >(avail--));
            nh_pending = NH_INITIAL;
            population[idx[loc]].change_state(
                *this,
                static_cast< epiworld_fast_uint >(init.first),
                Queue<TSeq>::NoOne
            );
            nh_pending = -1;
            std::swap(idx[loc], idx[avail]);
        }

    }

    events_run();

}

template<typename TSeq>
inline epiworld_double Model<TSeq>::nh_infection_prob(
    Agent<TSeq> & target,
    const Virus<TSeq> & v
)
{
    const int tr = nh->stages[target.state].infection;
    if (tr < 0)
        return 0.0;

    return nh->transitions[tr].value.eval(target, &v, *this);
}

#endif
