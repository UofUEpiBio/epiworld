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
 * @brief 
 * 
 * @tparam TSeq 
 * @details
 * Raw transmisibility of a virus should be a function of its genetic
 * sequence. Nonetheless, transmisibility can be reduced as a result of
 * having one or more tools to fight the virus. Because of this, transmisibility
 * should be a function of the person.
 */
template<typename TSeq>
class Virus {
    friend class PersonViruses<TSeq>;
    friend class DataBase<TSeq>;
private:
    std::shared_ptr<TSeq> baseline_sequence;
    Person<TSeq> * person;
    int date = -99;
    int id   = -99;
    std::shared_ptr<MutFun<TSeq>> mutation_fun = nullptr;

public:
    Virus() {};
    Virus(TSeq sequence);
    void mutate();
    // double transmisibility();
    void set_mutation(MutFun<TSeq> fun);
    const TSeq* get_sequence();
    void set_sequence(TSeq sequence);
    Person<TSeq> * get_person();
    Model<TSeq> * get_model();
    void set_date(int d);
    int get_date() const;
    void set_id(int idx);
    int get_id() const;

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
            person->get_model()->register_variant(this);

            if (get_model()->get_db().get_today_variant("ninfected")[tmpid] < 0)
                printf_epiworld("Epa!\n");
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
inline Person<TSeq> * Virus<TSeq>::get_person() {
    return person;
}

template<typename TSeq>
inline Model<TSeq> * Virus<TSeq>::get_model() {
    return person->get_model();
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
class PersonViruses {
    friend class Person<TSeq>;

private:
    Person<TSeq> * person;
    std::vector< Virus<TSeq> > viruses;
    std::vector< int > dates;
    std::vector< bool > active;

public:
    void add_virus(int date, Virus<TSeq> v);
    size_t size() const;
    Virus<TSeq> & operator()(int i);
    void mutate();
    void clear();

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
    viruses.at(viruses.size() - 1u).person = this->person;
    dates.push_back(date);
    active.push_back(true);

    // Pointing
    viruses[viruses.size() - 1u].person = this->person; 

    person->get_model()->get_db().up_infected(&v);

}

template<typename TSeq>
inline size_t PersonViruses<TSeq>::size() const {
    return viruses.size();
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
    this->dates.clear();
    this->active.clear();

}



#undef DEFAULT_TRANSMISIBILITY

#endif