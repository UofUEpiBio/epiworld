
#ifndef EPIWORLD_TOOLS_MEAT_HPP
#define EPIWORLD_TOOLS_MEAT_HPP

#define DEFAULT_EFFICACY        0.0
#define DEFAULT_TRANSMISIBILITY 1.0
#define DEFAULT_RECOVERY        0.0
#define DEFAULT_DEATH           1.0

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

    if (!contagion_reduction)
        return DEFAULT_EFFICACY;
    
    return contagion_reduction(this, person, v, person->model);

}

template<typename TSeq>
inline double Tool<TSeq>::get_transmisibility(
    Virus<TSeq> * v
) {

    if (transmisibility)
    {
        double t = transmisibility(this, this->person, v, person->get_model());
        return t;
    }
    
    return DEFAULT_TRANSMISIBILITY;

}

template<typename TSeq>
inline double Tool<TSeq>::get_recovery(
    Virus<TSeq> * v
) {

    if (!recovery)
        return DEFAULT_RECOVERY;
    
    return recovery(this, person, v, person->model);

}

template<typename TSeq>
inline double Tool<TSeq>::get_death(
    Virus<TSeq> * v
) {

    if (!death)
        return DEFAULT_DEATH;
    
    return death(this, person, v, person->model);

}

template<typename TSeq>
inline void Tool<TSeq>::set_contagion_reduction(
    ToolFun<TSeq> fun
) {
    contagion_reduction = fun;
}

template<typename TSeq>
inline void Tool<TSeq>::set_recovery(
    ToolFun<TSeq> fun
) {
    recovery = fun;
}

template<typename TSeq>
inline void Tool<TSeq>::set_death(
    ToolFun<TSeq> fun
) {
    death = ToolFun<TSeq>(fun);
}

template<typename TSeq>
inline void Tool<TSeq>::set_transmisibility(
    ToolFun<TSeq> fun
) {
    transmisibility = ToolFun<TSeq>(fun);
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

#define CASE_PAR(a,b) case a: b = &(parmap[pname]);break;
#define CASES_PAR(a) \
    switch (a) \
    { \
    CASE_PAR(0u, p00) \
    CASE_PAR(1u, p01) \
    CASE_PAR(2u, p02) \
    CASE_PAR(3u, p03) \
    CASE_PAR(4u, p04) \
    CASE_PAR(5u, p05) \
    CASE_PAR(6u, p06) \
    CASE_PAR(7u, p07) \
    CASE_PAR(8u, p08) \
    CASE_PAR(9u, p09) \
    CASE_PAR(10u, p10) \
    default: \
        break; \
    }

template<typename TSeq>
inline double Tool<TSeq>::add_param(
    double initial_value,
    std::string pname,
    Model<TSeq> & m
    ) {

    std::map<std::string,double> & parmap = m.params();

    if (parmap.find(pname) == parmap.end())
        parmap[pname] = initial_value;

    /*
    "References and pointers to either key or data stored in the container are
    only invalidated by erasing that element, even when the corresponding
    iterator is invalidated."
    https://en.cppreference.com/w/cpp/container/unordered_map
    */
    params.push_back(&(parmap[pname]));

    unsigned int res = static_cast<unsigned int >(params.size()) - 1u;
    CASES_PAR(res)
    
    return initial_value;

}

template<typename TSeq>
inline double Tool<TSeq>::set_param(
    std::string pname,
    Model<TSeq> & m
    ) {

    std::map<std::string,double> & parmap = m.params();

    if (parmap.find(pname) == parmap.end())
        throw std::logic_error("The parameter " + pname + " does not exists.");

    params.push_back(&(parmap[pname]));
    
    unsigned int res = static_cast<unsigned int >(params.size()) - 1u;
    CASES_PAR(res)

    return parmap[pname];
    
}


template<typename TSeq>
inline double Tool<TSeq>::get_param(unsigned int k)
{
    return *(params[k]);
}

template<typename TSeq>
inline double Tool<TSeq>::get_param(std::string pname)
{
    return person->model->params()[pname];
}

template<typename TSeq>
inline double Tool<TSeq>::par(unsigned int k)
{
    return *(params[k]);
}

template<typename TSeq>
inline double Tool<TSeq>::par(std::string pname)
{
    return person->model->params()[pname];
}

template<typename TSeq>
inline Person<TSeq> * Tool<TSeq>::get_person()
{
    return person;
}

#undef CASES_PAR
#undef CASE_PAR
#undef DEFAULT_EFFICACY
#undef DEFAULT_RECOVERY
#undef DEFAULT_DEATH
#endif