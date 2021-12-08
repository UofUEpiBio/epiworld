#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <map>

#ifndef EPIWORLD_READ_HPP
#define EPIWORLD_READ_HPP

class ReadGraph {
private:

    std::vector< int > source;
    std::vector< int > target;
    std::string filename;
    std::map<int,int> nodelist;

    int N = 0;
    int E = 0;

public:

    void read_edgelist(std::string fn);
    // std::pair<int,int> operator()(int i) const;

    const std::vector< int > & get_source() const;
    const std::vector< int > & get_target() const;
    void print(int limit = 20) const;

};


inline void ReadGraph::read_edgelist(
    std::string fn
) {

    int i,j;
    std::ifstream filei(fn);

    if (!filei)
        throw std::logic_error("The file " + fn + " was not found.");

    int linenum = 0;
    while (!filei.eof())
    {

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
        if (nodelist.find(i) == nodelist.end())
            nodelist[i] = 0;
        else
            nodelist[i]++;

        if (nodelist.find(j) == nodelist.end())
            nodelist[j] = 0;
        else
            nodelist[j]++;

        source.push_back(i);
        target.push_back(j);

        E++;

    }

    if (!filei.eof())
        throw std::logic_error(
            "Wrong format found in the edgelist file " +
            fn + " in line " + std::to_string(linenum)
        );
    
    return;

}

const std::vector< int > & ReadGraph::get_source() const {
    return source;
}

const std::vector< int > & ReadGraph::get_target() const {
    return target;
}
void ReadGraph::print(int limit) const {

    limit = std::min<int>(limit, E);

    for (auto & n : nodelist)
        printf("% 3i: %i connections\n", n.first, n.second);

    for (int i = 0; i < limit; ++i)
        printf_epiworld("[,% 3i] % 3i -> % 3i\n", i, source[i], target[i]);

}

#endif

