#include <vector>

#ifndef EPIWORLD_MODEL_HPP
#define EPIWORLD_MODEL_HPP

template<typename TSeq>
class Person;

template<typename TSeq>
class Model {
private:
    std::vector< Person<TSeq> > persons;

public:

    Model() {};
    Model(int size);
    std::vector< Person<TSeq> > * get_persons();

    Person<TSeq> & operator()(int i);

    size_t size() const;

};

template<typename TSeq>
inline Model<TSeq>::Model(int size) : persons(size) {
    for (auto & p : persons)
        p.model = this;
}

template<typename TSeq>
inline Person<TSeq> & Model<TSeq>::operator()(int i) {
    return persons.at(i);
}

template<typename TSeq>
inline size_t Model<TSeq>::size() const {
    return persons.size();
}

#endif