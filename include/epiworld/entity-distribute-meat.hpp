#ifndef EPIWORLD_ENTITY_DISTRIBUTE_MEAT_HPP
#define EPIWORLD_ENTITY_DISTRIBUTE_MEAT_HPP


template <typename TSeq = EPI_DEFAULT_TSEQ>
/**
 * Distributes an entity to unassigned agents in the model.
 * 
 * @return An EntityToAgentFun object that distributes the entity to unassigned
 * agents.
 */
inline EntityToAgentFun<TSeq> distribute_entity_to_unassigned()
{

    return [](Entity<TSeq> & e, Model<TSeq> * m) -> void {

        
        // Preparing the sampling space
        std::vector< size_t > idx;
        for (const auto & a: m->get_agents())
            if (a.get_n_entities() == 0)
                idx.push_back(a.get_id());
        size_t n = idx.size();

        // Figuring out how many to sample
        int n_to_sample;
        if (e.get_prevalence_as_proportion())
        {
            n_to_sample = static_cast<int>(std::floor(e.get_prevalence() * n));
            if (n_to_sample > static_cast<int>(n))
                --n_to_sample;

        } else
        {
            n_to_sample = static_cast<int>(e.get_prevalence());
            if (n_to_sample > static_cast<int>(n))
                throw std::range_error("There are only " + std::to_string(n) + 
                " individuals in the population. Cannot add the entity to " +
                    std::to_string(n_to_sample));
        }

        int n_left = n;
        for (int i = 0; i < n_to_sample; ++i)
        {
            int loc = static_cast<epiworld_fast_uint>(
                floor(m->runif() * n_left--)
                );

            // Correcting for possible overflow
            if ((loc > 0) && (loc >= n_left))
                loc = n_left - 1;

            m->get_agent(idx[loc]).add_entity(e, m);

            std::swap(idx[loc], idx[n_left]);

        }

    };

}

template<typename TSeq = EPI_DEFAULT_TSEQ>
/**
 * Distributes an entity to a range of agents.
 *
 * @param from The starting index of the range.
 * @param to The ending index of the range.
 * @param to_unassigned Flag indicating whether to distribute the entity only
 * to unassigned agents.
 * @return A lambda function that distributes the entity to the specified range
 * of agents.
 */
inline EntityToAgentFun<TSeq> distribute_entity_to_range(
    int from,
    int to,
    bool to_unassigned = false
    ) {

    if (to_unassigned)
    {

        return [from, to](Entity<TSeq> & e, Model<TSeq> * m) -> void {

            auto & agents = m->get_agents();
            for (size_t i = from; i < to; ++i)
            {
                if (agents[i].get_n_entities() == 0)
                    e.add_agent(&agents[i], m);
                else
                    throw std::logic_error(
                        "Agent " + std::to_string(i) + " already has an entity."
                    );
            }
            
            return;

        };

    }
    else
    {

        return [from, to](Entity<TSeq> & e, Model<TSeq> * m) -> void {

            auto & agents = m->get_agents();
            for (size_t i = from; i < to; ++i)
            {
                e.add_agent(&agents[i], m);
            }
            
            return;

        };

    }
}


template<typename TSeq = EPI_DEFAULT_TSEQ>
inline EntityToAgentFun<TSeq> distribute_entity_to_set(
    std::vector< size_t > & idx
    ) {

    return [idx](Entity<TSeq> & e, Model<TSeq> * m) -> void {

        for (const auto & i: idx)
        {
            e.add_agent(&m->get_agent(i), m);
        }

    };

}

#endif