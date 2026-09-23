#ifndef EPIWORLD_MODEL_MEAT_TRANSMISSION_HPP
#define EPIWORLD_MODEL_MEAT_TRANSMISSION_HPP

/**
 * @file model-meat-transmission.hpp
 * @brief Network transmission by pushing infection odds.
 *
 * @details A susceptible agent `i` whose update function is
 * `default_update_susceptible` (or `sampler::make_update_susceptible()`)
 * *pulls*: it scans its neighbors, collects the per-contact probabilities
 * `p_ij` of those carrying a virus, and `roulette()` draws "no infection" or a
 * single infector. That draw depends only on the odds `r_ij = p_ij / (1 - p_ij)`:
 *
 *     P(no infection) = 1 / (1 + R_i),   P(infected by j) = r_ij / (1 + R_i),
 *
 * with `R_i` the sum of the odds. So the same outcome can be *pushed*: every
 * agent carrying a virus adds its odds to its susceptible neighbors, each of
 * which keeps a weighted reservoir sample of its infectors; then each touched
 * agent makes one draw. The cost follows the carriers' ties instead of the
 * susceptibles', which is much cheaper while an outbreak is small.
 *
 * Contacts with `p_ij >= 1` are certain: the infection happens and the
 * infector is drawn uniformly among them (the limit of the odds as `p -> 1`).
 *
 * The docs page "Push and Pull Transmission" (docs/impl/transmission-sampling.md)
 * has the derivation.
 */

template<typename TSeq>
inline bool Model<TSeq>::transmission_prepare()
{

    const size_t ns = static_cast< size_t >(nstates);

    push_pushable.assign(ns, 0);
    push_default.assign(ns, 0);
    push_excluded.assign(ns * ns, 0);
    push_source_ok.assign(ns, 0);

    bool any = false;
    for (size_t t = 0u; t < ns; ++t)
    {

        const auto & fun = state_fun[t];
        if (!fun)
            continue;

        using FunPtr = void(*)(Agent<TSeq>*, Model<TSeq>*);
        const FunPtr * fp = fun.template target< FunPtr >();

        if ((fp != nullptr) && (*fp == &default_update_susceptible<TSeq>))
        {
            push_pushable[t] = 1;
            push_default[t] = 1;
            any = true;
        }
        else if (
            const auto * us = fun.template target< sampler::UpdateSusceptible<TSeq> >()
        )
        {

            push_pushable[t] = 1;
            any = true;

            for (auto s : us->exclude)
            {
                if (s >= ns)
                    throw std::logic_error(
                        std::string("You are trying to exclude a state that is out of range: ") +
                        std::to_string(s) + std::string(". There are only ") +
                        std::to_string(ns) + std::string(" states in the model.")
                        );

                push_excluded[t * ns + s] = 1;
            }

        }

    }

    // A state can be a source if some pushable state takes infections from it
    for (size_t t = 0u; t < ns; ++t)
        if (push_pushable[t])
            for (size_t s = 0u; s < ns; ++s)
                if (!push_excluded[t * ns + s])
                    push_source_ok[s] = 1;

    return any;

}

template<typename TSeq>
inline bool Model<TSeq>::transmission_choose_push() const
{

    if (transmission_mode == TransmissionMode::pull)
        return false;

    if (transmission_mode == TransmissionMode::push)
        return true;

    // Pushing walks every tie of every carrier that can transmit; pulling walks
    // every tie of every susceptible agent. Both sums are kept per state, so
    // this is O(number of states). It deliberately ignores the queue: the
    // decision -- and so the random stream -- is the same with queuing on or
    // off.
    double cost_push = 0.0;
    double cost_pull = 0.0;
    for (size_t s = 0u; s < static_cast< size_t >(nstates); ++s)
    {
        if (push_source_ok[s])
            cost_push += static_cast< double >(state_carrier_degree[s]);
        if (push_pushable[s])
            cost_pull += static_cast< double >(state_degree[s]);
    }

    return cost_push <= transmission_kappa * cost_pull;

}

