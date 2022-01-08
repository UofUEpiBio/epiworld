#ifndef EPIWORLD_VIRUS_MEAT_HPP
#define EPIWORLD_VIRUS_MEAT_HPP

template<typename TSeq>
inline Virus<TSeq>::Virus(std::string name) {
    set_name(name);
}

template<typename TSeq>
inline Virus<TSeq>::Virus(TSeq sequence, std::string name) {
    baseline_sequence = std::make_shared<TSeq>(sequence);
    set_name(name);
}

template<typename TSeq>
inline void Virus<TSeq>::mutate() {

    if (mutation_fun)
        if (mutation_fun(host, this, this->get_model()))
            host->get_model()->record_variant(this);
    

    return;
}

template<typename TSeq>
inline void Virus<TSeq>::set_mutation(
    MutFun<TSeq> fun
) {
    mutation_fun = MutFun<TSeq>(fun);
}

template<typename TSeq>
inline const TSeq * Virus<TSeq>::get_sequence() {
    return &(*baseline_sequence);
}

template<typename TSeq>
inline void Virus<TSeq>::set_sequence(TSeq sequence) {
    baseline_sequence = std::make_shared<TSeq>(sequence);
    return;
}

template<typename TSeq>
inline Person<TSeq> * Virus<TSeq>::get_host() {
    return host;
}

template<typename TSeq>
inline Model<TSeq> * Virus<TSeq>::get_model() {
    return host->get_model();
}

template<typename TSeq>
inline void Virus<TSeq>::set_id(int idx) {
    id = idx;
    return;
}

template<typename TSeq>
inline int Virus<TSeq>::get_id() const {
    
    return id;
}

template<typename TSeq>
inline void Virus<TSeq>::set_date(int d) {
    date = d;
    return;
}

template<typename TSeq>
inline int Virus<TSeq>::get_date() const {
    
    return date;
}

template<typename TSeq>
inline bool Virus<TSeq>::is_active() const {
    return active;
}

template<typename TSeq>
inline double Virus<TSeq>::get_infectiousness()
{
    if (infectiousness)
        infectiousness(host, this, host->get_model());

    return DEFAULT_VIRUS_INFECTIOUSNESS;
}

template<typename TSeq>
inline double Virus<TSeq>::get_persistance()
{
    if (persistance)
        persistance(host, this, host->get_model());

    return DEFAULT_VIRUS_PERSISTANCE;
}

template<typename TSeq>
inline double Virus<TSeq>::get_death()
{
    if (death)
        death(host, this, host->get_model());

    return DEFAULT_VIRUS_DEATH;
}


template<typename TSeq>
inline void Virus<TSeq>::set_post_recovery(VirusFun<TSeq> fun)
{
    post_recovery = VirusFun<TSeq>(fun);
}

template<typename TSeq>
inline void Virus<TSeq>::get_post_recovery()
{

    if (post_recovery)
        return post_recovery(host, this, host->get_model());    

    return;
        
}

template<typename TSeq>
inline void Virus<TSeq>::set_name(std::string name)
{

    if (name == "")
        virus_name = nullptr;
    else
        virus_name = std::make_shared<std::string>(name);

}

template<typename TSeq>
inline std::string Virus<TSeq>::get_name() const
{

    if (virus_name)
        return *virus_name;
    
    return "unknown virus";

}

template<typename TSeq>
inline std::vector< double > & Virus<TSeq>::get_data() {
    return data;
}

#endif
