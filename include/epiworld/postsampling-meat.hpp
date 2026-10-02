#ifndef EPIWORLD_POSTSAMPLING_MEAT_HPP
#define EPIWORLD_POSTSAMPLING_MEAT_HPP

template<typename TSeq>
inline Model<TSeq> & Model<TSeq>::set_post_sampling(PostSamplingFun<TSeq> fun)
{

    post_sampling_fun = std::move(fun);
    post_sampling_on = static_cast< bool >(post_sampling_fun);

    // The scratch is sized at reset(); a callback installed on a model that
    // already has agents sizes it now.
    if (post_sampling_on && (population.size() > 0u))
    {
        if (population.size() >= (size_t(1) << 32))
            throw std::length_error(
                "The post-sampling callback supports populations below 2^32 agents."
            );
        post_sampling_scratch.reset(population.size());
    }

    return *this;

}

template<typename TSeq>
inline Model<TSeq> & Model<TSeq>::clear_post_sampling()
{
    return set_post_sampling(nullptr);
}

template<typename TSeq>
inline void Model<TSeq>::register_sampled_contact(
    size_t infectious_id,
    size_t contacted_id
)
{
    post_sampling_scratch.pairs.push_back(static_cast< uint32_t >(infectious_id));
    post_sampling_scratch.pairs.push_back(static_cast< uint32_t >(contacted_id));
}

template<typename TSeq>
inline void Model<TSeq>::register_sampled_contacts(
    const size_t * infectious_ids,
    size_t n,
    size_t contacted_id
)
{
    auto & pairs = post_sampling_scratch.pairs;
    for (size_t k = 0u; k < n; ++k)
    {
        pairs.push_back(static_cast< uint32_t >(infectious_ids[k]));
        pairs.push_back(static_cast< uint32_t >(contacted_id));
    }
}

/**
 * Groups the pairs of the step by infectious agent (only the distinct ids are
 * sorted, never an N-sized array) and runs the callback on each group, in
 * ascending id order. The scratch is left clean even if the callback throws.
 */
template<typename TSeq>
inline void Model<TSeq>::post_sampling_dispatch()
{

    auto & sc = post_sampling_scratch;
    const size_t npairs = sc.pairs.size() / 2u;

    if (npairs == 0u)
        return;

    try
    {

        // Counting per infectious agent, remembering the distinct ones
        for (size_t k = 0u; k < npairs; ++k)
        {
            const uint32_t i = sc.pairs[2u * k];
            if (sc.counts[i]++ == 0u)
                sc.touched.push_back(i);
        }

        std::sort(sc.touched.begin(), sc.touched.end());

        // Turning the counts into start positions
        const size_t nt = sc.touched.size();
        sc.starts.resize(nt + 1u);
        size_t pos = 0u;
        for (size_t t = 0u; t < nt; ++t)
        {
            const uint32_t i = sc.touched[t];
            sc.starts[t] = pos;
            pos += sc.counts[i];
            sc.counts[i] = static_cast< uint32_t >(sc.starts[t]);
        }
        sc.starts[nt] = pos;

        // Scattering the contacted ids
        sc.grouped.resize(npairs);
        for (size_t k = 0u; k < npairs; ++k)
            sc.grouped[sc.counts[sc.pairs[2u * k]]++] = sc.pairs[2u * k + 1u];

        // Callbacks must not retain the view: the buffers are reused.
        for (size_t t = 0u; t < nt; ++t)
        {

            const size_t len = sc.starts[t + 1u] - sc.starts[t];
            SampledContactsView view(sc.grouped.data() + sc.starts[t], len);
            post_sampling_fun(&population[sc.touched[t]], view, this);

        }

    }
    catch (...)
    {
        sc.clear();
        throw;
    }

    sc.clear();

}

/**
 * @brief A `PostSamplingFun` that records each batch in the model's
 * `ContactTracing`.
 *
 * @details For every contact, it calls
 * `add_contact(infectious_id, contacted_id, today)`, which is what the
 * built-in models with tracing used to do inline. The model needs
 * `contact_tracing_on()`; turning tracing on does not install this callback.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline PostSamplingFun<TSeq> make_contact_tracing_post_sampling()
{

    return [](
        Agent<TSeq> * infectious,
        const SampledContactsView & contacts,
        Model<TSeq> * m
    ) -> void {

        if (!m->is_contact_tracing_on())
            return;

        auto & ct = m->get_contact_tracing();
        const size_t id = infectious->get_id();
        const size_t today = static_cast< size_t >(m->today());
        for (const auto c : contacts)
            ct.add_contact(id, c, today);

    };

}

#endif
