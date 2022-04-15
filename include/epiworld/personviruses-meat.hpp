#ifndef EPIWORLD_PERSONVIRUSES_MEAT_HPP
#define EPIWORLD_PERSONVIRUSES_MEAT_HPP

template<typename TSeq>
inline void PersonViruses<TSeq>::add_virus(
    epiworld_fast_uint new_status,
    Virus<TSeq> v
) {

    // This will make an independent copy of the virus.
    // Will keep the original sequence and will point to the
    // mutation and transmisibility functions.
    viruses.push_back(v);
    int vloc = viruses.size() - 1u;
    viruses[vloc].host = host;
    viruses[vloc].date = host->get_model()->today();

    nactive++;

}

template<typename TSeq>
inline size_t PersonViruses<TSeq>::size() const {
    return viruses.size();
}

template<typename TSeq>
inline int PersonViruses<TSeq>::size_active() const {
    return nactive;
}


template<typename TSeq>
inline Virus<TSeq> & PersonViruses<TSeq>::operator()(
    int i
) {

    return viruses.at(i);

}

template<typename TSeq>
inline void PersonViruses<TSeq>::mutate()
{
    for (auto & v : viruses)
        v.mutate();
}

template<typename TSeq>
inline void PersonViruses<TSeq>::reset()
{

    this->viruses.clear();
    nactive = 0;

}

template<typename TSeq>
inline void PersonViruses<TSeq>::deactivate(Virus<TSeq> & v)
{

    if (v.get_host()->id != host->id)
        throw std::logic_error("A host cannot deactivate someone else's virus.");

    v.deactivate();

}

template<typename TSeq>
inline Person<TSeq> * PersonViruses<TSeq>::get_host() {
    return host;
}

template<typename TSeq>
inline bool PersonViruses<TSeq>::has_virus(unsigned int v) const
{
    int v2 = static_cast<int>(v);
    for (auto & virus : viruses)
        if (v2 == virus.get_id())
            return true;

    return false;
}

template<typename TSeq>
inline bool PersonViruses<TSeq>::has_virus(std::string vname) const
{
    for (auto & virus : viruses)
        if (vname == virus.get_name())
            return true;
            
    return false;
}


#endif