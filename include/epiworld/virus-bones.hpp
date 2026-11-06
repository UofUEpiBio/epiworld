#ifndef EPIWORLD_VIRUS_HPP
#define EPIWORLD_VIRUS_HPP

template<typename TSeq>
class Agent;

template<typename TSeq>
class Virus;

template<typename TSeq>
class Model;

template<typename TSeq>
class Tool;

/**
 * @brief Virus
 * 
 * @tparam TSeq 
 * @details
 * Raw transmisibility of a virus should be a function of its genetic
 * sequence. Nonetheless, transmisibility can be reduced as a result of
 * having one or more tools to fight the virus. Because of this, transmisibility
 * should be a function of the agent.
 */
template<typename TSeq>
class Virus {
    friend class Agent<TSeq>;
    friend class Model<TSeq>;
    friend class DataBase<TSeq>;
    friend class Tool<TSeq>;
    friend class ContactLayer<TSeq>;
private:
    
    Agent<TSeq> * agent = nullptr;

    EPI_TYPENAME_TRAITS(TSeq, int) baseline_sequence = 
        EPI_TYPENAME_TRAITS(TSeq, int)(); 

    std::string virus_name = "unknown virus";
    int date = -99;
    int id   = -99;    
    int lineage_id = -99; ///< Id of the founding virus (kept across mutations).
    uint64_t lineage_bit = uint64_t(1) << 63; ///< Bit matched against Tool::target_mask.
    MutFun<TSeq>          mutation                 = nullptr;

    /// Parameters this variant reads in place of others (see `use_param()`)
    std::vector< std::pair< ParamRef, ParamRef > > param_redirects;

    /// Its natural history (shared by its copies; copied on the first edit)
    std::shared_ptr< NaturalHistory<TSeq> > nh_def = nullptr;
    NaturalHistory<TSeq> & nh_edit();
        
public:

    #ifdef EPI_DEBUG_VIRUS
    static std::atomic<int> counter_construct;          // Default and parameterized constructors
    static std::atomic<int> counter_copy_construct;     // Copy constructor
    static std::atomic<int> counter_move_construct;     // Move constructor
    static std::atomic<int> counter_copy_assign;        // Copy assignment
    static std::atomic<int> counter_move_assign;        // Move assignment
    static std::atomic<int> counter_destruct;           // Destructor
    #endif

    Virus();

    Virus(std::string name = "unknown virus");
    
    
    #ifdef EPI_DEBUG_VIRUS
    
    // Copy and move operations for debugging
    Virus(const Virus<TSeq>& other);                    // Copy constructor
    Virus(Virus<TSeq>&& other) noexcept;                // Move constructor
    Virus<TSeq>& operator=(const Virus<TSeq>& other);   // Copy assignment
    Virus<TSeq>& operator=(Virus<TSeq>&& other) noexcept; // Move assignment

    ~Virus();
    #endif
    
    void mutate(Model<TSeq> * model);
    void set_mutation(MutFun<TSeq> fun);
    
    EPI_TYPENAME_TRAITS(TSeq, int) get_sequence();
    void set_sequence(TSeq sequence);
    
    Agent<TSeq> * get_agent();
    void set_agent(Agent<TSeq> * p);
    
    void set_date(int d);
    int get_date() const;

    void set_id(int idx);
    int get_id() const;

    /**
     * @brief Id of the virus lineage
     * 
     * The id assigned to the founding virus when it was added to the model
     * (see `Model::add_virus()`). Mutations change the virus id but keep
     * the lineage id, so all variants of a virus share it. Tools target
     * viruses by lineage (see `Tool::add_target()`).
     * 
     * @return int The lineage id (-99 if the virus has not been recorded).
     */
    int get_lineage_id() const;

    void set_name(std::string name);
    std::string get_name() const;

    bool operator==(const Virus<TSeq> & other) const;
    bool operator!=(const Virus<TSeq> & other) const {return !operator==(other);};

    void print() const;


    /**
     * @name Natural history
     * @brief The disease's stages and the transitions between them, declared
     * on the virus (see `NaturalHistory` for each method).
     * @details `Model::add_virus()` gives the model the virus's stages (its
     * states). A copy of the virus (a strain, or a variant from a mutation)
     * keeps the natural history; it can read other parameters with
     * `use_param()`. The viruses of one model share their stages.
     */
    ///@{
    int add_stage(std::string label, unsigned int flags = 0u);
    void set_contact(int s, epiworld_double weight);
    TransitionRef<TSeq> infection(Nodes from, int to, Value<TSeq> per_contact);
    TransitionRef<TSeq> rate(Nodes from, int to, Value<TSeq> daily_probability);
    TransitionRef<TSeq> after(int from, int to, Duration<TSeq> d);
    TransitionRef<TSeq> external(Nodes from, int to, std::string label);

    /// The natural history (`nullptr` if the virus has none).
    const NaturalHistory<TSeq> * get_natural_history() const { return nh_def.get(); }
    ///@}

    /**
     * @brief This variant reads the model parameter `replacement` wherever a
     * transition of the natural history reads `param`.
     * @details Every parameter lives in the model; a variant changes a value
     * by pointing to another parameter (e.g., `"Recovery rate (Delta)"`).
     */
    void use_param(std::string param, std::string replacement);

    /// The parameter this variant reads in place of `id`.
    ParamId param_redirect(ParamId id, const Model<TSeq> & model) const;

    /// Throws if a parameter it reads (see `use_param()`) is not in `model`.
    void check_params(const Model<TSeq> & model) const;

    virtual ~Virus() = default;
    virtual std::unique_ptr<Virus<TSeq>> clone_ptr() const;

};

#endif