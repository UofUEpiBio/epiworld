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
    std::map<std::string, double > parameters;
    unsigned int ndays;
    Progress pb;

    std::vector< int > status_susceptible;
    std::vector< std::string > status_susceptible_labels;
    std::vector< int > status_infected;
    std::vector< std::string > status_infected_labels;
    std::vector< int > status_removed;
    std::vector< std::string > status_removed_labels;
    
    bool verbose     = true;
    bool initialized = false;
    int current_date = 0;

    void dist_tools();
    void dist_virus();

public:

    Model() {};
    Model(const Model<TSeq> & m);
    Model(Model<TSeq> && m) = delete;
    Model<TSeq> & operator=(const Model<TSeq> & m) = delete;

    DataBase<TSeq> & get_db();
    std::vector< Person<TSeq> > * get_persons();

    double & operator()(std::string pname);

    size_t size() const;
    void init(unsigned int seed, unsigned int ndays);

    /**
     * @brief Random number generation
     * 
     * @param eng 
     */
    ///@[
    void set_rand_engine(std::mt19937 & eng);
    std::mt19937 * get_rand_endgine();
    double runif();
    void seed(unsigned int s);
    ///@]

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
    unsigned int get_ndays() const;
    void set_ndays(unsigned int ndays);
    bool get_verbose() const;
    void verbose_off();
    void verbose_on();

    /**
     * @brief Rewire the network preserving the degree sequence.
     *
     * @details This implementation assumes an undirected network,
     * thus if {(i,j), (k,l)} -> {(i,l), (k,j)}, the reciprocal
     * is also true, i.e., {(j,i), (l,k)} -> {(j,k), (l,i)}.
     * 
     * @param nrewires Number of rewires (or tries).
     * 
     * @result A rewired version of the network.
     */
    void rewire_degseq(int nrewires);

    /**
     * @brief Wrapper of `DataBase::write_data`
     * 
     * @param fn_variant_info 
     * @param fn_variant_hist 
     * @param fn_total_hist 
     * @param fn_transmission 
     */
    void write_data(
        std::string fn_variant_info,
        std::string fn_variant_hist,
        std::string fn_total_hist,
        std::string fn_transmission
        ) const;

    void write_edgelist(
        std::string fn
        ) const;

    std::map<std::string, double> & params();

    void reset();
    void print() const;

    Model<TSeq> && clone() const;

    /**
     * @brief Adds extra statuses to the model
     * 
     * @details
     * Adding values of `s` that are already present in the model will
     * result in an error.
     * 
     * The functions `get_status_*` return the current values for the 
     * statuses included in the model.
     * 
     * @param s `int` Code of the status
     * @param lab `std::string` Name of the status.
     * 
     * @return `add_status*` returns nothing.
     * @return `get_status_*` returns a vector of pairs with the 
     * statuses and their labels.
     */
    ///@[
    void add_status_susceptible(int s, std::string lab);
    void add_status_infected(int s, std::string lab);
    void add_status_removed(int s, std::string lab);
    void add_status_susceptible(std::string lab);
    void add_status_infected(std::string lab);
    void add_status_removed(std::string lab);
    std::vector< std::pair<int,std::string> > get_status_susceptible() const;
    std::vector< std::pair<int,std::string> > get_status_infected() const;
    std::vector< std::pair<int,std::string> > get_status_removed() const;
    ///@]

};

#endif