#ifndef EPIWORLD_LAYER_MIXING_HPP
#define EPIWORLD_LAYER_MIXING_HPP

namespace layer {

template<typename TSeq>
class Mixing;

/**
 * @brief A mixing layer (see `Mixing`).
 * @param name Name of the layer (unique in a model).
 * @param infectious States whose agents can be sampled as contacts.
 * @param available States whose agents take part in the mixing.
 * @ingroup contact_layers
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
std::shared_ptr< Mixing<TSeq> > mixing(
    std::string name,
    std::vector< int > infectious,
    std::vector< int > available
);

/**
 * @brief Contact layer with mixing: contacts drawn fresh every step, by group.
 *
 * @details Created with `layer::mixing()`. The layer is configured with two
 * lists of states: the
 * *infectious* states (whose agents can be contacted and transmit) and the
 * *available* states (agents that take part in the mixing, which is what the
 * contact rate is spread over).
 *
 * ### Groups
 *
 * The layer owns the grouping: `set_groups()` assigns every agent a group (or
 * `NO_GROUP`, which leaves it out of the layer). If no grouping is set, the
 * layer builds one from the model's entities once they are distributed (at the
 * first `update()` of a run), each agent going to the group of its first entity
 * (and agents without entities to none), which is how the mixing models are
 * built. Either way, from then on the layer works on its own table, not on the
 * entities.
 *
 * ### Contacts: a matrix or a rate per group
 *
 * The contacts are given in one of two ways:
 * - `set_contact_matrix()`: contacts across groups, for a few groups that meet
 *   each other (age groups in public spaces).
 * - `set_rates()`: contacts within groups only, for many small groups
 *   (households, classrooms, transit trips), at a rate per group -- the
 *   diagonal of a contact matrix that is never stored. One number sets the
 *   same rate for every group. It needs the grouping from `set_groups()`.
 *
 * Setting one between runs drops the other. A within-group layer is seen by
 * the model's queue (an infectious agent reaches only its own group), and
 * `members_of()` lists a group.
 *
 * ### Sampling
 *
 * Each step, `update()` rebuilds the pool of infectious agents of each group,
 * and the number of available agents per group, without scanning the
 * population: infectious agents come from the model's per-state index (and are
 * walked in ascending id order), and available counts are the group sizes
 * minus the few agents in non-available states.
 *
 * `sample()` then draws, for each group `g`, a binomial number of contacts
 * (with probability `contact_rate(group of agent, g) / available(g)` over the
 * infectious agents of `g`), picks the infectious agents with replacement
 * (duplicates are kept) and rejects contacts with the agent itself. With rates,
 * the only group is the agent's own, and the rate is `rates[g]`: the same draw
 * a diagonal matrix gives.
 *
 * ### Edits
 *
 * `set_group()`, `set_contact_matrix()` and `set_rates()` called during a run
 * apply to that run only: they set the dirty flag, and the next `reset()`
 * restores the baseline. Called between runs, they set the baseline (so a
 * calibration can reuse one model). `set_groups()` sets the baseline grouping
 * and can only be called between runs.
 *
 * @tparam TSeq Type of sequence.
 * @ingroup contact_layers
 */
template<typename TSeq>
class Mixing : public ContactLayer<TSeq>
{
public:

    /// Group of the agents that are not in the layer.
    static constexpr size_t NO_GROUP = std::numeric_limits< size_t >::max();

private:

    std::vector< char > is_infectious; ///< Lookup table, one entry per state
    std::vector< char > is_available;  ///< Lookup table, one entry per state
    std::vector< int > infectious_states;
    std::vector< int > available_states;

    // Grouping
    std::vector< size_t > groups;          ///< Group of each agent
    std::vector< size_t > groups_baseline; ///< Set by set_groups()
    size_t n_groups = 0u;
    bool explicit_groups = false; ///< Set by set_groups(), else from entities
    bool groups_pending = false;  ///< Groups to read from the entities
    bool layout_stale = true;     ///< Group sizes and pool offsets to rebuild
    std::vector< size_t > group_size; ///< Agents in each group

    std::vector< double > contact_matrix;          ///< Column-major
    std::vector< double > contact_matrix_baseline; ///< Set between runs
    std::vector< double > rates;          ///< Within-group rate per group
    std::vector< double > rates_baseline; ///< Set between runs (one entry: every group)

    std::vector< std::vector< size_t > > members; ///< Agents of each group

