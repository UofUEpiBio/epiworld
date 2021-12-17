#include <vector>
#include <functional>
#include <memory>

#ifndef EPIWORLD_VIRUS_HPP
#define EPIWORLD_VIRUS_HPP

template<typename TSeq>
class Person;

template<typename TSeq>
class Virus;

template<typename TSeq>
class PersonViruses;

template<typename TSeq>
class Model;

template<typename TSeq>
using MutFun = std::function<bool(Virus<TSeq>*)>;

template<typename TSeq>
using TransFun = std::function<double(Virus<TSeq>*,Person<TSeq>*)>;

/**
 * @brief Virus
 * 
 * @tparam TSeq 
 * @details
 * Raw transmisibility of a virus should be a function of its genetic
 * sequence. Nonetheless, transmisibility can be reduced as a result of
 * having one or more tools to fight the virus. Because of this, transmisibility
 * should be a function of the host.
 */
template<typename TSeq>
class Virus {
    friend class PersonViruses<TSeq>;
    friend class DataBase<TSeq>;
private:
    std::shared_ptr<TSeq> baseline_sequence;
    Person<TSeq> * host;
    int date = -99;
    int id   = -99;
    bool active = true;
    std::shared_ptr<MutFun<TSeq>> mutation_fun = nullptr;

public:
    Virus() {};
    Virus(TSeq sequence);
    void mutate();
    // double transmisibility();
    void set_mutation(MutFun<TSeq> fun);
    const TSeq* get_sequence();
    void set_sequence(TSeq sequence);
    Person<TSeq> * get_host();
    Model<TSeq> * get_model();
    void set_date(int d);
    int get_date() const;
    void set_id(int idx);
    int get_id() const;
    bool is_active() const;

};

template<typename TSeq>
inline Virus<TSeq>::Virus(TSeq sequence) {
    baseline_sequence = std::make_shared<TSeq>(sequence);
}

template<typename TSeq>
inline void Virus<TSeq>::mutate() {

    if (mutation_fun)
        if ((*mutation_fun)(this))
        {
            int tmpid = get_id();
            host->get_model()->record_variant(this);

            if (get_model()->get_db().get_today_variant("ninfected")[tmpid] < 0)
            {
                printf_epiworld("Epa!\n");
            }
        }
    

    return;
}

template<typename TSeq>
inline void Virus<TSeq>::set_mutation(
    MutFun<TSeq> fun
) {
    mutation_fun = std::make_shared<MutFun<TSeq>>(fun);
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


/**
 * @brief Set of viruses in host
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class PersonViruses {
    friend class Person<TSeq>;

private:
    Person<TSeq> * host;
    std::vector< Virus<TSeq> > viruses;
    int nactive = 0;

public:
    void add_virus(int date, Virus<TSeq> v);
    size_t size() const;
    int size_active() const;
    Virus<TSeq> & operator()(int i);
    void mutate();
    void clear();
    void deactivate(Virus<TSeq> & v);

};

template<typename TSeq>
inline void PersonViruses<TSeq>::add_virus(
    int date,
    Virus<TSeq> v
) {

    // This will make an independent copy of the virus.
    // Will keep the original sequence and will point to the
    // mutation and transmisibility functions.
    viruses.push_back(v);
    int vloc = viruses.size() - 1u;
    viruses[vloc].host = host;
    viruses[vloc].date   = host->get_model()->today();

    host->get_model()->get_db().up_infected(&v);

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
inline void PersonViruses<TSeq>::clear()
{

    this->viruses.clear();

}

template<typename TSeq>
inline void PersonViruses<TSeq>::deactivate(Virus<TSeq> & v)
{

    if (v.get_host()->id != host->id)
        throw std::logic_error("A host cannot deactivate someone else's virus.");

    nactive--;
    v.active = false;

}

#undef DEFAULT_TRANSMISIBILITY

#endif