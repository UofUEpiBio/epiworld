
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
 * @brief A tool is a 
 * 
 */

template<typename TSeq> 
class Tool {
    friend class PersonTools<TSeq>;
    friend class Person<TSeq>;
private:

    Person<TSeq> * person;
    std::shared_ptr<TSeq> sequence = nullptr;
    TSeq sequence_unique;
    ToolFun<TSeq> efficacy        = nullptr;
    ToolFun<TSeq> transmisibility = nullptr;
    ToolFun<TSeq> recovery        = nullptr;
    ToolFun<TSeq> death           = nullptr;


public:
    Tool() {};
    Tool(TSeq d);

    void set_sequence(TSeq d);
    void set_sequence_unique(TSeq d);
    void set_sequence(std::shared_ptr<TSeq> d);
    std::shared_ptr<TSeq> get_sequence();
    TSeq & get_sequence_unique();

    /**
     * @brief Get the efficacy of tool
     * 
     * @param tool_id The index of the tool in the person
     * @param p The corresponding person.
     * @param m The corresponding model.
     * @return double 
     * 
     * @details If there is no efficacy or recovery function specified, it will return
     * the DEFAULT_EFFICACY or DEFAULT_RECOVERY as defined in the program.
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

};

#endif