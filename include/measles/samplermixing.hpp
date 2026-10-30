#ifndef EPIWORLD_MODELS_SAMPLERMIXING_HPP
#define EPIWORLD_MODELS_SAMPLERMIXING_HPP

using namespace epiworld;

/**
 * @brief The layer a measles mixing model samples its contacts from: the
 * model's only layer, a `layer::Mixing` with a contact matrix.
 *
 * @details The measles models keep their own samplers (with replacement, see
 * `SamplerMixing`), which read the layer's groups and contact matrix (times
 * its contact rate) but would ignore any other layer, so a model with none or
 * with more throws instead.
 */
template<typename TSeq>
inline layer::Mixing<TSeq> & measles_mixing_layer(Model<TSeq> & model)
{

    auto * mix = (model.get_n_layers() == 1u) ?
        dynamic_cast< layer::Mixing<TSeq> * >(&model.get_layer(0u)) : nullptr;

    if ((mix == nullptr) || mix->within_groups())
        throw std::logic_error(
            model.get_name() + ": the model samples its contacts from one "
            "mixing layer with a contact matrix (add it with "
            "add_layer(layer::mixing(...))) and cannot use other layers; it "
            "has " + std::to_string(model.get_n_layers()) + " layer(s)."
        );

    return *mix;

}

/**
 * @brief Reusable contact sampler for mixing models with two infectious stages.
 *
 * The sampler keeps the per-group infectious-agent accounting and contact
 * sampling buffer used by ModelMeaslesMixing; the groups and the contact
 * matrix (times its contact rate) are those of the model's mixing layer (see
 * `measles_mixing_layer()`), and the contacts go to its log. The primary
 * infectious state is sampled at full weight; the reduced infectious state is
 * sampled with a relative contact rate computed as 1 minus a model parameter
 * such as "Rash reduction contact rate".
 *
 * @tparam TSeq Type for genetic sequences (default: EPI_DEFAULT_TSEQ)
 * @ingroup disease_specific
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class SamplerMixing
{
private:

    int primary_infectious_state = -1;
    int reduced_infectious_state = -1;
    ParamRef reduced_contact_reduction_param{""};
    std::vector< int > contact_states;

    layer::Mixing<TSeq> * mixing = nullptr; ///< The model's layer (update())
    double contact_rate = 1.0;              ///< Its contact rate (update())

    std::vector< size_t > primary_infectious;
    std::vector< size_t > reduced_infectious;

    std::vector< size_t > n_primary_infectious_per_group;
    std::vector< size_t > n_reduced_infectious_per_group;

    std::vector< size_t > group_start; ///< Where each group starts in the pools
    std::vector< double > adjusted_contact_rate;
    std::vector< size_t > sampled_agents;

    double get_reduced_contact_rate(Model<TSeq> & model) const;
    bool is_available_for_contact(int state, bool include_reduced) const;

    #ifdef EPI_DEBUG
    std::vector< int > sampled_sizes;
    #endif

public:

    /**
     * @brief Default constructor.
     */
    SamplerMixing() = default;

    /**
     * @brief Configures state indices and contact-selection settings.
     *
     * @param primary_infectious_state_ State index for fully infectious agents.
     * @param reduced_infectious_state_ State index for reduced-infectious agents.
     * @param reduced_contact_reduction_param_ Model parameter name controlling
     * reduced infectious contact reduction.
     * @param contact_states_ Explicit states eligible for contact.
     */
    void configure(
        int primary_infectious_state_,
        int reduced_infectious_state_,
        std::string reduced_contact_reduction_param_,
        std::vector< int > contact_states_
    );

    /**
     * @brief Resets internal buffers and recomputes infectious pools.
     *
     * @param model Model containing current agents and its mixing layer.
     */
    void reset(Model<TSeq> & model);

    /**
     * @brief Updates infectious pools and per-group sampling weights (and
     * reads the layer's groups, which may have changed, and contact rate).
     *
     * @param model Model containing current agents and its mixing layer.
     */
    void update(Model<TSeq> & model);

    /**
     * @brief Samples infectious contacts into the internal sampled-agent
     * buffer, and logs them in the layer if it is traced.
     *
     * @param agent Agent for which contacts are sampled.
     * @param model Model containing current agent states.
     *
     * @return Number of sampled contacts written to the internal buffer.
     */
    size_t sample(Agent<TSeq> * agent, Model<TSeq> & model);

    /**
     * @brief Returns the internal sampled-agent buffer.
     */
    const std::vector< size_t > & get_sampled_agents() const
    {
        return sampled_agents;
    }

    #ifdef EPI_DEBUG
    /**
     * @brief Records the number of sampled contacts in debug mode.
     *
     * @param n Number of contacts sampled in a call.
     */
    void record_sampled_size(size_t n)
    {
        sampled_sizes.push_back(static_cast<int>(n));
    }

    /**
     * @brief Returns the debug history of sampled-contact counts.
     */
    const std::vector< int > & get_sampled_sizes() const
    {
        return sampled_sizes;
    }
    #endif

};

