#ifndef EPIWORLD_PERSON_MEAT_STATUS_HPP 
#define EPIWORLD_PERSON_MEAT_STATUS_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Person;

template<typename TSeq = int>
inline void default_update_susceptible(
    Person<TSeq> * p,
    Model<TSeq> * m
    )
{

    // This computes the prob of getting any neighbor variant
    unsigned int nvariants_tmp = 0u;
    for (auto & neighbor: p->get_neighbors()) 
    {
                 
        for (auto & v : neighbor->get_viruses()) 
        { 
                
            /* And it is a function of susceptibility_reduction as well */ 
            m->array_double_tmp[nvariants_tmp] =
                (1.0 - p->get_susceptibility_reduction(v)) * 
                v->get_prob_infecting() * 
                (1.0 - neighbor->get_transmission_reduction(v)) 
                ; 
        
            m->array_virus_tmp[nvariants_tmp++] = &v;
            
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
inline void default_update_exposed(Person<TSeq> * p, Model<TSeq> * m) {

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