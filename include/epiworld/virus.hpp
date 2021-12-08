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

#define DEFAULT_TRANSMISIBILITY 0.9

template<typename TSeq>
using MutFun = std::function<void(Virus<TSeq>*)>;

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
private:
    TSeq baseline_sequence;
    Person<TSeq> * person;
    int timestamp;
    std::shared_ptr<MutFun<TSeq>> mutation_fun = nullptr;
    std::shared_ptr<TransFun<TSeq>> transmisibility_fun = nullptr;

public:
    Virus() {};
    Virus(TSeq sequence);
    void mutate();
    double transmisibility();
    void set_mutation(MutFun<TSeq> fun);
    void set_transmisibility(TransFun<TSeq> fun);
    TSeq* get_sequence();

};

template<typename TSeq>
inline Virus<TSeq>::Virus(TSeq sequence) {
    baseline_sequence = sequence;
}

template<typename TSeq>
inline void Virus<TSeq>::mutate() {
    if (mutation_fun != nullptr)
        mutation_fun(this);

    return;
}

template<typename TSeq>
inline double Virus<TSeq>::transmisibility() {
    if (transmisibility_fun != nullptr)
        transmisibility_fun(this,person);

    return DEFAULT_TRANSMISIBILITY;
}

template<typename TSeq>
inline void Virus<TSeq>::set_mutation(
    MutFun<TSeq> fun
) {
    mutation_fun = std::make_shared<MutFun<TSeq>>(fun);
}

template<typename TSeq>
inline void Virus<TSeq>::set_transmisibility(
    TransFun<TSeq> fun
) {
    transmisibility_fun = std::make_shared<TransFun<TSeq>>(fun);
}

template<typename TSeq>
inline TSeq * Virus<TSeq>::get_sequence() {
    return &this->baseline_sequence;
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
    Virus<TSeq> & operator()(int i);
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
    dates.push_back(date);
    active.push_back(true);

    // Pointing
    viruses[viruses.size() - 1u].person = this->person; 

}

template<typename TSeq>
inline Virus<TSeq> & PersonViruses<TSeq>::operator()(
    int i
) {

    return viruses.at(i);

}

#undef DEFAULT_TRANSMISIBILITY

#endif