    std::vector< size_t > pool;      ///< Infectious agents, partitioned by group
    std::vector< size_t > offsets;   ///< Where each group starts in `pool`
    std::vector< size_t > counts;    ///< Infectious agents per group
    std::vector< double > inv_available; ///< 1 / (available agents), per group
    std::vector< size_t > n_available;

    std::vector< uint64_t > marks;   ///< Bitset used to order the pool
    std::vector< size_t > ordered;   ///< Infectious ids (ascending), scratch

    void rebuild_layout();
    void rebuild_members();

    /// Draws `rbinom(infectious in g, p)` contacts from group `g`.
    void draw(size_t g, double p, size_t agent, Model<TSeq> & model,
        std::vector< size_t > & out);

protected:

    // Created with layer::mixing()
    Mixing(
        std::string name,
        std::vector< int > infectious,
        std::vector< int > available
    ) : ContactLayer<TSeq>(std::move(name)),
        infectious_states(std::move(infectious)),
        available_states(std::move(available)) {}

    template<typename T>
    friend std::shared_ptr< Mixing<T> > mixing(
        std::string, std::vector< int >, std::vector< int >
    );

public:

    /**
     * @brief Sets the grouping of the layer (between runs only).
     * @param group_of_agent One entry per agent: a group in `[0, n_groups)`, or
     * `NO_GROUP` to leave the agent out of the layer.
     * @param n_groups_ Number of groups (the contact matrix is
     * `n_groups x n_groups`).
     */
    void set_groups(std::vector< size_t > group_of_agent, size_t n_groups_);

    /// Group of an agent (`NO_GROUP` if it is not in the layer).
    size_t get_group(size_t agent_id) const { return groups[agent_id]; }

    /// Whether the agent takes part in the layer (valid after `reset()`).
    bool in_layer(size_t agent_id) const { return groups[agent_id] != NO_GROUP; }

    /**
     * @brief Moves an agent to another group (`NO_GROUP` takes it out of the
     * layer).
     * @details Like every change made during a step, it is in force for the
     * contacts of the next step (the pools are rebuilt at the end of the step).
     */
    void set_group(size_t agent_id, size_t group);

    size_t get_n_groups() const { return n_groups; }

    /**
     * @brief Sets the contact matrix.
     * @param cmat Column-major `n_groups x n_groups` matrix: entry `(i, j)`
     * is the expected number of contacts per day that an agent in group `i` has
     * with agents in group `j`.
     */
    void set_contact_matrix(std::vector< double > cmat);

    const std::vector< double > & get_contact_matrix() const
    {
        return contact_matrix;
    }

    /**
     * @brief Sets the contact rate within each group (contacts within groups
     * only).
     * @param rates_ One rate per group, the expected number of contacts per day
     * an agent has with the members of its group; or a single rate for every
     * group.
     */
    void set_rates(std::vector< double > rates_);
    void set_rates(double rate) { set_rates(std::vector< double >{rate}); }

    /// The within-group rates (one per group after `reset()`; empty with a
    /// contact matrix).
    const std::vector< double > & get_rates() const { return rates; }

    /// Whether the layer mixes within groups only (`set_rates()`).
    bool within_groups() const { return !rates_baseline.empty(); }

    /// The agents of group `g`, in no particular order (valid after `reset()`).
    const std::vector< size_t > & members_of(size_t g) const
    {
        return members[g];
    }

    layer::Mixing<TSeq> * as_mixing() override { return this; }

    /// Within groups only: a carrier reaches only its own group.
    bool supports_queue() const override { return within_groups(); }

    /**
     * @brief Restores the baseline and sizes all buffers for the model (the
     * first `update()`, on day 0, builds the pools).
     */
    void reset(Model<TSeq> & model) override;

    /**
     * @brief Rebuilds the infectious pools and the available counts from the
     * current state of the model (once per step).
     */
    void update(Model<TSeq> & model) override;

    std::shared_ptr< ContactLayer<TSeq> > clone() const override
    {
        return std::make_shared< Mixing<TSeq> >(*this);
    }

    /**
     * @brief Samples the infectious contacts of `agent` (none if it is not in
     * the layer), appending them to `out`.
     * @details `exclude` is ignored: the infectious states of the layer say
     * who can be contacted. See `ContactLayer::sample()`.
     */
    size_t sample(
        size_t agent,
        Model<TSeq> & model,
        const std::vector< bool > * exclude,
        std::vector< size_t > & out
    ) override;

