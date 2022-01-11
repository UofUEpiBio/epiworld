#ifndef EPIWORLD_PERSONTOOLS_MEAT_HPP
#define EPIWORLD_PERSONTOOLS_MEAT_HPP

/**
 * @brief Default function for combining susceptibility_reduction levels
 * 
 * @tparam TSeq 
 * @param pt 
 * @return epiworld_double 
 */
///@[
template<typename TSeq>
inline epiworld_double susceptibility_reduction_mixer_default(
    PersonTools<TSeq>* pt,
    Person<TSeq>* p,
    Virus<TSeq>* v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
        total *= (1.0 - pt->operator()(i).get_susceptibility_reduction(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline epiworld_double transmission_reduction_mixer_default(
    PersonTools<TSeq>* pt,
    Person<TSeq>* p,
    Virus<TSeq>* v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
        total *= (1.0 - pt->operator()(i).get_transmission_reduction(v));

    return (1.0 - total);
    
}

template<typename TSeq>
inline epiworld_double recovery_enhancer_mixer_default(
    PersonTools<TSeq>* pt,
    Person<TSeq>* p,
    Virus<TSeq>* v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
        total *= (1.0 - pt->operator()(i).get_recovery_enhancer(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline epiworld_double death_reduction_mixer_default(
    PersonTools<TSeq>* pt,
    Person<TSeq>* p,
    Virus<TSeq>* v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
    {
        total *= (1.0 - pt->operator()(i).get_death_reduction(v));
    } 

    return 1.0 - total;
    
}
///@]

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
inline epiworld_double PersonTools<TSeq>::get_susceptibility_reduction(
    Virus<TSeq> * v
) {

    if (!susceptibility_reduction_mixer)
        set_susceptibility_reduction_mixer(susceptibility_reduction_mixer_default<TSeq>);

    return susceptibility_reduction_mixer(this, person, v, person->get_model());

}

template<typename TSeq>
inline epiworld_double PersonTools<TSeq>::get_transmission_reduction(
    Virus<TSeq> * v
) {

    if (!transmission_reduction_mixer)
        set_transmission_reduction_mixer(transmission_reduction_mixer_default<TSeq>);

    return transmission_reduction_mixer(this, person, v, person->get_model());

}

template<typename TSeq>
inline epiworld_double PersonTools<TSeq>::get_recovery_enhancer(
    Virus<TSeq> * v
) {

    if (!recovery_enhancer_mixer)
        set_recovery_enhancer_mixer(recovery_enhancer_mixer_default<TSeq>);

    return recovery_enhancer_mixer(this, person, v, person->get_model());

}


template<typename TSeq>
inline epiworld_double PersonTools<TSeq>::get_death_reduction(
    Virus<TSeq> * v
) {

    if (!death_reduction_mixer)
        set_death_reduction_mixer(death_reduction_mixer_default<TSeq>);

    return death_reduction_mixer(this, person, v, person->get_model());

}

template<typename TSeq>
inline void PersonTools<TSeq>::set_susceptibility_reduction_mixer(
    MixerFun<TSeq> fun
) {
    susceptibility_reduction_mixer = MixerFun<TSeq>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_transmission_reduction_mixer(
    MixerFun<TSeq> fun
) {
    transmission_reduction_mixer = MixerFun<TSeq>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_recovery_enhancer_mixer(
    MixerFun<TSeq> fun
) {
    recovery_enhancer_mixer = MixerFun<TSeq>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_death_reduction_mixer(
    MixerFun<TSeq> fun
) {
    death_reduction_mixer = MixerFun<TSeq>(fun);
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

template<typename TSeq>
inline void PersonTools<TSeq>::reset()
{
    
    this->tools.clear();
    this->dates.clear();

}

template<typename TSeq>
inline bool PersonTools<TSeq>::has_tool(unsigned int t) const {

    for (auto & tool : tools)
        if (tool.get_id() == t)
            return true;

    return false;

}

template<typename TSeq>
inline bool PersonTools<TSeq>::has_tool(std::string name) const {

    for (auto * tool : tools)
        if (tool->get_name() == name)
            return true;

    return false;
    
}

#endif