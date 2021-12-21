#ifndef EPIWORLD_PERSON_BONES_HPP
#define EPIWORLD_PERSON_BONES_HPP

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
    std::vector< Person<TSeq> * > neighbors;
    int status_next = HEALTHY; // Placeholder
    int status      = HEALTHY;
    int id          = -1;

public:

    Person();
    void init();

    void add_tool(int d, Tool<TSeq> tool);
    void add_virus(int d, Virus<TSeq> virus);

    double get_efficacy(Virus<TSeq> * v);
    double get_transmisibility(Virus<TSeq> * v);
    double get_recovery(Virus<TSeq> * v);
    double get_death(Virus<TSeq> * v);
    int get_id() const;
    
    std::mt19937 * get_rand_endgine();
    Model<TSeq> * get_model(); 

    Virus<TSeq> & get_virus(int i);
    PersonViruses<TSeq> & get_viruses();

    void mutate_variant();
    void add_neighbor(Person<TSeq> * p);

    void update_status();
    int get_status() const;

};



#endif