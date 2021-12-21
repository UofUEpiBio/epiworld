#ifndef EPIWORLD_PERSONTOOLS_MEAT_HPP
#define EPIWORLD_PERSONTOOLS_MEAT_HPP

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
    PersonTools<TSeq>* pt,
    Person<TSeq>* p,
    Virus<TSeq>* v,
    Model<TSeq>* m
)
{
    double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
        total *= (1.0 - pt->operator()(i).get_efficacy(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline double transmisibility_mixer_default(
    PersonTools<TSeq>* pt,
    Person<TSeq>* p,
    Virus<TSeq>* v,
    Model<TSeq>* m
)
{
    double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
        total *= (1.0 - pt->operator()(i).get_transmisibility(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline double recovery_mixer_default(
    PersonTools<TSeq>* pt,
    Person<TSeq>* p,
    Virus<TSeq>* v,
    Model<TSeq>* m
)
{
    double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
        total *= (1.0 - pt->operator()(i).get_recovery(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline double death_mixer_default(
    PersonTools<TSeq>* pt,
    Person<TSeq>* p,
    Virus<TSeq>* v,
    Model<TSeq>* m
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

    return efficacy_mixer(this, person, v, person->get_model());

}

template<typename TSeq>
inline double PersonTools<TSeq>::get_transmisibility(
    Virus<TSeq> * v
) {

    if (!transmisibility_mixer)
        set_transmisibility_mixer(transmisibility_mixer_default<TSeq>);

    return transmisibility_mixer(this, person, v, person->get_model());

}

template<typename TSeq>
inline double PersonTools<TSeq>::get_recovery(
    Virus<TSeq> * v
) {

    if (!recovery_mixer)
        set_recovery_mixer(recovery_mixer_default<TSeq>);

    return recovery_mixer(this, person, v, person->get_model());

}


template<typename TSeq>
inline double PersonTools<TSeq>::get_death(
    Virus<TSeq> * v
) {

    if (!death_mixer)
        set_death_mixer(death_mixer_default<TSeq>);

    return death_mixer(this, person, v, person->get_model());

}

template<typename TSeq>
inline void PersonTools<TSeq>::set_efficacy_mixer(
    MixerFun<TSeq> fun
) {
    efficacy_mixer = MixerFun<TSeq>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_transmisibility_mixer(
    MixerFun<TSeq> fun
) {
    transmisibility_mixer = MixerFun<TSeq>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_recovery_mixer(
    MixerFun<TSeq> fun
) {
    recovery_mixer = MixerFun<TSeq>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_death_mixer(
    MixerFun<TSeq> fun
) {
    death_mixer = MixerFun<TSeq>(fun);
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

#endif