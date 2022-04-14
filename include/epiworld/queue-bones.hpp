#ifndef EPIWORLD_QUEUE_BONES_HPP
#define EPIWORLD_QUEUE_BONES_HPP

/**
 * @brief Controls which agents are verified at each step
 * 
 * @details The idea is that only agents who are either in
 * an infected state or have an infected neighbor should be
 * checked. Otherwise it makes no sense (no chance to recover
 * or capture the disease).
 * 
 * @tparam TSeq 
 */
template<typename TSeq = bool>
class Queue
{

private:

    /**
     * @brief Count of ego's neighbors in queue (including ego)
     */
    std::vector< epiworld_fast_int > active;
    std::vector< epiworld_fast_int > active_next;
    Model<TSeq> * model = nullptr;

    // Auxiliary variable that checks how many steps
    // left are there
    // int n_steps_left;
    // bool queuing_started   = false;

public:

    void operator+=(Person<TSeq> * p);
    void operator-=(Person<TSeq> * p);
    epiworld_fast_int operator[](unsigned int i) const;

    // void initialize(Model<TSeq> * m, Person<TSeq> * p);
    void set_model(Model<TSeq> * m);

    // unsigned int size();
    
    void update();

};

template<typename TSeq>
inline void Queue<TSeq>::operator+=(Person<TSeq> * p)
{

    active_next[p->index]++;
    for (auto * n : p->neighbors)
        active_next[n->index]++;

}

template<typename TSeq>
inline void Queue<TSeq>::operator-=(Person<TSeq> * p)
{

    active_next[p->index]--;
    for (auto * n : p->neighbors)
        active_next[n->index]--;

}

template<typename TSeq>
inline void Queue<TSeq>::update()
{
    for (unsigned int i = 0u; i < active_next.size(); ++i)
    {
        active[i] += active_next[i];
        active_next[i] = 0;
    }

    EPI_DEBUG_ALL_NON_NEGATIVE(active);
}

template<typename TSeq>
inline epiworld_fast_int Queue<TSeq>::operator[](unsigned int i) const
{
    return active[i];
}


// template<typename TSeq>
// inline void Queue<TSeq>::initialize(Model<TSeq> * m, Person<TSeq> * p)
// {

//     // In the first step we need to set up everything    
//     if (!queuing_started)
//     {

//         model = m;
//         p     = &model->population[0u];

//         n_steps_left = m->size();
//         active.resize(n_steps_left, 0);
//         active_next.resize(n_steps_left, 0);

//         // The queuing is now running
//         queuing_started = true;

//     }

//     if (--n_steps_left < 0)
//     {
//         model->toggle_visited();
//         return;
//     }

    
//     // Visited marks whether we checked if it was infected or not
//     bool add_all = false;
//     if (!p->visited()) 
//     {

//         // We just visit the individual.
//         p->toggle_visited();

//         if (IN(p->get_status(), model->status_exposed))
//         {

//             // Increasing the number in the queue
//             active[p->index]++;

//             // For sure add all the neighbors, but still need to check
//             add_all = true;

//         }

//     }
//     else
//     {
//         return;
//     }
    
//     // Checking on the neighbors
//     for (auto * n : p->neighbors)
//     {

//         // Not in queue, but still need to add b/c p is infected.
//         // (if visited, no need to go again).
//         if (add_all)
//         {

//             // Since neighbor is infected
//             active[n->index]++;

//         }
        
//         // Now going inside
//         if (!n->visited())
//             initialize(model, n);

//     }

//     return;

// }

template<typename TSeq>
inline void Queue<TSeq>::set_model(Model<TSeq> * m)
{

    model = m;
    active.resize(m->size(), 0);
    active_next.resize(m->size(), 0);

}

#endif