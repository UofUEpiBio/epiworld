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
    m->get_db().up_infected(virus_ptr, p->get_status(), new_state);


#define EPIWORLD_UPDATE_SUSCEPTIBLE_CALC_PROBS(probs,variants) \
    /* Step 1: Compute the individual efficcacy */ \
    std::vector< double > probs; \
    std::vector< Virus<TSeq>* > variants; \
    /* Computing the contagion_reduction */ \
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
            /* Computing the corresponding contagion_reduction */ \
            Virus<TSeq> * tmp_v = &(nviruses(v)); \
            /* And it is a function of contagion_reduction as well */ \
            tmp_transmision = \
                (1.0 - p->get_contagion_reduction(tmp_v)) * \
                tmp_v->get_infectiousness() * \
                neighbor->get_contagion_reduction(tmp_v) \
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

#define EPIWORLD_UPDATE_INFECTED_CALC_PROBS(prob_rec, prob_die) \
    Virus<TSeq> * v = &(p->get_virus(0u)); \
    double prob_rec =  (1 - v->get_persistance())/p->get_recovery(v); \
    double prob_die = v->get_death() / p->get_death(v); 

#define EPIWORLD_UPDATE_INFECTED_REMOVE(newstatus) \
    {m->get_db().down_infected(v, p->get_status(), newstatus);\
    return static_cast<unsigned int>(newstatus);}

#define EPIWORLD_UPDATE_INFECTED_RECOVER(newstatus) \
    {m->get_db().down_infected(v, p->get_status(), newstatus);\
    v->get_post_recovery();p->get_viruses().reset();\
    return static_cast<unsigned int>(newstatus);}

template<typename TSeq>
inline unsigned int default_update_infected(Person<TSeq> * p, Model<TSeq> * m) {

    EPIWORLD_UPDATE_INFECTED_CALC_PROBS(p_rec, p_die)
    double r = EPI_RUNIF();

    double cumsum = p_die * (1 - p_rec) / (1.0 - p_die * p_rec); 
    if (r < cumsum)
        EPIWORLD_UPDATE_INFECTED_REMOVE(STATUS::REMOVED);
    
    cumsum += p_rec * (1 - p_die) / (1.0 - p_die * p_rec);
    if (r < cumsum)
        EPIWORLD_UPDATE_INFECTED_RECOVER(STATUS::RECOVERED)

    return p->get_status();

}

#endif