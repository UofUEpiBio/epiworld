#ifndef EPIWORLD_MODELS_SAMPLERMIXING_HPP
#define EPIWORLD_MODELS_SAMPLERMIXING_HPP

using namespace epiworld;

/**
 * @brief Reusable contact sampler for mixing models with two infectious stages.
 *
 * The sampler keeps the per-entity infectious-agent accounting and contact
 * sampling buffer used by ModelMeaslesMixing. The primary infectious state is
 * sampled at full weight; the reduced infectious state is sampled with a
 * relative contact rate computed as 1 minus a model parameter such as
 * "Rash reduction contact rate".
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
    int recovered_state = -1;
    std::string reduced_contact_reduction_param;
    std::vector< int > contact_states;

    std::vector< size_t > primary_infectious;
    std::vector< size_t > reduced_infectious;

    std::vector< size_t > n_primary_infectious_per_group;
    std::vector< size_t > n_reduced_infectious_per_group;

    std::vector< double > ingroup_weights_primary;
    std::vector< size_t > entity_indices;
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
     * @brief Constructs and configures the sampler.
     *
     * @param primary_infectious_state_ State index for fully infectious agents.
     * @param reduced_infectious_state_ State index for reduced-infectious agents.
     * @param recovered_state_ State index considered recovered/immune.
     * @param reduced_contact_reduction_param_ Model parameter name controlling
     * reduced infectious contact reduction.
     * @param contact_states_ Explicit states eligible for contact.
     */
    SamplerMixing(
        int primary_infectious_state_,
        int reduced_infectious_state_,
        int recovered_state_,
        std::string reduced_contact_reduction_param_,
        std::vector< int > contact_states_
    );

    /**
     * @brief Configures state indices and contact-selection settings.
     *
     * @param primary_infectious_state_ State index for fully infectious agents.
     * @param reduced_infectious_state_ State index for reduced-infectious agents.
     * @param recovered_state_ State index considered recovered/immune.
     * @param reduced_contact_reduction_param_ Model parameter name controlling
     * reduced infectious contact reduction.
     * @param contact_states_ Explicit states eligible for contact.
     */
    void configure(
        int primary_infectious_state_,
        int reduced_infectious_state_,
        int recovered_state_,
        std::string reduced_contact_reduction_param_,
        std::vector< int > contact_states_
    );

    /**
     * @brief Resets internal buffers and recomputes infectious pools.
     *
     * @param model Model containing current agents and entities.
     */
    void reset(Model<TSeq> & model);

    /**
     * @brief Updates infectious pools and per-group sampling weights.
     *
     * @param model Model containing current agents and entities.
     */
    void update(Model<TSeq> & model);

    /**
     * @brief Samples infectious contacts into the internal sampled-agent buffer.
     *
     * @param agent Agent for which contacts are sampled.
     * @param model Model containing current agent states.
     * @param contact_matrix Matrix with between-group contact rates.
     *
     * @return Number of sampled contacts written to the internal buffer.
     */
    size_t sample(
        Agent<TSeq> * agent,
        Model<TSeq> & model,
        const ContactMatrix & contact_matrix
    );

    /**
     * @brief Samples infectious contacts into a user-provided buffer.
     *
     * @param agent Agent for which contacts are sampled.
     * @param model Model containing current agent states.
     * @param contact_matrix Matrix with between-group contact rates.
     * @param sampled_agents_ Output buffer receiving sampled agent ids.
     *
     * @return Number of sampled contacts written to @p sampled_agents_.
     */
    size_t sample(
        Agent<TSeq> * agent,
        Model<TSeq> & model,
        const ContactMatrix & contact_matrix,
        std::vector< size_t > & sampled_agents_
    );

    /**
     * @brief Returns the internal sampled-agent buffer (const).
     */
    const std::vector< size_t > & get_sampled_agents() const
    {
        return sampled_agents;
    }

    /**
     * @brief Returns the internal sampled-agent buffer.
     */
    std::vector< size_t > & get_sampled_agents()
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
inline SamplerMixing<TSeq>::SamplerMixing(
    int primary_infectious_state_,
    int reduced_infectious_state_,
    int recovered_state_,
    std::string reduced_contact_reduction_param_,
    std::vector< int > contact_states_
)
{
    configure(
        primary_infectious_state_,
        reduced_infectious_state_,
        recovered_state_,
        reduced_contact_reduction_param_,
        contact_states_
    );
}

template<typename TSeq>
inline void SamplerMixing<TSeq>::configure(
    int primary_infectious_state_,
    int reduced_infectious_state_,
    int recovered_state_,
    std::string reduced_contact_reduction_param_,
    std::vector< int > contact_states_
)
{
    primary_infectious_state = primary_infectious_state_;
    reduced_infectious_state = reduced_infectious_state_;
    recovered_state = recovered_state_;
    reduced_contact_reduction_param = reduced_contact_reduction_param_;
    contact_states = contact_states_;
}

