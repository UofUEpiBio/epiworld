#ifndef EPIWORLD_PERSON_MEAT_STATUS_HPP 
#define EPIWORLD_PERSON_MEAT_STATUS_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Agent;

template<typename TSeq = int>
inline void default_update_susceptible(
    Agent<TSeq> * p,
    Model<TSeq> * m
    )
{

    if (p->get_n_viruses() > 0u)
        throw std::logic_error(
            std::string("Using the -default_update_susceptible- on agents WITH viruses makes no sense! ") +
            std::string("Agent id ") + std::to_string(p->get_id()) +
            std::string(" has ") + std::to_string(p->get_n_viruses()) +
            std::string(" viruses.")
            );

    // This computes the prob of getting any neighbor variant
    size_t nvariants_tmp = 0u;
    for (auto & neighbor: p->get_neighbors()) 
    {
                 
        for (const VirusPtr<TSeq> & v : neighbor->get_viruses()) 
        { 

            #ifdef EPI_DEBUG
            if (nvariants_tmp >= m->array_virus_tmp.size())
                throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                // printf_epiworld("N used %d\n", v.use_count());
            #endif
                
            /* And it is a function of susceptibility_reduction as well */ 
            m->array_double_tmp[nvariants_tmp] =
                (1.0 - p->get_susceptibility_reduction(v)) * 
                v->get_prob_infecting() * 
                (1.0 - neighbor->get_transmission_reduction(v)) 
                ; 
        
            m->array_virus_tmp[nvariants_tmp++] = &(*v);
            
        } 
    }

    // No virus to compute
    if (nvariants_tmp == 0u)
        return;

    // Running the roulette
    int which = roulette(nvariants_tmp, m);

    if (which < 0)
        return;

    p->add_virus(*m->array_virus_tmp[which]); 

    return;

}

template<typename TSeq = int>
inline void default_update_exposed(Agent<TSeq> * p, Model<TSeq> * m) {

    if (p->get_n_viruses() == 0u)
        throw std::logic_error(
            std::string("Using the -default_update_exposed- on agents WITHOUT viruses makes no sense! ") +
            std::string("Agent id ") + std::to_string(p->get_id()) + std::string(" has no virus registered.")
            );

    // Odd: Die, Even: Recover
    epiworld_fast_uint n_events = 0u;
    for (const auto & v : p->get_viruses())
    {

        // Die
        m->array_double_tmp[n_events++] = 
            v->get_prob_death() * (1.0 - p->get_death_reduction(v)); 

        // Recover
        m->array_double_tmp[n_events++] = 
            v->get_prob_recovery() * (1.0 - p->get_recovery_enhancer(v)); 

    }
    
    if (n_events == 0u)
        return;

    // Running the roulette
    int which = roulette(n_events, m);

    if (which < 0)
        return;

    // Which roulette happen?
    if ((which % 2) == 0) // If odd
    {

        size_t which_v = std::ceil(which / 2);
        
        // Retrieving the default values of the virus
        epiworld::VirusPtr<TSeq> & v = p->get_viruses()[which_v];
        epiworld_fast_int dead_status, dead_queue;
        v->get_status(nullptr, nullptr, &dead_status);
        v->get_queue(nullptr, nullptr, &dead_queue);

        // Applying change of status
        p->change_status(
            // Either preserve the current status or apply a new one
            (dead_status < 0) ? p->get_status() : static_cast<epiworld_fast_uint>(dead_status),

            // By default, it will be removed from the queue... unless the user
            // says the contrary!
            (dead_queue == -99) ? -m->get_queue()[p->get_index()] : dead_queue
            );

    } else {

        size_t which_v = std::floor(which / 2);
        p->rm_virus(which_v);

    }

    return ;

}

#endif