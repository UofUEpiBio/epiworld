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
    unsigned int index; ///< Location in the Model
    unsigned int status_next = STATUS::HEALTHY; // Placeholder
    unsigned int status      = STATUS::HEALTHY;
    int id          = -1;
    std::function<unsigned int(Person<TSeq>*,Model<TSeq>*)> update_susceptible = default_update_susceptible<TSeq>;
    std::function<unsigned int(Person<TSeq>*,Model<TSeq>*)> update_infected = default_update_infected<TSeq>;
    std::function<unsigned int(Person<TSeq>*,Model<TSeq>*)> update_removed = nullptr;

public:

    Person();
    void init();

    void add_tool(int d, Tool<TSeq> tool);
    void add_virus(int d, Virus<TSeq> virus);

    double get_contagion_reduction(Virus<TSeq> * v);
    double get_transmisibility(Virus<TSeq> * v);
    double get_recovery(Virus<TSeq> * v);
    double get_death(Virus<TSeq> * v);
    int get_id() const;
    unsigned int get_index() const;
    
    std::mt19937 * get_rand_endgine();
    Model<TSeq> * get_model(); 

    Virus<TSeq> & get_virus(int i);
    PersonViruses<TSeq> & get_viruses();

    Tool<TSeq> & get_tool(int i);
    PersonTools<TSeq> & get_tools();

    void mutate_variant();
    void add_neighbor(
        Person<TSeq> * p,
        bool check_source = true,
        bool check_target = true
        );

    std::vector< Person<TSeq> * > & get_neighbors();

    void update_status();
    unsigned int get_status() const;

    void reset();

};



#endif