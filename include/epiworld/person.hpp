#ifndef EPIWORLD_PERSON_HPP
#define EPIWORLD_PERSON_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class PersonViruses;

template<typename TSeq>
class PersonTools;

template<typename TSeq>
class Person {
private:
    PersonViruses<TSeq> viruses;
    PersonTools<TSeq> tools;

public:

};

#endif