template<typename TSeq>
inline double SamplerMixing<TSeq>::get_reduced_contact_rate(
    Model<TSeq> & model
) const
{
    if (reduced_contact_reduction_param.empty())
        return 0.0;

    return 1.0 - model.par(reduced_contact_reduction_param);
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
    auto & entities = model.get_entities();

    sampled_agents.resize(model.size());
    n_primary_infectious_per_group.assign(entities.size(), 0u);
    n_reduced_infectious_per_group.assign(entities.size(), 0u);
    ingroup_weights_primary.assign(entities.size(), 1.0);

    primary_infectious.assign(model.size(), 0u);
    reduced_infectious.assign(model.size(), 0u);

    entity_indices.assign(entities.size(), 0u);

    for (size_t i = 1u; i < entities.size(); ++i)
    {
        entity_indices[i] +=
            entities[i - 1].size() +
            entity_indices[i - 1]
            ;
    }

    update(model);
}

template<typename TSeq>
inline void SamplerMixing<TSeq>::update(Model<TSeq> & model)
{
    auto & agents = model.get_agents();
    auto & entities = model.get_entities();

    std::fill(
        n_primary_infectious_per_group.begin(),
        n_primary_infectious_per_group.end(),
        0u
    );
    std::fill(
        n_reduced_infectious_per_group.begin(),
        n_reduced_infectious_per_group.end(),
        0u
    );

    adjusted_contact_rate.assign(entities.size(), 0.0);

    double reduced_contact_rate = get_reduced_contact_rate(model);
    bool include_reduced = reduced_contact_rate > 0.0;

    // We need to check all agents to see which ones
    // will be included in the infectious pools
    for (const auto & a : agents)
    {
        int state = static_cast<int>(a.get_state());
        auto n_entities = a.get_n_entities();

        if (state == primary_infectious_state)
        {
            if (n_entities > 0u)
            {
                const auto & entity = a.get_entity(0u, model);
                primary_infectious[
                    entity_indices[entity.get_id()] +
                    n_primary_infectious_per_group[entity.get_id()]++
                ] = a.get_id();
            }
        }
        else if (include_reduced && (state == reduced_infectious_state))
        {
            if (n_entities > 0u)
            {
                const auto & entity = a.get_entity(0u, model);
                reduced_infectious[
                    entity_indices[entity.get_id()] +
                    n_reduced_infectious_per_group[entity.get_id()]++
                ] = a.get_id();
            }
        }

        if (
            is_available_for_contact(state, include_reduced) &&
            (n_entities > 0u)
        )
        {
            adjusted_contact_rate[
                a.get_entity(0u, model).get_id()
            ] += 1.0;
        }
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

    if (include_reduced)
    {
        ingroup_weights_primary.assign(entities.size(), 0.0);

        for (size_t g = 0; g < entities.size(); ++g)
        {
            auto tot =
                n_primary_infectious_per_group[g] +
                n_reduced_infectious_per_group[g] * reduced_contact_rate;

            if (tot > 0.0)
            {
                ingroup_weights_primary[g] =
                    static_cast<double>(n_primary_infectious_per_group[g]) /
                    tot;
            }
        }
    }
    else
    {
        ingroup_weights_primary.assign(entities.size(), 1.0);
    }
}

template<typename TSeq>
inline size_t SamplerMixing<TSeq>::sample(
    Agent<TSeq> * agent,
    Model<TSeq> & model,
    const ContactMatrix & contact_matrix
)
{
    return sample(agent, model, contact_matrix, sampled_agents);
}

template<typename TSeq>
inline size_t SamplerMixing<TSeq>::sample(
    Agent<TSeq> * agent,
    Model<TSeq> & model,
    const ContactMatrix & contact_matrix,
    std::vector< size_t > & sampled_agents_
)
{
    // If the agent is not in any entity, we cannot 
    // find them in the contact matrix.
    if (agent->get_n_entities() == 0u)
        return 0u;

    size_t agent_group_id = agent->get_entity(0u, model).get_id();
    size_t ngroups = model.get_entities().size();

    // The reduced-contact rate for the secondary infectious pool (e.g. rash).
    // r = 0  means no contacts at all; r = 1 means full contact rate.
    double r = get_reduced_contact_rate(model);

    // Sampling accross all groups
    size_t samp_id = 0u;
    for (size_t g = 0; g < ngroups; ++g)
    {
        double base_rate =
            adjusted_contact_rate[g] *
            contact_matrix.get_contact_rate(agent_group_id, g, false);

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
                    primary_infectious.at(entity_indices[g] + which)
                );
                if (static_cast<int>(a.get_state()) != primary_infectious_state)
                    throw std::logic_error(
                        "The sampled agent is not in the primary infectious state."
                    );
                #else
                auto & a = model.get_agent(
                    primary_infectious[entity_indices[g] + which]
                );
                #endif

                if (a.get_id() == agent->get_id())
                    continue;

                sampled_agents_[samp_id++] = a.get_id();
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
                    reduced_infectious.at(entity_indices[g] + which)
                );
                if (static_cast<int>(a.get_state()) != reduced_infectious_state)
                    throw std::logic_error(
                        "The sampled agent is not in the reduced infectious state."
                    );
                #else
                auto & a = model.get_agent(
                    reduced_infectious[entity_indices[g] + which]
                );
                #endif

                if (a.get_id() == agent->get_id())
                    continue;

                sampled_agents_[samp_id++] = a.get_id();
            }
        }
    }

    return samp_id;
}

#endif
