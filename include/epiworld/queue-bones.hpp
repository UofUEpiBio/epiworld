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
template<typename TSeq>
class Queue
{

private:

    /**
     * @brief Count of ego's neighbors in queue (including ego)
     */
    std::vector< epiworld_fast_int > active;
    Model<TSeq> * model = nullptr;
    int n_in_queue = 0;

    // Auxiliary variable that checks how many steps
    // left are there
    // int n_steps_left;
    // bool queuing_started   = false;

public:

    void operator+=(Agent<TSeq> * p);
    void operator-=(Agent<TSeq> * p);
    epiworld_fast_int & operator[](epiworld_fast_uint i);

    // void initialize(Model<TSeq> * m, Agent<TSeq> * p);
    void set_model(Model<TSeq> * m);

};

template<typename TSeq>
inline void Queue<TSeq>::operator+=(Agent<TSeq> * p)
{

    if (++active[p->id] == 1)
        n_in_queue++;

    for (auto n : p->neighbors)
    {

        if (++active[n] == 1)
            n_in_queue++;

    }

}

template<typename TSeq>
inline void Queue<TSeq>::operator-=(Agent<TSeq> * p)
{

    if (--active[p->id] == 0)
        n_in_queue--;

    for (auto n : p->neighbors)
    {
        if (--active[n] == 0)
            n_in_queue--;
    }

}

template<typename TSeq>
inline epiworld_fast_int & Queue<TSeq>::operator[](epiworld_fast_uint i)
{
    return active[i];
}

template<typename TSeq>
inline void Queue<TSeq>::set_model(Model<TSeq> * m)
{

    model = m;
    if (n_in_queue)
    {

        for (auto & q : this->active)
            q = 0;

        n_in_queue = 0;
        
    }

    active.resize(m->size(), 0);

}

#endif