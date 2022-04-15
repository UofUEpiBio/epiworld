#ifndef EPIWORLD_DATABASE_BONES_HPP
#define EPIWORLD_DATABASE_BONES_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Virus;

template<typename TSeq>
class UserData;

/**
 * @brief Statistical data about the process
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class DataBase {
    friend class Model<TSeq>;
private:
    Model<TSeq> * model;

    // Variants information 
    MapVec_type<int,int> variant_id; ///< The squence is the key
    std::vector< TSeq> sequence;
    std::vector< int > origin_date;
    std::vector< int > parent_id;

    std::function<std::vector<int>(const TSeq&)> seq_hasher = default_seq_hasher<TSeq>;
    std::function<std::string(const TSeq &)> seq_writer = default_seq_writer<TSeq>;

    // {Variant 1: {Status 1, Status 2, etc.}, Variant 2: {...}, ...}
    std::vector< std::vector<int> > today_variant;
    
    // {Variant 1: {Status 1, Status 2, etc.}, Variant 2: {...}, ...} to update
    std::vector< std::vector<int> > today_variant_next;

    // {Susceptible, Infected, etc.}
    std::vector< int > today_total;

    // {Susceptible, Infected, etc.} to update
    std::vector< int > today_total_next;

    // Totals
    int today_total_nvariants_active = 0;
    
    int sampling_freq;

    // Variants history
    std::vector< int > hist_variant_date;
    std::vector< int > hist_variant_id;
    std::vector< epiworld_fast_uint > hist_variant_status;
    std::vector< int > hist_variant_counts;

    // Overall hist
    std::vector< int > hist_total_date;
    std::vector< int > hist_total_nvariants_active;
    std::vector< epiworld_fast_uint > hist_total_status;
    std::vector< int > hist_total_counts;
    std::vector< int > hist_transition_matrix;

    // Transmission network
    std::vector< int > transmission_date;
    std::vector< int > transmission_source;
    std::vector< int > transmission_target;
    std::vector< int > transmission_variant;

    std::vector< int > transition_matrix;
    std::vector< int > transition_matrix_next;

    UserData<TSeq> user_data;

public:

    DataBase(int freq = 1) : sampling_freq(freq) {};

    /**
     * @brief Registering a new variant
     * 
     * @param v Pointer to the new variant.
     * Since variants are originated in the host, the numbers simply move around.
     * From the parent variant to the new variant. And the total number of infected
     * does not change.
     */
    void record_variant(Virus<TSeq> * v); 
    void set_seq_hasher(std::function<std::vector<int>(TSeq)> fun);
    void set_model(Model<TSeq> & m);
    Model<TSeq> * get_model();
    void record();

    const std::vector< TSeq > & get_sequence() const;
    const std::vector< int > & get_nexposed() const;
    size_t size() const;

    void up_exposed(
        Virus<TSeq> * v,
        epiworld_fast_uint new_status
        );

    void down_exposed(
        Virus<TSeq> * v,
        epiworld_fast_uint prev_status
        );

    void state_change(
        epiworld_fast_uint prev_status,
        epiworld_fast_uint new_status
    );

    void record_transition(epiworld_fast_uint from, epiworld_fast_uint to);

    /**
     * @brief Get recorded information from the model
     * 
     * @param what std::string, The status, e.g., 0, 1, 2, ...
     * @return In `get_today_total`, the current counts of `what`.
     * @return In `get_today_variant`, the current counts of `what` for
     * each variant.
     * @return In `get_hist_total`, the time series of `what`
     * @return In `get_hist_variant`, the time series of what for each variant.
     * @return In `get_hist_total_date` and `get_hist_variant_date` the
     * corresponding dates
     */
    ///@[
    int get_today_total(std::string what) const;
    int get_today_total(epiworld_fast_uint what) const;
    void get_today_total(
        std::vector< std::string > * status = nullptr,
        std::vector< int > * counts = nullptr
    ) const;

    void get_today_variant(
        std::vector< std::string > & status,
        std::vector< int > & id,
        std::vector< int > & counts
    ) const;

    void get_hist_total(
        std::vector< int > * date,
        std::vector< std::string > * status,
        std::vector< int > * counts
    ) const;

    void get_hist_variant(
        std::vector< int > & date,
        std::vector< int > & id,
        std::vector< std::string > & status,
        std::vector< int > & counts
    ) const;
    ///@]

    void write_data(
        std::string fn_variant_info,
        std::string fn_variant_hist,
        std::string fn_total_hist,
        std::string fn_transmission,
        std::string fn_transition
        ) const;
    
    void record_transmission(int i, int j, int variant);

    size_t get_nvariants() const;

    void reset();

    
    void set_user_data(std::vector< std::string > names);
    void add_user_data(std::vector< epiworld_double > x);
    void add_user_data(unsigned int j, epiworld_double x);
    UserData<TSeq> & get_user_data();


};


#endif