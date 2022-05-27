#ifndef EPIWORLD_PERSON_BONES_HPP
#define EPIWORLD_PERSON_BONES_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Viruses;

template<typename TSeq>
class Viruses_const;

template<typename TSeq>
class Tool;

template<typename TSeq>
class Tools;

template<typename TSeq>
class Tools_const;

template<typename TSeq>
class Queue;

template<typename TSeq>
struct Action;

template<typename TSeq>
inline void default_add_virus(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_add_tool(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_virus(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_tool(Action<TSeq> & a, Model<TSeq> * m);

/**
 * @brief Agent (agents)
 * 
 * @tparam TSeq Sequence type (should match `TSeq` across the model)
 */
template<typename TSeq = int>
class Agent {
    friend class Model<TSeq>;
    friend class Virus<TSeq>;
    friend class Viruses<TSeq>;
    friend class Viruses_const<TSeq>;
    friend class Tool<TSeq>;
    friend class Tools<TSeq>;
    friend class Queue<TSeq>;
    friend void default_add_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_add_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
private:
    Model<TSeq> * model;
    
    std::vector< Agent<TSeq> * > neighbors;

    unsigned int index; ///< Location in the Model
    epiworld_fast_uint status = 0u;
    int id = -1;
    
    bool in_queue = false;
    // size_t actions_queued = 0u;
    
    std::vector< VirusPtr<TSeq> > viruses;
    epiworld_fast_uint n_viruses = 0u;

    std::vector< ToolPtr<TSeq> > tools;
    epiworld_fast_uint n_tools = 0u;

    ActionFun<TSeq> add_virus_ = default_add_virus<TSeq>;
    ActionFun<TSeq> add_tool_  = default_add_tool<TSeq>;
    ActionFun<TSeq> rm_virus_  = default_rm_virus<TSeq>;
    ActionFun<TSeq> rm_tool_   = default_rm_tool<TSeq>;

    epiworld_fast_uint action_counter = 0u;

public:

    Agent();
    Agent(const Agent<TSeq> & p);

    /**
     * @name Add/Remove Virus/Tool
     * 
     * Any of these is ultimately reflected at the end of the iteration.
     * 
     * @param tool Tool to add
     * @param virus Virus to add
     * @param status_new Status after the change
     * @param queue 
     */
    ///@{
    void add_tool(
        ToolPtr<TSeq> tool,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
        );

    void add_tool(
        Tool<TSeq> tool,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
        );

    void add_virus(
        VirusPtr<TSeq> virus,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
        );

    void add_virus(
        Virus<TSeq> virus,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
        );

    void rm_tool(
        epiworld_fast_uint tool_idx,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_tool(
        ToolPtr<TSeq> & tool,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_virus(
        epiworld_fast_uint virus_idx,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_virus(
        VirusPtr<TSeq> & virus,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
    );
    ///@}
    
    /**
     * @name Get the rates (multipliers) for the agent
     * 
     * @param v A pointer to a virus.
     * @return epiworld_double 
     */
    ///@{
    epiworld_double get_susceptibility_reduction(VirusPtr<TSeq> v);
    epiworld_double get_transmission_reduction(VirusPtr<TSeq> v);
    epiworld_double get_recovery_enhancer(VirusPtr<TSeq> v);
    epiworld_double get_death_reduction(VirusPtr<TSeq> v);
    ///@}

    int get_id() const; ///< Id of the individual
    unsigned int get_index() const; ///< Location (0, ..., n-1).
    
    std::mt19937 * get_rand_endgine();
    Model<TSeq> * get_model(); 

    VirusPtr<TSeq> & get_virus(int i);
    Viruses<TSeq> get_viruses();
    const Viruses_const<TSeq> get_viruses() const;
    size_t get_n_viruses() const noexcept;

    ToolPtr<TSeq> & get_tool(int i);
    Tools<TSeq> get_tools();
    const Tools_const<TSeq> get_tools() const;
    size_t get_n_tools() const noexcept;

    void mutate_variant();
    void add_neighbor(
        Agent<TSeq> * p,
        bool check_source = true,
        bool check_target = true
        );

    std::vector< Agent<TSeq> * > & get_neighbors();

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