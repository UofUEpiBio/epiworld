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

#define EPIWORLD_GET_V(suffix,macroname) \
template<typename TSeq> \
inline double Virus<TSeq>:: EPI_TOKENPASTE(get_,suffix)() \
{ \
    if (suffix) \
        suffix(host, this, host->get_model()); \
    return EPI_TOKENPASTE(DEFAULT_VIRUS_,macroname);\
}

EPIWORLD_GET_V(infectiousness,INFECTIOUSNESS)
EPIWORLD_GET_V(persistance,PERSISTANCE)
EPIWORLD_GET_V(death,DEATH)

#undef EPIWORLD_GET_V

#define EPIWORLD_SET_V(suffix) \
template<typename TSeq> \
inline void Virus<TSeq>:: EPI_TOKENPASTE(set_,suffix)(\
    VirusFun<TSeq> fun) \
{ \
    suffix = fun;\
}

EPIWORLD_SET_V(infectiousness)
EPIWORLD_SET_V(persistance)
EPIWORLD_SET_V(death)

#undef EPIWORLD_SET_V

#define EPIWORLD_SET_LAMBDA(suffix) \
    template<typename TSeq> \
    inline void Virus<TSeq>:: EPI_TOKENPASTE(set_,suffix) (\
    double * prob) { \
    VirusFun<TSeq> tmpfun = \
        [&prob](Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m) { \
        return * prob; }; \
    EPI_TOKENPASTE(set_,suffix)(tmpfun);}

EPIWORLD_SET_LAMBDA(infectiousness)
EPIWORLD_SET_LAMBDA(persistance)
EPIWORLD_SET_LAMBDA(death)

#undef EPIWORLD_SET_LAMBDA

#define EPIWORLD_SET_LAMBDA2(suffix) \
    template<typename TSeq> \
    inline void Virus<TSeq>:: EPI_TOKENPASTE(set_,suffix) (\
    double prob) { \
    VirusFun<TSeq> tmpfun = \
        [prob](Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m) { \
        return prob; }; \
    EPI_TOKENPASTE(set_,suffix)(tmpfun);}

EPIWORLD_SET_LAMBDA2(infectiousness)
EPIWORLD_SET_LAMBDA2(persistance)
EPIWORLD_SET_LAMBDA2(death)

#undef EPIWORLD_SET_LAMBDA2

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
