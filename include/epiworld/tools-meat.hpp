
#ifndef EPIWORLD_TOOLS_MEAT_HPP
#define EPIWORLD_TOOLS_MEAT_HPP

template<typename TSeq>
inline Tool<TSeq>::Tool(std::string name)
{
    set_name(name);
}

template<typename TSeq>
inline Tool<TSeq>::Tool(TSeq d, std::string name) {
    sequence = std::make_shared<TSeq>(d);
    tool_name = std::make_shared<std::string>(name);
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
inline double Tool<TSeq>::get_contagion_reduction(
    Virus<TSeq> * v
) {

    if (contagion_reduction)
        return contagion_reduction(this, person, v, person->model);
    
    return DEFAULT_TOOL_EFFICACY;

}

template<typename TSeq>
inline double Tool<TSeq>::get_transmission_reduction(
    Virus<TSeq> * v
) {

    if (transmission_reduction)
        return transmission_reduction(this, this->person, v, person->get_model());
    
    return DEFAULT_TOOL_TRANSMISIBILITY;

}

template<typename TSeq>
inline double Tool<TSeq>::get_recovery_enhancer(
    Virus<TSeq> * v
) {

    if (recovery_enhancer)
        return recovery_enhancer(this, person, v, person->model);
    
    return DEFAULT_TOOL_RECOVERY;
    

}

template<typename TSeq>
inline double Tool<TSeq>::get_death_reduction(
    Virus<TSeq> * v
) {

    if (death_reduction)
        return death_reduction(this, person, v, person->model);
    
    return DEFAULT_DEATH;

}

template<typename TSeq>
inline void Tool<TSeq>::set_contagion_reduction(
    ToolFun<TSeq> fun
) {
    contagion_reduction = fun;
}

template<typename TSeq>
inline void Tool<TSeq>::set_recovery_enhancer(
    ToolFun<TSeq> fun
) {
    recovery_enhancer = fun;
}

template<typename TSeq>
inline void Tool<TSeq>::set_death_reduction(
    ToolFun<TSeq> fun
) {
    death_reduction = ToolFun<TSeq>(fun);
}

template<typename TSeq>
inline void Tool<TSeq>::set_contagion_reduction(
    ToolFun<TSeq> fun
) {
    contagion_reduction = ToolFun<TSeq>(fun);
}

template<typename TSeq>
inline void Tool<TSeq>::set_contagion_reduction(
    double * prob
    ) {
    
    ToolFun<TSeq> tmpfun =
        [&prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return *prob;
        };
    set_contagion_reduction(tmpfun);

}

template<typename TSeq>
inline void Tool<TSeq>::set_transmission_reduction(
    double * prob
    ) {
    
    ToolFun<TSeq> tmpfun =
        [&prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return *prob;
        };
    set_transmission_reduction(tmpfun);

}

template<typename TSeq>
inline void Tool<TSeq>::set_recovery_enhancer(
    double * prob
    ) {
    
    ToolFun<TSeq> tmpfun =
        [&prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return *prob;
        };
    set_recovery_enhancer(tmpfun);

}

template<typename TSeq>
inline void Tool<TSeq>::set_death_reduction(
    double * prob
    ) {
    
    ToolFun<TSeq> tmpfun =
        [&prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return *prob;
        };
    set_death_reduction(tmpfun);

}

template<typename TSeq>
inline void Tool<TSeq>::set_contagion_reduction(
    double prob
    ) {
    
    ToolFun<TSeq> tmpfun =
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return prob;
        };
    set_contagion_reduction(tmpfun);

}

template<typename TSeq>
inline void Tool<TSeq>::set_transmission_reduction(
    double prob
    ) {
    
    ToolFun<TSeq> tmpfun =
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return prob;
        };
    set_transmission_reduction(tmpfun);

}

template<typename TSeq>
inline void Tool<TSeq>::set_recovery_enhancer(
    double prob
    ) {
    
    ToolFun<TSeq> tmpfun =
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return prob;
        };
    set_recovery_enhancer(tmpfun);

}

template<typename TSeq>
inline void Tool<TSeq>::set_death_reduction(
    double prob
    ) {
    
    ToolFun<TSeq> tmpfun =
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return prob;
        };
    set_death_reduction(tmpfun);

}


template<typename TSeq>
inline void Tool<TSeq>::set_name(std::string name)
{
    if (name != "")
        tool_name = std::make_shared<std::string>(name);
}

template<typename TSeq>
inline std::string Tool<TSeq>::get_name() const {

    if (tool_name)
        return *tool_name;

    return "unknown tool";

}

template<typename TSeq>
inline Person<TSeq> * Tool<TSeq>::get_person()
{
    return person;
}

template<typename TSeq>
inline unsigned int Tool<TSeq>::get_id() const {
    return id;
}

#undef DEFAULT_EFFICACY
#undef DEFAULT_RECOVERY
#undef DEFAULT_DEATH
#endif