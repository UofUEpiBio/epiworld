
#ifndef EPIWORLD_TOOL_BONES_HPP
#define EPIWORLD_TOOL_BONES_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Person;

template<typename TSeq>
class Model;

template<typename TSeq>
class Tool;

/**
 * @brief Tools for defending the host against the virus
 * 
 * @tparam TSeq Type of sequence
 */
template<typename TSeq = int> 
class Tool {
    friend class Person<TSeq>;
    friend class Model<TSeq>;
    friend void default_add_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_add_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
private:

    Person<TSeq> * person = nullptr;
    int person_idx        = -99;

    int date = -99;
    int id   = -99;
    std::shared_ptr<std::string> tool_name = nullptr;
    std::shared_ptr<TSeq> sequence = std::make_shared<TSeq>(default_sequence<TSeq>());
    TSeq sequence_unique  = default_sequence<TSeq>();
    ToolFun<TSeq> susceptibility_reduction_fun = nullptr;
    ToolFun<TSeq> transmission_reduction_fun   = nullptr;
    ToolFun<TSeq> recovery_enhancer_fun        = nullptr;
    ToolFun<TSeq> death_reduction_fun          = nullptr;

    // Setup parameters
    std::vector< epiworld_double * > params;  

    epiworld_fast_int status_init = -99;
    epiworld_fast_int status_post = -99;

    epiworld_fast_int queue_init = 0; ///< Change of status when added to host.
    epiworld_fast_int queue_post = 0; ///< Change of status when removed from host.

    void set_person(Person<TSeq> * p, size_t idx);

public:
    Tool(std::string name = "unknown tool");
    // Tool(TSeq d, std::string name = "unknown tool");

    void set_sequence(TSeq d);
    void set_sequence_unique(TSeq d);
    void set_sequence(std::shared_ptr<TSeq> d);
    std::shared_ptr<TSeq> get_sequence();
    TSeq & get_sequence_unique();

    /**
     * @name Get and set the tool functions
     * 
     * @param v The virus over which to operate
     * @param fun the function to be used
     * 
     * @return epiworld_double 
     */
    ///@{
    epiworld_double get_susceptibility_reduction(VirusPtr<TSeq> v);
    epiworld_double get_transmission_reduction(VirusPtr<TSeq> v);
    epiworld_double get_recovery_enhancer(VirusPtr<TSeq> v);
    epiworld_double get_death_reduction(VirusPtr<TSeq> v);
    
    void set_susceptibility_reduction_fun(ToolFun<TSeq> fun);
    void set_transmission_reduction_fun(ToolFun<TSeq> fun);
    void set_recovery_enhancer_fun(ToolFun<TSeq> fun);
    void set_death_reduction_fun(ToolFun<TSeq> fun);

    void set_susceptibility_reduction(epiworld_double * prob);
    void set_transmission_reduction(epiworld_double * prob);
    void set_recovery_enhancer(epiworld_double * prob);
    void set_death_reduction(epiworld_double * prob);

    void set_susceptibility_reduction(epiworld_double prob);
    void set_transmission_reduction(epiworld_double prob);
    void set_recovery_enhancer(epiworld_double prob);
    void set_death_reduction(epiworld_double prob);
    ///@}

    void set_name(std::string name);
    std::string get_name() const;

    Person<TSeq> * get_person();
    int get_id() const;
    void set_id(int id);
    void set_date(int d);

    void set_status(epiworld_fast_int init, epiworld_fast_int post);
    void set_queue(epiworld_fast_int init, epiworld_fast_int post);
    void get_status(epiworld_fast_int * init, epiworld_fast_int * post);
    void get_queue(epiworld_fast_int * init, epiworld_fast_int * post);

};

#endif