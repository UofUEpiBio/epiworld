#ifndef EPIWORLD_DATABASE_HPP
#define EPIWORLD_DATABASE_HPP

#include <unordered_map>
#include <functional>
#include <vector>
#include "misc.hpp"


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
 * @brief Statistical data about the process
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class DataBase {
private:
    Model<TSeq> * model;

    // Variants information 
    MapVec_type<int,int> variant_id; ///< The squence is the key
    std::vector< TSeq> sequence;
    std::vector< int > origin_date;
    std::vector< int > parent_id;

    std::function<std::vector<int>(const TSeq&)> seq_hasher = default_seq_hasher<TSeq>;

    // Running sum of the variant's information
    std::vector< int > today_variant_ninfected;  ///< Running sum
    std::vector< int > today_variant_nrecovered; ///< Running sum
    std::vector< int > today_variant_ndeceased;  ///< Running sum

    // Totals
    int today_total_nvariants_active = 0;
    int today_total_nhealthy   = 0;
    int today_total_nrecovered = 0;
    int today_total_ninfected  = 0;
    int today_total_ndeceased  = 0;
    
    int sampling_freq;

    // Variants history
    std::vector< int > hist_variant_date;
    std::vector< int > hist_variant_id;
    std::vector< int > hist_variant_ninfected;
    std::vector< int > hist_variant_nrecovered;
    std::vector< int > hist_variant_ndeceased;

    // Overall hist
    std::vector< int > hist_total_date;
    std::vector< int > hist_total_nvariants_active;
    std::vector< int > hist_total_nhealthy;
    std::vector< int > hist_total_nrecovered;
    std::vector< int > hist_total_ninfected;
    std::vector< int > hist_total_ndeceased;  


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
    void register_variant(Virus<TSeq> * v); 
    void set_seq_hasher(std::function<std::vector<int>(TSeq)> fun);
    void set_model(Model<TSeq> & m);
    void record();

    const std::vector< TSeq > & get_sequence() const;
    const std::vector< int > & get_ninfected() const;
    size_t size() const;

    void up_infected(Virus<TSeq> * v);
    void up_recovered(Virus<TSeq> * v);
    void up_deceased(Virus<TSeq> * v);

    int get_today_total(std::string what) const;
    const std::vector< int > & get_today_variant(std::string what) const;
    const std::vector< int > & get_hist_total(std::string what) const;
    const std::vector< int > & get_hist_variant(std::string what) const;

    void write_data(
        std::string fn_variant,
        std::string fn_total
        ) const;
    
};

template<typename TSeq>
inline void DataBase<TSeq>::set_model(Model<TSeq> & m)
{
    model = &m;
    today_total_nhealthy = model->get_persons()->size();
    return;
}

template<typename TSeq>
inline const std::vector< TSeq > & DataBase<TSeq>::get_sequence() const {
    return sequence;
}

template<typename TSeq>
inline void DataBase<TSeq>::record() 
{
    // Only store every now and then
    if ((model->today() % sampling_freq) == 0)
    {

        // Recording variant's history
        
        for (auto & p : variant_id)
        {
            hist_variant_date.push_back(model->today());
            hist_variant_id.push_back(p.second);
            hist_variant_ninfected.push_back(today_variant_ninfected[p.second]);
            hist_variant_nrecovered.push_back(today_variant_nrecovered[p.second]);
            hist_variant_ndeceased.push_back(today_variant_ndeceased[p.second]);

        }

        // Recording the overall history
        hist_total_date.push_back(model->today());
        hist_total_nvariants_active.push_back(today_total_nvariants_active);
        hist_total_nhealthy.push_back(today_total_nhealthy);
        hist_total_nrecovered.push_back(today_total_nrecovered);
        hist_total_ninfected.push_back(today_total_ninfected);
        hist_total_ndeceased.push_back(today_total_ndeceased);

    }
}

template<typename TSeq>
inline void DataBase<TSeq>::register_variant(Virus<TSeq> * v) {

    // Updating registry
    std::vector< int > hash = seq_hasher(*v->get_sequence());
    if (variant_id.find(hash) == variant_id.end())
    {
        variant_id[hash] = variant_id.size();
        sequence.push_back(*v->get_sequence());
        origin_date.push_back(model->today());

        // Collecting old ID to make the accounting
        int tmp_id = v->get_id();

        parent_id.push_back(tmp_id);
        
        // Moving statistics
        if (variant_id.size() > 1)
            today_variant_ninfected[tmp_id]--;
            
        today_variant_ninfected.push_back(1);
        today_variant_nrecovered.push_back(0);
        today_variant_ndeceased.push_back(0);
        
        // Updating the variant
        v->set_id(variant_id.size() - 1);
        v->set_date(model->today());

        today_total_nvariants_active++;

    } else {

        int new_id = variant_id[hash];

        // Accounting (from the old to the new)
        today_variant_ninfected[v->get_id()]--;
        today_variant_ninfected[new_id]++;

        // Reflecting the change
        v->set_id(new_id);
        v->set_date(origin_date[new_id]);

    }    

    // today_total_nhealthy--;
    // today_total_ninfected++;

    
    return;
} 

