#ifndef EPIWORLD_USERDATA_MEAT_HPP
#define EPIWORLD_USERDATA_MEAT_HPP


#include "epiworld.hpp"

template<typename TSeq>
class UserData;

template<typename TSeq>
inline UserData<TSeq>::UserData(std::vector< std::string > names)
{

    k = names.size();
    data_names = names;

}

template<typename TSeq>
inline void UserData<TSeq>::add(std::vector<epiworld_double> x)
{

    if (x.size() != k)
        throw std::out_of_range(
            "The size of -x-, " + std::to_string(x.size()) + ", does not match " +
            "the number of elements registered (" + std::to_string(k));

    for (auto & i : x)
        data_data.push_back(i);

    data_dates.push_back(model->today());

    n++;

}

template<typename TSeq>
inline void UserData<TSeq>::add(unsigned int j, epiworld_double x)
{

    if (model->today() != (n - 1))
    {
        std::vector< epiworld_double > tmp(k, 0.0);
        tmp[j] = x;
        this->add(x);
    }
    else
    {

        this->operator()(n - 1, k) = x;

    }

}

template<typename TSeq>
inline std::vector< std::string > & UserData<TSeq>::get_names() 
{
    return data_names;
}

template<typename TSeq>
inline std::vector< int > & UserData<TSeq>::get_dates() 
{
    return data_dates;
}

template<typename TSeq>
inline std::vector< epiworld_double > & UserData<TSeq>::get_data() 
{
    return data_data;
}

template<typename TSeq>
inline void UserData<TSeq>::get_all(
    std::vector< std::string > * names,
    std::vector< int > * dates,
    std::vector< epiworld_double > * data
) 
{
    
    if (names != nullptr)
        names = &this->data_names;

    if (dates != nullptr)
        dates = &this->data_dates;

    if (data != nullptr)
        data = &this->data_data;

}

template<typename TSeq>
inline epiworld_double & UserData<TSeq>::operator()(
    unsigned int i,
    unsigned int j,
    bool auto_fill
)
{

    if (j >= k)
        throw std::out_of_range("j cannot be greater than k - 1.");

    if (!auto_fill && (i >= n))
    {

            throw std::out_of_range("j cannot be greater than n - 1.");

    }
    else if (i >= (n + 1u))
    {

        data_dates.resize(i + 1u);
        data_data.resize(i * k + (j + 1u), 0.0);

        for (unsigned int l = (n - 1); l < data_dates.size(); ++l)
            data_dates = l;

        n = i + 1u;

    }

    return data_data[k * i + j];

}

template<typename TSeq>
inline epiworld_double & UserData<TSeq>::operator()(
    unsigned int i,
    std::string name,
    bool auto_fill
)
{
    int loc = -1;
    for (unsigned int l = 0u; l < k; ++l)
    {

        if (name == data_names[l])
        {

            loc = l;
            break;

        }

    }

    if (loc < 0)
        throw std::range_error(
            "The variable \"" + name + "\" is not present " +
            "in the user UserData database."
        );

    return operator()(i, static_cast<unsigned int>(loc), auto_fill);

}

template<typename TSeq>
inline unsigned int UserData<TSeq>::nrow() const
{
    return n;
}

template<typename TSeq>
inline unsigned int UserData<TSeq>::ncol() const
{
    return k;
}

#endif