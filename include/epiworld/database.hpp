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
    std::vector< int > ninfected; ///< Running sum
    std::vector< int > nrecovered; ///< Running sum
    std::vector< int > ndeceased; ///< Running sum

    // Variants history
    std::vector< int > date;
    std::vector< int > variant_id_dyn;
    std::vector< int > ninfected_dyn;
    std::vector< int > nrecovered_dyn;
    std::vector< int > ndeceased_dyn;

    int sampling_freq;

public:

    DataBase(int freq = 1) : sampling_freq(freq) {};
    void register_variant(Virus<TSeq> * v); 
    void set_seq_hasher(std::function<std::vector<int>(TSeq)> fun);
    void record();

    const std::vector< TSeq > & get_sequence() const;
    const std::vector< int > & get_ninfected() const;
    size_t size() const;

    void up_infected(Virus<TSeq> * v);
    void up_recovered(Virus<TSeq> * v);
    void up_deceased(Virus<TSeq> * v);

};

template<typename TSeq>
inline void DataBase<TSeq>::record() 
{
    // Only store every now and then
    if ((model->today() % sampling_freq) == 0)
    {

        date.push_back(model->today());
        for (auto & p : variant_id)
        {
            variant_id_dyn.push_back(p.second);
            ninfected_dyn.push_back(ninfected[p.second]);
            nrecovered_dyn.push_back(nrecovered[p.second]);
            ndeceased.push_back(ndeceased[p.second]);

        }

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
        parent_id.push_back(v->get_id());
        
        ninfected.push_back(1);
        nrecovered.push_back(0);
        ndeceased.push_back(0);

        // Updating the variant
        v->set_id(variant_id.size() - 1);
        v->set_date(model->today());

    } else {

        int tmp_id = variant_id[hash];

        ninfected[tmp_id]++;

        // Updating the variant
        v->set_id(tmp_id);
        v->set_date(origin_date[tmp_id]);

    }    
    
    return;
} 

template<typename TSeq>
inline const std::vector<TSeq> & DataBase<TSeq>::get_sequence() const
{
    return sequence;
}

template<typename TSeq>
inline const std::vector<int> & DataBase<TSeq>::get_ninfected() const
{
    return ninfected;
}

template<typename TSeq>
inline size_t DataBase<TSeq>::size() const
{
    return variant_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::up_infected(Virus<TSeq> * v) {
    ninfected[v->get_id()]++;
}

template<typename TSeq>
inline void DataBase<TSeq>::up_recovered(Virus<TSeq> * v) {
    nrecovered[v->get_id()]++;
}

template<typename TSeq>
inline void DataBase<TSeq>::up_deceased(Virus<TSeq> * v) {
    ndeceased[v->get_id()]++;
}

#endif