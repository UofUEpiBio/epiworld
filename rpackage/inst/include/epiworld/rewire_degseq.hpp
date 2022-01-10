#ifndef EPIWORLD_REWIRE_DEGSEQ_HPP
#define EPIWORLD_REWIRE_DEGSEQ_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Person;

template<typename TSeq>
inline void rewire_degseq(Model<TSeq> * model, epiworld_double proportion)
{

    // Identifying individuals with degree > 0
    std::vector< unsigned int > non_isolates;
    std::vector< epiworld_double > weights;
    epiworld_double nedges = 0.0;
    std::vector< Person<TSeq> > * persons = model->get_population();
    for (unsigned int i = 0u; i < persons->size(); ++i)
    {
        if (persons->operator[](i).get_neighbors().size() > 0u)
        {
            non_isolates.push_back(i);
            weights.push_back(
                static_cast<epiworld_double>(persons->operator[](i).get_neighbors().size())
                );
            nedges += 1.0;
        }
    }

    if (non_isolates.size() == 0u)
        throw std::logic_error("The graph is completely disconnected.");

    // Cumulative probs
    weights[0u] /= nedges;
    for (unsigned int i = 1u; i < non_isolates.size(); ++i)
    {
         weights[i] /= nedges;
         weights[i] += weights[i - 1u];
    }

    // Only swap if needed
    unsigned int N = non_isolates.size();
    epiworld_double prob;
    int nrewires = floor(proportion * nedges);
    while (nrewires-- > 0)
    {

        // Picking egos
        prob = model->runif();
        int id0 = N - 1;
        for (unsigned int i = 0u; i < N; ++i)
            if (prob <= weights[i])
            {
                id0 = i;
                break;
            }

        prob = model->runif();
        int id1 = N - 1;
        for (unsigned int i = 0u; i < N; ++i)
            if (prob <= weights[i])
            {
                id1 = i;
                break;
            }

        // Correcting for under or overflow.
        if (id1 == id0)
            id1++;

        if (id1 >= static_cast<int>(N))
            id1 = 0;

        Person<TSeq> & p0 = persons->operator[](non_isolates[id0]);
        Person<TSeq> & p1 = persons->operator[](non_isolates[id1]);

        // Picking alters (relative location in their lists)
        // In this case, these are uniformly distributed within the list
        int id01 = std::floor(p0.get_neighbors().size() * model->runif());
        int id11 = std::floor(p1.get_neighbors().size() * model->runif());

        // When rewiring, we need to flip the individuals from the other
        // end as well, since we are dealing withi an undirected graph

        // Finding what neighbour is id0
        unsigned int n0,n1;
        Person<TSeq> & p01 = persons->operator[](p0.get_neighbors()[id01]->get_index());
        for (n0 = 0; n0 < p01.get_neighbors().size(); ++n0)
        {
            if (p0.get_id() == p01.get_neighbors()[n0]->get_id())
                break;            
        }

        Person<TSeq> & p11 = persons->operator[](p1.get_neighbors()[id11]->get_index());
        for (n1 = 0; n1 < p11.get_neighbors().size(); ++n1)
        {
            if (p1.get_id() == p11.get_neighbors()[n1]->get_id())
                break;            
        }

        // Moving alter first
        std::swap(p0.get_neighbors()[id01], p1.get_neighbors()[id11]);
        std::swap(p01.get_neighbors()[n0], p11.get_neighbors()[n1]);
        
    }

    return;

}

#endif