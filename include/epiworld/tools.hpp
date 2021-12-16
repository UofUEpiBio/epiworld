#include <vector>
#include <functional>
#include <memory>

#ifndef EPIWORLD_TOOLS_HPP
#define EPIWORLD_TOOLS_HPP

#define DEFAULT_EFFICACY        0.0
#define DEFAULT_TRANSMISIBILITY 1.0
#define DEFAULT_RECOVERY        0.0
#define DEFAULT_DEATH           1.0

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
using MixerFun = std::function<double(Virus<TSeq>*,PersonTools<TSeq>*)>;

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
    std::shared_ptr<ToolFun<TSeq>> efficacy        = nullptr;
    std::shared_ptr<ToolFun<TSeq>> transmisibility = nullptr;
    std::shared_ptr<ToolFun<TSeq>> recovery        = nullptr;
    std::shared_ptr<ToolFun<TSeq>> death           = nullptr;


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



template<typename TSeq>
inline Tool<TSeq>::Tool(TSeq d) {
    sequence = std::make_shared<TSeq>(d);
}

template<typename TSeq>
inline void Tool<TSeq>::set_sequence(TSeq d) {
    sequence = std::make_shared<TSeq>(d);
}

template<typename TSeq>
inline void Tool<TSeq>::set_sequence_unique(TSeq d) {
    sequence_unique = d;
}

template<typename TSeq>
inline void Tool<TSeq>::set_sequence(std::shared_ptr<TSeq> d) {
    sequence = d;
}

template<typename TSeq>
inline std::shared_ptr<TSeq> Tool<TSeq>::get_sequence() {
    return sequence;
}

template<typename TSeq>
inline TSeq & Tool<TSeq>::get_sequence_unique() {
    return sequence_unique;
}

template<typename TSeq>
inline double Tool<TSeq>::get_efficacy(
    Virus<TSeq> * v
) {

    if (!efficacy)
        return DEFAULT_EFFICACY;
    
    return (*this->efficacy)(this, person, v, person->model);

}

template<typename TSeq>
inline double Tool<TSeq>::get_transmisibility(
    Virus<TSeq> * v
) {

    if (transmisibility)
        return (*transmisibility)(this, this->person, v, person->get_model());
    
    return DEFAULT_TRANSMISIBILITY;

}

template<typename TSeq>
inline double Tool<TSeq>::get_recovery(
    Virus<TSeq> * v
) {

    if (!recovery)
        return DEFAULT_RECOVERY;
    
    return (*this->recovery)(this, person, v, person->model);

}

