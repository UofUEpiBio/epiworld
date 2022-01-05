#ifndef EPIWORLD_ADJLIST_MEAT_HPP
#define EPIWORLD_ADJLIST_MEAT_HPP

inline AdjList::AdjList(
    const std::vector< unsigned int > & source,
    const std::vector< unsigned int > & target,
    bool directed,
    int min_id,
    int max_id
) : directed(directed) {

    id_min = static_cast<unsigned int>(INT_MAX);
    id_max = 0u;

    int i,j;
    for (unsigned int m = 0; m < source.size(); ++m)
    {

        i = source[m];
        j = target[m];

        if ((max_id > 0) && (i > max_id))
            throw std::range_error(
                "The source["+std::to_string(m)+"] = " + std::to_string(i) +
                " is above the max_id " + std::to_string(max_id)
                );

        if ((min_id > 0) && (i < min_id))
            throw std::range_error(
                "The source["+std::to_string(m)+"] = " + std::to_string(i) + 
                " is below the min_id " + std::to_string(min_id)
                );

        if ((max_id > 0) && (j > max_id))
            throw std::range_error(
                "The target["+std::to_string(m)+"] = " + std::to_string(j) +
                " is above the max_id " + std::to_string(max_id)
                );

        if ((min_id > 0) && (j < min_id))
            throw std::range_error(
                "The target["+std::to_string(m)+"] = " + std::to_string(i) +
                " is below the min_id " + std::to_string(min_id)
                );

        // Adding nodes
        if (dat.find(i) == dat.end())
            dat[i].insert(std::pair<unsigned int, unsigned int>(j, 1u));
        else { // Or simply increasing the counter

            auto & dat_i = dat[i];
            if (dat_i.find(j) == dat_i.end())
                dat_i[j] = 1u;
            else
                dat_i[j]++;

        }

        if (dat.find(j) == dat.end())
            dat[j] = std::map<unsigned int, unsigned int>();            
        
        if (!directed)
        {

            if (dat[j].find(i) == dat[j].end())
            {
                dat[j][i] = 1u;
                
            } else
                dat[j][i]++;

        }

        // Recalculating the limits
        if (i < static_cast<int>(id_min))
            id_min = static_cast<unsigned int>(i);

        if (j < static_cast<int>(id_min))
            id_min = static_cast<unsigned int>(j);

        if (i > static_cast<int>(id_max))
            id_max = static_cast<unsigned int>(i);

        if (j > static_cast<int>(id_max))
            id_max = static_cast<unsigned int>(j);

        E++;

    }

    // Checking if the max found matches the max identified
    if (max_id >= 0)
        id_max = static_cast<unsigned int>(max_id);

    if (min_id >= 0)
        id_min = static_cast<unsigned int>(min_id);

    N = id_max - id_min + 1u;

    return;

}

inline void AdjList::read_edgelist(
    std::string fn,
    int skip,
    bool directed,
    int min_id,
    int max_id
) {

    int i,j;
    std::ifstream filei(fn);

    if (!filei)
        throw std::logic_error("The file " + fn + " was not found.");

    id_min = INT_MAX;
    id_max = INT_MIN;

    int linenum = 0;
    std::vector< unsigned int > source_;
    std::vector< unsigned int > target_;

    while (!filei.eof())
    {

        if (linenum < skip)
            continue;

        filei >> i >> j;

        // Looking for exceptions
        if (filei.bad())
            throw std::logic_error(
                "I/O error while reading the file " +
                fn
            );

        if (filei.fail())
            break;

        if ((max_id > 0) && (i > max_id))
            throw std::range_error(
                "The source["+std::to_string(linenum)+"] = " + std::to_string(i) +
                " is above the max_id " + std::to_string(max_id)
                );

        if ((min_id > 0) && (i < min_id))
            throw std::range_error(
                "The source["+std::to_string(linenum)+"] = " + std::to_string(i) + 
                " is below the min_id " + std::to_string(min_id)
                );

        if ((max_id > 0) && (j > max_id))
            throw std::range_error(
                "The target["+std::to_string(linenum)+"] = " + std::to_string(j) +
                " is above the max_id " + std::to_string(max_id)
                );

        if ((min_id > 0) && (j < min_id))
            throw std::range_error(
                "The target["+std::to_string(linenum)+"] = " + std::to_string(i) +
                " is below the min_id " + std::to_string(min_id)
                );

        source_.push_back(i);
        target_.push_back(j);

    }

    if (!filei.eof())
        throw std::logic_error(
            "Wrong format found in the AdjList file " +
            fn + " in line " + std::to_string(linenum)
        );
    
    // Now using the right constructor
    *this = AdjList(source_,target_,directed,min_id,max_id);

    return;

}

inline std::map<unsigned int,unsigned int> AdjList::operator()(
    unsigned int i
    ) const {

    if ((i < id_min) | (i > id_max))
        throw std::range_error(
            "The vertex id " + std::to_string(i) + " is not in the network."
            );

    if (dat.find(i) == dat.end())
        return std::map<unsigned int,unsigned int>();
    else
        return dat.find(i)->second;

}
inline void AdjList::print(unsigned int limit) const {


    unsigned int counter = 0;
    printf_epiworld("Nodeset:\n");
    for (auto & n : dat)
    {

        if (counter++ > limit)
            break;

        int n_neighbors = n.second.size();

        printf_epiworld("  % 3i: {", n.first);
        int niter = 0;
        for (auto n_n : n.second)
            if (++niter < n_neighbors)
            {    
                printf_epiworld("%i, ", n_n.first);
            }
            else {
                printf_epiworld("%i}\n", n_n.first);
            }
    }

    if (limit < dat.size())
    {
        printf_epiworld(
            "  (... skipping %i records ...)\n",
            static_cast<int>(dat.size() - limit)
            );
    }

}

inline unsigned int AdjList::get_id_max() const 
{
    return id_max;
}

inline unsigned int AdjList::get_id_min() const 
{
    return id_min;
}

inline size_t AdjList::vcount() const 
{
    return N;
}

inline size_t AdjList::ecount() const 
{
    return E;
}

inline bool AdjList::is_directed() const {

    if (dat.size() == 0u)
        throw std::logic_error("The edgelist is empty.");
    
    return directed;
    
}

#endif