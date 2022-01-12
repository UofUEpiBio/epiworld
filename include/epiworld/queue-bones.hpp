#ifndef EPIWORLD_QUEUE_BONES_HPP
#define EPIWORLD_QUEUE_BONES_HPP

template<typename TSeq>
class Queue
{

private:
    std::vector< Person<TSeq> * > queue;
    std::map<int,unsigned int> queue_location; 
    unsigned int n_active;
    unsigned int n_available; 

    std::map<int,bool> removed;
    
    Model<TSeq> * model = nullptr;
    unsigned int current_post = 0u;
    
    std::vector< int > next_to_add;
    unsigned int n_next_to_add;
    
    std::vector< int > next_to_pause;
    unsigned int n_next_to_pause;

    std::vector< int > next_to_remove;
    unsigned int n_next_to_remove;

public:

    void add(int id);
    void pause(int id);
    void remove(int id);
    void initialize(Model<TSeq> * m);
    unsigned int next();
    unsigned int size();
    void update();

};

template<typename TSeq>
inline void Queue<TSeq>::add(int id)
{

    next_to_add[n_next_to_add++];

}


template<typename TSeq>
inline void Queue<TSeq>::pause(int id)
{

    next_to_pause[n_next_to_pause++];

}

template<typename TSeq>
inline void Queue<TSeq>::remove(int id)
{

    next_to_remove[n_next_to_remove++];

}

template<typename TSeq>
inline void Queue<TSeq>::initialize(Model<TSeq> * m)
{

    model = m;

    queue.reserve(m->size()/2);
    removed.reserve(m->size()/2);
    n_available = queue.size();
    n_active    = 0u;

    for (auto & p : m->get_population())
    {

        // Seen before?
        int id = p.get_id();
        
        // Infected need to be added for sure
        if (IN(p.get_status(), model->status_infected))
        {

            // For sure need to add it
            queue_location[id] = nactive;
            queue[nactive++]   = &p;

            // But let's see about the neighbors
            for (auto * n : p->neighbors)
            {
                
                // Already added?
                if (queue_location[id] != queue_location.end())
                    continue;

                // And only if the neighbor hasn't been removed
                if (!IN(n->get_status(), model->status_removed))
                {
                    queue_location[n->get_id()] = nactive;
                    queue[nactive++]            = n;
                }
                else // Otherwise these are not counted for in the future
                {
                    n_non_removed--;
                }

            }

        }
        else // Non infected are only added if their peers are infected
        {

            for (auto * n: p->neighbors)
            {

                

            }

        }

    }

}

#endif