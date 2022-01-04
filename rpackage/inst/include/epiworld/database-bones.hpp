#ifndef EPIWORLD_DATABASE_BONES_HPP
#define EPIWORLD_DATABASE_BONES_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Virus;

/**
 * @brief Hasher function to turn the sequence into an integer vector
 * 
 * @tparam TSeq 
 * @param x 
 * @return std::vector<int> 
 */
template<typename TSeq>
inline std::vector<int> default_seq_hasher(const TSeq & x);

template<>
inline std::vector<int> default_seq_hasher<std::vector<int>>(const std::vector<int> & x) {
    return x;
}

template<>
inline std::vector<int> default_seq_hasher<std::vector<bool>>(const std::vector<bool> & x) {
    std::vector<int> ans;
    for (const auto & i : x)
        ans.push_back(i? 1 : 0);
    return ans;
}

template<>
inline std::vector<int> default_seq_hasher<int>(const int & x) {
    return {x};
}

template<>
inline std::vector<int> default_seq_hasher<bool>(const bool & x) {
    return {x ? 1 : 0};
}

/**
 * @brief Default way to write sequences
 * 
 * @tparam TSeq 
 * @param seq 
 * @return std::string 
 */
template<typename TSeq>
inline std::string default_seq_writer(const TSeq & seq);

template<>
inline std::string default_seq_writer<std::vector<int>>(
    const std::vector<int> & seq
) {

    std::string out = "";
    for (const auto & s : seq)
        out = out + std::to_string(s);

    return out;

}

template<>
inline std::string default_seq_writer<std::vector<bool>>(
    const std::vector<bool> & seq
) {

    std::string out = "";
    for (const auto & s : seq)
        out = out + (s ? "1" : "0");

    return out;

}

template<>
inline std::string default_seq_writer<bool>(
    const bool & seq
) {

    return seq ? "1" : "0";

}

template<>
inline std::string default_seq_writer<int>(
    const int & seq
) {

    return std::to_string(seq);

}

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

    // {Healthy, Infected, etc.}
    std::vector< int > today_total;

    // Totals
    int today_total_nvariants_active = 0;
    
    int sampling_freq;

    // Variants history
    std::vector< int > hist_variant_date;
    std::vector< int > hist_variant_id;
    std::vector< int > hist_variant_status;
    std::vector< int > hist_variant_counts;

    // Overall hist
    std::vector< int > hist_total_date;
    std::vector< int > hist_total_nvariants_active;
    std::vector< int > hist_total_status;
    std::vector< int > hist_total_counts;

    // Transmission network
    std::vector< int > transmision_date;
    std::vector< int > transmision_source;
    std::vector< int > transmision_target;
    std::vector< int > transmision_variant;

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
    const std::vector< int > & get_ninfected() const;
    size_t size() const;

    void up_infected(
        Virus<TSeq> * v,
        unsigned int prev_status,
        unsigned int new_status
        );

    void down_infected(
        Virus<TSeq> * v,
        unsigned int prev_status,
        unsigned int new_status
        );

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
    void get_today_total(
        std::vector< std::string > & status,
        std::vector< int > & counts
    ) const;

    void get_today_variant(
        std::vector< std::string > & status,
        std::vector< int > & id,
        std::vector< int > & counts
    ) const;

    void get_hist_total(
        std::vector< int > & date,
        std::vector< std::string > & status,
        std::vector< int > & counts
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
        std::string fn_transmision
        ) const;
    
    void record_transmision(int i, int j, int variant);

    size_t get_nvariants() const;

    void reset();

};


#endif