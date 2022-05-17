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

template<typename TSeq>
inline void default_add_virus(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_add_tool(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_virus(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_tool(Action<TSeq> & a, Model<TSeq> * m);

/**
 * @brief Person (agents)
 * 
 * @tparam TSeq Sequence type (should match `TSeq` across the model)
 */
template<typename TSeq = bool>
class Person {
    friend class Model<TSeq>;
    friend class Tool<TSeq>;
    friend class Queue<TSeq>;
    friend void default_add_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_add_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
private:
    Model<TSeq> * model;
    
    std::vector< Person<TSeq> * > neighbors;

    unsigned int index; ///< Location in the Model
    epiworld_fast_uint status;
    int id          = -1;
    
    bool in_queue = false;
    bool locked   = false;
    
    std::vector< std::shared_ptr< Virus<TSeq> > > viruses;
    std::shared_ptr< Virus<TSeq> > virus_tmp;
    epiworld_fast_int virus_to_remove_idx;
    epiworld_fast_uint n_viruses;

    std::vector< std::shared_ptr< Tool<TSeq> > > tools;
    std::shared_ptr< Tool<TSeq> > tool_tmp;
    epiworld_fast_int tool_to_remove_idx;
    epiworld_fast_uint n_tools;

    ActionFun<TSeq> add_virus_ = default_add_virus<TSeq>;
    ActionFun<TSeq> add_tool_ = default_add_tools<TSeq>;
    ActionFun<TSeq> rm_virus_ = default_rm_virus<TSeq>;
    ActionFun<TSeq> rm_tool_ = default_rm_tools<TSeq>;

public:

    Person();
    void init(epiworld_fast_uint baseline_status);

    /**
     * @name Add/Remove Virus/Tool
     * 
     * Calling any of these functions will lock the agent (person)
     * until the action is applied at the end of the iteration. Calling
     * any of this functions when the agent is locked will cause an 
     * error.
     * 
     * @param tool Tool to add
     * @param virus Virus to add
     * @param status_new Status after the change
     * @param queue 
     */
    ///@{
    void add_tool(
        std::shared_ptr< Tool<TSeq> > tool,
        epiworld_fast_int status_new = -1,
        epiworld_fast_int queue = 0
        );

    void add_virus(
        std::shared_ptr< Virus<TSeq> > virus,
        epiworld_fast_int status_new = -1,
        epiworld_fast_int queue = 0
        );

    void rm_tool(
        epiworld_fast_uint tool_idx,
        epiworld_fast_int status_new = -1,
        epiworld_fast_int queue = 0
    );

    void rm_virus(
        epiworld_fast_uint virus_idx,
        epiworld_fast_int status_new = -1,
        epiworld_fast_int queue = 0
    );
    ///@}
    
    /**
     * @name Get the rates (multipliers) for the agent
     * 
     * @param v A pointer to a virus.
     * @return epiworld_double 
     */
    ///@{
    epiworld_double get_susceptibility_reduction(Virus<TSeq> * v);
    epiworld_double get_transmission_reduction(Virus<TSeq> * v);
    epiworld_double get_recovery_enhancer(Virus<TSeq> * v);
    epiworld_double get_death_reduction(Virus<TSeq> * v);
    ///@}

    int get_id() const; ///< Id of the individual
    unsigned int get_index() const; ///< Location (0, ..., n-1).
    
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

    void change_status(
        epiworld_fast_uint new_status,
        epiworld_fast_int queue = 0
        );

    const epiworld_fast_uint & get_status() const;

    void reset();

    bool has_tool(unsigned int t) const;
    bool has_tool(std::string name) const;
    bool has_virus(unsigned int t) const;
    bool has_virus(std::string name) const;

};



#endif