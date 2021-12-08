#ifndef EPIWORLD_PERSON_HPP
#define EPIWORLD_PERSON_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Tool;

template<typename TSeq>
class PersonViruses;

template<typename TSeq>
class PersonTools;

template<typename TSeq>
class Person {
    friend class Model<TSeq>;
    friend class Tool<TSeq>;

private:
    Model<TSeq> * model;
    PersonViruses<TSeq> viruses;
    PersonTools<TSeq> tools;

public:

    Person();

    void add_tool(int d, Tool<TSeq> tool);
    void add_virus(int d, Virus<TSeq> virus);

    double get_efficacy();
    double get_recovery();
    double get_death();
    std::mt19937 * get_rand_endgine();
    Model<TSeq> * get_model(); 

    Virus<TSeq> & get_virus(int i);
    void mutate_virus();

};

template<typename TSeq>
inline Person<TSeq>::Person() {
    tools.person = this;
    viruses.person = this;
}

template<typename TSeq>
inline void Person<TSeq>::add_tool(
    int d,
    Tool<TSeq> tool
) {
    tools.add_tool(d, tool);
}

template<typename TSeq>
inline void Person<TSeq>::add_virus(
    int d,
    Virus<TSeq> virus
) {
    viruses.add_virus(d, virus);
}

template<typename TSeq>
inline double Person<TSeq>::get_efficacy() {
    return tools.get_efficacy(&viruses(0u));
}

template<typename TSeq>
inline double Person<TSeq>::get_recovery() {
    return tools.get_recovery(&viruses(0u));
}

template<typename TSeq>
inline double Person<TSeq>::get_death() {
    return tools.get_death(&viruses(0u));
}

template<typename TSeq>
inline std::mt19937 * Person<TSeq>::get_rand_endgine() {
    return model->get_rand_endgine();
}

template<typename TSeq>
inline Model<TSeq> * Person<TSeq>::get_model() {
    return model;
}

template<typename TSeq>
inline Virus<TSeq> & Person<TSeq>::get_virus(int i) {
    return viruses(i);
}

template<typename TSeq>
inline void Person<TSeq>::mutate_virus() {
    viruses.mutate();
}

#endif