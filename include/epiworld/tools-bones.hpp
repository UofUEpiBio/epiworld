
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

// template<typename TSeq>
// using ToolFun = std::function<double(Tool<TSeq>*,Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

// template<typename TSeq>
// using MixerFun = std::function<double(PersonTools<TSeq>*,Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

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
    unsigned int id = 0u;
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

    Person<TSeq> * get_person();
    unsigned int get_id() const;


};

#endif