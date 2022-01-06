
#ifndef EPIWORLD_TOOLS_BONES_HPP
#define EPIWORLD_TOOLS_BONES_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Person;

template<typename TSeq>
class Model;

template<typename TSeq>
class PersonTools;

template<typename TSeq>
class Tool;

template<typename TSeq>
using ToolFun = std::function<double(Tool<TSeq>*,Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using MixerFun = std::function<double(PersonTools<TSeq>*,Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

/**
 * @brief Tools for defending the host against the virus
 * 
 * @tparam TSeq Type of sequence
 */
template<typename TSeq> 
class Tool {
    friend class PersonTools<TSeq>;
    friend class Person<TSeq>;
    friend class Model<TSeq>;
private:

    Person<TSeq> * person;
    std::shared_ptr<std::string> tool_name = nullptr;
    std::shared_ptr<TSeq> sequence = nullptr;
    TSeq sequence_unique  = default_sequence<TSeq>();
    ToolFun<TSeq> efficacy        = nullptr;
    ToolFun<TSeq> transmisibility = nullptr;
    ToolFun<TSeq> recovery        = nullptr;
    ToolFun<TSeq> death           = nullptr;

    // Setup parameters
    std::vector< double * > params;  

public:
    Tool(std::string name = "unknown tool");
    Tool(TSeq d, std::string name = "unknown tool");

    void set_sequence(TSeq d);
    void set_sequence_unique(TSeq d);
    void set_sequence(std::shared_ptr<TSeq> d);
    std::shared_ptr<TSeq> get_sequence();
    TSeq & get_sequence_unique();

    /**
     * @brief Get and set the tool functions
     * 
     * @param v The virus over which to operate
     * @param fun the function to be used
     * 
     * @return double 
     */
    ///@[
    double get_efficacy(Virus<TSeq> * v);
    double get_transmisibility(Virus<TSeq> * v);
    double get_recovery(Virus<TSeq> * v);
    double get_death(Virus<TSeq> * v);
    void set_efficacy(ToolFun<TSeq> fun);
    void set_transmisibility(ToolFun<TSeq> fun);
    void set_recovery(ToolFun<TSeq> fun);
    void set_death(ToolFun<TSeq> fun);
    ///@]

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

    Person<TSeq> * get_person();

};

#endif