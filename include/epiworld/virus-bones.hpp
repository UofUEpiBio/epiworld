#ifndef EPIWORLD_VIRUS_HPP
#define EPIWORLD_VIRUS_HPP

template<typename TSeq>
class Person;

template<typename TSeq>
class Virus;

template<typename TSeq>
class PersonViruses;

template<typename TSeq>
class Model;



/**
 * @brief Virus
 * 
 * @tparam TSeq 
 * @details
 * Raw transmisibility of a virus should be a function of its genetic
 * sequence. Nonetheless, transmisibility can be reduced as a result of
 * having one or more tools to fight the virus. Because of this, transmisibility
 * should be a function of the host.
 */
template<typename TSeq = bool>
class Virus {
    friend class Person<TSeq>;
    friend class Model<TSeq>;
    friend class PersonViruses<TSeq>;
    friend class DataBase<TSeq>;
private:
    Person<TSeq> * host = nullptr;
    std::shared_ptr<TSeq> baseline_sequence = std::make_shared<TSeq>(default_sequence<TSeq>());
    std::shared_ptr<std::string> virus_name = nullptr;
    int date = -99;
    int id   = -99;
    bool active = true;
    MutFun<TSeq>          mutation_fun                 = nullptr;
    PostRecoveryFun<TSeq> post_recovery_fun            = nullptr;
    VirusFun<TSeq>        probability_of_infecting_fun = nullptr;
    VirusFun<TSeq>        probability_of_recovery_fun  = nullptr;
    VirusFun<TSeq>        probability_of_death_fun     = nullptr;

    // Setup parameters
    std::vector< epiworld_double * > params;
    std::vector< epiworld_double > data;

    epiworld_fast_int status_init    = -99; ///< Change of status when added to host.
    epiworld_fast_int status_post    = -99; ///< Change of status when removed from host.
    epiworld_fast_int status_removed = -99; ///< Change of status when host is removed

    epiworld_fast_int queue_init    = 1; ///< Change of status when added to host.
    epiworld_fast_int queue_post    = -1; ///< Change of status when removed from host.
    epiworld_fast_int queue_removed = -99; ///< Change of status when host is removed

public:
    Virus(std::string name = "unknown virus");

    void mutate();
    void set_mutation(MutFun<TSeq> fun);
    
    const TSeq* get_sequence();
    void set_sequence(TSeq sequence);
    
    Person<TSeq> * get_host();
    Model<TSeq> * get_model();
    
    void set_date(int d);
    int get_date() const;

    void set_id(int idx);
    int get_id() const;

    /**
     * @name Get and set the tool functions
     * 
     * @param v The virus over which to operate
     * @param fun the function to be used
     * 
     * @return epiworld_double 
     */
    ///@{
    epiworld_double get_prob_infecting();
    epiworld_double get_prob_recovery();
    epiworld_double get_prob_death();
    
    void post_recovery();
    void set_post_recovery(PostRecoveryFun<TSeq> fun);
    void set_post_immunity(epiworld_double prob);
    void set_post_immunity(epiworld_double * prob);

    void set_prob_infecting_fun(VirusFun<TSeq> fun);
    void set_prob_recovery_fun(VirusFun<TSeq> fun);
    void set_prob_death_fun(VirusFun<TSeq> fun);
    
    void set_prob_infecting(epiworld_double * prob);
    void set_prob_recovery(epiworld_double * prob);
    void set_prob_death(epiworld_double * prob);
    
    void set_prob_infecting(epiworld_double prob);
    void set_prob_recovery(epiworld_double prob);
    void set_prob_death(epiworld_double prob);
    ///@}


    void set_name(std::string name);
    std::string get_name() const;

    std::vector< epiworld_double > & get_data();

    /**
     * @name Get and set the status and queue
     * 
     * After applied, viruses can change the status and affect
     * the queue of agents. These function sets the default values,
     * which are retrieved when adding or removing a virus does not
     * specify a change in status or in queue.
     * 
     * @param init After the virus/tool is added to the host.
     * @param end After the virus/tool is removed.
     * @param removed After the host (Person) is removed.
     */
    ///@{
    void set_status(
        epiworld_fast_int init,
        epiworld_fast_int end,
        epiworld_fast_int removed
        );
        
    void set_queue(
        epiworld_fast_int init,
        epiworld_fast_int end,
        epiworld_fast_int removed
        );

    void get_status(
        epiworld_fast_int * init,
        epiworld_fast_int * end,
        epiworld_fast_int * removed
        );

    void get_queue(
        epiworld_fast_int * init,
        epiworld_fast_int * end,
        epiworld_fast_int * removed
        );
    ///@}

};

#endif