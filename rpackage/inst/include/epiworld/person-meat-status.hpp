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
    p->add_virus(m->today(), *virus_ptr); \
    /* Recording information in the database */ \
    m->get_db().record_transmision( \
        p->get_id(), \
        virus_ptr->get_host()->get_id(), \
        virus_ptr->get_id() \
    ); \
    m->get_db().up_infected(virus_ptr, p->get_status(), STATUS::INFECTED);


#define EPIWORLD_UPDATE_SUSCEPTIBLE_CALC_PROBS(probs,variants) \
    /* Step 1: Compute the individual efficcacy */ \
    std::vector< double > probs; \
    std::vector< Virus<TSeq>* > variants; \
    /* Computing the efficacy */ \
    for (unsigned int n = 0; n < p->get_neighbors().size(); ++n) \
    { \
        Person<TSeq> * neighbor = p->get_neighbors()[n]; \
        /* Non-infected individuals make no difference */ \
        if (!IN(neighbor->get_status(), m->get_status_infected())) \
            continue; \
        PersonViruses<TSeq> & nviruses = neighbor->get_viruses(); \
        /* Now over the neighbor's viruses */ \
        double tmp_transmision; \
        for (unsigned int v = 0; v < nviruses.size(); ++v) \
        { \
            /* Computing the corresponding efficacy */ \
            Virus<TSeq> * tmp_v = &(nviruses(v)); \
            /* And it is a function of transmisibility as well */ \
            tmp_transmision = \
                (1.0 - p->get_efficacy(tmp_v)) * \
                neighbor->get_transmisibility(tmp_v) \
                ; \
            probs.push_back(tmp_transmision); \
            variants.push_back(tmp_v); \
        } \
    }

template<typename TSeq>
inline unsigned int default_update_susceptible(Person<TSeq> * p, Model<TSeq> * m)
{

    // This computes the prob of getting any neighbor variant
    EPIWORLD_UPDATE_SUSCEPTIBLE_CALC_PROBS(probs,variants)

    // No virus to compute on
    if (probs.size() == 0)
        return p->get_status();

    // Running the roulette
    int which = roulette(probs, m);

    if (which < 0)
        return p->get_status();

    EPIWORLD_ADD_VIRUS(variants[which], STATUS::INFECTED)

    return static_cast<unsigned int>(STATUS::INFECTED); 

}

template<typename TSeq>
inline unsigned int default_update_infected(Person<TSeq> * p, Model<TSeq> * m) {

    Virus<TSeq> * vptr = &(p->get_virus(0u));

    double p_die = p->get_death(vptr);
    double p_rec = p->get_recovery(vptr);
    double r = m->runif();

    double cumsum = p_die * (1 - p_rec) / (1.0 - p_die * p_rec); 

    if (r < cumsum)
    {
    
        m->get_db().down_infected(vptr,p->get_status(), STATUS::REMOVED);
        return STATUS::REMOVED;

    } 
    
    cumsum += p_rec * (1 - p_die) / (1.0 - p_die * p_rec);
    if (r < cumsum)
    {
        // Updating db and running actions
        m->get_db().down_infected(vptr,p->get_status(), STATUS::RECOVERED);

        // Checking if something happens after recovery
        // (e.g., full immunity)
        vptr->post_recovery(); 

        p->get_viruses().reset();
        
        return static_cast<unsigned int>(STATUS::RECOVERED);

    }

    return p->get_status();

}

#endif