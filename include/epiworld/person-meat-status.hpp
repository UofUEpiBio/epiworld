#ifndef EPIWORLD_PERSON_MEAT_STATUS_HPP 
#define EPIWORLD_PERSON_MEAT_STATUS_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Person;

/**
 * @brief Helper macro to add virus and update the db
 * 
 */
#define EPIWORLD_ADD_VIRUS(virus_ptr,new_state) \
    {p->add_virus(m->today(), *virus_ptr); \
    /* Recording information in the database */ \
    if (m->is_queuing_on()) m->get_queue() += p; \
    m->get_db().record_transmision( \
        p->get_id(), \
        virus_ptr->get_host()->get_id(), \
        virus_ptr->get_id() \
    ); \
    m->get_db().up_infected(virus_ptr, p->get_status(), new_state);\
    return new_state;}


#define EPIWORLD_UPDATE_SUSCEPTIBLE_CALC_PROBS(probs,variants) \
    /* Step 1: Compute the individual efficcacy */ \
    std::vector< epiworld_double > probs; \
    std::vector< epiworld::Virus<TSeq>* > variants; \
    /* Computing the susceptibility_reduction */ \
    for (unsigned int n = 0; n < p->get_neighbors().size(); ++n) \
    { \
        epiworld::Person<TSeq> * neighbor = p->get_neighbors()[n]; \
        /* Non-infected individuals make no difference */ \
        if (!epiworld::IN(neighbor->get_status(), m->get_status_infected())) \
            continue; \
        epiworld::PersonViruses<TSeq> & nviruses = neighbor->get_viruses(); \
        /* Now over the neighbor's viruses */ \
        epiworld_double tmp_transmision; \
        for (unsigned int v = 0; v < nviruses.size(); ++v) \
        { \
            /* Computing the corresponding susceptibility_reduction */ \
            epiworld::Virus<TSeq> * tmp_v = &(nviruses(v)); \
            /* And it is a function of susceptibility_reduction as well */ \
            tmp_transmision = \
                (1.0 - p->get_susceptibility_reduction(tmp_v)) * \
                tmp_v->get_prob_infecting() * \
                (1.0 - neighbor->get_transmission_reduction(tmp_v)) \
                ; \
            probs.push_back(tmp_transmision); \
            variants.push_back(tmp_v); \
        } \
    }

template<typename TSeq>
inline epiworld_fast_uint default_update_susceptible(
    Person<TSeq> * p,
    Model<TSeq> * m
    )
{

    // This computes the prob of getting any neighbor variant
    unsigned int nvariants_tmp = 0u;
    for (unsigned int n = 0; n < p->get_neighbors().size(); ++n) 
    { 

        Person<TSeq> * neighbor = p->get_neighbors()[n]; 
        
        /* Non-infected individuals make no difference */ 
        if (!IN(neighbor->get_status(), m->get_status_infected())) 
            continue; 
        
        PersonViruses<TSeq> & nviruses = neighbor->get_viruses(); 
        /* Now over the neighbor's viruses */ 
        
        epiworld_double tmp_transmision; 
        for (unsigned int v = 0; v < nviruses.size(); ++v) 
        { 
        
            /* Computing the corresponding susceptibility_reduction */ 
            Virus<TSeq> * tmp_v = &(nviruses(v)); 
        
            /* And it is a function of susceptibility_reduction as well */ 
            tmp_transmision = 
                (1.0 - p->get_susceptibility_reduction(tmp_v)) * 
                tmp_v->get_prob_infecting() * 
                (1.0 - neighbor->get_transmission_reduction(tmp_v)) 
                ; 
        
            m->array_double_tmp[nvariants_tmp]  = tmp_transmision;
            m->array_virus_tmp[nvariants_tmp++] = tmp_v;
            
        } 
    }

    // No virus to compute
    if (nvariants_tmp == 0u)
        return p->get_status();

    // Running the roulette
    int which = roulette(nvariants_tmp, m);

    if (which < 0)
        return p->get_status();

    EPIWORLD_ADD_VIRUS(
        m->array_virus_tmp[which],
        m->get_default_infected()
        )

}

#define EPIWORLD_UPDATE_INFECTED_CALC_PROBS(prob_rec, prob_die) \
    epiworld::Virus<TSeq> * v = &(p->get_virus(0u)); \
    epiworld_double prob_rec = v->get_prob_recovery() * (1.0 - p->get_recovery_enhancer(v)); \
    epiworld_double prob_die = v->get_prob_death() * (1.0 - p->get_death_reduction(v)); 

#define EPIWORLD_UPDATE_INFECTED_REMOVE(newstatus) \
    {m->get_db().down_infected(v, p->get_status(), newstatus);\
    if (m->is_queuing_on()) m->get_queue() -= p; \
    p->get_viruses().reset();\
    return static_cast<epiworld_fast_uint>(newstatus);}

#define EPIWORLD_UPDATE_INFECTED_RECOVER(newstatus) \
    {m->get_db().down_infected(v, p->get_status(), newstatus);\
    if (m->is_queuing_on()) m->get_queue() -= p; \
    v->post_recovery();p->get_viruses().reset();\
    return static_cast<epiworld_fast_uint>(newstatus);}

template<typename TSeq>
inline epiworld_fast_uint default_update_infected(Person<TSeq> * p, Model<TSeq> * m) {

    epiworld::Virus<TSeq> * v = &(p->get_virus(0u)); 
    epiworld_double p_rec = v->get_prob_recovery() * (1.0 - p->get_recovery_enhancer(v)); 
    epiworld_double p_die = v->get_prob_death() * (1.0 - p->get_death_reduction(v)); 
    
    epiworld_double r = EPI_RUNIF();
    epiworld_double cumsum = p_die * (1 - p_rec) / (1.0 - p_die * p_rec); 

    if (r < cumsum)
    {
        EPIWORLD_UPDATE_INFECTED_REMOVE(m->get_default_removed());
    }
    
    cumsum += p_rec * (1 - p_die) / (1.0 - p_die * p_rec);
    
    if (r < cumsum)
    {
        EPIWORLD_UPDATE_INFECTED_RECOVER(m->get_default_recovered())
    }

    return p->get_status();

}

#endif