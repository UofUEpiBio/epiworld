#include <vector>
#include <random>
#include <unordered_map>
#include "misc.hpp"

#ifndef EPIWORLD_MODEL_HPP
#define EPIWORLD_MODEL_HPP

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
    std::map< int,int >         persons_ids;
    
    std::vector< Virus<TSeq> > viruses;
    std::vector< double > prevalence_virus; ///< Initial prevalence_virus of each virus
    
    std::vector< Tool<TSeq> > tools;
    std::vector< double > prevalence_tool;

    std::shared_ptr< std::mt19937 > engine;
    std::shared_ptr< std::uniform_real_distribution<> > runifd;
    std::vector< double > parameters;
    int ndays;
    Progress pb;
    bool verbose     = true;
    bool initialized = false;
    int current_date = 0;

    void dist_tools();
    void dist_virus();

public:

    Model() {};
    void set_rand_engine(std::mt19937 & eng);

    DataBase<TSeq> & get_db();
    std::vector< Person<TSeq> > * get_persons();

    double & operator()(int i);

    size_t size() const;
    void init(int seed, int ndays);
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

    void record_variant(Virus<TSeq> * v);
    int get_nvariants() const;
    const std::vector< TSeq > & get_variant_sequence() const;
    const std::vector< int > & get_variant_nifected() const;
    int get_ndays() const;
    bool get_verbose() const;

    void rewire_degseq(int nrewires);

    void write_data(
        std::string fn_variant,
        std::string fn_total
        ) const;

    void write_edgelist(
        std::string fn
        ) const;

    std::vector<double> & params();

    void reset();
    void print() const;

};


#endif