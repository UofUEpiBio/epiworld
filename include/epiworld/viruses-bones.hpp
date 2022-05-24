#ifndef EPIWORLD_VIRUSES_BONES_HPP
#define EPIWORLD_VIRUSES_BONES_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Agent;

#define VIRUSPTR std::shared_ptr< Virus<TSeq> >

/**
 * @brief Set of viruses (useful for building iterators)
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class Viruses {
    friend class Virus<TSeq>;
    friend class Agent<TSeq>;
private:
    std::vector< VIRUSPTR > * dat;
    const epiworld_fast_uint * n_viruses;

public:

    Viruses() = delete;
    Viruses(Agent<TSeq> & p) : dat(&p.viruses), n_viruses(&p.n_viruses) {};

    typename std::vector< VIRUSPTR >::iterator begin();
    typename std::vector< VIRUSPTR >::iterator end();

    VIRUSPTR & operator()(size_t i);
    VIRUSPTR & operator[](size_t i);

    size_t size() const noexcept;

};

template<typename TSeq>
inline typename std::vector< VIRUSPTR >::iterator Viruses<TSeq>::begin()
{

    if (*n_viruses == 0u)
        return dat->end();
    
    return dat->begin();
}

template<typename TSeq>
inline typename std::vector< VIRUSPTR >::iterator Viruses<TSeq>::end()
{
     
    return begin() + *n_viruses;
}

template<typename TSeq>
inline VIRUSPTR & Viruses<TSeq>::operator()(size_t i)
{

    if (i >= *n_viruses)
        throw std::range_error("Virus index out of range.");

    return dat->operator[](i);

}

template<typename TSeq>
inline VIRUSPTR & Viruses<TSeq>::operator[](size_t i)
{

    return dat->operator[](i);

}

template<typename TSeq>
inline size_t Viruses<TSeq>::size() const noexcept 
{
    return *n_viruses;
}

/**
 * @brief Set of Viruses (const) (useful for iterators)
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class Viruses_const {
    friend class Virus<TSeq>;
    friend class Agent<TSeq>;
private:
    const std::vector< VIRUSPTR > * dat;
    const epiworld_fast_uint * n_viruses;

public:

    Viruses_const() = delete;
    Viruses_const(const Agent<TSeq> & p) : dat(&p.viruses), n_viruses(&p.n_viruses) {};

    typename std::vector< VIRUSPTR >::const_iterator begin();
    typename std::vector< VIRUSPTR >::const_iterator end();

    const VIRUSPTR & operator()(size_t i);
    const VIRUSPTR & operator[](size_t i);

    size_t size() const noexcept;

};

template<typename TSeq>
inline typename std::vector< VIRUSPTR >::const_iterator Viruses_const<TSeq>::begin() {

    if (*n_viruses == 0u)
        return dat->end();
    
    return dat->begin();
}

template<typename TSeq>
inline typename std::vector< VIRUSPTR >::const_iterator Viruses_const<TSeq>::end() {
     
    return begin() + *n_viruses;
}

template<typename TSeq>
inline const VIRUSPTR & Viruses_const<TSeq>::operator()(size_t i)
{

    if (i >= *n_viruses)
        throw std::range_error("Virus index out of range.");

    return dat->operator[](i);

}

template<typename TSeq>
inline const VIRUSPTR & Viruses_const<TSeq>::operator[](size_t i)
{

    return dat->operator[](i);

}

template<typename TSeq>
inline size_t Viruses_const<TSeq>::size() const noexcept 
{
    return *n_viruses;
}

#undef VIRUSPTR



#endif