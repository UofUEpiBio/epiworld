#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <map>
#include <limits.h>

#ifndef EPIWORLD_ADJLIST_HPP
#define EPIWORLD_ADJLIST_HPP

class AdjList {
private:

    std::string filename;
    std::map<int,std::map<int,int>> dat;

    int id_min,id_max;
    int N = 0;
    int E = 0;

public:

    void read_edgelist(std::string fn, int skip = 0, bool directed = true);
    const std::map<int,int> & operator()(int i) const;
        
    void print(int limit = 20) const;
    int get_id_max() const;
    int get_id_min() const;
    size_t vcount() const;
    size_t ecount() const;
    
    const std::map<int,std::map<int,int>> & get_dat() const {
        return dat;
    };

};


inline void AdjList::read_edgelist(
    std::string fn,
    int skip,
    bool directed
) {

    int i,j;
    std::ifstream filei(fn);

    if (!filei)
        throw std::logic_error("The file " + fn + " was not found.");

    id_min = INT_MAX;
    id_max = INT_MIN;

    int linenum = 0;
    while (!filei.eof())
    {

        if (linenum < skip)
            continue;

        linenum++;

        filei >> i >> j;

        // Looking for exceptions
        if (filei.bad())
            throw std::logic_error(
                "I/O error while reading the file " +
                fn
            );

        if (filei.fail())
            break;

        // Adding nodes
        if (dat.find(i) == dat.end())
        {

            dat[i].insert(std::pair<int,int>(j, 0));
            N++;

        } else { // Or simply increasing the counter

            auto & dat_i = dat[i];
            if (dat_i.find(j) == dat_i.end())
                dat_i[j] = 0;
            else
                dat_i[j]++;

        }
        
        if (!directed)
        {

            if (dat.find(j) == dat.end())
            {
                dat[j].insert(std::pair<int,int>(i,0));
                N++;
            } else
                dat[j][i]++;

        }

        // Recalculating the limits
        if (i < id_min)
            id_min = i;

        if (j < id_min)
            id_min = j;

        if (i > id_max)
            id_max = i;

        if (j > id_max)
            id_max = j;

        E++;

    }

    if (!filei.eof())
        throw std::logic_error(
            "Wrong format found in the AdjList file " +
            fn + " in line " + std::to_string(linenum)
        );
    
    return;

}

inline const std::map<int,int> & AdjList::operator()(int i) const {

    if (dat.find(i) == dat.end())
        throw std::range_error(
            "The vertex id " + std::to_string(i) + " is not in the network."
            );

    return dat.find(i)->second;

}
void AdjList::print(int limit) const {


    int counter = 0;
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
                printf_epiworld("%i, ", n_n.first);
            else
                printf_epiworld("%i}\n", n_n.first);
    }

    if (limit < dat.size())
        printf_epiworld(
            "  (... skipping %i records ...)\n",
            static_cast<int>(dat.size() - limit)
            );

}

inline int AdjList::get_id_max() const 
{
    return id_max;
}

inline int AdjList::get_id_min() const 
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


#endif

