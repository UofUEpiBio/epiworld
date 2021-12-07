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
    mutation_fun = std::make_shared(fun);
}

template<typename TSeq>
inline void Virus<TSeq>::set_transmisibility(
    ransFun<TSeq> fun
) {
    transmisibility_fun = std::make_shared(fun);
}

template<typename TSeq>
inline TSeq * Virus<TSeq>::get_sequence() {
    return &this->baseline_sequence;
}


template<typename TSeq>
class PersonViruses {
private:
    Person<TSeq> * person;
    std::vector< Virus<TSeq> > viruses;
    std::vector< bool > active;
public:
    void add_virus(Virus<TSeq> & v);
};

template<typename TSeq>
inline void PersonViruses<TSeq>::add_virus(
    Virus<TSeq> & v
) {

    // This will make an independent copy of the virus.
    // Will keep the original sequence and will point to the
    // mutation and transmisibility functions.
    viruses.push_back(v);
    active.push_back(true);

    // Pointing
    viruses[viruses.size() - 1u].person = this->person; 

}

#undef DEFAULT_TRANSMISIBILITY

#endif