#ifndef EPIWORLD_TOOLS_BONES_HPP
#define EPIWORLD_TOOLS_BONES_HPP

template<typename TSeq>
class Tool;

template<typename TSeq>
class Agent;

#define TOOLPTR std::shared_ptr< Tool<TSeq> >

/**
 * @brief Set of tools (useful for building iterators)
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class Tools {
    friend class Tool<TSeq>;
    friend class Agent<TSeq>;
private:
    std::vector< TOOLPTR > * dat;
    const epiworld_fast_uint * n_tools;

public:

    Tools() = delete;
    Tools(Agent<TSeq> & p) : dat(&p.tools), n_tools(&p.n_tools) {};

    typename std::vector< TOOLPTR >::iterator begin();
    typename std::vector< TOOLPTR >::iterator end();

    TOOLPTR & operator()(size_t i);
    TOOLPTR & operator[](size_t i);

    size_t size() const noexcept;

};

template<typename TSeq>
inline typename std::vector< TOOLPTR >::iterator Tools<TSeq>::begin()
{

    if (*n_tools == 0u)
        return dat->end();
    
    return dat->begin();
}

template<typename TSeq>
inline typename std::vector< TOOLPTR >::iterator Tools<TSeq>::end()
{
     
    return begin() + *n_tools;
}

template<typename TSeq>
inline TOOLPTR & Tools<TSeq>::operator()(size_t i)
{

    if (i >= *n_tools)
        throw std::range_error("Tool index out of range.");

    return dat->operator[](i);

}

template<typename TSeq>
inline TOOLPTR & Tools<TSeq>::operator[](size_t i)
{

    return dat->operator[](i);

}

template<typename TSeq>
inline size_t Tools<TSeq>::size() const noexcept 
{
    return *n_tools;
}

/**
 * @brief Set of Tools (const) (useful for iterators)
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class Tools_const {
    friend class Tool<TSeq>;
    friend class Agent<TSeq>;
private:
    const std::vector< TOOLPTR > * dat;
    const epiworld_fast_uint * n_tools;

public:

    Tools_const() = delete;
    Tools_const(const Agent<TSeq> & p) : dat(&p.tools), n_tools(&p.n_tools) {};

    typename std::vector< TOOLPTR >::const_iterator begin();
    typename std::vector< TOOLPTR >::const_iterator end();

    const TOOLPTR & operator()(size_t i);
    const TOOLPTR & operator[](size_t i);

    size_t size() const noexcept;

};

template<typename TSeq>
inline typename std::vector< TOOLPTR >::const_iterator Tools_const<TSeq>::begin() {

    if (*n_tools == 0u)
        return dat->end();
    
    return dat->begin();
}

template<typename TSeq>
inline typename std::vector< TOOLPTR >::const_iterator Tools_const<TSeq>::end() {
     
    return begin() + *n_tools;
}

template<typename TSeq>
inline const TOOLPTR & Tools_const<TSeq>::operator()(size_t i)
{

    if (i >= *n_tools)
        throw std::range_error("Tool index out of range.");

    return dat->operator[](i);

}

template<typename TSeq>
inline const TOOLPTR & Tools_const<TSeq>::operator[](size_t i)
{

    return dat->operator[](i);

}

template<typename TSeq>
inline size_t Tools_const<TSeq>::size() const noexcept 
{
    return *n_tools;
}

#undef TOOLPTR



#endif