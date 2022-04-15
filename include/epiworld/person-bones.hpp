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
class Queue;

template<typename TSeq = bool>
class Person {
    friend class Model<TSeq>;
    friend class Tool<TSeq>;
    friend class Queue<TSeq>;
private:
    Model<TSeq> * model;
    PersonViruses<TSeq> viruses;
    PersonTools<TSeq> tools;
    std::vector< Person<TSeq> * > neighbors;
    unsigned int index; ///< Location in the Model
    epiworld_fast_uint status_next; // Placeholder
    epiworld_fast_uint status;
    int id          = -1;
    UpdateFun<TSeq> update_susceptible = default_update_susceptible<TSeq>;
    UpdateFun<TSeq> update_exposed     = default_update_exposed<TSeq>;
    UpdateFun<TSeq> update_removed     = nullptr;

    bool visited_person = !EPI_DEFAULT_VISITED;
    bool in_queue       = false;

public:

    Person();
    void init(epiworld_fast_uint baseline_status);

    void add_tool(int d, Tool<TSeq> tool);
    void add_virus(Virus<TSeq> * virus);
    void rm_virus(Virus<TSeq> * virus);

    epiworld_double get_susceptibility_reduction(Virus<TSeq> * v);
    epiworld_double get_transmission_reduction(Virus<TSeq> * v);
    epiworld_double get_recovery_enhancer(Virus<TSeq> * v);
    epiworld_double get_death_reduction(Virus<TSeq> * v);
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
    void update_status(epiworld_fast_uint new_status);
    const epiworld_fast_uint & get_status() const;
    const epiworld_fast_uint & get_status_next() const;

    void reset();

    void set_update_susceptible(UpdateFun<TSeq> fun);
    void set_update_exposed(UpdateFun<TSeq> fun);
    void set_update_removed(UpdateFun<TSeq> fun);
    bool has_tool(unsigned int t) const;
    bool has_tool(std::string name) const;
    bool has_virus(unsigned int t) const;
    bool has_virus(std::string name) const;

    bool visited() const;
    void toggle_visited();

};



#endif