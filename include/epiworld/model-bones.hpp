#ifndef EPIWORLD_MODEL_HPP
#define EPIWORLD_MODEL_HPP

template<typename TSeq>
class Person;

template<typename TSeq>
class Virus;

template<typename TSeq>
class Tool;

class AdjList;

template<typename TSeq>
class DataBase;

template<typename TSeq>
class Queue;

template<typename TSeq = bool>
class Model {
    friend class Person<TSeq>;
    friend class DataBase<TSeq>;
    friend class Queue<TSeq>;
private:

    DataBase<TSeq> db;

    std::vector< Person<TSeq> > population;
    std::map< int,int >         population_ids;
    bool directed;
    
    std::vector< Virus<TSeq> > viruses;
    std::vector< epiworld_double > prevalence_virus; ///< Initial prevalence_virus of each virus
    std::vector< bool > prevalence_virus_as_proportion;
    
    std::vector< Tool<TSeq> > tools;
    std::vector< epiworld_double > prevalence_tool;
    std::vector< bool > prevalence_tool_as_proportion;

    std::shared_ptr< std::mt19937 > engine =
        std::make_shared< std::mt19937 >();
    
    std::shared_ptr< std::uniform_real_distribution<> > runifd =
        std::make_shared< std::uniform_real_distribution<> >(0.0, 1.0);

    std::shared_ptr< std::normal_distribution<> > rnormd =
        std::make_shared< std::normal_distribution<> >(0.0);

    std::shared_ptr< std::gamma_distribution<> > rgammad = 
        std::make_shared< std::gamma_distribution<> >();

    std::function<void(std::vector<Person<TSeq>>*,Model<TSeq>*,epiworld_double)> rewire_fun;
    epiworld_double rewire_prop;
        
    std::map<std::string, epiworld_double > parameters;
    unsigned int ndays;
    Progress pb;

    std::vector< epiworld_fast_uint > status_susceptible = {STATUS::SUSCEPTIBLE};
    std::vector< std::string > status_susceptible_labels = {"susceptible"};

    std::vector< epiworld_fast_uint > status_exposed = {STATUS::EXPOSED};
    std::vector< std::string > status_exposed_labels = {"exposed"};

    std::vector< epiworld_fast_uint > status_removed = {STATUS::REMOVED};
    std::vector< std::string > status_removed_labels = {"removed"};

    epiworld_fast_uint nstatus = 3u;
    epiworld_fast_uint baseline_status_susceptible = STATUS::SUSCEPTIBLE;
    epiworld_fast_uint baseline_status_exposed     = STATUS::EXPOSED;
    epiworld_fast_uint baseline_status_removed     = STATUS::REMOVED;
    
    bool verbose     = true;
    bool initialized = false;
    int current_date = 0;

    void dist_tools();
    void dist_virus();

    std::chrono::time_point<std::chrono::steady_clock> time_start;
    std::chrono::time_point<std::chrono::steady_clock> time_end;

    // std::chrono::milliseconds
    std::chrono::duration<epiworld_double,std::micro> time_elapsed = 
        std::chrono::duration<epiworld_double,std::micro>::zero();
    unsigned int n_replicates = 0u;
    void chrono_start();
    void chrono_end();

    std::unique_ptr< Model<TSeq> > backup = nullptr;

    UpdateFun<TSeq> update_susceptible = nullptr;
    UpdateFun<TSeq> update_exposed     = nullptr;
    UpdateFun<TSeq> update_removed     = nullptr;

    std::vector<std::function<void(Model<TSeq>*)>> global_action_functions;
    std::vector< int > global_action_dates;

    Queue<TSeq> queue;
    bool visited_model = EPI_DEFAULT_VISITED;
    bool use_queuing   = true;

    /**
     * @brief Variables used to keep track of the actions
     * to be made regarding viruses.
     */
    ///@{
    std::vector< Virus<TSeq> * >  virus_to_remove;
    std::vector< Virus<TSeq> * >  virus_to_add;
    std::vector< Person<TSeq> * > virus_to_add_person;
    ///@}

public:

    std::vector<epiworld_double> array_double_tmp;
    std::vector<Virus<TSeq> *> array_virus_tmp;

    Model() {};
    Model(const Model<TSeq> & m);
    Model(Model<TSeq> && m);
    Model<TSeq> & operator=(const Model<TSeq> & m);

    void clone_population(
        std::vector< Person<TSeq> > & p,
        std::map<int,int> & p_ids,
        bool & d,
        Model<TSeq> * m = nullptr
    ) const ;

    void clone_population(
        const Model<TSeq> & m
    );

    /**
     * @brief Set the backup object
     * @details `backup` can be used to restore the entire object
     * after a run. This can be useful if the user wishes to have
     * individuals start with the same network from the beginning.
     * 
     */
    ///@[
    void set_backup();
    void restore_backup();
    ///@]