    /// Number of infectious agents in a group.
    size_t get_n_infectious(size_t group) const { return counts[group]; }

};

template<typename TSeq>
inline std::shared_ptr< Mixing<TSeq> > mixing(
    std::string name,
    std::vector< int > infectious,
    std::vector< int > available
)
{
    return std::shared_ptr< Mixing<TSeq> >(new Mixing<TSeq>(
        std::move(name), std::move(infectious), std::move(available)
    ));
}

template<typename TSeq>
inline void Mixing<TSeq>::set_groups(
    std::vector< size_t > group_of_agent,
    size_t n_groups_
)
{

    for (size_t g : group_of_agent)
        if ((g != NO_GROUP) && (g >= n_groups_))
            throw std::range_error(
                "Mixing layer '" + this->get_name() + "': group " +
                std::to_string(g) + " is out of range (" +
                std::to_string(n_groups_) + " groups)."
            );

    if (!this->edits_baseline())
        throw std::logic_error(
            "Mixing layer '" + this->get_name() + "': set_groups() sets the "
            "baseline and cannot be called during a run; use set_group()."
        );

    groups_baseline = std::move(group_of_agent);
    groups          = groups_baseline;
    n_groups        = n_groups_;
    explicit_groups = true;
    layout_stale    = true;
    rebuild_members();

}

template<typename TSeq>
inline void Mixing<TSeq>::set_group(size_t agent_id, size_t group)
{

    if (agent_id >= groups.size())
        throw std::range_error(
            "Mixing layer '" + this->get_name() + "': agent " +
            std::to_string(agent_id) + " is out of range (the layer has " +
            std::to_string(groups.size()) + " agents; was it reset?)."
        );

    if ((group != NO_GROUP) && (group >= n_groups))
        throw std::range_error(
            "Mixing layer '" + this->get_name() + "': group " +
            std::to_string(group) + " is out of range (" +
            std::to_string(n_groups) + " groups)."
        );

    const bool baseline = this->edits_baseline();
    if (baseline)
    {
        if (!explicit_groups)
            throw std::logic_error(
                "Mixing layer '" + this->get_name() + "': set_groups() must be "
                "called before set_group() outside a run."
            );
        groups_baseline[agent_id] = group;
    }

    const size_t old = groups[agent_id];
    if (old == group)
        return;

    // During a run, the queue follows the move (it sees within-group layers)
    if ((this->model != nullptr) && !baseline && within_groups())
        this->model->group_member_moved(
            agent_id,
            old   == NO_GROUP ? nullptr : &members[old],
            group == NO_GROUP ? nullptr : &members[group]
        );

    if (old != NO_GROUP)
    {
        auto & m = members[old];
        auto it = std::find(m.begin(), m.end(), agent_id);
        *it = m.back();
        m.pop_back();
    }

    if (group != NO_GROUP)
        members[group].push_back(agent_id);

    groups[agent_id] = group;
    layout_stale = true;

}

template<typename TSeq>
inline void Mixing<TSeq>::set_contact_matrix(std::vector< double > cmat)
{

    for (double c : cmat)
        if (c < 0.0)
            throw std::range_error(
                "Mixing layer '" + this->get_name() + "': the contact matrix "
                "must be non-negative (" + std::to_string(c) + " < 0)."
            );

    // Between runs, the size is checked by reset() (the grouping may not be
    // set yet); during a run, the grouping and the form are fixed
    if (this->edits_baseline())
    {
        contact_matrix_baseline = cmat;
        rates_baseline.clear();
    }
    else if (within_groups())
        throw std::logic_error(
            "Mixing layer '" + this->get_name() + "': the layer mixes within "
            "groups (set_rates()); a contact matrix can only replace the rates "
            "between runs."
        );
    else if (cmat.size() != n_groups * n_groups)
        throw std::length_error(
            "Mixing layer '" + this->get_name() + "': the contact matrix has " +
            std::to_string(cmat.size()) + " entries, but the layer has " +
            std::to_string(n_groups) + " groups."
        );

    contact_matrix = std::move(cmat);

}

template<typename TSeq>
inline void Mixing<TSeq>::set_rates(std::vector< double > rates_)
{

    if (rates_.empty())
        throw std::length_error(
            "Mixing layer '" + this->get_name() + "': set_rates() needs at "
            "least one rate."
        );

    for (double r : rates_)
        if (!(r >= 0.0))
            throw std::range_error(
                "Mixing layer '" + this->get_name() + "': contact rates must be "
                "non-negative (" + std::to_string(r) + ")."
            );

    // Between runs, the size is checked by reset(); during a run, the
    // grouping and the form are fixed
    if (this->edits_baseline())
    {
        rates_baseline = rates_;
        contact_matrix_baseline.clear();
    }
    else
    {

        if (!within_groups())
            throw std::logic_error(
                "Mixing layer '" + this->get_name() + "': the layer has a "
                "contact matrix; rates can only replace it between runs."
            );

        // A copy: assign() may free the element before reading it
        if (rates_.size() == 1u)
            rates_.assign(n_groups, double(rates_[0u]));
        else if (rates_.size() != n_groups)
            throw std::length_error(
                "Mixing layer '" + this->get_name() + "': " +
                std::to_string(rates_.size()) + " rates, but the layer has " +
                std::to_string(n_groups) + " groups."
            );

    }

    rates = std::move(rates_);

}

template<typename TSeq>
inline void Mixing<TSeq>::rebuild_members()
{

    members.assign(n_groups, {});
    for (size_t i = 0u; i < groups.size(); ++i)
        if (groups[i] != NO_GROUP)
            members[groups[i]].push_back(i);

}

template<typename TSeq>
inline void Mixing<TSeq>::rebuild_layout()
{

    group_size.assign(n_groups, 0u);
    for (size_t g : groups)
        if (g != NO_GROUP)
            ++group_size[g];

    counts.assign(n_groups, 0u);
    offsets.assign(n_groups, 0u);
    n_available.assign(n_groups, 0u);
    inv_available.assign(n_groups, 0.0);

    // Groups start where the previous one ends (at most its size)
    for (size_t i = 1u; i < n_groups; ++i)
        offsets[i] = offsets[i - 1u] + group_size[i - 1u];

    layout_stale = false;

}

template<typename TSeq>
inline void Mixing<TSeq>::reset(Model<TSeq> & model)
{

    const size_t ns = model.get_n_states();
    is_infectious.assign(ns, 0);
    is_available.assign(ns, 0);

    for (int s : infectious_states)
    {
        if ((s < 0) || (static_cast< size_t >(s) >= ns))
            throw std::range_error("Mixing layer: infectious state out of range.");
        is_infectious[s] = 1;
    }

    for (int s : available_states)
    {
        if ((s < 0) || (static_cast< size_t >(s) >= ns))
            throw std::range_error("Mixing layer: available state out of range.");
        is_available[s] = 1;
    }

    const size_t n = model.size();

    if (explicit_groups)
    {

        // Edits of the last run are dropped
        groups = groups_baseline;

        if (groups.size() != n)
            throw std::length_error(
                "Mixing layer '" + this->get_name() + "': the grouping has " +
                std::to_string(groups.size()) + " agents, but the model has " +
                std::to_string(n) + "."
            );

    }
    else
    {

        // One group per entity; the agents are placed at the first update,
        // once the entities are distributed
        n_groups = model.get_entities().size();
        groups.assign(n, NO_GROUP);
        groups_pending = true;

    }

    // Edits of the last run are dropped
    if (within_groups())
    {

        // The queue registers carriers before the first update, so the
        // members must be known now, not taken from the entities later
        if (!explicit_groups)
            throw std::logic_error(
                "Mixing layer '" + this->get_name() + "': a layer with rates "
                "(within groups) needs its groups from set_groups()."
            );

        rates = rates_baseline;
        // A copy: assign() may free the element before reading it
        if (rates.size() == 1u)
            rates.assign(n_groups, double(rates[0u]));
        else if (rates.size() != n_groups)
            throw std::length_error(
                "Mixing layer '" + this->get_name() + "': " +
                std::to_string(rates.size()) + " rates, but the layer has " +
                std::to_string(n_groups) + " groups."
            );

        contact_matrix.clear();

    }
    else
    {

        contact_matrix = contact_matrix_baseline;
        if (contact_matrix.size() != n_groups * n_groups)
            throw std::length_error(
                "Mixing layer '" + this->get_name() + "': the contact matrix "
                "has " + std::to_string(contact_matrix.size()) + " entries, but "
                "the layer has " + std::to_string(n_groups) + " groups."
            );

        rates.clear();

    }

    rebuild_members();

    pool.assign(n, 0u);
    ordered.clear();
    ordered.reserve(n);
    marks.assign((n + 63u) / 64u, 0u);

    rebuild_layout();
    this->mark_clean();

}

template<typename TSeq>
inline void Mixing<TSeq>::update(Model<TSeq> & model)
{

    // The agent goes to the group of its first entity
    if (groups_pending)
    {
        for (size_t i = 0u; i < groups.size(); ++i)
        {
            auto & a = model.get_agent(i);
            if (a.get_n_entities() != 0u)
                groups[i] = a.get_entity(0u, model).get_id();
        }

        groups_pending = false;
        layout_stale = true;
        rebuild_members();
    }

    if (layout_stale)
        rebuild_layout();

    const size_t ns = is_infectious.size();

    std::fill(counts.begin(), counts.end(), 0u);

    // Marking the infectious agents; walking the bitset gives ascending ids
    for (size_t s = 0u; s < ns; ++s)
    {
        if (!is_infectious[s])
            continue;

        for (size_t id : model.get_agents_in_state(s))
            marks[id >> 6] |= (uint64_t(1) << (id & 63u));
    }

    ordered.clear();
    for (size_t w = 0u; w < marks.size(); ++w)
    {
        uint64_t word = marks[w];
        marks[w] = 0u;
        while (word != 0u)
        {
            ordered.push_back((w << 6) + epi_ctz64(word));
            word &= (word - 1u);
        }
    }

    // Placing them in their group (agents out of the layer are not in the mixing)
    for (size_t id : ordered)
    {
        const size_t g = groups[id];
        if (g == NO_GROUP)
            continue;

        pool[offsets[g] + counts[g]++] = id;
    }

    // Available = group size - agents in non-available states
    for (size_t g = 0u; g < n_groups; ++g)
        n_available[g] = group_size[g];

    for (size_t s = 0u; s < ns; ++s)
    {
        if (is_available[s])
            continue;

        for (size_t id : model.get_agents_in_state(s))
        {
            const size_t g = groups[id];
            if (g == NO_GROUP)
                continue;

            if (n_available[g] > 0u)
                --n_available[g];
        }
    }

    for (size_t g = 0u; g < n_groups; ++g)
        inv_available[g] = (n_available[g] > 0u) ?
            1.0 / static_cast< double >(n_available[g]) : 0.0;

}

template<typename TSeq>
inline void Mixing<TSeq>::draw(
    size_t g,
    double p,
    size_t agent,
    Model<TSeq> & model,
    std::vector< size_t > & out
)
{

    const size_t n_pool = counts[g];

    if (n_pool == 0u)
        return;

    // How many from this group?
    int nsamples = model.rbinom(n_pool, p);

    for (int s = 0; s < nsamples; ++s)
    {

        // Randomly selecting an agent
        int which = model.runif() * n_pool;

        // Correcting overflow error
        if (which >= static_cast< int >(n_pool))
            which = static_cast< int >(n_pool) - 1;

        const size_t id = pool[offsets[g] + which];

        #ifdef EPI_DEBUG
        if (!is_infectious[model.get_agent(id).get_state()])
            throw std::logic_error(
                "The agent is not infected, but it should be."
            );
        #endif

        // Can't sample itself
        if (id == agent)
            continue;

        out.push_back(id);

    }

}

template<typename TSeq>
inline size_t Mixing<TSeq>::sample(
    size_t agent,
    Model<TSeq> & model,
    const std::vector< bool > *,
    std::vector< size_t > & out
)
{

    if (!in_layer(agent))
        return 0u;

    const size_t first = out.size();

    if (within_groups())
    {
        // Only the agent's own group
        const size_t g = groups[agent];
        draw(g, inv_available[g] * rates[g], agent, model, out);
    }
    else
    {
        // Column of the agent's group: entry (agent's group, g) is rate[g]
        const double * rate = contact_matrix.data() + groups[agent];
        for (size_t g = 0u; g < n_groups; ++g)
            draw(g, inv_available[g] * rate[g * n_groups], agent, model, out);
    }

    // Reporting the interactions, only if somebody listens
    const size_t n_sampled = out.size() - first;
    if (model.has_post_sampling())
        model.register_sampled_contacts(
            out.data() + first, n_sampled, static_cast< int >(agent)
        );

    return n_sampled;

}

}

#endif
