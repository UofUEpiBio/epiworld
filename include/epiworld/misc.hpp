#ifndef EPIWORLD_MISC_HPP 
#define EPIWORLD_MISC_HPP

// Relevant for anything using vecHasher function ------------------------------
template <typename T>
struct vecHasher {
    std::size_t operator()(std::vector< T > const&  dat) const noexcept {
        
        std::hash< T > hasher;
        std::size_t hash = hasher(dat[0u]);
        
        // ^ makes bitwise XOR
        // 0x9e3779b9 is a 32 bit constant (comes from the golden ratio)
        // << is a shift operator, something like lhs * 2^(rhs)
        if (dat.size() > 1u)
            for (unsigned int i = 1u; i < dat.size(); ++i)
                hash ^= hasher(dat[i]) + 0x9e3779b9 + (hash<<6) + (hash>>2);
        
        return hash;
        
    }
};

template<typename Ta = double, typename Tb = unsigned int> 
using MapVec_type = std::unordered_map< std::vector< Ta >, Tb, vecHasher<Ta>>;

/**
 * @brief Default sequence initializers
 * 
 * @details 
 * If the user does not provide a default sequence, this function is used when
 * a sequence needs to be initialized. Some examples: `Person`, `Virus`, and
 * `Tool` need a default sequence.
 * 
 * @tparam TSeq 
 * @return TSeq 
 */
///@[
template<typename TSeq>
inline TSeq default_sequence();

template<>
inline bool default_sequence() {
    return false;
}

template<>
inline int default_sequence() {
    return 0;
}

template<>
inline double default_sequence() {
    return 0.0;
}

template<>
inline std::vector<bool> default_sequence() {
    return {false};
}

template<>
inline std::vector<int> default_sequence() {
    return {0};
}

template<>
inline std::vector<double> default_sequence() {
    return {0.0};
}
///@]


#endif