template<typename TSeq>
inline void Model<TSeq>::transmission_push()
{

    const size_t ns = static_cast< size_t >(nstates);

    if (push_slot.size() != population.size())
        push_slot.assign(population.size(), -1);

    push_targets.clear();

    // Phase 1: every carrier that can transmit adds its odds to its eligible
    // neighbors. Nothing changes state until events_run(), so this sees the
    // model as it was at the start of the step, as pulling does.
    for (size_t s = 0u; s < ns; ++s)
    {

        if (!push_source_ok[s] || (state_carriers[s] == 0u))
            continue;

        for (size_t j_id : state_members[s])
        {

            Agent<TSeq> & j = population[j_id];
            if ((j.virus == nullptr) || (j.n_neighbors == 0u))
                continue;

            VirusPtr<TSeq> & v = j.virus;

            for (size_t i_id : *j.neighbors)
            {

                Agent<TSeq> & i = population[i_id];
                const size_t t = i.state;

                if (!push_pushable[t] || push_excluded[t * ns + s])
                    continue;

                // An agent with a virus is not susceptible (pulling would
                // refuse it; update_state() reports it).
                if (i.virus != nullptr)
                    continue;

                // Pulling only updates queued agents.
                if (use_queuing && (queue[i_id] <= 0))
                    continue;

                // Exactly the expression the pull uses, in the same order.
                epiworld_double p =
                    (1.0 - i.get_susceptibility_reduction(v, *this)) *
                    v->get_prob_infecting(this) *
                    (1.0 - j.get_transmission_reduction(v, *this))
                    ;

                // No chance of transmission (also catches NaN)
                if (!(p > 0.0))
                    continue;

                int & slot = push_slot[i_id];
                if (slot < 0)
                {

                    slot = static_cast< int >(push_targets.size());
                    push_targets.push_back({i_id, 0.0, 0u, nullptr});

                    #ifdef EPI_DEBUG
                    if (push_default[t])
                        db.n_transmissions_potential++;
                    #endif

                }

                PushTarget & target = push_targets[static_cast< size_t >(slot)];

                // Certain transmission: uniform reservoir among these
                if (p >= 1.0)
                {

                    if (
                        (++target.n_certain == 1u) ||
                        (runif() * static_cast< double >(target.n_certain) < 1.0)
                    )
                        target.candidate = &(*v);

                    continue;

                }

                // A certain transmission wins outright
                if (target.n_certain > 0u)
                    continue;

                // Weighted reservoir: keep this infector with probability
                // r / (sum of odds so far).
                const double odds = static_cast< double >(p) /
                    (1.0 - static_cast< double >(p));
                target.odds += odds;

                if (
                    (target.candidate == nullptr) ||
                    (runif() * target.odds < odds)
                )
                    target.candidate = &(*v);

            }

        }

    }

    // Phase 2: one draw per touched agent. P(no infection) = 1 / (1 + R).
    for (auto & target : push_targets)
    {

        push_slot[target.id] = -1;

        if (target.candidate == nullptr)
            continue;

        if (
            (target.n_certain == 0u) &&
            (runif() < 1.0 / (1.0 + target.odds))
        )
            continue;

        Agent<TSeq> & i = population[target.id];

        #ifdef EPI_DEBUG
        if (push_default[i.state])
            db.n_transmissions_today++;
        #endif

        i.set_virus(*this, *target.candidate);

    }

}

template<typename TSeq>
inline void Model<TSeq>::transmission_update_others()
{

    const size_t ns = static_cast< size_t >(nstates);

    // Pulling refuses an agent in a susceptible state that carries a virus;
    // so does pushing, for the agents a pull would have visited.
    for (size_t s = 0u; s < ns; ++s)
    {

        if (!push_pushable[s] || (state_carriers[s] == 0u))
            continue;

        for (size_t id : state_members[s])
        {

            const auto & p = population[id];
            if ((p.virus != nullptr) && (!use_queuing || (queue[id] > 0)))
                throw std::logic_error(
                    std::string("Using the -default_update_susceptible- on agents WITH viruses makes no sense! ") +
                    std::string("Agent id ") + std::to_string(p.get_id()) +
                    std::string(" has a virus.")
                    );

        }

    }

    // Everyone in a state with an update function, other than the pushed ones.
    // The index gives them directly, so neither the population nor the queue
    // (which holds every neighbor of every carrier) needs to be scanned. Sorted
    // so the visiting order is ascending id, whether queuing is on or off.
    push_visit.clear();
    for (size_t s = 0u; s < ns; ++s)
        if (state_fun[s] && !push_pushable[s])
            push_visit.insert(
                push_visit.end(), state_members[s].begin(), state_members[s].end()
            );

    std::sort(push_visit.begin(), push_visit.end());

    for (size_t id : push_visit)
    {

        // Queued agents only, read as the loop reaches them (a state function
        // may change the queue by editing ties).
        if (use_queuing && (queue[id] <= 0))
            continue;

        auto & p = population[id];
        state_fun[p.state](&p, this);

    }

}

template<typename TSeq>
inline Model<TSeq> & Model<TSeq>::set_transmission_mode(TransmissionMode mode)
{
    transmission_mode = mode;
    return *this;
}

template<typename TSeq>
inline Model<TSeq> & Model<TSeq>::set_transmission_mode(std::string_view mode)
{

    if (mode == "auto")
        transmission_mode = TransmissionMode::automatic;
    else if (mode == "push")
        transmission_mode = TransmissionMode::push;
    else if (mode == "pull")
        transmission_mode = TransmissionMode::pull;
    else
        throw std::invalid_argument(
            "Unknown transmission mode \"" + std::string(mode) +
            "\". Use \"auto\", \"push\", or \"pull\"."
        );

    return *this;

}

template<typename TSeq>
inline TransmissionMode Model<TSeq>::get_transmission_mode() const
{
    return transmission_mode;
}

template<typename TSeq>
inline TransmissionMode Model<TSeq>::get_last_transmission_mode() const
{
    return transmission_mode_last;
}

template<typename TSeq>
inline Model<TSeq> & Model<TSeq>::set_transmission_kappa(double kappa)
{

    if (!(kappa >= 0.0) || std::isinf(kappa))
        throw std::range_error(
            "The transmission kappa must be a finite, non-negative number."
        );

    transmission_kappa = kappa;
    return *this;

}

template<typename TSeq>
inline double Model<TSeq>::get_transmission_kappa() const
{
    return transmission_kappa;
}

#endif
