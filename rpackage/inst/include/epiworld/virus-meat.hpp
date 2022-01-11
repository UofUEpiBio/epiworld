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
inline void Virus<TSeq>::set_post_recovery(VirusFun<TSeq> fun)
{
    post_recovery_fun = VirusFun<TSeq>(fun);
}

template<typename TSeq>
inline void Virus<TSeq>::post_recovery()
{

    if (post_recovery_fun)
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
inline epiworld_double Virus<TSeq>::add_param(
    epiworld_double initial_value,
    std::string pname,
    Model<TSeq> & m
    ) {

    std::map<std::string,epiworld_double> & parmap = m.params();

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
inline epiworld_double Virus<TSeq>::set_param(
    std::string pname,
    Model<TSeq> & m
    ) {

    std::map<std::string,epiworld_double> & parmap = m.params();

    if (parmap.find(pname) == parmap.end())
        throw std::logic_error("The parameter " + pname + " does not exists.");

    params.push_back(&(parmap[pname]));

    unsigned int res = static_cast<unsigned int >(params.size()) - 1u;
    CASES_PAR(res)

    return parmap[pname];
    
}


template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_param(unsigned int k)
{
    return *(params[k]);
}

template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_param(std::string pname)
{
    return host->model->params()[pname];
}

template<typename TSeq>
inline epiworld_double Virus<TSeq>::par(unsigned int k)
{
    return *(params[k]);
}

template<typename TSeq>
inline epiworld_double Virus<TSeq>::par(std::string pname)
{
    return host->model->params()[pname];
}

#undef CASES_PAR
#undef CASE_PAR

#endif