template<typename TSeq>
inline void SamplerMixing<TSeq>::configure(
    int primary_infectious_state_,
    int reduced_infectious_state_,
    std::string reduced_contact_reduction_param_,
    std::vector< int > contact_states_
)
{
    primary_infectious_state = primary_infectious_state_;
    reduced_infectious_state = reduced_infectious_state_;
    reduced_contact_reduction_param = ParamRef(
        std::move(reduced_contact_reduction_param_)
    );
    contact_states = contact_states_;
}

template<typename TSeq>
inline double SamplerMixing<TSeq>::get_reduced_contact_rate(
    Model<TSeq> & model
) const
{
    if (reduced_contact_reduction_param.name().empty())
        return 0.0;

    return 1.0 - reduced_contact_reduction_param(model);
}

template<typename TSeq>
inline bool SamplerMixing<TSeq>::is_available_for_contact(
    int state,
    bool include_reduced
) const
{
    if (include_reduced && (state == reduced_infectious_state))
    {
        return true;
    }

    for (auto contact_state: contact_states)
        if (state == contact_state)
            return true;

    return false;
}

template<typename TSeq>
inline void SamplerMixing<TSeq>::reset(Model<TSeq> & model)
{
    sampled_agents.resize(model.size());
    primary_infectious.assign(model.size(), 0u);
    reduced_infectious.assign(model.size(), 0u);

    update(model);
}

template<typename TSeq>
inline void SamplerMixing<TSeq>::update(Model<TSeq> & model)
{
    mixing = &measles_mixing_layer(model);
    contact_rate = mixing->get_contact_rate(model);

    const size_t ngroups = mixing->get_n_groups();
    constexpr size_t none = layer::Mixing<TSeq>::NO_GROUP;

    n_primary_infectious_per_group.assign(ngroups, 0u);
    n_reduced_infectious_per_group.assign(ngroups, 0u);
    adjusted_contact_rate.assign(ngroups, 0.0);

    // The pools of a group take as many slots as it has members
    group_start.assign(ngroups, 0u);
    for (size_t g = 1u; g < ngroups; ++g)
        group_start[g] = group_start[g - 1] + mixing->members_of(g - 1).size();

    double reduced_contact_rate = get_reduced_contact_rate(model);
    bool include_reduced = reduced_contact_rate > 0.0;

    // We need to check all agents to see which ones
    // will be included in the infectious pools
    for (const auto & a : model.get_agents())
    {
        const size_t g = mixing->get_group(a.get_id());
        if (g == none)
            continue;

        int state = static_cast<int>(a.get_state());

        if (state == primary_infectious_state)
        {
            primary_infectious[
                group_start[g] + n_primary_infectious_per_group[g]++
            ] = a.get_id();
        }
        else if (include_reduced && (state == reduced_infectious_state))
        {
            reduced_infectious[
                group_start[g] + n_reduced_infectious_per_group[g]++
            ] = a.get_id();
        }

        if (is_available_for_contact(state, include_reduced))
            adjusted_contact_rate[g] += 1.0;
    }

    for (auto & rate: adjusted_contact_rate)
    {
        if (rate > 0.0)
            rate = 1.0 / rate;
        else
            rate = 0.0;

        if (rate > 1.0)
            rate = 1.0;
    }
}

