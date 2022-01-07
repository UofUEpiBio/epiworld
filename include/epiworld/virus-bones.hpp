#include <vector>
#include <functional>
#include <memory>

#ifndef EPIWORLD_VIRUS_HPP
#define EPIWORLD_VIRUS_HPP

template<typename TSeq>
class Person;

template<typename TSeq>
class Virus;

template<typename TSeq>
class PersonViruses;

template<typename TSeq>
class Model;



/**
 * @brief Virus
 * 
 * @tparam TSeq 
 * @details
 * Raw transmisibility of a virus should be a function of its genetic
 * sequence. Nonetheless, transmisibility can be reduced as a result of
 * having one or more tools to fight the virus. Because of this, transmisibility
 * should be a function of the host.
 */
template<typename TSeq>
class Virus {
    friend class PersonViruses<TSeq>;
    friend class DataBase<TSeq>;
private:
    Person<TSeq> * host = nullptr;
    std::shared_ptr<TSeq> baseline_sequence = std::make_shared<TSeq>(default_sequence<TSeq>());
    std::shared_ptr<std::string> virus_name = nullptr;
    int date = -99;
    int id   = -99;
    bool active = true;
    MutFun<TSeq> mutation_fun = nullptr;
    VirusFun<TSeq> post_recovery  = nullptr;
    VirusFun<TSeq> infectiousness = nullptr;
    VirusFun<TSeq> persistance    = nullptr;
    VirusFun<TSeq> death          = nullptr;

    // Setup parameters
    std::vector< double * > params;
    std::vector< double > data;

public:
    Virus(std::string name = "unknown virus");
    Virus(TSeq sequence, std::string name = "unknown virus");
    void mutate();
    void set_mutation(MutFun<TSeq> fun);
    const TSeq* get_sequence();
    void set_sequence(TSeq sequence);
    Person<TSeq> * get_host();
    Model<TSeq> * get_model();
    void set_date(int d);
    int get_date() const;
    void set_id(int idx);
    int get_id() const;
    bool is_active() const;

        /**
     * @brief Get and set the tool functions
     * 
     * @param v The virus over which to operate
     * @param fun the function to be used
     * 
     * @return double 
     */
    ///@[
    double get_infectiousness(Virus<TSeq> * v);
    double get_persistance(Virus<TSeq> * v);
    double get_death(Virus<TSeq> * v);
    void get_post_recovery();
    void set_infectiousness(VirusFun<TSeq> fun);
    void set_persistance(VirusFun<TSeq> fun);
    void set_death(VirusFun<TSeq> fun);
    void set_post_recovery(VirusFun<TSeq> fun);
    void set_infectiousness(double prob);
    void set_persistance(double prob);
    void set_death(double prob);
    void set_post_recovery(double prob);
    ///@]


    void set_name(std::string name);
    std::string get_name() const;

    std::vector< double > & get_data();

};

#endif