template<typename TSeq>
inline double Tool<TSeq>::get_death(
    Virus<TSeq> * v
) {

    if (!death)
        return DEFAULT_DEATH;
    
    return (*this->death)(this, person, v, person->model);

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

template<typename TSeq>
inline void Tool<TSeq>::set_transmisibility(
    ToolFun<TSeq> fun
) {
    transmisibility = std::make_shared<ToolFun<TSeq>>(fun);
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
    for (unsigned int i = 0; i < pt->size(); ++i)
        total *= (1.0 - pt->operator()(i).get_efficacy(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline double transmisibility_mixer_default(
    Virus<TSeq>* v,
    PersonTools<TSeq>* pt
)
{
    double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
        total *= (1.0 - pt->operator()(i).get_transmisibility(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline double recovery_mixer_default(
    Virus<TSeq>* v,
    PersonTools<TSeq>* pt
)
{
    double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
        total *= (1.0 - pt->operator()(i).get_recovery(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline double death_mixer_default(
    Virus<TSeq>* v,
    PersonTools<TSeq>* pt
)
{
    double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
    {
        total *= pt->operator()(i).get_death(v);
    }

    

    return total;
    
}
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
    std::vector<Tool<TSeq>> tools;
    std::vector< int > dates;
    std::shared_ptr<MixerFun<TSeq>> efficacy_mixer;
    std::shared_ptr<MixerFun<TSeq>> transmisibility_mixer;
    std::shared_ptr<MixerFun<TSeq>> recovery_mixer;
    std::shared_ptr<MixerFun<TSeq>> death_mixer;
    std::shared_ptr<ToolFun<TSeq>>  post_recovery;

public:
    PersonTools() {};
    void add_tool(int date, Tool<TSeq> tool);
    double get_efficacy(Virus<TSeq> * v);
    double get_transmisibility(Virus<TSeq> * v);
    double get_recovery(Virus<TSeq> * v);
    double get_death(Virus<TSeq> * v);

    void set_efficacy_mixer(MixerFun<TSeq> fun);
    void set_transmisibility_mixer(MixerFun<TSeq> fun);
    void set_recovery_mixer(MixerFun<TSeq> fun);
    void set_death_mixer(MixerFun<TSeq> fun);
    void set_post_recovery(ToolFun<TSeq> fun);

    size_t size() const;
    Tool<TSeq> & operator()(int i);

    Person<TSeq> * get_person();
    Model<TSeq> * get_model();

};

template<typename TSeq>
inline void PersonTools<TSeq>::add_tool(
    int date,
    Tool<TSeq> tool
) {
    tools.push_back(tool);
    tools[tools.size() - 1].person = this->person;
    dates.push_back(date);
}

template<typename TSeq>
inline double PersonTools<TSeq>::get_efficacy(
    Virus<TSeq> * v
) {

    if (!efficacy_mixer)
        set_efficacy_mixer(efficacy_mixer_default<TSeq>);

    return (*efficacy_mixer)(v, this);

}

template<typename TSeq>
inline double PersonTools<TSeq>::get_transmisibility(
    Virus<TSeq> * v
) {

    if (!transmisibility_mixer)
        set_transmisibility_mixer(transmisibility_mixer_default<TSeq>);

    return (*transmisibility_mixer)(v, this);

}

template<typename TSeq>
inline double PersonTools<TSeq>::get_recovery(
    Virus<TSeq> * v
) {

    if (!recovery_mixer)
        set_recovery_mixer(recovery_mixer_default<TSeq>);

    return (*recovery_mixer)(v, this);

}


template<typename TSeq>
inline double PersonTools<TSeq>::get_death(
    Virus<TSeq> * v
) {

    if (!death_mixer)
        set_death_mixer(death_mixer_default<TSeq>);

    return (*death_mixer)(v, this);

}

template<typename TSeq>
inline void PersonTools<TSeq>::set_efficacy_mixer(
    MixerFun<TSeq> fun
) {
    efficacy_mixer = std::make_shared<MixerFun<TSeq>>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_transmisibility_mixer(
    MixerFun<TSeq> fun
) {
    transmisibility_mixer = std::make_shared<MixerFun<TSeq>>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_recovery_mixer(
    MixerFun<TSeq> fun
) {
    recovery_mixer = std::make_shared<MixerFun<TSeq>>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_death_mixer(
    MixerFun<TSeq> fun
) {
    death_mixer = std::make_shared<MixerFun<TSeq>>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_post_recovery(
    ToolFun<TSeq> fun
) {
    post_recovery = std::make_shared<ToolFun<TSeq>>(fun);
}

template<typename TSeq>
inline size_t PersonTools<TSeq>::size() const {
    return tools.size();
}

template<typename TSeq>
inline Tool<TSeq> & PersonTools<TSeq>::operator()(int i) {
    return tools.at(i);
}

template<typename TSeq>
inline Person<TSeq> * PersonTools<TSeq>::get_person() {
    return person;
}

template<typename TSeq>
inline Model<TSeq> * PersonTools<TSeq>::get_model() {
    return person->get_model();
}

#undef DEFAULT_EFFICACY
#undef DEFAULT_RECOVERY
#undef DEFAULT_DEATH

#endif