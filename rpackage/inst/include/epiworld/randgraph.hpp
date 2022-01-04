#ifndef EPIWORLD_RANDGRA
#define EPIWORLD_RANDGRA

template<typename TSeq>
class Model;

class AdjList;

template<typename TSeq>
inline AdjList rgraph_bernoulli_undirected(
    unsigned int n,
    double p,
    Model<TSeq> & m
) {

    std::vector< unsigned int > source;
    std::vector< unsigned int > target;

    int a,b;
    for (unsigned int i = 0u; i < floor(n * p); ++i)
    {
        a = floor(m.runif() * n);
        b = floor(m.runif() * a);

        source.push_back(a);
        target.push_back(b);

    }

    AdjList al(source, target, false, 0,static_cast<int>(n) - 1);

    return al;
    
}

#endif
