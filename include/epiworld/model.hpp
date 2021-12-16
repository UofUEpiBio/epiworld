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

template<typename TSeq>
class Tool;

class AdjList;

template<typename TSeq>
class DataBase;

template<typename TSeq>
class Model {
private:

    DataBase<TSeq> db;

    std::vector< Person<TSeq> > persons;
    
    std::vector< Virus<TSeq> > viruses;
    std::vector< double > prevalence_virus; ///< Initial prevalence_virus of each virus
    
    std::vector< Tool<TSeq> > tools;
    std::vector< double > prevalence_tool;

    std::shared_ptr< std::mt19937 > engine;
    std::shared_ptr< std::uniform_real_distribution<> > runifd;
    bool initialized = false;
    int current_date = 0;

public:

    Model() {};
    void set_rand_engine(std::mt19937 & eng);

    DataBase<TSeq> & get_db();
    std::vector< Person<TSeq> > * get_persons();

    Person<TSeq> & operator()(int i);

    size_t size() const;
    void init(int seed = 0);
    std::mt19937 * get_rand_endgine();
    double runif();

    void add_virus(Virus<TSeq> v, double preval);
    void add_tool(Tool<TSeq> t, double preval);

    void pop_from_adjlist(std::string fn, int skip = 0, bool directed = false);
    void pop_from_adjlist(AdjList al);
    int today() const;
    void update_status();
    void mutate_variant();
    void next();

    void register_variant(Virus<TSeq> * v);
    int get_nvariants() const;
    const std::vector< TSeq > & get_variant_sequence() const;
    const std::vector< int > & get_variant_nifected() const;

    void write_data(
        std::string fn_variant,
        std::string fn_total
        ) const;

    void write_edgelist(
        std::string fn
        ) const;

};

template<typename TSeq>
inline DataBase<TSeq> & Model<TSeq>::get_db()
{
    return db;
}

template<typename TSeq>
inline std::vector<Person<TSeq>> * Model<TSeq>::get_persons()
{
    return &persons;
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

    // Initializing persons
    for (auto & p : persons)
    {
        p.model = this;
        p.init();
    }

    // Has to happen after setting the persons
    db.set_model(*this);

    if (!engine)
        engine = std::make_shared< std::mt19937 >();

    engine->seed(seed);

    if (!runifd)
        runifd = std::make_shared< std::uniform_real_distribution<> >(0.0, 1.0);

    initialized = true;

    // Starting first infection
    for (int v = 0; v < viruses.size(); ++v)
    {
        for (auto & p : persons)
            if (runif() < prevalence_virus[v])
                p.add_virus(0, viruses[v]);

    }

    // Tools
    for (int t = 0; t < tools.size(); ++t)
    {
        for (auto & p : persons)
            if (runif() < prevalence_tool[t])
                p.add_tool(0, tools[t]);

    }

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
    prevalence_virus.push_back(preval);
    register_variant(&viruses[viruses.size() - 1]);

}

template<typename TSeq>
inline void Model<TSeq>::add_tool(Tool<TSeq> t, double preval)
{
    tools.push_back(t);
    prevalence_tool.push_back(preval);
}

template<typename TSeq>
inline void Model<TSeq>::pop_from_adjlist(std::string fn, int skip, bool directed) {

    AdjList al;
    al.read_edgelist(fn, skip, directed);
    this->pop_from_adjlist(al);

}

template<typename TSeq>
inline void Model<TSeq>::pop_from_adjlist(AdjList al) {

    // Resizing the people
    persons.resize(al.vcount(), Person<TSeq>());

    const auto & tmpdat = al.get_dat();
    for (const auto & n : tmpdat)
    {        
        persons[n.first].model = this;
        persons[n.first].id    = n.first;
        for (const auto & link: n.second)
            persons[n.first].add_neighbor(&persons[link.first]);

    }

}

template<typename TSeq>
inline int Model<TSeq>::today() const {
    return this->current_date;
}

template<typename TSeq>
inline void Model<TSeq>::next() {
    db.record();
    ++this->current_date;
    return ;
}

template<typename TSeq>
inline void Model<TSeq>::update_status() {

    // Next status
    for (auto & p: persons)
        p.update_status();

    // Making the change effective
    for (auto & p: persons)
        p.status = p.status_next;

}

template<typename TSeq>
inline void Model<TSeq>::mutate_variant() {

    for (auto & p: persons)
    {
        if (p.get_status() == INFECTED)
            p.mutate_variant();

    }

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
    return db.get_today_variant("ninfected");
}

template<typename TSeq>
inline void Model<TSeq>::write_data(
    std::string fn_variant,
    std::string fn_total
    ) const
{

}

template<typename TSeq>
inline void Model<TSeq>::write_edgelist(
    std::string fn
    ) const
{

    std::ofstream efile(fn, std::ios_base::out);
    efile << "source target\n";
    for (const auto & p : persons)
    {
        for (auto & n : p.neighbors)
            efile << p.id << " " << n->id << "\n";
    }

}

#undef CHECK_INIT

#endif