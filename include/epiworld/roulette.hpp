#ifndef EPIWORLD_ROULETTE_HPP 
#define EPIWORLD_ROULETTE_HPP

template<typename TSeq>
class Model;

/**
 * @brief Conditional Weighted Sampling
 * 
 * @details 
 * The sampling function will draw one of `{-1, 0,...,probs.size() - 1}` in a
 * weighted fashion. The probabilities are drawn given that either one or none
 * of the cases is drawn; in the latter returns -1.
 * 
 * @param probs Vector of probabilities.
 * @param m A `Model`. This is used to draw random uniform numbers.
 * @return int If -1 then it means that none got sampled, otherwise the index
 * of the entry that got drawn.
 */
template<typename TSeq>
inline int roulette(
    const std::vector< double > & probs,
    Model<TSeq> * m
    )
{

    // Step 1: Computing the prob on none 
    double p_none = 1.0;
    std::vector< int > certain_infection;
    for (unsigned int p = 0u; p < probs.size(); ++p)
    {
        p_none *= (1.0 - probs[p]);

        if (probs[p] > (1 - 1e-100))
            certain_infection.push_back(p);
        
    }

    double r = m->runif();
    // If there are one or more probs that go close to 1, sample
    // uniformly
    if (certain_infection.size() > 0)
        return certain_infection[std::floor(r * certain_infection.size())];

    // Step 2: Calculating the prob of none or single
    std::vector< double > probs_only_p;
    double p_none_or_single = p_none;
    for (unsigned int p = 0u; p < probs.size(); ++p)
    {
        probs_only_p.push_back(probs[p] * (p_none / (1.0 - probs[p])));
        p_none_or_single += probs_only_p[p];
    }

    // Step 3: Roulette
    double cumsum = p_none/p_none_or_single;
    if (r < cumsum)
        return -1;

    for (unsigned int p = 0u; v < probs.size(); ++p)
    {
        // If it yield here, then bingo, the individual will acquire the disease
        cumsum += probs_only_p[p]/(p_none_or_single);
        if (r < cumsum)
            return static_cast<int>(p);
        
    }

    return static_cast<int>(probs.size() - 1u);

}

template<typename TSeq>
inline int roulette_state(Person<TSeq> * p, Model<TSeq> * m)
{

    // Step 1: Compute the individual efficcacy
    std::vector< double > probs;
    std::vector< Virus<TSeq>* > variants;

    // Computing the efficacy
    double tmp_efficacy;
    for (unsigned int n = 0; n < p->neighbors.size(); ++n)
    {

        Person<TSeq> * neighbor = p->neighbors[n];

        // Non-infected individuals make no difference
        if ((neighbor->get_status() != STATUS::INFECTED) & (neighbor->get_status() != STATUS::ASYMPTOMATIC))
            continue;

        PersonViruses<TSeq> & nviruses = neighbor->get_viruses();
        
        // Now over the neighbor's viruses
        for (unsigned int v = 0; v < nviruses.size(); ++v)
        {

            // Computing the corresponding efficacy
            Virus<TSeq> * tmp_v = &(nviruses(v));

            // And it is a function of transmisibility as well
            tmp_efficacy = std::max(
                p->get_efficacy(tmp_v),
                (1.0 - neighbor->get_transmisibility(tmp_v))
                );
            
            probs.push_back(1.0 - tmp_efficacy);
            variants.push_back(tmp_v);

        }
        
    }

    // No virus to compute on
    if (probs.size() == 0)
        return;

    // Running the roulette
    int which = roulette(probs, m);

    if (which < 0)
        return p->get_status();

    p->add_virus(m->today(), *variants[which]);

    m->get_db().record_transmision(
        p->get_id(),
        variants[which]->get_host()->get_id(),
        variants[which]->get_id()
    );

    return STATUS::INFECTED; 
}

#endif