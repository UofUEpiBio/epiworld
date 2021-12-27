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

template<typename TSeq>
using MutFun = std::function<bool(Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using PostRecFun = std::function<void(Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

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
    Person<TSeq> * host;
    std::shared_ptr<TSeq> baseline_sequence = std::make_shared<TSeq>(default_sequence<TSeq>());
    std::shared_ptr<std::string> virus_name = nullptr;
    int date = -99;
    int id   = -99;
    bool active = true;
    MutFun<TSeq> mutation_fun = nullptr;
    PostRecFun<TSeq> post_recovery_fun = nullptr;

    // Setup parameters
    std::vector< double * > params;  

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

    void set_post_recovery(PostRecFun<TSeq> fun);
    void post_recovery();

    void set_name(std::string name);
    std::string get_name() const;

    /**
     * @brief Setting and accessing parameters from the model
     * 
     * @details Tools can incorporate parameters included in the model.
     * Internally, parameters in the tool are stored as pointers to
     * an std::map<> of parameters in the model. Using the `unsigned int`
     * method directly fetches the parameters in the order these were
     * added to the tool. Accessing parameters via the `std::string` method
     * involves searching the parameter directly in the std::map<> member
     * of the model (so it is not recommended.)
     * 
     * The function `set_param()` can be used when the parameter already
     * exists in the model.
     * 
     * The `par()` function members are aliases for `get_param()`.
     * 
     * @param initial_val 
     * @param pname Name of the parameter to add or to fetch
     * @return The current value of the parameter
     * in the model.
     * 
     */
    ///@[
    double add_param(double initial_val, std::string pname, Model<TSeq> & m);
    double set_param(std::string pname, Model<TSeq> & m);
    double get_param(unsigned int k);
    double get_param(std::string pname);
    double par(unsigned int k);
    double par(std::string pname);
    double * p00,*p01,*p02,*p03,*p04,*p05,*p06,*p07,*p08,*p09,*p10;
    ///@]

};

#endif