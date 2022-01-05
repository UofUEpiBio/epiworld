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
    friend class Person<TSeq>;
    friend class DataBase<TSeq>;
private:

    DataBase<TSeq> db;

    std::vector< Person<TSeq> > population;
    std::map< int,int >         population_ids;
    bool directed;
    
    std::vector< Virus<TSeq> > viruses;
    std::vector< double > prevalence_virus; ///< Initial prevalence_virus of each virus
    
    std::vector< Tool<TSeq> > tools;
    std::vector< double > prevalence_tool;

    std::shared_ptr< std::mt19937 > engine =
        std::make_shared< std::mt19937 >();
    
    std::shared_ptr< std::uniform_real_distribution<> > runifd =
        std::make_shared< std::uniform_real_distribution<> >(0.0, 1.0);

    std::shared_ptr< std::normal_distribution<> > rnormd =
        std::make_shared< std::normal_distribution<> >(0.0);

    std::function<void(Model<TSeq>*,double)> rewire_fun;
    double rewire_prop;
        
    std::map<std::string, double > parameters;
    unsigned int ndays;
    Progress pb;

    std::vector< unsigned int > status_susceptible = {STATUS::HEALTHY, STATUS::RECOVERED};
    std::vector< std::string > status_susceptible_labels = {"healthy", "recovered"};
    std::vector< unsigned int > status_infected = {STATUS::INFECTED};
    std::vector< std::string > status_infected_labels = {"infected"};
    std::vector< unsigned int > status_removed = {STATUS::REMOVED};
    std::vector< std::string > status_removed_labels = {"removed"};

    unsigned int nstatus = 4u;
    
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
    double & operator()(std::string pname);

    size_t size() const;

    /**
     * @brief Random number generation
     * 
     * @param eng 
     */
    ///@[
    void set_rand_engine(std::mt19937 & eng);
    std::mt19937 * get_rand_endgine();
    double runif();
    double rnorm(double mean = 0.0, double sd = 1.0);
    void seed(unsigned int s);
    ///@]

    void add_virus(Virus<TSeq> v, double preval);
    void add_tool(Tool<TSeq> t, double preval);

    /**
     * @brief Accessing population of the model
     * 
     * @param fn std::string Filename of the edgelist file.
     * @param skip int Number of lines to skip in `fn`.
     * @param directed bool Whether the graph is directed or not.
     * @param min_id int Minimum id number (if negative, the program will
     * try to guess from the data.)
     * @param max_id int Maximum id number (if negative, the program will
     * try to guess from the data.)
     * @param al AdjList to read into the model.
     */
    ///@[
    void pop_from_adjlist(
        std::string fn,
        int skip = 0,
        bool directed = false,
        int min_id = -1,
        int max_id = -1
        );
    void pop_from_adjlist(AdjList al);
    bool is_directed() const;
    std::vector< Person<TSeq> > * get_population();
    ///@]

    /**
     * @brief Functions to run the model
     * 
     * @param seed Seed to be used for Pseudo-RNG.
     * @param ndays Number of days (steps) of the simulation.
     * 
     */
    ///@[
    void init(unsigned int seed, unsigned int ndays);
    void update_status();
    void mutate_variant();
    void next();
    void run();
    ///@]

    void record_variant(Virus<TSeq> * v);

    int get_nvariants() const;
    unsigned int get_ndays() const;
    void set_ndays(unsigned int ndays);
    bool get_verbose() const;
    void verbose_off();
    void verbose_on();
    int today() const;

    /**
     * @brief Rewire the network preserving the degree sequence.
     *
     * @details This implementation assumes an undirected network,
     * thus if {(i,j), (k,l)} -> {(i,l), (k,j)}, the reciprocal
     * is also true, i.e., {(j,i), (l,k)} -> {(j,k), (l,i)}.
     * 
     * @param proportion Proportion of ties to be rewired.
     * 
     * @result A rewired version of the network.
     */
    ///@[
    void set_rewire_fun(std::function<void(Model<TSeq>*,double)> fun);
    void set_rewire_prop(double prop);
    double get_rewire_prop() const;
    void rewire();
    ///@]

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

    /**
     * @brief Export the network data in edgelist form
     * 
     * @param fn std::string. File name.
     * @param source Integer vector
     * @param target Integer vector
     * 
     * @details When passing the source and target, the function will
     * write the edgelist on those.
     */
    ///[@
    void write_edgelist(
        std::string fn
        ) const;

    void write_edgelist(
        std::vector< unsigned int > & source,
        std::vector< unsigned int > & target
        ) const;
    ///@]

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
     * @param s `unsigned int` Code of the status
     * @param lab `std::string` Name of the status.
     * 
     * @return `add_status*` returns nothing.
     * @return `get_status_*` returns a vector of pairs with the 
     * statuses and their labels.
     */
    ///@[
    void add_status_susceptible(unsigned int s, std::string lab);
    void add_status_infected(unsigned int s, std::string lab);
    void add_status_removed(unsigned int s, std::string lab);
    void add_status_susceptible(std::string lab);
    void add_status_infected(std::string lab);
    void add_status_removed(std::string lab);
    std::vector< std::pair<unsigned int,std::string> > get_status_susceptible() const;
    std::vector< std::pair<unsigned int,std::string> > get_status_infected() const;
    std::vector< std::pair<unsigned int,std::string> > get_status_removed() const;
    ///@]

};

#endif