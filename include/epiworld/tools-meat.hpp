
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


#define EPIWORLD_SET_TOOL(suffix,macroname) \
    template<typename TSeq> \
    inline double Tool<TSeq>:: EPI_TOKENPASTE(get_,suffix) ( \
        Virus<TSeq> * v \
    ) { \
        if (suffix) \
            return suffix(this, this->person, v, person->get_model()); \
        return EPI_TOKENPASTE(DEFAULT_TOOL_,macroname);\
    }

EPIWORLD_SET_TOOL(contagion_reduction, CONTAGION_REDUCTION)
EPIWORLD_SET_TOOL(transmission_reduction, TRANSMISSION_REDUCTION)
EPIWORLD_SET_TOOL(recovery_enhancer, RECOVERY_ENHANCER)
EPIWORLD_SET_TOOL(death_reduction, DEATH_REDUCTION)

#undef EPIWORLD_SET_TOOL

#define EPIWORLD_SET(suffix,input) \
    template<typename TSeq> \
    inline void Tool<TSeq>:: EPI_TOKENPASTE(set_,suffix) (\
    input) { suffix = fun;}

EPIWORLD_SET(contagion_reduction,ToolFun<TSeq> fun)
EPIWORLD_SET(transmission_reduction,ToolFun<TSeq> fun)
EPIWORLD_SET(recovery_enhancer,ToolFun<TSeq> fun)
EPIWORLD_SET(death_reduction,ToolFun<TSeq> fun)

#undef EPIWORLD_SET

#define EPIWORLD_SET_LAMBDA(suffix,astk) \
    template<typename TSeq> \
    inline void Tool<TSeq>:: EPI_TOKENPASTE(set_,suffix) (\
    double astk prob) { \
    ToolFun<TSeq> tmpfun = \
        [&prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m) { \
        return astk prob; }; \
    EPI_TOKENPASTE(set_,suffix)(tmpfun);}

EPIWORLD_SET_LAMBDA(contagion_reduction,*)
EPIWORLD_SET_LAMBDA(transmission_reduction,*)
EPIWORLD_SET_LAMBDA(recovery_enhancer,*)
EPIWORLD_SET_LAMBDA(death_reduction,*)

#undef EPIWORLD_SET_LAMBDA

#define EPIWORLD_SET_LAMBDA2(suffix) \
    template<typename TSeq> \
    inline void Tool<TSeq>:: EPI_TOKENPASTE(set_,suffix) (\
    double prob) { \
    ToolFun<TSeq> tmpfun = \
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m) { \
        return prob; }; \
    EPI_TOKENPASTE(set_,suffix)(tmpfun);}

EPIWORLD_SET_LAMBDA2(contagion_reduction)
EPIWORLD_SET_LAMBDA2(transmission_reduction)
EPIWORLD_SET_LAMBDA2(recovery_enhancer)
EPIWORLD_SET_LAMBDA2(death_reduction)

#undef EPIWORLD_SET_LAMBDA2


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

#endif