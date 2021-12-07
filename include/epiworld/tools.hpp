#include <vector>
#include <functional>
#include <memory>

#ifndef EPIWORLD_TOOLS_HPP
#define EPIWORLD_TOOLS_HPP

#define DEFAULT_EFFICACY .9
#define DEFAULT_RECOVERY .5
#define DEFAULT_DEATH    .1

template<typename TSeq>
class Virus;

template<typename TSeq>
class Person;

template<typename TSeq>
class Model;

template<typename TSeq>
class PersonTools;

template<typename TSeq>
using ToolFun = std::function<double(Virus<TSeq>*,PersonTools<TSeq>*)>;

/**
 * @brief A tool is a 
 * 
 */

template<typename TSeq> 
class Tool {
    friend class PersonTools<TSeq>;
private:
    TSeq dat;
    std::shared_ptr<ToolFun<TSeq>> efficacy = nullptr;
    std::shared_ptr<ToolFun<TSeq>> recovery = nullptr;
    std::shared_ptr<ToolFun<TSeq>> death = nullptr;


public:
    Tool() {};

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
    double get_efficacy(int tool_id, Virus<TSeq> * v, Person<TSeq> * p, Model<TSeq> * m);
    double get_recovery(int tool_id, Virus<TSeq> * v, Person<TSeq> * p, Model<TSeq> * m);
    double get_death(int tool_id, Virus<TSeq> * v, Person<TSeq> * p, Model<TSeq> * m);
    void set_efficacy(ToolFun<TSeq> fun);
    void set_recovery(ToolFun<TSeq> fun);
    void set_death(ToolFun<TSeq> fun);
    ///@]

};

template<typename TSeq>
inline double Tool<TSeq>::get_efficacy(
    int tool_id,
    Virus<TSeq> * v,
    Person<TSeq> * p,
    Model<TSeq> * m
) {

    if (efficacy == nullptr)
        return DEFAULT_EFFICACY;
    
    return this->efficacy(tool_id, v, p, m);

}

template<typename TSeq>
inline double Tool<TSeq>::get_recovery(
    int tool_id,
    Virus<TSeq> * v,
    Person<TSeq> * p,
    Model<TSeq> * m
) {

    if (efficacy == nullptr)
        return DEFAULT_RECOVERY;
    
    return this->recovery(tool_id, v, p, m);

}

template<typename TSeq>
inline double Tool<TSeq>::get_death(
    int tool_id,
    Virus<TSeq> * v,
    Person<TSeq> * p,
    Model<TSeq> * m
) {

    if (efficacy == nullptr)
        return DEFAULT_DEATH;
    
    return this->death(tool_id, v, p, m);

}

template<typename TSeq>
inline void Tool<TSeq>::set_efficacy(
    ToolFun<TSeq> fun
) {
    efficacy = std::make_shared<ToolFun<TSeq>>(fun);
}

template<typename TSeq>
inline void Tool<TSeq>::set_recovery(
    ToolFun<TSeq> fun
) {
    recovery = std::make_shared<ToolFun<TSeq>>(fun);
}

template<typename TSeq>
inline void Tool<TSeq>::set_death(
    ToolFun<TSeq> fun
) {
    death = std::make_shared<ToolFun<TSeq>>(fun);
}

/**
 * @brief Default function for combining efficacy levels
 * 
 * @tparam TSeq 
 * @param pt 
 * @return double 
 */
///@[
template<typename TSeq>
inline double efficacy_mixer_default(
    Virus<TSeq>* v,
    PersonTools<TSeq>* pt
)
{
    double total = 1.0;
    for (int i = 0; i < pt->tools.size(); ++i)
        total *= (1.0 - pt->tools[i]->get_efficacy(i, v, pt->person, pt->model));

    return 1.0 - total;
    
};

template<typename TSeq>
inline double recovery_mixer_default(
    Virus<TSeq>* v,
    PersonTools<TSeq>* pt
)
{
    double total = 1.0;
    for (int i = 0; i < pt->tools.size(); ++i)
        total *= (1.0 - pt->tools[i]->get_recovery(i, v, pt->person, pt->model));

    return 1.0 - total;
    
};

template<typename TSeq>
inline double death_mixer_default(
    Virus<TSeq>* v,
    PersonTools<TSeq>* pt
)
{
    double total = 1.0;
    for (int i = 0; i < pt->tools.size(); ++i)
        total *= (1.0 - pt->tools[i]->get_death(i, v, pt->person, pt->model));

    return 1.0 - total;
    
};
///@]



/**
 * @brief List of tools available for the individual to 
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class PersonTools {
    friend class Virus<TSeq>;
    friend class Tool<TSeq>;
    friend class Person<TSeq>;
private:
    Person<TSeq> * person; 
    Model<TSeq> * model;
    std::vector<Tool<TSeq>*> tools;
    std::vector< int > date;
    std::function<double(Virus<TSeq>*,PersonTools<TSeq>*)> efficacy_mixer;
    std::function<double(Virus<TSeq>*,PersonTools<TSeq>*)> recovery_mixer;
    std::function<double(Virus<TSeq>*,PersonTools<TSeq>*)> death_mixer;

public:
    PersonTools() {};
    void add_tool(int d, Tool<TSeq> * tool);
    double get_efficacy(Virus<TSeq> * v);
    double get_recovery(Virus<TSeq> * v);
    double get_death(Virus<TSeq> * v);

};

template<typename TSeq>
inline void PersonTools<TSeq>::add_tool(
    int d,
    Tool<TSeq> * tool
) {
    tools.push_back(tool);
    date.push_back(d);
}

template<typename TSeq>
inline double PersonTools<TSeq>::get_efficacy(
    Virus<TSeq> * v
) {

    return efficacy_mixer(v, this);

}

template<typename TSeq>
inline double PersonTools<TSeq>::get_recovery(
    Virus<TSeq> * v
) {

    return recovery_mixer(v, this);

}


template<typename TSeq>
inline double PersonTools<TSeq>::get_death(
    Virus<TSeq> * v
) {

    return death_mixer(v, this);

}


#undef DEFAULT_EFFICACY
#undef DEFAULT_RECOVERY
#undef DEFAULT_DEATH

#endif