template<typename TSeq>
inline int DataBase<TSeq>::get_today_total(std::string what) const
{

    if (what == "nvariants_active")
        return today_total_nvariants_active;
    else if (what == "nhealthty")
        return today_total_nhealthy;
    else if (what == "nrecovered")
        return today_total_nrecovered;
    else if (what == "ninfected")
        return today_total_ninfected;
    else if (what == "ndeceased")
        return today_total_ndeceased;
    else 
        throw std::logic_error("\""+ what + "\" is not included in the database.");   

}

template<typename TSeq>
inline const std::vector<int> & DataBase<TSeq>::get_today_variant(std::string what) const
{
    
    if (what == "nrecovered")
        return today_variant_nrecovered;
    else if (what == "ninfected")
        return today_variant_ninfected;
    else if (what == "ndeceased")
        return today_variant_ndeceased;
    else 
        throw std::logic_error("\""+ what + "\" is not included in the database.");   

}

template<typename TSeq>
inline const std::vector<int> & DataBase<TSeq>::get_hist_total(std::string what) const
{
    if (what == "date")
        return hist_total_date;
    else if (what == "nvariants_active")
        return hist_total_nvariants_active;
    else if (what == "nhealthty")
        return hist_total_nhealthy;
    else if (what == "nrecovered")
        return hist_total_nrecovered;
    else if (what == "ninfected")
        return hist_total_ninfected;
    else if (what == "ndeceased")
        return hist_total_ndeceased;
    else 
        throw std::logic_error("\""+ what + "\" is not included in the database.");   

}

template<typename TSeq>
inline const std::vector<int> & DataBase<TSeq>::get_hist_variant(std::string what) const
{
    
    if (what == "date")
        return hist_variant_date;
    else if (what == "id")
        return hist_variant_id;
    else if (what == "nrecovered")
        return hist_variant_nrecovered;
    else if (what == "ninfected")
        return hist_variant_ninfected;
    else if (what == "ndeceased")
        return hist_variant_ndeceased;
    else 
        throw std::logic_error("\""+ what + "\" is not included in the database.");   

}

template<typename TSeq>
inline size_t DataBase<TSeq>::size() const
{
    return variant_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::up_infected(Virus<TSeq> * v) {

    today_variant_ninfected[v->get_id()]++;
    today_total_nhealthy--;
    today_total_ninfected++;

}

template<typename TSeq>
inline void DataBase<TSeq>::up_recovered(Virus<TSeq> * v) {

    int tmp_id = v->get_id();
    today_variant_ninfected[tmp_id]--;
    today_variant_nrecovered[tmp_id]++;
    today_total_ninfected--;
    today_total_nhealthy++;
    today_total_nrecovered++;

}

template<typename TSeq>
inline void DataBase<TSeq>::up_deceased(Virus<TSeq> * v) {

    int tmp_id = v->get_id();
    today_variant_ninfected[tmp_id]--;
    today_variant_ndeceased[tmp_id]++;
    
    today_total_ninfected--;
    today_total_ndeceased++;

}

template<typename TSeq>
inline void DataBase<TSeq>::write_data(
    std::string fn_variant,
    std::string fn_total
) const {

    std::ofstream file_variant(fn_variant, std::ios_base::out);
    std::ofstream file_total(fn_total, std::ios_base::out);

    file_variant <<
        "date " << "id " << "ninfected " << "nrecovered " << "ndeceased\n";

    for (int i = 0; i < hist_variant_id.size(); ++i)
        file_variant <<
            hist_variant_date[i] << " " <<
            hist_variant_id[i] << " " <<
            hist_variant_ninfected[i] << " " <<
            hist_variant_nrecovered[i] << " " <<
            hist_variant_ndeceased[i] << "\n";

    file_total <<
        "date " << "nvariants " << "nhealthy " << "ninfected " << "nrecovered " << "ndeceased\n";

    for (int i = 0; i < hist_total_nhealthy.size(); ++i)
        file_total <<
            hist_total_date[i] << " " <<
            hist_total_nvariants_active[i] << " " <<
            hist_total_nhealthy[i] << " " <<
            hist_total_ninfected[i] << " " <<
            hist_total_nrecovered[i] << " " <<
            hist_total_ndeceased[i] << "\n";

}


#endif