    DataBase<TSeq> & get_db();
    epiworld_double & operator()(std::string pname);

    size_t size() const;

    /**
     * @brief Random number generation
     * 
     * @param eng 
     */
    ///@[
    void set_rand_engine(std::mt19937 & eng);
    std::mt19937 * get_rand_endgine();
    void seed(unsigned int s);
    void set_rand_gamma(epiworld_double alpha, epiworld_double beta);
    epiworld_double runif();
    epiworld_double rnorm();
    epiworld_double rnorm(epiworld_double mean, epiworld_double sd);
    epiworld_double rgamma();
    epiworld_double rgamma(epiworld_double alpha, epiworld_double beta);
    ///@]

    void add_virus(Virus<TSeq> v, epiworld_double preval);
    void add_virus_n(Virus<TSeq> v, unsigned int preval);
    void add_tool(Tool<TSeq> t, epiworld_double preval);
    void add_tool_n(Tool<TSeq> t, unsigned int preval);

    /**
     * @brief Accessing population of the model
     * 
     * @param fn std::string Filename of the edgelist file.
     * @param skip int Number of lines to skip in `fn`.
     * @param directed bool Whether the graph is directed or not.
     * @param min_id int Minimum id number (if negative, the program will
     * try to guess from the data.)
     * @param max_id int Maximum id number (if negative, the program will
     * try to guess from the data.)
     * @param al AdjList to read into the model.
     */
    ///@[
    void pop_from_adjlist(
        std::string fn,
        int skip = 0,
        bool directed = false,
        int min_id = -1,
        int max_id = -1
        );
    void pop_from_adjlist(AdjList al);
    bool is_directed() const;
    std::vector< Person<TSeq> > * get_population();
    void pop_from_random(
        unsigned int n = 1000,
        unsigned int k = 5,
        bool d = false,
        epiworld_double p = .01
        );
    ///@]

    /**
     * @brief Functions to run the model
     * 
     * @param seed Seed to be used for Pseudo-RNG.
     * @param ndays Number of days (steps) of the simulation.
     * @param fun In the case of `run_multiple`, a function that is called
     * after each experiment.
     * 
     */
    ///@[
    void init(unsigned int ndays, unsigned int seed);
    void update_status();
    void mutate_variant();
    void next();
    void run();
    void run_multiple(
        unsigned int nexperiments,
        std::function<void(Model<TSeq>*)> fun,
        bool reset,
        bool verbose
        );
    ///@]

    void record_variant(Virus<TSeq> * v);

    int get_nvariants() const;
    unsigned int get_ndays() const;
    unsigned int get_n_replicates() const;
    void set_ndays(unsigned int ndays);
    bool get_verbose() const;
    void verbose_off();
    void verbose_on();
    int today() const;

    /**
     * @brief Rewire the network preserving the degree sequence.
     *
     * @details This implementation assumes an undirected network,
     * thus if {(i,j), (k,l)} -> {(i,l), (k,j)}, the reciprocal
     * is also true, i.e., {(j,i), (l,k)} -> {(j,k), (l,i)}.
     * 
     * @param proportion Proportion of ties to be rewired.
     * 
     * @result A rewired version of the network.
     */
    ///@[
    void set_rewire_fun(std::function<void(std::vector<Person<TSeq>>*,Model<TSeq>*,epiworld_double)> fun);
    void set_rewire_prop(epiworld_double prop);
    epiworld_double get_rewire_prop() const;
    void rewire();
    ///@]

    inline void set_update_susceptible(UpdateFun<TSeq> fun);
    inline void set_update_exposed(UpdateFun<TSeq> fun);
    inline void set_update_removed(UpdateFun<TSeq> fun);
    /**
     * @brief Wrapper of `DataBase::write_data`
     * 
     * @param fn_variant_info Filename. Information about the variant.
     * @param fn_variant_hist Filename. History of the variant.
     * @param fn_total_hist   Filename. Aggregated history (status)
     * @param fn_transmission Filename. Transmission history.
     * @param fn_transition   Filename. Markov transition history.
     */
    void write_data(
        std::string fn_variant_info,
        std::string fn_variant_hist,
        std::string fn_total_hist,
        std::string fn_transmission,
        std::string fn_transition
        ) const;

    /**
     * @brief Export the network data in edgelist form
     * 
     * @param fn std::string. File name.
     * @param source Integer vector
     * @param target Integer vector
     * 
     * @details When passing the source and target, the function will
     * write the edgelist on those.
     */
    ///[@
    void write_edgelist(
        std::string fn
        ) const;

    void write_edgelist(
        std::vector< unsigned int > & source,
        std::vector< unsigned int > & target
        ) const;
    ///@]