template<typename TSeq>
inline size_t SamplerMixing<TSeq>::sample(
    Agent<TSeq> * agent,
    Model<TSeq> & model
)
{
    // An agent in no group meets nobody
    const size_t agent_group_id = mixing->get_group(agent->get_id());
    if (agent_group_id == layer::Mixing<TSeq>::NO_GROUP)
        return 0u;

    const size_t ngroups = mixing->get_n_groups();
    const auto & contact_matrix = mixing->get_contact_matrix();

    // The reduced-contact rate for the secondary infectious pool (e.g. rash).
    // r = 0  means no contacts at all; r = 1 means full contact rate.
    double r = get_reduced_contact_rate(model);

    // Sampling accross all groups
    size_t samp_id = 0u;
    for (size_t g = 0; g < ngroups; ++g)
    {
        // Contacts per day of the agent's group with group g (column-major)
        double base_rate =
            adjusted_contact_rate[g] * contact_rate *
            contact_matrix[g * ngroups + agent_group_id];

        // --- Primary infectious pool (e.g. prodromal) ---
        if (n_primary_infectious_per_group[g] > 0u)
        {
            int nsamples = model.rbinom(
                static_cast<int>(n_primary_infectious_per_group[g]),
                base_rate
            );

            for (int s = 0; s < nsamples; ++s)
            {
                uint32_t which = model.runif_index(
                    n_primary_infectious_per_group[g]
                );

                #ifdef EPI_DEBUG
                auto & a = model.get_agents().at(
                    primary_infectious.at(group_start[g] + which)
                );
                if (static_cast<int>(a.get_state()) != primary_infectious_state)
                    throw std::logic_error(
                        "The sampled agent is not in the primary infectious state."
                    );
                #else
                auto & a = model.get_agent(
                    primary_infectious[group_start[g] + which]
                );
                #endif

                if (a.get_id() == agent->get_id())
                    continue;

                sampled_agents[samp_id++] = a.get_id();
            }
        }

        // --- Reduced infectious pool (e.g. rash) ---
        // The binomial probability is scaled by r so that the *expected*
        // number of contacts with reduced-infectious agents is proportional
        // to r, not just the per-contact selection weight.
        if (n_reduced_infectious_per_group[g] > 0u && r > 0.0)
        {
            int nsamples = model.rbinom(
                static_cast<int>(n_reduced_infectious_per_group[g]),
                r * base_rate
            );

            for (int s = 0; s < nsamples; ++s)
            {
                uint32_t which = model.runif_index(
                    n_reduced_infectious_per_group[g]
                );

                #ifdef EPI_DEBUG
                auto & a = model.get_agents().at(
                    reduced_infectious.at(group_start[g] + which)
                );
                if (static_cast<int>(a.get_state()) != reduced_infectious_state)
                    throw std::logic_error(
                        "The sampled agent is not in the reduced infectious state."
                    );
                #else
                auto & a = model.get_agent(
                    reduced_infectious[group_start[g] + which]
                );
                #endif

                if (a.get_id() == agent->get_id())
                    continue;

                sampled_agents[samp_id++] = a.get_id();
            }
        }
    }

    // The contacts go to the layer's log, and to the post-sampling callback
    mixing->trace(sampled_agents.data(), samp_id, agent->get_id());
    if (model.has_post_sampling())
        model.register_sampled_contacts(
            sampled_agents.data(), samp_id, agent->get_id()
        );

    return samp_id;
}

#endif
