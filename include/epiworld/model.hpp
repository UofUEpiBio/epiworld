#include <vector>
#include <random>
#include <unordered_map>
#include "misc.hpp"

#ifndef EPIWORLD_MODEL_HPP
#define EPIWORLD_MODEL_HPP

#define CHECK_INIT() if (!initialized) \
        throw std::logic_error("Model not initialized.");

template<typename TSeq>
class Person;

template<typename TSeq>
class Virus;

class AdjList;

template<typename TSeq>
class DataBase;

template<typename TSeq>
class Model {
private:

    DataBase<TSeq> db;

    std::vector< Person<TSeq> > persons;
    std::vector< Virus<TSeq> > viruses;
    std::vector< double > prevalence; ///< Initial prevalence of each virus
    std::shared_ptr< std::mt19937 > engine;
    std::shared_ptr< std::uniform_real_distribution<> > runifd;
    bool initialized = false;
    int current_date = 0;


public:

    Model() {};
    Model(int size);
    void set_rand_engine(std::mt19937 & eng);

    DataBase<TSeq> & get_db();
    std::vector< Person<TSeq> > * get_persons();

    Person<TSeq> & operator()(int i);

    size_t size() const;
    void init(int seed = 0);
    void seed(unsigned int s);
    std::mt19937 * get_rand_endgine();
    double runif();

    void add_virus(Virus<TSeq> v, double preval);

    void pop_from_adjlist(std::string fn, int skip = 0);
    void pop_from_adjlist(AdjList al, int skip = 0);
    int today() const;
    void next();

    void register_variant(Virus<TSeq> * v);
    int get_nvariants() const;
    const std::vector< TSeq > & get_variant_sequence() const;
    const std::vector< int > & get_variant_nifected() const;

};

template<typename TSeq>
inline DataBase<TSeq> & Model<TSeq>::get_db()
{
    return db;
}

template<typename TSeq>
inline Model<TSeq>::Model(int size) : persons(size) {
    
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_engine(std::mt19937 & eng)
{
    engine = std::make_shared< std::mt19937 >(eng);
}

template<typename TSeq>
inline Person<TSeq> & Model<TSeq>::operator()(int i) {
    return persons.at(i);
}

template<typename TSeq>
inline size_t Model<TSeq>::size() const {
    return persons.size();
}

template<typename TSeq>
inline void Model<TSeq>::init(int seed) {

    if (initialized) 
        throw std::logic_error("Model already initialized.");

    for (auto & p : persons)
        p.model = this;

    if (!engine)
        engine = std::make_shared< std::mt19937 >();

    engine->seed(seed);

    if (!runifd)
        runifd = std::make_shared< std::uniform_real_distribution<> >(0.0, 1.0);

    initialized = true;

    // Starting first infection
    for (int v = 0; v < viruses.size(); ++v)
    {
        for (int p = 0; p < persons.size(); ++p)
        {
            if (runif() < prevalence[v])
                persons[p].add_virus(0, viruses[v]);
        }
    }

}

template<typename TSeq>
inline void Model<TSeq>::seed(unsigned int s) {
    engine->seed(s);
}

template<typename TSeq>
inline std::mt19937 * Model<TSeq>::get_rand_endgine()
{
    return engine.get();
}

template<typename TSeq>
inline double Model<TSeq>::runif() {
    CHECK_INIT()
    return runifd->operator()(*engine);
}

template<typename TSeq>
inline void Model<TSeq>::add_virus(Virus<TSeq> v, double preval)
{
    viruses.push_back(v);
    prevalence.push_back(preval);
    register_variant(&v);

}

template<typename TSeq>
inline void Model<TSeq>::pop_from_adjlist(std::string fn, int skip) {

    AdjList al;
    al.read_edgelist(fn);

    this->pop_from_adjlist(al, skip);

}

template<typename TSeq>
inline void Model<TSeq>::pop_from_adjlist(AdjList al, int skip) {

    // Resizing the people
    persons.resize(al.vcount(), Person<TSeq>());

    const auto & tmpdat = al.get_dat();
    int i = 0;
    for (const auto & n : tmpdat)
    {        
        for (const auto & link: n.second)
            persons[i].add_neighbor(&persons[link]);

        i++;
    }

}

template<typename TSeq>
inline int Model<TSeq>::today() const {
    return this->current_date;
}

template<typename TSeq>
inline void Model<TSeq>::next() {
    ++this->current_date;
    return ;
}

template<typename TSeq>
inline void Model<TSeq>::register_variant(Virus<TSeq> * v) {

    // Updating registry
    db.register_variant(v);
    
    return;
} 

template<typename TSeq>
inline int Model<TSeq>::get_nvariants() const {
    return db.size();
}

template<typename TSeq>
inline const std::vector<TSeq> & Model<TSeq>::get_variant_sequence() const {
    return db.get_sequence();
}

template<typename TSeq>
inline const std::vector<int> & Model<TSeq>::get_variant_nifected() const {
    return db.get_ninfected();
}

#undef CHECK_INIT

#endif