    std::map<std::string, epiworld_double> & params();

    /**
     * @brief Reset the model
     * 
     * @details Resetting the model will:
     * - clear the database
     * - restore the population (if `set_backup()` was called before)
     * - re-distribute tools
     * - re-distribute viruses
     * - set the date to 0
     * 
     */
    void reset();
    void print() const;

    Model<TSeq> && clone() const;

    /**
     * @brief Adds extra statuses to the model
     * 
     * @details
     * Adding values of `s` that are already present in the model will
     * result in an error.
     * 
     * The functions `get_status_*` return the current values for the 
     * statuses included in the model.
     * 
     * @param s `unsigned int` Code of the status
     * @param lab `std::string` Name of the status.
     * 
     * @return `add_status*` returns nothing.
     * @return `get_status_*` returns a vector of pairs with the 
     * statuses and their labels.
     */
    ///@[
    void add_status_susceptible(epiworld_fast_uint s, std::string lab);
    void add_status_exposed(epiworld_fast_uint s, std::string lab);
    void add_status_removed(epiworld_fast_uint s, std::string lab);
    void add_status_susceptible(std::string lab);
    void add_status_exposed(std::string lab);
    void add_status_removed(std::string lab);
    const std::vector< epiworld_fast_uint > & get_status_susceptible() const;
    const std::vector< epiworld_fast_uint > & get_status_exposed() const;
    const std::vector< epiworld_fast_uint > & get_status_removed() const;
    const std::vector< std::string > & get_status_susceptible_labels() const;
    const std::vector< std::string > & get_status_exposed_labels() const;
    const std::vector< std::string > & get_status_removed_labels() const;
    void print_status_codes() const;
    epiworld_fast_uint get_default_susceptible() const;
    epiworld_fast_uint get_default_exposed() const;
    epiworld_fast_uint get_default_removed() const;
    ///@]

    /**
     * @brief Reset all the status codes of the model
     * 
     * @details 
     * The default values are those specified in the enum STATUS.
     * 
     * @param codes In the following order: Susceptible, Infected, Removed
     * @param names Names matching the codes
     * @param verbose When `true`, it will print the new mappings.
     */
    void reset_status_codes(
        std::vector< epiworld_fast_uint > codes,
        std::vector< std::string > names,
        bool verbose = true
    );

    /**
     * @brief Setting and accessing parameters from the model
     * 
     * @details Tools can incorporate parameters included in the model.
     * Internally, parameters in the tool are stored as pointers to
     * an std::map<> of parameters in the model. Using the `unsigned int`
     * method directly fetches the parameters in the order these were
     * added to the tool. Accessing parameters via the `std::string` method
     * involves searching the parameter directly in the std::map<> member
     * of the model (so it is not recommended.)
     * 
     * The function `set_param()` can be used when the parameter already
     * exists in the model.
     * 
     * The `par()` function members are aliases for `get_param()`.
     * 
     * @param initial_val 
     * @param pname Name of the parameter to add or to fetch
     * @return The current value of the parameter
     * in the model.
     * 
     */
    ///@[
    epiworld_double add_param(epiworld_double initial_val, std::string pname);
    epiworld_double set_param(std::string pname);
    epiworld_double get_param(unsigned int k);
    epiworld_double get_param(std::string pname);
    epiworld_double par(unsigned int k);
    epiworld_double par(std::string pname);
    epiworld_double 
        *p0,*p1,*p2,*p3,*p4,*p5,*p6,*p7,*p8,*p9,
        *p10,*p11,*p12,*p13,*p14,*p15,*p16,*p17,*p18,*p19,
        *p20,*p21,*p22,*p23,*p24,*p25,*p26,*p27,*p28,*p29,
        *p30,*p31,*p32,*p33,*p34,*p35,*p36,*p37,*p38,*p39;
    unsigned int npar_used = 0u;
    ///@]

    void get_elapsed(
        std::string unit = "auto",
        epiworld_double * last_elapsed = nullptr,
        epiworld_double * total_elapsed = nullptr,
        std::string * unit_abbr = nullptr,
        bool print = true
    ) const;

    /**
     * @brief Set the user data object
     * 
     * @param names 
     */
    ///[@
    void set_user_data(std::vector< std::string > names);
    void add_user_data(unsigned int j, epiworld_double x);
    void add_user_data(std::vector< epiworld_double > x);
    UserData<TSeq> & get_user_data();
    ///@]

    void add_global_action(
        std::function<void(Model<TSeq>*)> fun,
        int date
        );

    void run_global_actions();

    void clear_status_set();

    void toggle_visited();

    void queuing_on();
    void queuing_off();
    bool is_queuing_on() const;
    Queue<TSeq> & get_queue();

};

#endif