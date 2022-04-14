#include <vector>
#include <functional>
#include <memory>
#include <stdexcept>
#include <random>
#include <fstream>
#include <string>
#include <map>
#include <unordered_map>
#include <chrono>
#include <climits>
#include <cstdint>

#ifndef EPIWORLD_HPP
#define EPIWORLD_HPP

namespace epiworld {

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/config.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_CONFIG_HPP
#define EPIWORLD_CONFIG_HPP

#ifndef printf_epiworld
    #define printf_epiworld fflush(stdout);printf
#endif

#ifndef EPIWORLD_MAXNEIGHBORS
    #define EPIWORLD_MAXNEIGHBORS 100000
#endif

#ifdef EPIWORLD_USE_OMP
    #include <omp.h>
#else

#ifndef epiworld_double
    #define epiworld_double float
#endif

#ifndef epiworld_fast_int
    #define epiworld_fast_int std::int_fast16_t
#endif

#ifndef epiworld_fast_uint
    #define epiworld_fast_uint std::uint_fast8_t
#endif

#endif

template<typename TSeq>
class Model;

template<typename TSeq>
class Person;

template<typename TSeq>
class PersonTools;

template<typename TSeq>
class Virus;

template<typename TSeq>
class Tool;


template<typename TSeq>
using ToolFun = std::function<epiworld_double(Tool<TSeq>*,Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using MixerFun = std::function<epiworld_double(PersonTools<TSeq>*,Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using MutFun = std::function<bool(Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using PostRecoveryFun = std::function<void(Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using VirusFun = std::function<epiworld_double(Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using UpdateFun = std::function<epiworld_fast_uint(Person<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using GlobalFun = std::function<void(Model<TSeq>*)>;

/**
 * @brief List of possible states in the model
 *  
 */
enum STATUS {
    SUSCEPTIBLE,
    EXPOSED,
    REMOVED
};

#ifndef DEFAULT_TOOL_CONTAGION_REDUCTION
    #define DEFAULT_TOOL_CONTAGION_REDUCTION    0.0
#endif

#ifndef DEFAULT_TOOL_TRANSMISSION_REDUCTION
    #define DEFAULT_TOOL_TRANSMISSION_REDUCTION 0.0
#endif

#ifndef DEFAULT_TOOL_RECOVERY_ENHANCER
    #define DEFAULT_TOOL_RECOVERY_ENHANCER      0.0
#endif

#ifndef DEFAULT_TOOL_DEATH_REDUCTION
    #define DEFAULT_TOOL_DEATH_REDUCTION        0.0
#endif

#ifndef EPI_DEFAULT_VIRUS_PROB_INFECTION
    #define EPI_DEFAULT_VIRUS_PROB_INFECTION    1.0
#endif

#ifndef EPI_DEFAULT_VIRUS_PROB_RECOVERY
    #define EPI_DEFAULT_VIRUS_PROB_RECOVERY     0.5
#endif

#ifndef EPI_DEFAULT_VIRUS_PROB_DEATH
    #define EPI_DEFAULT_VIRUS_PROB_DEATH        0.0
#endif

/**
 * @brief A utility for bookeeping
 * 
 * @details The `Model<TSeq>` class will keep this value
 * and every new `Person<TSeq>` will have the oposit at construction.
 * The idea is that users can make use of the function `Person::visited()`
 * when needing to keep track of whether the agent has been seen before
 * within a routine. The member function `Model<TSeq>::toggle_visited()`
 * will change the value of the model object automatically when
 * the next states are updated.
 * 
 * Users can call `Model::toggle_visited()` when needed to reset the
 * book-kepping.
 * 
 * 
 */
#ifndef EPI_DEFAULT_VISITED
    #define EPI_DEFAULT_VISITED true
#endif

#ifdef EPI_DEBUG
    #define EPI_DEBUG_NOTIFY_ACTIVE() \
        printf_epiworld("[epiworld-debug] DEBUGGING ON (compiled with EPI_DEBUG defined)\n");
    #define EPI_DEBUG_ALL_NON_NEGATIVE(vect) \
        for (auto & v : vect) \
            if (static_cast<double>(v) < 0.0) \
                throw std::logic_error("A negative value not allowed.");

    #define EPI_DEBUG_SUM_DBL(vect, num) \
        double _epi_debug_sum = 0.0; \
        for (auto & v : vect) \
        {   \
            _epi_debug_sum += static_cast<double>(v);\
            if (_epi_debug_sum > static_cast<double>(num)) \
                throw std::logic_error("[epiworld-debug] The sum of elements not reached."); \
        }

    #define EPI_DEBUG_SUM_INT(vect, num) \
        int _epi_debug_sum = 0; \
        for (auto & v : vect) \
        {   \
            _epi_debug_sum += static_cast<int>(v);\
            if (_epi_debug_sum > static_cast<int>(num)) \
                throw std::logic_error("[epiworld-debug] The sum of elements not reached."); \
        }

    #define EPI_DEBUG_VECTOR_MATCH_INT(a, b) \
        if (a.size() != b.size())  \
            throw std::length_error("[epiworld-debug] The vectors do not match size."); \
        for (size_t _i = 0u; _i < a.size(); ++_i) \
            if (a[_i] != b[_i]) \
                throw std::logic_error("[epiworld-debug] The vectors do not match.");


#else
    #define EPI_DEBUG_NOTIFY_ACTIVE()
    #define EPI_DEBUG_ALL_NON_NEGATIVE(vect)
    #define EPI_DEBUG_SUM_DBL(vect, num)
    #define EPI_DEBUG_SUM_INT(vect, num)
    #define EPI_DEBUG_VECTOR_MATCH_INT(a, b)
#endif

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/config.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/epiworld-macros.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MACROS_HPP
#define EPIWORLD_MACROS_HPP


/**
 * @brief Helper macro to define a new tool
 * 
 */
#define EPI_NEW_TOOL(fname,tseq) inline epiworld_double \
(fname)(\
    epiworld::Tool< tseq > * t, \
    epiworld::Person< tseq > * p, \
    epiworld::Virus< tseq > * v, \
    epiworld::Model< tseq > * m\
    )

/**
 * @brief Create a Tool within a function
 * 
 */
#define EPI_NEW_TOOL_LAMBDA(funname,tseq) \
    epiworld::ToolFun<tseq> funname = \
    [](epiworld::Tool<tseq> * t, \
    epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq> * v, \
    epiworld::Model<tseq> * m)

/**
 * @brief Helper macro for accessing model parameters
 * 
 */
#define EPI_PARAMS(i) m->operator()(i)

/**
 * @brief Helper macro for defining Mutation Functions
 * 
 */
#define EPI_NEW_MUTFUN(funname,tseq) inline bool \
    (funname)(\
    epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq> * v, \
    epiworld::Model<tseq> * m )

#define EPI_NEW_MUTFUN_LAMBDA(funname,tseq) \
    epiworld::MutFun<tseq> funname = \
    [](epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq> * v, \
    epiworld::Model<tseq> * m)

#define EPI_NEW_POSTRECOVERYFUN(funname,tseq) inline void \
    (funname)( \
    epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq>* v, \
    epiworld::Model<tseq> * m\
    )

#define EPI_NEW_POSTRECOVERYFUN_LAMBDA(funname,tseq) \
    epiworld::PostRecoveryFun<tseq> funname = \
    [](epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq>* v, \
    epiworld::Model<tseq> * m)

#define EPI_NEW_VIRUSFUN(funname,tseq) inline void \
    (funname)( \
    epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq>* v, \
    epiworld::Model<tseq> * m\
    )

#define EPI_NEW_VIRUSFUN_LAMBDA(funname,tseq) \
    epiworld::VirusFun<tseq> funname = \
    [](epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq>* v, \
    epiworld::Model<tseq> * m)

#define EPI_RUNIF() m->runif()

#define EPIWORLD_RUN(a) \
    if (a.get_verbose()) \
    { \
        printf_epiworld("Running the model...\n");\
    } \
    for (unsigned int niter = 0; niter < a.get_ndays(); ++niter)

#define EPI_TOKENPASTE(a,b) a ## b
#define MPAR(num) *(m->EPI_TOKENPASTE(p,num))

#define EPI_NEW_UPDATEFUN(funname,tseq) inline epiworld_fast_uint \
    (funname)(epiworld::Person<tseq> * p, epiworld::Model<tseq> * m)

#define EPI_NEW_UPDATEFUN_LAMBDA(funname,tseq) inline epiworld_fast_uint \
    epiworld::UpdateFun<tseq> funname = \
    [](epiworld::Person<tseq> * p, epiworld::Model<tseq> * m)

#define EPI_NEW_GLOBALFUN(funname,tseq) inline void \
    (funname)(epiworld::Model<tseq>* m)

#define EPI_NEW_GLOBALFUN_LAMBDA(funname,tseq) inline void \
    epiworld::GlobalFun<tseq> funname = \
    [](epiworld::Model<tseq>* m)

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/epiworld-macros.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/misc.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MISC_HPP 
#define EPIWORLD_MISC_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Person;

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

template<typename Ta = epiworld_double, typename Tb = unsigned int> 
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
inline epiworld_double default_sequence() {
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
inline std::vector<epiworld_double> default_sequence() {
    return {0.0};
}
///@]

/**
 * @brief Check whether `a` is included in `b`
 * 
 * @tparam Ta Type of `a`. Could be int, epiworld_double, etc.
 * @param a Scalar of class `Ta`.
 * @param b Vector `std::vector` of class `Ta`.
 * @return `true` if `a in b`, and `false` otherwise.
 */
template<typename Ta>
inline bool IN(const Ta & a, const std::vector< Ta > & b)
{
    for (const auto & i : b)
        if (a == i)
            return true;

    return false;
}

/**
 * @brief Conditional Weighted Sampling
 * 
 * @details 
 * The sampling function will draw one of `{-1, 0,...,probs.size() - 1}` in a
 * weighted fashion. The probabilities are drawn given that either one or none
 * of the cases is drawn; in the latter returns -1.
 * 
 * @param probs Vector of probabilities.
 * @param m A `Model`. This is used to draw random uniform numbers.
 * @return int If -1 then it means that none got sampled, otherwise the index
 * of the entry that got drawn.
 */
template<typename TSeq>
inline int roulette(
    const std::vector< epiworld_double > & probs,
    Model<TSeq> * m
    )
{

    // Step 1: Computing the prob on none 
    epiworld_double p_none = 1.0;
    std::vector< int > certain_infection;
    for (unsigned int p = 0u; p < probs.size(); ++p)
    {
        p_none *= (1.0 - probs[p]);

        if (probs[p] > (1 - 1e-100))
            certain_infection.push_back(p);
        
    }

    epiworld_double r = m->runif();
    // If there are one or more probs that go close to 1, sample
    // uniformly
    if (certain_infection.size() > 0)
        return certain_infection[std::floor(r * certain_infection.size())];

    // Step 2: Calculating the prob of none or single
    std::vector< epiworld_double > probs_only_p;
    epiworld_double p_none_or_single = p_none;
    for (unsigned int p = 0u; p < probs.size(); ++p)
    {
        probs_only_p.push_back(probs[p] * (p_none / (1.0 - probs[p])));
        p_none_or_single += probs_only_p[p];
    }

    // Step 3: Roulette
    epiworld_double cumsum = p_none/p_none_or_single;
    if (r < cumsum)
        return -1;

    for (unsigned int p = 0u; p < probs.size(); ++p)
    {
        // If it yield here, then bingo, the individual will acquire the disease
        cumsum += probs_only_p[p]/(p_none_or_single);
        if (r < cumsum)
            return static_cast<int>(p);
        
    }

    return static_cast<int>(probs.size() - 1u);

}

template<typename TSeq>
inline int roulette(
    unsigned int nelements,
    Model<TSeq> * m
    )
{

    // Step 1: Computing the prob on none 
    epiworld_double p_none = 1.0;
    unsigned int ncertain = 0u;
    // std::vector< int > certain_infection;
    for (unsigned int p = 0u; p < nelements; ++p)
    {
        p_none *= (1.0 - m->array_double_tmp[p]);

        if (m->array_double_tmp[p] > (1 - 1e-100))
            m->array_double_tmp[nelements + ncertain++] = p;
            // certain_infection.push_back(p);
        
    }

    epiworld_double r = m->runif();
    // If there are one or more probs that go close to 1, sample
    // uniformly
    if (ncertain > 0u)
        return m->array_double_tmp[nelements + std::floor(ncertain * r)]; //    certain_infection[std::floor(r * certain_infection.size())];

    // Step 2: Calculating the prob of none or single
    // std::vector< epiworld_double > probs_only_p;
    epiworld_double p_none_or_single = p_none;
    for (unsigned int p = 0u; p < nelements; ++p)
    {
        m->array_double_tmp[nelements + p] = 
            m->array_double_tmp[p] * (p_none / (1.0 - m->array_double_tmp[p]));
        p_none_or_single += m->array_double_tmp[nelements + p];
    }

    // Step 3: Roulette
    epiworld_double cumsum = p_none/p_none_or_single;
    if (r < cumsum)
        return -1;

    for (unsigned int p = 0u; p < nelements; ++p)
    {
        // If it yield here, then bingo, the individual will acquire the disease
        cumsum += m->array_double_tmp[nelements + p]/(p_none_or_single);
        if (r < cumsum)
            return static_cast<int>(p);
        
    }

    return static_cast<int>(nelements - 1u);

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/misc.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/progress.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PROGRESS_HPP
#define EPIWORLD_PROGRESS_HPP

#ifndef EPIWORLD_PROGRESS_BAR_WIDTH
#define EPIWORLD_PROGRESS_BAR_WIDTH 80
#endif

/**
 * @brief A simple progress bar
  */
class Progress {
private:
    int    width;     ///< Total width size (number of bars)
    int    n;         ///< Total number of iterations
    epiworld_double step_size; ///< Size of the step
    int last_loc;     ///< Last location of the bar
    int cur_loc;      ///< Last location of the bar
    int i;            ///< Current iteration step
    
public:
    Progress() {};
    Progress(int n_, int width_);
    ~Progress() {};

    void start();
    void next();
    void end();

};

inline Progress::Progress(int n_, int width_) {


    width     = std::max(7, width_ - 7);
    n         = n_;
    step_size = static_cast<epiworld_double>(width)/static_cast<epiworld_double>(n);
    last_loc  = 0;
    i         = 0;

}

inline void Progress::start()
{
    for (int j = 0; j < (width); ++j)
    {
        printf_epiworld("_");
    }
    printf_epiworld("\n");
}

inline void Progress::next() {

    if (i == 0)
        start();

    cur_loc = std::floor((++i) * step_size);


    for (int j = 0; j < (cur_loc - last_loc); ++j)
    {
        printf_epiworld("|");
    }
    if (i >= n)
        end();

    last_loc = cur_loc;

}

inline void Progress::end() {

    printf_epiworld(" done.\n");

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/progress.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/userdata-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_USERDATA_BONES_HPP
#define EPIWORLD_USERDATA_BONES_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class DataBase;

template<typename TSeq>
class UserData
{
    friend class Model<TSeq>;
    friend class DataBase<TSeq>;

private:
    Model<TSeq> * model;

    std::vector< std::string > data_names;
    std::vector< int > data_dates;
    std::vector< epiworld_double > data_data;

    unsigned int k = 0u;
    unsigned int n = 0u;

    int last_day = -1;

public:

    UserData() {};

    UserData(std::vector< std::string > names);

    void add(std::vector<epiworld_double> x);
    void add(
        unsigned int j,
        epiworld_double x
        );

    epiworld_double & operator()(
        unsigned int i,
        unsigned int j
        );

    epiworld_double & operator()(
        unsigned int i,
        std::string name
        );

    std::vector< std::string > & get_names();

    std::vector< int > & get_dates();

    std::vector< epiworld_double > & get_data();

    void get_all(
        std::vector< std::string > * names    = nullptr,
        std::vector< int > * date             = nullptr,
        std::vector< epiworld_double > * data = nullptr
    );

    unsigned int nrow() const;
    unsigned int ncol() const;

    void write(std::string fn);
    void print() const;

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/userdata-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/userdata-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_USERDATA_MEAT_HPP
#define EPIWORLD_USERDATA_MEAT_HPP

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
    last_day = model->today();

}

template<typename TSeq>
inline void UserData<TSeq>::add(unsigned int j, epiworld_double x)
{

    // Starting with a new day?
    if (static_cast<int>(model->today()) != last_day)
    {

        std::vector< epiworld_double > tmp(k, 0.0);

        tmp[j] = x;

        add(tmp);

    }
    else
    {

        this->operator()(n - 1, j) = x;

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
    unsigned int j
)
{

    if (j >= k)
        throw std::out_of_range("j cannot be greater than k - 1.");

    if (i >= n)
        throw std::out_of_range("j cannot be greater than n - 1.");

    return data_data[k * i + j];

}

template<typename TSeq>
inline epiworld_double & UserData<TSeq>::operator()(
    unsigned int i,
    std::string name
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

    return operator()(i, static_cast<unsigned int>(loc));

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

template<typename TSeq>
inline void UserData<TSeq>::write(std::string fn)
{
    std::ofstream file_ud(fn, std::ios_base::out);

    // File header
    file_ud << "\"date\"";
    for (auto & cn : data_names)
        file_ud << " \"" + cn + "\"";
    file_ud << "\n";
    
    unsigned int ndata = 0u;
    for (unsigned int i = 0u; i < n; ++i)
    {
        file_ud << data_dates[i];

        for (unsigned int j = 0u; j < k; ++j)
            file_ud << " " << data_data[ndata++];

        file_ud << "\n";
    }

    return;
}

template<typename TSeq>
inline void UserData<TSeq>::print() const
{
    // File header
    printf_epiworld("Total records: %i\n", n);
    printf_epiworld("date");

    for (auto & cn : data_names)
    {

        printf_epiworld(" %s", cn.c_str());

    }

    printf_epiworld("\n");
    
    unsigned int ndata = 0u;
    
    for (unsigned int i = 0u; i < n; ++i)
    {

        printf_epiworld("%i", data_dates[i]);

        for (unsigned int j = 0u; j < k; ++j)
        {

            printf_epiworld(" %.2f", data_data[ndata++]);

        }

        printf_epiworld("\n");

    }

    return;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/userdata-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/seq_processing.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_SEQ_PROCESSING_HPP 
#define EPIWORLD_SEQ_PROCESSING_HPP

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
template<typename TSeq = bool>
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



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/seq_processing.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/database-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


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
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/database-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/database-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_DATABASE_MEAT_HPP
#define EPIWORLD_DATABASE_MEAT_HPP

template<typename TSeq>
inline void DataBase<TSeq>::set_model(Model<TSeq> & m)
{
    model = &m;
    user_data.model = &m;

    reset();

    // Initializing the counts
    today_total.resize(m.nstatus);
    std::fill(today_total.begin(), today_total.end(), 0);
    for (auto & p : *m.get_population())
        ++today_total[p.get_status()];
    
    today_total_next.resize(m.nstatus);
    std::fill(today_total_next.begin(), today_total_next.end(), 0);

    transition_matrix.resize(m.nstatus * m.nstatus);
    std::fill(transition_matrix.begin(), transition_matrix.end(), 0);

    transition_matrix_next.resize(m.nstatus * m.nstatus);
    std::fill(transition_matrix_next.begin(), transition_matrix_next.end(), 0);

    return;

}

template<typename TSeq>
inline Model<TSeq> * DataBase<TSeq>::get_model() {
    return model;
}

template<typename TSeq>
inline const std::vector< TSeq > & DataBase<TSeq>::get_sequence() const {
    return sequence;
}

template<typename TSeq>
inline void DataBase<TSeq>::record() 
{

    // Updating values according to today's changes
    for (auto i = 0u; i < model->nstatus; ++i)
    {
        today_total[i] += today_total_next[i];
        today_total_next[i] = 0;
    }

    ////////////////////////////////////////////////////////////////////////////
    // DEBUGGING BLOCK
    ////////////////////////////////////////////////////////////////////////////
    EPI_DEBUG_SUM_INT(today_total, model->size())
    EPI_DEBUG_ALL_NON_NEGATIVE(today_total)

    #ifdef EPI_DEBUG
    // Checking whether the sums correspond
    std::vector< int > _today_total_cp(today_total.size(), 0);
    for (auto & p : model->population)
        _today_total_cp[p.get_status()]++;
    
    EPI_DEBUG_VECTOR_MATCH_INT(_today_total_cp, today_total)
    #endif
    ////////////////////////////////////////////////////////////////////////////

    for (auto v = 0u; v < today_variant.size(); ++v)
    {

        for (auto i = 0u; i < model->nstatus; ++i)
        {
            today_variant[v][i] += today_variant_next[v][i];
            today_variant_next[v][i] = 0;
        }

    }
    
    for (auto i = 0u; i < transition_matrix.size(); ++i)
    {
        transition_matrix[i] += transition_matrix_next[i];
        transition_matrix_next[i] = 0;
    }

    // Only store every now and then
    if ((model->today() % sampling_freq) == 0)
    {

        // Recording variant's history
        for (auto & p : variant_id)
        {

            for (unsigned int s = 0u; s < model->nstatus; ++s)
            {

                hist_variant_date.push_back(model->today());
                hist_variant_id.push_back(p.second);
                hist_variant_status.push_back(s);
                hist_variant_counts.push_back(today_variant[p.second][s]);

            }

        }

        // Recording the overall history
        for (unsigned int s = 0u; s < model->nstatus; ++s)
        {
            hist_total_date.push_back(model->today());
            hist_total_nvariants_active.push_back(today_total_nvariants_active);
            hist_total_status.push_back(s);
            hist_total_counts.push_back(today_total[s]);
        }

        for (auto cell : transition_matrix)
            hist_transition_matrix.push_back(cell);

    }

}

template<typename TSeq>
inline void DataBase<TSeq>::record_variant(Virus<TSeq> * v)
{

    // Updating registry
    std::vector< int > hash = seq_hasher(*v->get_sequence());
    unsigned int old_id = v->get_id();
    unsigned int new_id;
    if (variant_id.find(hash) == variant_id.end())
    {

        new_id = variant_id.size();
        variant_id[hash] = new_id;
        sequence.push_back(*v->get_sequence());
        origin_date.push_back(model->today());
        
        parent_id.push_back(old_id);
        
        today_variant.push_back({});
        today_variant[new_id].resize(
            model->status_susceptible.size() +
            model->status_exposed.size() +
            model->status_removed.size(),
            0
        );

        today_variant_next.push_back(today_variant[new_id]);
        
        // Updating the variant
        v->set_id(new_id);
        v->set_date(model->today());

        today_total_nvariants_active++;

    } else {

        // Finding the id
        new_id = variant_id[hash];

        // Reflecting the change
        v->set_id(new_id);
        v->set_date(origin_date[new_id]);

    }

    // Moving statistics (only if we are affecting an individual)
    if (v->get_host() != nullptr)
    {
        // Correcting math
        epiworld_fast_uint tmp_status = v->get_host()->get_status();
        today_variant_next[old_id][tmp_status]--;
        today_variant_next[new_id][tmp_status]++;

    }
    
    return;
} 

template<typename TSeq>
inline size_t DataBase<TSeq>::size() const
{
    return variant_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::up_exposed(
    Virus<TSeq> * v,
    epiworld_fast_uint new_status
) {

    today_variant_next[v->get_id()][new_status]++;

}

template<typename TSeq>
inline void DataBase<TSeq>::down_exposed( 
    Virus<TSeq> * v,
    epiworld_fast_uint prev_status
) {

    today_variant_next[v->get_id()][prev_status]--;

}

template<typename TSeq>
inline void DataBase<TSeq>::state_change(
        epiworld_fast_uint prev_status,
        epiworld_fast_uint new_status
) {
    today_total_next[prev_status]--;
    today_total_next[new_status]++;
    return;
}

template<typename TSeq>
inline void DataBase<TSeq>::record_transition(
    epiworld_fast_uint from,
    epiworld_fast_uint to
) {

    transition_matrix_next[to * model->nstatus + from]++;

}

#define EPIWORLD_GET_STATUS_LABELS(stdstrvec) \
    stdstrvec.resize(model->nstatus); \
    for (epiworld_fast_uint i = 0u; i < model->status_susceptible.size(); ++i) \
        stdstrvec[model->status_susceptible[i]] = model->status_susceptible_labels[i]; \
    for (epiworld_fast_uint i = 0u; i < model->status_exposed.size(); ++i) \
        stdstrvec[model->status_exposed[i]] = model->status_exposed_labels[i]; \
    for (epiworld_fast_uint i = 0u; i < model->status_removed.size(); ++i) \
        stdstrvec[model->status_removed[i]] = model->status_removed_labels[i];


template<typename TSeq>
inline int DataBase<TSeq>::get_today_total(
    std::string what
) const
{
    std::vector< std::string > labels;
    EPIWORLD_GET_STATUS_LABELS(labels)

    for (auto i = 0u; i < labels.size(); ++i)
    {
        if (labels[i] == what)
            return today_total[i];
    }

    throw std::range_error("The value '" + what + "' is not in the model.");

}

template<typename TSeq>
inline void DataBase<TSeq>::get_today_total(
    std::vector< std::string > * status,
    std::vector< int > * counts
) const
{
    if (status != nullptr)
    {
        EPIWORLD_GET_STATUS_LABELS((*status))
    }

    if (counts != nullptr)
        *counts = today_total;

}

template<typename TSeq>
inline void DataBase<TSeq>::get_today_variant(
    std::vector< std::string > & status,
    std::vector< int > & id,
    std::vector< int > & counts
    ) const
{
    
    std::vector< std::string > labels;
    EPIWORLD_GET_STATUS_LABELS(labels)    
    status.resize(today_variant.size(), "");
    id.resize(today_variant.size(), 0);
    counts.resize(today_variant.size(),0);

    int n = 0u;
    for (unsigned int v = 0u; v < today_variant.size(); ++v)
        for (unsigned int s = 0u; s < labels.size(); ++s)
        {
            status[n]   = labels[s];
            id[n]       = static_cast<int>(v);
            counts[n++] = today_variant[v][s];

        }

}

template<typename TSeq>
inline void DataBase<TSeq>::get_hist_total(
    std::vector< int > * date,
    std::vector< std::string > * status,
    std::vector< int > * counts
) const
{

    if (date != nullptr)
        *date = hist_total_date;

    std::vector< std::string > labels;
    EPIWORLD_GET_STATUS_LABELS(labels)

    if (status != nullptr)
    {
        status->resize(hist_total_status.size(), "");
        for (unsigned int i = 0u; i < hist_total_status.size(); ++i)
            status->operator[](i) = labels[hist_total_status[i]];
    }

    if (counts != nullptr)
        *counts = hist_total_counts;

    return;

}

template<typename TSeq>
inline void DataBase<TSeq>::get_hist_variant(
    std::vector< int > & date,
    std::vector< int > & id,
    std::vector< std::string > & status,
    std::vector< int > & counts
) const {

    date = hist_variant_date;
    std::vector< std::string > labels;
    EPIWORLD_GET_STATUS_LABELS(labels)
    id = hist_variant_id;
    status.resize(hist_variant_status.size(), "");
    for (unsigned int i = 0u; i < hist_variant_status.size(); ++i)
        status[i] = labels[hist_variant_status[i]];

    counts = hist_variant_counts;

    return;

}

#undef EPIWORLD_GET_STATUS_LABELS

template<typename TSeq>
inline void DataBase<TSeq>::write_data(
    std::string fn_variant_info,
    std::string fn_variant_hist,
    std::string fn_total_hist,
    std::string fn_transmission,
    std::string fn_transition
) const
{

    if (fn_variant_info != "")
    {
        std::ofstream file_variant_info(fn_variant_info, std::ios_base::out);

        file_variant_info <<
            "id " << "sequence " << "date " << "parent " << "patiente\n";

        for (const auto & v : variant_id)
        {
            int id = v.second;
            file_variant_info <<
                id << " " <<
                seq_writer(sequence[id]) << " " <<
                origin_date[id] << " " <<
                parent_id[id] << " " <<
                parent_id[id] << "\n";
        }

    }

    // Preparing labels
    std::vector< std::string > labels(model->nstatus);
    for (unsigned int i = 0u; i < model->status_susceptible.size(); ++i)
        labels[model->status_susceptible[i]] =
            model->status_susceptible_labels[i];
    
    for (unsigned int i = 0u; i < model->status_exposed.size(); ++i)
        labels[model->status_exposed[i]] =
            model->status_exposed_labels[i];

    for (unsigned int i = 0u; i < model->status_removed.size(); ++i)
        labels[model->status_removed[i]] =
            model->status_removed_labels[i];

    if (fn_variant_hist != "")
    {
        std::ofstream file_variant(fn_variant_hist, std::ios_base::out);
        
        file_variant <<
            "date " << "id " << "status " << "n\n";

        for (unsigned int i = 0; i < hist_variant_id.size(); ++i)
            file_variant <<
                hist_variant_date[i] << " " <<
                hist_variant_id[i] << " " <<
                labels[hist_variant_status[i]] << " " <<
                hist_variant_counts[i] << "\n";
    }

    if (fn_total_hist != "")
    {
        std::ofstream file_total(fn_total_hist, std::ios_base::out);

        file_total <<
            "date " << "nvariants " << "status " << "counts\n";

        for (unsigned int i = 0; i < hist_total_date.size(); ++i)
            file_total <<
                hist_total_date[i] << " " <<
                hist_total_nvariants_active[i] << " \"" <<
                labels[hist_total_status[i]] << "\" " << 
                hist_total_counts[i] << "\n";
    }

    if (fn_transmission != "")
    {
        std::ofstream file_transmission(fn_transmission, std::ios_base::out);
        file_transmission <<
            "date " << "variant " << "source " << "target\n";

        for (unsigned int i = 0; i < transmission_target.size(); ++i)
            file_transmission <<
                transmission_date[i] << " " <<
                transmission_variant[i] << " " <<
                transmission_source[i] << " " <<
                transmission_target[i] << "\n";
                
    }

    if (fn_transition != "")
    {
        std::ofstream file_transition(fn_transition, std::ios_base::out);
        file_transition <<
            "date " << "from " << "to " << "counts\n";

        int ns = model->nstatus;

        for (int i = 0; i <= model->today(); ++i)
        {

            for (int from = 0u; from < ns; ++from)
                for (int to = 0u; to < ns; ++to)
                    file_transition <<
                        i << " " <<
                        labels[from] << " " <<
                        labels[to] << " " <<
                        hist_transition_matrix[i * (ns * ns) + to * ns + from] << "\n";
                
        }
                
    }

}

template<typename TSeq>
inline void DataBase<TSeq>::record_transmission(
    int i,
    int j,
    int variant
) {

    transmission_date.push_back(model->today());
    transmission_source.push_back(i);
    transmission_target.push_back(j);
    transmission_variant.push_back(variant);

}

template<typename TSeq>
inline size_t DataBase<TSeq>::get_nvariants() const
{
    return variant_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::reset()
{

    variant_id.clear();
    sequence.clear();
    origin_date.clear();
    parent_id.clear();
    
    hist_variant_date.clear();
    hist_variant_id.clear();
    hist_variant_status.clear();
    hist_variant_counts.clear();
    
    hist_total_date.clear();
    hist_total_nvariants_active.clear();
    hist_total_status.clear();
    hist_total_counts.clear();
    
    transmission_date.clear();
    transmission_source.clear();
    transmission_target.clear();
    transmission_variant.clear();

    today_total_nvariants_active = 0;

    today_total.clear();
    today_total_next.clear();
    
    today_variant.clear();
    today_variant_next.clear();

}

template<typename TSeq>
inline void DataBase<TSeq>::set_user_data(
    std::vector< std::string > names
)
{
    user_data = UserData<TSeq>(names);
    user_data.model = model;
}

template<typename TSeq>
inline void DataBase<TSeq>::add_user_data(
    std::vector< epiworld_double > x
)
{

    user_data.add(x);

}

template<typename TSeq>
inline void DataBase<TSeq>::add_user_data(
    unsigned int k,
    epiworld_double x
)
{

    user_data.add(k, x);

}

template<typename TSeq>
inline UserData<TSeq> & DataBase<TSeq>::get_user_data()
{
    return user_data;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/database-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/adjlist-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_ADJLIST_BONES_HPP
#define EPIWORLD_ADJLIST_BONES_HPP

class AdjList {
private:

    std::map<unsigned int,std::map<unsigned int, unsigned int>> dat;
    bool directed;
    unsigned int id_min,id_max;
    unsigned int N = 0;
    unsigned int E = 0;

public:

    AdjList() {};

    /**
     * @brief Construct a new Adj List object
     * 
     * @details 
     * It will create an adjacency list object with `maxid - minid + 1`
     * nodes. If min_id and max_id are not specified (both < 0), then the program will
     * try to figure them out automatically by looking at the range of the observed
     * ids.
     * 
     * @param source Unsigned int vector with the source
     * @param target Unsigned int vector with the target
     * @param directed Bool true if the network is directed
     * @param min_id int min id.
     * @param max_id int max id.
     */
    AdjList(
        const std::vector< unsigned int > & source,
        const std::vector< unsigned int > & target,
        bool directed,
        int min_id = -1,
        int max_id = -1
        );

    void read_edgelist(
        std::string fn,
        int skip = 0,
        bool directed = true,
        int min_id = -1,
        int max_id = -1
        );

    std::map<unsigned int, unsigned int> operator()(
        unsigned int i
        ) const;
        
    void print(unsigned int limit = 20u) const;
    unsigned int get_id_max() const;
    unsigned int get_id_min() const;
    size_t vcount() const;
    size_t ecount() const;
    
    std::map<unsigned int,std::map<unsigned int,unsigned int>> & get_dat() {
        return dat;
    };

    bool is_directed() const;

};


#endif

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/adjlist-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/adjlist-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_ADJLIST_MEAT_HPP
#define EPIWORLD_ADJLIST_MEAT_HPP

inline AdjList::AdjList(
    const std::vector< unsigned int > & source,
    const std::vector< unsigned int > & target,
    bool directed,
    int min_id,
    int max_id
) : directed(directed) {

    id_min = static_cast<unsigned int>(INT_MAX);
    id_max = 0u;

    int i,j;
    for (unsigned int m = 0; m < source.size(); ++m)
    {

        i = source[m];
        j = target[m];

        if ((max_id > 0) && (i > max_id))
            throw std::range_error(
                "The source["+std::to_string(m)+"] = " + std::to_string(i) +
                " is above the max_id " + std::to_string(max_id)
                );

        if ((min_id > 0) && (i < min_id))
            throw std::range_error(
                "The source["+std::to_string(m)+"] = " + std::to_string(i) + 
                " is below the min_id " + std::to_string(min_id)
                );

        if ((max_id > 0) && (j > max_id))
            throw std::range_error(
                "The target["+std::to_string(m)+"] = " + std::to_string(j) +
                " is above the max_id " + std::to_string(max_id)
                );

        if ((min_id > 0) && (j < min_id))
            throw std::range_error(
                "The target["+std::to_string(m)+"] = " + std::to_string(i) +
                " is below the min_id " + std::to_string(min_id)
                );

        // Adding nodes
        if (dat.find(i) == dat.end())
            dat[i].insert(std::pair<unsigned int, unsigned int>(j, 1u));
        else { // Or simply increasing the counter

            auto & dat_i = dat[i];
            if (dat_i.find(j) == dat_i.end())
                dat_i[j] = 1u;
            else
                dat_i[j]++;

        }

        if (dat.find(j) == dat.end())
            dat[j] = std::map<unsigned int, unsigned int>();            
        
        if (!directed)
        {

            if (dat[j].find(i) == dat[j].end())
            {
                dat[j][i] = 1u;
                
            } else
                dat[j][i]++;

        }

        // Recalculating the limits
        if (i < static_cast<int>(id_min))
            id_min = static_cast<unsigned int>(i);

        if (j < static_cast<int>(id_min))
            id_min = static_cast<unsigned int>(j);

        if (i > static_cast<int>(id_max))
            id_max = static_cast<unsigned int>(i);

        if (j > static_cast<int>(id_max))
            id_max = static_cast<unsigned int>(j);

        E++;

    }

    // Checking if the max found matches the max identified
    if (max_id >= 0)
        id_max = static_cast<unsigned int>(max_id);

    if (min_id >= 0)
        id_min = static_cast<unsigned int>(min_id);

    N = id_max - id_min + 1u;

    return;

}

inline void AdjList::read_edgelist(
    std::string fn,
    int skip,
    bool directed,
    int min_id,
    int max_id
) {

    int i,j;
    std::ifstream filei(fn);

    if (!filei)
        throw std::logic_error("The file " + fn + " was not found.");

    id_min = INT_MAX;
    id_max = INT_MIN;

    int linenum = 0;
    std::vector< unsigned int > source_;
    std::vector< unsigned int > target_;

    while (!filei.eof())
    {

        if (linenum < skip)
            continue;

        filei >> i >> j;

        // Looking for exceptions
        if (filei.bad())
            throw std::logic_error(
                "I/O error while reading the file " +
                fn
            );

        if (filei.fail())
            break;

        if ((max_id > 0) && (i > max_id))
            throw std::range_error(
                "The source["+std::to_string(linenum)+"] = " + std::to_string(i) +
                " is above the max_id " + std::to_string(max_id)
                );

        if ((min_id > 0) && (i < min_id))
            throw std::range_error(
                "The source["+std::to_string(linenum)+"] = " + std::to_string(i) + 
                " is below the min_id " + std::to_string(min_id)
                );

        if ((max_id > 0) && (j > max_id))
            throw std::range_error(
                "The target["+std::to_string(linenum)+"] = " + std::to_string(j) +
                " is above the max_id " + std::to_string(max_id)
                );

        if ((min_id > 0) && (j < min_id))
            throw std::range_error(
                "The target["+std::to_string(linenum)+"] = " + std::to_string(i) +
                " is below the min_id " + std::to_string(min_id)
                );

        source_.push_back(i);
        target_.push_back(j);

    }

    if (!filei.eof())
        throw std::logic_error(
            "Wrong format found in the AdjList file " +
            fn + " in line " + std::to_string(linenum)
        );
    
    // Now using the right constructor
    *this = AdjList(source_,target_,directed,min_id,max_id);

    return;

}

inline std::map<unsigned int,unsigned int> AdjList::operator()(
    unsigned int i
    ) const {

    if ((i < id_min) | (i > id_max))
        throw std::range_error(
            "The vertex id " + std::to_string(i) + " is not in the network."
            );

    if (dat.find(i) == dat.end())
        return std::map<unsigned int,unsigned int>();
    else
        return dat.find(i)->second;

}
inline void AdjList::print(unsigned int limit) const {


    unsigned int counter = 0;
    printf_epiworld("Nodeset:\n");
    for (auto & n : dat)
    {

        if (counter++ > limit)
            break;

        int n_neighbors = n.second.size();

        printf_epiworld("  % 3i: {", n.first);
        int niter = 0;
        for (auto n_n : n.second)
            if (++niter < n_neighbors)
            {    
                printf_epiworld("%i, ", n_n.first);
            }
            else {
                printf_epiworld("%i}\n", n_n.first);
            }
    }

    if (limit < dat.size())
    {
        printf_epiworld(
            "  (... skipping %i records ...)\n",
            static_cast<int>(dat.size() - limit)
            );
    }

}

inline unsigned int AdjList::get_id_max() const 
{
    return id_max;
}

inline unsigned int AdjList::get_id_min() const 
{
    return id_min;
}

inline size_t AdjList::vcount() const 
{
    return N;
}

inline size_t AdjList::ecount() const 
{
    return E;
}

inline bool AdjList::is_directed() const {

    if (dat.size() == 0u)
        throw std::logic_error("The edgelist is empty.");
    
    return directed;
    
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/adjlist-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/randgraph.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_RANDGRA
#define EPIWORLD_RANDGRA

template<typename TSeq>
class Model;

template<typename TSeq>
class Person;

class AdjList;


template<typename TSeq, typename TDat>
inline void rewire_degseq(
    TDat * persons,
    Model<TSeq> * model,
    epiworld_double proportion
    );

template<typename TSeq = bool>
inline void rewire_degseq(
    std::vector< Person<TSeq> > * persons,
    Model<TSeq> * model,
    epiworld_double proportion
    )
{

    // Identifying individuals with degree > 0
    std::vector< unsigned int > non_isolates;
    std::vector< epiworld_double > weights;
    epiworld_double nedges = 0.0;
    // std::vector< Person<TSeq> > * persons = model->get_population();
    for (unsigned int i = 0u; i < persons->size(); ++i)
    {
        if (persons->operator[](i).get_neighbors().size() > 0u)
        {
            non_isolates.push_back(i);
            epiworld_double wtemp = static_cast<epiworld_double>(persons->operator[](i).get_neighbors().size());
            weights.push_back(wtemp);
            nedges += wtemp;
        }
    }

    if (non_isolates.size() == 0u)
        throw std::logic_error("The graph is completely disconnected.");

    // Cumulative probs
    weights[0u] /= nedges;
    for (unsigned int i = 1u; i < non_isolates.size(); ++i)
    {
         weights[i] /= nedges;
         weights[i] += weights[i - 1u];
    }

    // Only swap if needed
    unsigned int N = non_isolates.size();
    epiworld_double prob;
    int nrewires = floor(proportion * nedges);
    while (nrewires-- > 0)
    {

        // Picking egos
        prob = model->runif();
        int id0 = N - 1;
        for (unsigned int i = 0u; i < N; ++i)
            if (prob <= weights[i])
            {
                id0 = i;
                break;
            }

        prob = model->runif();
        int id1 = N - 1;
        for (unsigned int i = 0u; i < N; ++i)
            if (prob <= weights[i])
            {
                id1 = i;
                break;
            }

        // Correcting for under or overflow.
        if (id1 == id0)
            id1++;

        if (id1 >= static_cast<int>(N))
            id1 = 0;

        Person<TSeq> & p0 = persons->operator[](non_isolates[id0]);
        Person<TSeq> & p1 = persons->operator[](non_isolates[id1]);

        // Picking alters (relative location in their lists)
        // In this case, these are uniformly distributed within the list
        int id01 = std::floor(p0.get_neighbors().size() * model->runif());
        int id11 = std::floor(p1.get_neighbors().size() * model->runif());

        // When rewiring, we need to flip the individuals from the other
        // end as well, since we are dealing withi an undirected graph
        
        // Finding what neighbour is id0
        if (!model->is_directed())
        {
            unsigned int n0,n1;
            Person<TSeq> & p01 = persons->operator[](p0.get_neighbors()[id01]->get_index());
            for (n0 = 0; n0 < p01.get_neighbors().size(); ++n0)
            {
                if (p0.get_id() == p01.get_neighbors()[n0]->get_id())
                    break;            
            }

            Person<TSeq> & p11 = persons->operator[](p1.get_neighbors()[id11]->get_index());
            for (n1 = 0; n1 < p11.get_neighbors().size(); ++n1)
            {
                if (p1.get_id() == p11.get_neighbors()[n1]->get_id())
                    break;            
            }

            std::swap(p01.get_neighbors()[n0], p11.get_neighbors()[n1]);    
            
        }

        // Moving alter first
        std::swap(p0.get_neighbors()[id01], p1.get_neighbors()[id11]);
        

    }

    return;

}

template<typename TSeq>
inline void rewire_degseq(
    AdjList * persons,
    Model<TSeq> * model,
    epiworld_double proportion
    )
{

    // Identifying individuals with degree > 0
    std::vector< unsigned int > non_isolates;
    std::vector< epiworld_double > weights;
    epiworld_double nedges = 0.0;
    // std::vector< Person<TSeq> > * persons = model->get_population();
    for (auto & p : persons->get_dat())
    {
        
        non_isolates.push_back(p.first);
        epiworld_double wtemp = static_cast<epiworld_double>(p.second.size());
        weights.push_back(wtemp);

        nedges += wtemp;

    }

    if (non_isolates.size() == 0u)
        throw std::logic_error("The graph is completely disconnected.");

    // Cumulative probs
    weights[0u] /= nedges;
    for (unsigned int i = 1u; i < non_isolates.size(); ++i)
    {
         weights[i] /= nedges;
         weights[i] += weights[i - 1u];
    }

    // Only swap if needed
    unsigned int N = non_isolates.size();
    epiworld_double prob;
    int nrewires = floor(proportion * nedges / (
        persons->is_directed() ? 1.0 : 2.0
    ));
    while (nrewires-- > 0)
    {

        // Picking egos
        prob = model->runif();
        int id0 = N - 1;
        for (unsigned int i = 0u; i < N; ++i)
            if (prob <= weights[i])
            {
                id0 = i;
                break;
            }

        prob = model->runif();
        int id1 = N - 1;
        for (unsigned int i = 0u; i < N; ++i)
            if (prob <= weights[i])
            {
                id1 = i;
                break;
            }

        // Correcting for under or overflow.
        if (id1 == id0)
            id1++;

        if (id1 >= static_cast<int>(N))
            id1 = 0;

        std::map<unsigned int,unsigned int> & p0 = persons->get_dat()[non_isolates[id0]];
        std::map<unsigned int,unsigned int> & p1 = persons->get_dat()[non_isolates[id1]];

        // Picking alters (relative location in their lists)
        // In this case, these are uniformly distributed within the list
        int id01 = std::floor(p0.size() * model->runif());
        int id11 = std::floor(p1.size() * model->runif());

        // Since it is a map, we need to find the actual ids (positions)
        // are not good enough.
        int count = 0;
        for (auto & n : p0)
            if (count++ == id01)
                id01 = n.first;

        count = 0;
        for (auto & n : p1)
            if (count++ == id11)
                id11 = n.first;

        // When rewiring, we need to flip the individuals from the other
        // end as well, since we are dealing withi an undirected graph
        
        // Finding what neighbour is id0
        if (!persons->is_directed())
        {

            std::map<unsigned int,unsigned int> & p01 = persons->get_dat()[id01];
            std::map<unsigned int,unsigned int> & p11 = persons->get_dat()[id11];

            std::swap(p01[id0], p11[id1]);
            
        }

        // Moving alter first
        std::swap(p0[id01], p1[id11]);

    }

    return;

}

template<typename TSeq>
inline AdjList rgraph_bernoulli(
    unsigned int n,
    epiworld_double p,
    bool directed,
    Model<TSeq> & model
) {

    std::vector< unsigned int > source;
    std::vector< unsigned int > target;

    // Checking the density (how many)
    std::binomial_distribution<> d(
        n * (n - 1.0) / (directed ? 1.0 : 2.0),
        p
    );

    unsigned int m = d(*model.get_rand_endgine());

    source.resize(m);
    target.resize(m);

    int a,b;
    for (unsigned int i = 0u; i < m; ++i)
    {
        a = floor(model.runif() * n);

        if (!directed)
            b = floor(model.runif() * a);
        else
        {
            b = floor(model.runif() * n);
            if (b == a)
                b++;
            
            if (b >= n)
                b = 0u;
        }

        source[i] = a;
        target[i] = b;

    }

    AdjList al(source, target, directed, 0, static_cast<int>(n) - 1);

    return al;
    
}

template<typename TSeq>
inline AdjList rgraph_bernoulli2(
    unsigned int n,
    epiworld_double p,
    bool directed,
    Model<TSeq> & model
) {

    std::vector< unsigned int > source;
    std::vector< unsigned int > target;

    // Checking the density (how many)
    std::binomial_distribution<> d(
        n * (n - 1.0) / (directed ? 1.0 : 2.0),
        p
    );

    // Need to compensate for the possible number of diagonal
    // elements sampled. If n * n, then each diag element has
    // 1/(n^2) chance of sampling

    unsigned int m = d(*model.get_rand_endgine());

    source.resize(m);
    target.resize(m);

    double n2 = static_cast<double>(n * n);

    int loc,row,col;
    for (unsigned int i = 0u; i < m; ++i)
    {
        loc = floor(model.runif() * n2);
        col = floor(static_cast<double>(loc)/static_cast<double>(n));
        row = loc - row * n;

        // Undirected needs to swap
        if (!directed && (col > row))
            std::swap(col, row);

        source[i] = row;
        target[i] = col;

    }

    AdjList al(source, target, directed, 0, static_cast<int>(n) - 1);

    return al;
    
}

inline AdjList rgraph_ring_lattice(
    unsigned int n,
    unsigned int k,
    bool directed = false
) {

    if ((n - 1u) < k)
        throw std::logic_error("k can be at most n - 1.");

    std::vector< unsigned int > source;
    std::vector< unsigned int > target;

    // if (!directed)
    //     if (k > 1u) k = static_cast< unsigned int >(floor(k / 2.0));

    for (unsigned int i = 0; i < n; ++i)
    {

        for (unsigned int j = 1u; j <= k; ++j)
        {

            // Next neighbor
            unsigned int l = i + j;
            if (l >= n) l = l - n;

            source.push_back(i);
            target.push_back(l);

        }

    }

    return AdjList(source, target, directed, 0u, n - 1);

}

template<typename TSeq>
inline AdjList rgraph_smallworld(
    unsigned int n,
    unsigned int k,
    epiworld_double p,
    bool directed,
    Model<TSeq> & model
) {

    // Creating the ring lattice
    AdjList ring = rgraph_ring_lattice(n,k,directed);
    
    // Rewiring and returning
    if (k > 0u)
        rewire_degseq(&ring, &model, p);
        
    return ring;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/randgraph.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/queue-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_QUEUE_BONES_HPP
#define EPIWORLD_QUEUE_BONES_HPP

/**
 * @brief Controls which agents are verified at each step
 * 
 * @details The idea is that only agents who are either in
 * an infected state or have an infected neighbor should be
 * checked. Otherwise it makes no sense (no chance to recover
 * or capture the disease).
 * 
 * @tparam TSeq 
 */
template<typename TSeq = bool>
class Queue
{

private:

    /**
     * @brief Count of ego's neighbors in queue (including ego)
     */
    std::vector< epiworld_fast_int > active;
    std::vector< epiworld_fast_int > active_next;
    Model<TSeq> * model = nullptr;

    // Auxiliary variable that checks how many steps
    // left are there
    // int n_steps_left;
    // bool queuing_started   = false;

public:

    void operator+=(Person<TSeq> * p);
    void operator-=(Person<TSeq> * p);
    epiworld_fast_int operator[](unsigned int i) const;

    // void initialize(Model<TSeq> * m, Person<TSeq> * p);
    void set_model(Model<TSeq> * m);

    // unsigned int size();
    
    void update();

};

template<typename TSeq>
inline void Queue<TSeq>::operator+=(Person<TSeq> * p)
{

    active_next[p->index]++;
    for (auto * n : p->neighbors)
        active_next[n->index]++;

}

template<typename TSeq>
inline void Queue<TSeq>::operator-=(Person<TSeq> * p)
{

    active_next[p->index]--;
    for (auto * n : p->neighbors)
        active_next[n->index]--;

}

template<typename TSeq>
inline void Queue<TSeq>::update()
{
    for (unsigned int i = 0u; i < active_next.size(); ++i)
    {
        active[i] += active_next[i];
        active_next[i] = 0;
    }

    EPI_DEBUG_ALL_NON_NEGATIVE(active);
}

template<typename TSeq>
inline epiworld_fast_int Queue<TSeq>::operator[](unsigned int i) const
{
    return active[i];
}


// template<typename TSeq>
// inline void Queue<TSeq>::initialize(Model<TSeq> * m, Person<TSeq> * p)
// {

//     // In the first step we need to set up everything    
//     if (!queuing_started)
//     {

//         model = m;
//         p     = &model->population[0u];

//         n_steps_left = m->size();
//         active.resize(n_steps_left, 0);
//         active_next.resize(n_steps_left, 0);

//         // The queuing is now running
//         queuing_started = true;

//     }

//     if (--n_steps_left < 0)
//     {
//         model->toggle_visited();
//         return;
//     }

    
//     // Visited marks whether we checked if it was infected or not
//     bool add_all = false;
//     if (!p->visited()) 
//     {

//         // We just visit the individual.
//         p->toggle_visited();

//         if (IN(p->get_status(), model->status_exposed))
//         {

//             // Increasing the number in the queue
//             active[p->index]++;

//             // For sure add all the neighbors, but still need to check
//             add_all = true;

//         }

//     }
//     else
//     {
//         return;
//     }
    
//     // Checking on the neighbors
//     for (auto * n : p->neighbors)
//     {

//         // Not in queue, but still need to add b/c p is infected.
//         // (if visited, no need to go again).
//         if (add_all)
//         {

//             // Since neighbor is infected
//             active[n->index]++;

//         }
        
//         // Now going inside
//         if (!n->visited())
//             initialize(model, n);

//     }

//     return;

// }

template<typename TSeq>
inline void Queue<TSeq>::set_model(Model<TSeq> * m)
{

    model = m;
    active.resize(m->size(), 0);
    active_next.resize(m->size(), 0);

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/queue-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/model-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


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
    unsigned int time_n = 0u;
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
        unsigned int * n_replicates = nullptr,
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
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/model-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/model-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_MEAT_HPP
#define EPIWORLD_MODEL_MEAT_HPP

#define CHECK_INIT() if (!initialized) \
        throw std::logic_error("Model not initialized.");

#define ADD_VIRUSES() \
    for (size_t v = 0u; v < virus_to_add.size(); ++v) \
    { \
        \
        Virus<TSeq> * virus   = virus_to_add[v]; \
        Person<TSeq> * person = virus_to_add_person[v]; \
        \
        /* Recording transmission */ \
        if (virus->get_host() != nullptr) \
            db.record_transmission( \
                virus->get_host()->get_id(),\
                person->get_id(),\
                virus->get_id()\
            );\
        \
        /*Accounting for the transmission */ \
        db.up_exposed(virus, person->status_next); \
        \
        /* Adding the virus */ \
        person->get_viruses().add_virus(person->status_next, *virus); \
        \
    } \
    virus_to_add.clear();virus_to_add_person.clear();

#define RM_VIRUSES() \
    for (auto v : virus_to_remove) \
    { \
        \
        if (IN(v->get_host()->get_status(), status_susceptible)) \
            v->post_recovery(); \
        \
        /* Accounting for the improve */ \
        db.down_exposed(v, v->get_host()->status); \
        \
        /* Removing the virus (THIS SHOULD BE DEACTIVATE INSTEAD) */ \
        v->get_host()->get_viruses().reset(); \
        \
    } \
    \
    virus_to_remove.clear(); 

#define UPDATE_QUEUE() \
    if (use_queuing) \
        queue.update(); 

#define UPDATE_STATUS() \
    for (auto & p : population) \
        p.status = p.status_next;

template<typename TSeq>
inline Model<TSeq>::Model(const Model<TSeq> & model) :
    db(model.db),
    viruses(model.viruses),
    prevalence_virus(model.prevalence_virus),
    tools(model.tools),
    prevalence_tool(model.prevalence_tool),
    engine(model.engine),
    runifd(model.runifd),
    parameters(model.parameters),
    ndays(model.ndays),
    pb(model.pb),
    status_susceptible(model.status_susceptible),
    status_susceptible_labels(model.status_susceptible_labels),
    status_exposed(model.status_exposed),
    status_exposed_labels(model.status_exposed_labels),
    status_removed(model.status_removed),
    status_removed_labels(model.status_removed_labels),
    nstatus(model.nstatus),
    baseline_status_susceptible(model.baseline_status_susceptible),
    baseline_status_exposed(model.baseline_status_exposed),
    baseline_status_removed(model.baseline_status_removed),
    verbose(model.verbose),
    initialized(model.initialized),
    current_date(model.current_date),
    global_action_functions(model.global_action_functions),
    global_action_dates(model.global_action_dates),
    visited_model(model.visited_model),
    queue(model.queue),
    use_queuing(model.use_queuing)
{

    // Pointing to the right place
    db.set_model(*this);

    // Removing old neighbors
    model.clone_population(
        population,
        population_ids,
        directed,
        this
        );

    // Figure out the queuing
    if (use_queuing)
        queue.set_model(this);

    // Finally, seeds are resetted automatically based on the original
    // engine
    seed(floor(runif() * UINT_MAX));

}

template<typename TSeq>
inline Model<TSeq>::Model(Model<TSeq> && model) :
    db(std::move(model.db)),
    viruses(std::move(model.viruses)),
    prevalence_virus(std::move(model.prevalence_virus)),
    tools(std::move(model.tools)),
    prevalence_tool(std::move(model.prevalence_tool)),
    engine(std::move(model.engine)),
    runifd(std::move(model.runifd)),
    parameters(std::move(model.parameters)),
    ndays(std::move(model.ndays)),
    pb(std::move(model.pb)),
    verbose(std::move(model.verbose)),
    initialized(std::move(model.initialized)),
    current_date(std::move(model.current_date)),
    population(std::move(model.population)),
    population_ids(std::move(model.population_ids)),
    directed(std::move(model.directed)),
    global_action_functions(std::move(model.global_action_functions)),
    global_action_dates(std::move(model.global_action_dates)),
    status_susceptible(std::move(model.status_susceptible)),
    status_susceptible_labels(std::move(model.status_susceptible_labels)),
    status_exposed(std::move(model.status_exposed)),
    status_exposed_labels(std::move(model.status_exposed_labels)),
    status_removed(std::move(model.status_removed)),
    status_removed_labels(std::move(model.status_removed_labels)),
    baseline_status_susceptible(model.baseline_status_susceptible),
    baseline_status_exposed(model.baseline_status_exposed),
    baseline_status_removed(model.baseline_status_removed),
    nstatus(model.nstatus),
    visited_model(model.visited_model),
    queue(std::move(model.queue)),
    use_queuing(model.use_queuing)
{

    // // Pointing to the right place
    // db.set_model(*this);

    // // Removing old neighbors
    // model.clone_population(
    //     population,
    //     population_ids,
    //     directed,
    //     this
    //     );

}

template<typename TSeq>
inline void Model<TSeq>::clone_population(
    std::vector< Person<TSeq> > & p,
    std::map<int,int> & p_ids,
    bool & d,
    Model<TSeq> * model
) const {

    // Copy and clean
    p     = population;
    p_ids = population_ids;
    d     = directed;

    for (auto & p: p)
        p.neighbors.clear();
    
    // Relinking individuals
    for (unsigned int i = 0u; i < size(); ++i)
    {
        // Making room
        const Person<TSeq> & person_this = population[i];
        Person<TSeq> & person_res        = p[i];

        // Person pointing to the right model and person
        if (model != nullptr)
            person_res.model        = model;
        person_res.viruses.host = &person_res;
        person_res.tools.person = &person_res;

        // Readding
        std::vector< Person<TSeq> * > neigh = person_this.neighbors;
        for (unsigned int n = 0u; n < neigh.size(); ++n)
        {
            // Point to the right neighbors
            int loc = p_ids[neigh[n]->get_id()];
            person_res.add_neighbor(&p[loc], true, true);

        }

    }
}

template<typename TSeq>
inline void Model<TSeq>::clone_population(const Model<TSeq> & m)
{
    m.clone_population(
        population,
        population_ids,
        directed,
        this
    );
}

template<typename TSeq>
inline DataBase<TSeq> & Model<TSeq>::get_db()
{
    return db;
}

template<typename TSeq>
inline std::vector<Person<TSeq>> * Model<TSeq>::get_population()
{
    return &population;
}

template<typename TSeq>
inline void Model<TSeq>::pop_from_random(
    unsigned int n,
    unsigned int k,
    bool d,
    epiworld_double p
)
{
    pop_from_adjlist(
        rgraph_smallworld(n, k, p, d, *this)
    );
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_engine(std::mt19937 & eng)
{
    engine = std::make_shared< std::mt19937 >(eng);
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_gamma(epiworld_double alpha, epiworld_double beta)
{
    rgammad = std::make_shared<std::gamma_distribution<>>(alpha,beta);
}

template<typename TSeq>
inline epiworld_double & Model<TSeq>::operator()(std::string pname) {

    if (parameters.find(pname) == parameters.end())
        throw std::range_error("The parameter "+ pname + "is not in the model.");

    return parameters[pname];

}

template<typename TSeq>
inline size_t Model<TSeq>::size() const {
    return population.size();
}

template<typename TSeq>
inline void Model<TSeq>::init(
    unsigned int ndays,
    unsigned int seed
    ) {

    if (initialized) 
        throw std::logic_error("Model already initialized.");

    // Setting up the number of steps
    this->ndays = ndays;

    // Initializing population
    for (auto & p : population)
    {
        p.model = this;
        p.init(baseline_status_susceptible);
    }

    engine->seed(seed);
    array_double_tmp.resize(size()/2, 0.0);
    array_virus_tmp.resize(size());

    initialized = true;

    queue.set_model(this);

    // Starting first infection and tools
    reset();



}

template<typename TSeq>
inline void Model<TSeq>::dist_virus()
{


    // Starting first infection
    int n = size();
    for (unsigned int v = 0; v < viruses.size(); ++v)
    {
        // Picking how many
        int nsampled;
        if (prevalence_virus_as_proportion[v])
        {
            nsampled = static_cast<int>(std::floor(prevalence_virus[v] * size()));
        }
        else
        {
            nsampled = static_cast<int>(prevalence_virus[v]);
        }

        if (nsampled > static_cast<int>(size()))
            throw std::range_error("There are only " + std::to_string(size()) + 
            " individuals in the population. Cannot add the virus to " + std::to_string(nsampled));

        std::vector < bool > sampled(size(), false);
        while (nsampled > 0)
        {


            int loc = static_cast<unsigned int>(floor(runif() * n));

            if (sampled[loc])
                continue;

            sampled[loc] = true;

            Person<TSeq> & person = population[loc];
            
            person.add_virus(&viruses[v]);
            person.status_next = baseline_status_exposed;
            db.state_change(person.status, baseline_status_exposed);

            nsampled--;

        }
    }      

    // Adding the next viruses
    ADD_VIRUSES()

    // Removing and deactivating viruses
    RM_VIRUSES()

    // Updating the queuing sequence
    UPDATE_QUEUE()

    // Moving to the next assigned status
    UPDATE_STATUS()

}

template<typename TSeq>
inline void Model<TSeq>::dist_tools()
{

    // Starting first infection
    int n = size();
    for (unsigned int t = 0; t < tools.size(); ++t)
    {
        // Picking how many
        int nsampled;
        if (prevalence_tool_as_proportion[t])
        {
            nsampled = static_cast<int>(std::floor(prevalence_tool[t] * size()));
        }
        else
        {
            nsampled = static_cast<int>(prevalence_tool[t]);
        }

        if (nsampled > static_cast<int>(size()))
            throw std::range_error("There are only " + std::to_string(size()) + 
            " individuals in the population. Cannot add the tool to " + std::to_string(nsampled));
        
        while (nsampled > 0)
        {
            int loc = static_cast<unsigned int>(floor(runif() * n));
            if (population[loc].has_tool(tools[t].get_id()))
                continue;
            
            population[loc].add_tool(today(), tools[t]);
            nsampled--;

        }
    }

}

template<typename TSeq>
inline void Model<TSeq>::chrono_start() {
    time_start = std::chrono::steady_clock::now();
}

template<typename TSeq>
inline void Model<TSeq>::chrono_end() {
    time_end = std::chrono::steady_clock::now();
    time_elapsed += (time_end - time_start);
    time_n++;
}

template<typename TSeq>
inline void Model<TSeq>::set_backup()
{

    backup = std::unique_ptr<Model<TSeq>>(new Model<TSeq>(*this));

}

template<typename TSeq>
inline void Model<TSeq>::restore_backup()
{

    if (backup != nullptr)
    {

        clone_population(*backup);

        db = backup->db;
        db.set_model(*this);

    }

}

template<typename TSeq>
inline std::mt19937 * Model<TSeq>::get_rand_endgine()
{
    return engine.get();
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::runif() {
    // CHECK_INIT()
    return runifd->operator()(*engine);
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rnorm() {
    // CHECK_INIT()
    return (rnormd->operator()(*engine));
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rnorm(epiworld_double mean, epiworld_double sd) {
    // CHECK_INIT()
    return (rnormd->operator()(*engine)) * sd + mean;
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rgamma() {
    return rgammad->operator()(*engine);
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rgamma(epiworld_double alpha, epiworld_double beta) {
    auto old_param = rgammad->param();
    rgammad->param(std::gamma_distribution<>::param_type(alpha, beta));
    epiworld_double ans = rgammad->operator()(*engine);
    rgammad->param(old_param);
    return ans;
}

template<typename TSeq>
inline void Model<TSeq>::seed(unsigned int s) {
    this->engine->seed(s);
}

template<typename TSeq>
inline void Model<TSeq>::add_virus(Virus<TSeq> v, epiworld_double preval)
{

    if (preval > 1.0)
        throw std::range_error("Prevalence of virus cannot be above 1.0");

    if (preval < 0.0)
        throw std::range_error("Prevalence of virus cannot be negative");

    // Setting the id
    v.set_id(viruses.size());
    
    // Adding new virus
    viruses.push_back(v);
    prevalence_virus.push_back(preval);
    prevalence_virus_as_proportion.push_back(true);

}

template<typename TSeq>
inline void Model<TSeq>::add_virus_n(Virus<TSeq> v, unsigned int preval)
{

    // Setting the id
    v.set_id(viruses.size());

    // Adding new virus
    viruses.push_back(v);
    prevalence_virus.push_back(preval);
    prevalence_virus_as_proportion.push_back(false);

}

template<typename TSeq>
inline void Model<TSeq>::add_tool(Tool<TSeq> t, epiworld_double preval)
{

    if (preval > 1.0)
        throw std::range_error("Prevalence of tool cannot be above 1.0");

    if (preval < 0.0)
        throw std::range_error("Prevalence of tool cannot be negative");

    t.id = tools.size();
    tools.push_back(t);
    prevalence_tool.push_back(preval);
    prevalence_tool_as_proportion.push_back(true);

}

template<typename TSeq>
inline void Model<TSeq>::add_tool_n(Tool<TSeq> t, unsigned int preval)
{
    t.id = tools.size();
    tools.push_back(t);
    prevalence_tool.push_back(preval);
    prevalence_tool_as_proportion.push_back(false);
}

template<typename TSeq>
inline void Model<TSeq>::pop_from_adjlist(
    std::string fn,
    int skip,
    bool directed,
    int min_id,
    int max_id
    ) {

    AdjList al;
    al.read_edgelist(fn, skip, directed, min_id, max_id);
    this->pop_from_adjlist(al);

}

template<typename TSeq>
inline void Model<TSeq>::pop_from_adjlist(AdjList al) {

    // Resizing the people
    population.clear();
    population_ids.clear();
    population.resize(al.vcount(), Person<TSeq>());

    const auto & tmpdat = al.get_dat();
    
    int loc;
    for (const auto & n : tmpdat)
    {
        if (population_ids.find(n.first) == population_ids.end())
            population_ids[n.first] = population_ids.size();

        loc = population_ids[n.first];

        population[loc].model = this;
        population[loc].id    = n.first;
        population[loc].index = loc;

        for (const auto & link: n.second)
        {

            if (population_ids.find(link.first) == population_ids.end())
                population_ids[link.first] = population_ids.size();

            unsigned int loc_link   = population_ids[link.first];
            population[loc_link].id    = link.first;
            population[loc_link].index = loc_link;

            population[loc].add_neighbor(
                &population[population_ids[link.first]],
                true, true
                );

        }

    }

}

template<typename TSeq>
inline bool Model<TSeq>::is_directed() const
{
    if (population.size() == 0u)
        throw std::logic_error("The population hasn't been initialized.");

    return directed;
}

template<typename TSeq>
inline int Model<TSeq>::today() const {
    return this->current_date;
}

template<typename TSeq>
inline void Model<TSeq>::next() {

    ++this->current_date;
    db.record();
    
    // Advicing the progress bar
    if (verbose)
        pb.next();

    return ;
}

template<typename TSeq>
inline void Model<TSeq>::run() 
{

    // Initializing the simulation
    chrono_start();
    EPIWORLD_RUN((*this))
    {

        // We can execute these components in whatever order the
        // user needs.
        this->update_status();
        this->mutate_variant();
        this->next();

        this->run_global_actions();

        // In this case we are applying degree sequence rewiring
        // to change the network just a bit.
        this->rewire();

    }
    chrono_end();

}

template<typename TSeq>
inline void Model<TSeq>::run_multiple(
    unsigned int nexperiments,
    std::function<void(Model<TSeq>*)> fun,
    bool reset,
    bool verbose
)
{

    if (reset)
        set_backup();

    bool old_verb = this->verbose;
    verbose_off();

    Progress pb_multiple(
        nexperiments,
        EPIWORLD_PROGRESS_BAR_WIDTH
        )
        ;
    if (verbose)
    {

        printf_epiworld(
            "Starting multiple runs (%i)\n", 
            static_cast<int>(nexperiments)
        );

        pb_multiple.start();

    }

    for (unsigned int n = 0u; n < nexperiments; ++n)
    {
        
        run();

        fun(this);

        if (n < (nexperiments - 1u) && reset)
            this->reset();

        if (verbose)
            pb_multiple.next();
    
    }

    if (verbose)
        pb_multiple.end();

    if (old_verb)
        verbose_on();

    return;

}

template<typename TSeq>
inline void Model<TSeq>::update_status() {

    // Next status
    if (use_queuing)
    {
        
        for (unsigned int p = 0u; p < size(); ++p)
            if (queue[p] > 0)
                population[p].update_status();            

    }
    else
    {

        for (auto & p: population)
            p.update_status();

    }
    
    // Adding the next viruses
    ADD_VIRUSES()

    // Removing and deactivating viruses
    RM_VIRUSES()

    // Updating the queuing sequence
    UPDATE_QUEUE()

    // Moving to the next assigned status
    UPDATE_STATUS()

}

template<typename TSeq>
inline void Model<TSeq>::mutate_variant() {

    for (auto & p: population)
    {
        if (IN(p.get_status(), status_exposed))
            p.mutate_variant();

    }

}

template<typename TSeq>
inline void Model<TSeq>::record_variant(Virus<TSeq> * v) {

    // Updating registry
    db.record_variant(v);
    return;
    
} 

template<typename TSeq>
inline int Model<TSeq>::get_nvariants() const {
    return db.size();
}

template<typename TSeq>
inline unsigned int Model<TSeq>::get_ndays() const {
    return ndays;
}

template<typename TSeq>
inline void Model<TSeq>::set_ndays(unsigned int ndays) {
    this->ndays = ndays;
}

template<typename TSeq>
inline bool Model<TSeq>::get_verbose() const {
    return verbose;
}

template<typename TSeq>
inline void Model<TSeq>::verbose_on() {
    verbose = true;
}

template<typename TSeq>
inline void Model<TSeq>::verbose_off() {
    verbose = false;
}

template<typename TSeq>
inline void Model<TSeq>::set_rewire_fun(
    std::function<void(std::vector<Person<TSeq>>*,Model<TSeq>*,epiworld_double)> fun
    ) {
    rewire_fun = fun;
}

template<typename TSeq>
inline void Model<TSeq>::set_rewire_prop(epiworld_double prop)
{

    if (prop < 0.0)
        throw std::range_error("Proportions cannot be negative.");

    if (prop > 1.0)
        throw std::range_error("Proportions cannot be above 1.0.");

    rewire_prop = prop;
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::get_rewire_prop() const {
    return rewire_prop;
}

template<typename TSeq>
inline void Model<TSeq>::rewire() {

    if (rewire_fun)
        rewire_fun(&population, this, rewire_prop);
}


template<typename TSeq>
inline void Model<TSeq>::write_data(
    std::string fn_variant_info,
    std::string fn_variant_hist,
    std::string fn_total_hist,
    std::string fn_transmission,
    std::string fn_transition
    ) const
{

    db.write_data(fn_variant_info,fn_variant_hist,fn_total_hist,fn_transmission,fn_transition);

}

template<typename TSeq>
inline void Model<TSeq>::write_edgelist(
    std::string fn
    ) const
{



    std::ofstream efile(fn, std::ios_base::out);
    efile << "source target\n";
    for (const auto & p : population)
    {
        for (auto & n : p.neighbors)
            efile << p.id << " " << n->id << "\n";
    }



}

template<typename TSeq>
inline std::map<std::string,epiworld_double> & Model<TSeq>::params()
{
    return parameters;
}

template<typename TSeq>
inline void Model<TSeq>::reset() {
    
    // Restablishing people
    pb = Progress(ndays, 80);

    if (backup != nullptr)
    {
        backup->clone_population(
            population,
            population_ids,
            directed,
            this
        );
    }

    for (auto & p : population)
    {
        p.reset();
        
        if (update_susceptible)
            p.set_update_susceptible(update_susceptible);
        else if (!p.update_susceptible)
            throw std::logic_error("No update_susceptible function set.");
        if (update_exposed)
            p.set_update_exposed(update_exposed);
        else if (!p.update_exposed)
            throw std::logic_error("No update_exposed function set.");
        if (update_removed)
            p.set_update_removed(update_removed);
        
    }
    
    current_date = 0;

    db.set_model(*this);

    // Recording variants
    for (Virus<TSeq> & v : viruses)
        record_variant(&v);

    if (use_queuing)
        queue.set_model(this);

    // Re distributing tools and virus
    dist_virus();
    dist_tools();

    // Recording the original state
    db.record();

}

// Too big to keep here
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//model-meat-print.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_MEAT_VISITED_HPP
#define EPIWORLD_MODEL_MEAT_VISITED_HPP

template<typename TSeq>
inline void Model<TSeq>::print() const
{

    // Horizontal line
    std::string line = "";
    for (unsigned int i = 0u; i < 80u; ++i)
        line += "_";

    // Prints a message if debugging is on
    EPI_DEBUG_NOTIFY_ACTIVE()

    printf_epiworld("\n%s\n%s\n\n",line.c_str(), "SIMULATION STUDY");
    printf_epiworld("Population size    : %i\n", static_cast<int>(size()));
    printf_epiworld("Days (duration)    : %i (of %i)\n", today(), ndays);
    printf_epiworld("Number of variants : %i\n", static_cast<int>(db.get_nvariants()));
    if (time_n > 0u)
    {
        std::string abbr;
        epiworld_double elapsed;
        epiworld_double total;
        get_elapsed("auto", &elapsed, &total, nullptr, &abbr, false);
        printf_epiworld("Last run elapsed t : %.2f%s\n", elapsed, abbr.c_str());
        if (time_n > 1u)
        {
            printf_epiworld("Total elapsed t    : %.2f%s (%i runs)\n", total, abbr.c_str(), time_n);
        }

    } else {
        printf_epiworld("Last run elapsed t : -\n");
    }
    
    if (rewire_fun)
    {
        printf_epiworld("Rewiring           : on (%.2f)\n\n", rewire_prop);
    } else {
        printf_epiworld("Rewiring           : off\n\n");
    }

    printf_epiworld("Virus(es):\n");
    int i = 0;
    for (auto & v : viruses)
    {    

        if (prevalence_virus_as_proportion[i])
        {

            printf_epiworld(
                " - %s (baseline prevalence: %.2f%%)\n",
                v.get_name().c_str(),
                prevalence_virus[i++] * 100.00
            );

        }
        else
        {

            printf_epiworld(
                " - %s (baseline prevalence: %i seeds)\n",
                v.get_name().c_str(),
                static_cast<int>(prevalence_virus[i++])
            );

        }

    }

    printf_epiworld("Tool(s):\n");
    i = 0;
    for (auto & t : tools)
    {   

        if (prevalence_tool_as_proportion[i])
        {

            printf_epiworld(
                " - %s (baseline prevalence: %.2f%%)\n",
                t.get_name().c_str(),
                prevalence_tool[i++] * 100.0
                );

        }
        else
        {

            printf_epiworld(
                " - %s (baseline prevalence: %i seeds)\n",
                t.get_name().c_str(),
                static_cast<int>(prevalence_tool[i++])
                );

        }
        

    }

    // Information about the parameters included
    printf_epiworld("\nModel parameters:\n");
    unsigned int nchar = 0u;
    for (auto & p : parameters)
        if (p.first.length() > nchar)
            nchar = p.first.length();

    std::string fmt = " - %-" + std::to_string(nchar + 1) + "s: ";
    for (auto & p : parameters)
    {
        std::string fmt_tmp = fmt;
        if (std::fabs(p.second) < 0.0001)
            fmt_tmp += "%.1e\n";
        else
            fmt_tmp += "%.4f\n";

        printf_epiworld(
            fmt_tmp.c_str(),
            p.first.c_str(),
            p.second
        );
        
    }


    nchar = 0u;
    for (auto & p : status_susceptible_labels)
        if (p.length() > nchar)
            nchar = p.length();
    
    for (auto & p : status_exposed_labels)
        if (p.length() > nchar)
            nchar = p.length();

    for (auto & p : status_removed_labels)
        if (p.length() > nchar)
            nchar = p.length();

    if (initialized) 
    {
        
        if (today() != 0)
            fmt = " - Total %-" + std::to_string(nchar + 1 + 4) + "s: %7i -> %i\n";
        else
            fmt = " - Total %-" + std::to_string(nchar + 1 + 4) + "s: %i\n";

    }
    else
        fmt = " - Total %-" + std::to_string(nchar + 1 + 4) + "s: %s\n";
        
    printf_epiworld("\nDistribution of the population at time %i:\n", today());
    for (unsigned int s = 0u; s < status_susceptible.size(); ++s)
    {
        if (initialized)
        {
            
            if (today() != 0)
            {

                printf_epiworld(
                    fmt.c_str(),
                    (status_susceptible_labels[s] + " (S)").c_str(),
                    db.hist_total_counts[status_susceptible[s]],
                    db.today_total[ status_susceptible[s] ]
                    );

            }
            else
            {

                printf_epiworld(
                    fmt.c_str(),
                    (status_susceptible_labels[s] + " (S)").c_str(),
                    db.today_total[ status_susceptible[s] ]
                    );

            }
            

        }
        else
        {

            printf_epiworld(
                fmt.c_str(),
                (status_susceptible_labels[s] + " (S)").c_str(),
                " - "
                );

        }
    }

    for (unsigned int s = 0u; s < status_exposed.size(); ++s)
    {
        if (initialized)
        {
            
            if (today() != 0)
            {
                printf_epiworld(
                    fmt.c_str(),
                    (status_exposed_labels[s] + " (E)").c_str(),
                    db.hist_total_counts[ status_exposed[s] ],
                    db.today_total[ status_exposed[s] ]
                    );
            }
            else
            {
                printf_epiworld(
                    fmt.c_str(),
                    (status_exposed_labels[s] + " (E)").c_str(),
                    db.today_total[ status_exposed[s] ]
                    );
            }
            

        } else {
            printf_epiworld(
                fmt.c_str(),
                (status_exposed_labels[s] + " (E)").c_str(),
                " - "
                );
        }
    }

    // printf_epiworld("\nStatistics (removed):\n");
    for (unsigned int s = 0u; s < status_removed.size(); ++s)
    {
        if (initialized)
        {
            
            if (today() != 0)
            {
                printf_epiworld(
                    fmt.c_str(),
                    (status_removed_labels[s] + " (R)").c_str(),
                    db.hist_total_counts[ status_removed[s] ],
                    db.today_total[ status_removed[s] ]
                    );
            }
            else
            {
                printf_epiworld(
                    fmt.c_str(),
                    (status_removed_labels[s] + " (R)").c_str(),
                    db.today_total[ status_removed[s] ]
                    );
            }
            

        } else {
            printf_epiworld(
                fmt.c_str(),
                (status_removed_labels[s] + " (R)").c_str(),
                " - "
                );
        }
    }
    
    printf_epiworld(
        "\n(S): Susceptible, (E): Exposed, (R): Removed\n%s\n\n",
        line.c_str()
        );

    return;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//model-meat-print.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



template<typename TSeq>
inline Model<TSeq> && Model<TSeq>::clone() const {

    // Step 1: Regen the individuals and make sure that:
    //  - Neighbors point to the right place
    //  - DB is pointing to the right place
    Model<TSeq> res(*this);

    // Pointing to the right place
    res.get_db().set_model(res);

    // Removing old neighbors
    for (auto & p: res.population)
        p.neighbors.clear();
    
    // Rechecking individuals
    for (unsigned int p = 0u; p < size(); ++p)
    {
        // Making room
        const Person<TSeq> & person_this = population[p];
        Person<TSeq> & person_res  = res.population[p];

        // Person pointing to the right model and person
        person_res.model        = &res;
        person_res.viruses.host = &person_res;
        person_res.tools.person = &person_res;

        // Readding
        std::vector< Person<TSeq> * > neigh = person_this.neighbors;
        for (unsigned int n = 0u; n < neigh.size(); ++n)
        {
            // Point to the right neighbors
            int loc = res.population_ids[neigh[n]->get_id()];
            person_res.add_neighbor(&res.population[loc], true, true);

        }

    }

    return res;

}

#define EPIWORLD_CHECK_STATUS(a, b) \
    for (auto & i : b) \
        if (a == i) \
            throw std::logic_error("The status " + std::to_string(i) + " already exists."); 
#define EPIWORLD_CHECK_ALL_STATUSES(a) \
    EPIWORLD_CHECK_STATUS(a, status_susceptible) \
    EPIWORLD_CHECK_STATUS(a, status_exposed) \
    EPIWORLD_CHECK_STATUS(a, status_removed)

template<typename TSeq>
inline void Model<TSeq>::add_status_susceptible(
    epiworld_fast_uint s,
    std::string lab
)
{

    EPIWORLD_CHECK_ALL_STATUSES(s)
    status_susceptible.push_back(s);
    status_susceptible_labels.push_back(lab);
    nstatus++;

}

template<typename TSeq>
inline void Model<TSeq>::add_status_exposed(
    epiworld_fast_uint s,
    std::string lab
)
{

    EPIWORLD_CHECK_ALL_STATUSES(s)
    status_exposed.push_back(s);
    status_exposed_labels.push_back(lab);
    nstatus++;

}

template<typename TSeq>
inline void Model<TSeq>::add_status_removed(
    epiworld_fast_uint s,
    std::string lab
)
{

    EPIWORLD_CHECK_ALL_STATUSES(s)
    status_removed.push_back(s);
    status_removed_labels.push_back(lab);
    nstatus++;

}

template<typename TSeq>
inline void Model<TSeq>::add_status_susceptible(std::string lab)
{
    status_susceptible.push_back(nstatus++);
    status_susceptible_labels.push_back(lab);
}

template<typename TSeq>
inline void Model<TSeq>::add_status_exposed(std::string lab)
{
    status_exposed.push_back(nstatus++);
    status_exposed_labels.push_back(lab);
}

template<typename TSeq>
inline void Model<TSeq>::add_status_removed(std::string lab)
{
    status_removed.push_back(nstatus++);
    status_removed_labels.push_back(lab);
}

#define EPIWORLD_COLLECT_STATUSES(out,id,lab) \
    std::vector< std::pair<epiworld_fast_uint, std::string> > out; \
    for (unsigned int i = 0; i < id.size(); ++i) \
        out.push_back( \
            std::pair<int,std::string>( \
                id[i], lab[i] \
            ) \
        );

template<typename TSeq>
inline const std::vector< epiworld_fast_uint > &
Model<TSeq>::get_status_susceptible() const
{
    return status_susceptible;
}

template<typename TSeq>
inline const std::vector< epiworld_fast_uint > &
Model<TSeq>::get_status_exposed() const
{
    return status_exposed;
}

template<typename TSeq>
inline const std::vector< epiworld_fast_uint > &
Model<TSeq>::get_status_removed() const
{
    return status_removed;
}

template<typename TSeq>
inline const std::vector< std::string > &
Model<TSeq>::get_status_susceptible_labels() const
{
    return status_susceptible_labels;
}

template<typename TSeq>
inline const std::vector< std::string > &
Model<TSeq>::get_status_exposed_labels() const
{
    return status_exposed_labels;
}

template<typename TSeq>
inline const std::vector< std::string > &
Model<TSeq>::get_status_removed_labels() const
{
    return status_removed_labels;
}

template<typename TSeq>
inline void Model<TSeq>::reset_status_codes(
    std::vector< epiworld_fast_uint > codes,
    std::vector< std::string > names,
    bool verbose
)
{

    if (codes.size() != 3u)
        throw std::length_error("The vector of codes should be of length 3.");

    if (names.size() != 3u)
        throw std::length_error("The vector of names should be of length 3.");

    status_susceptible.clear();
    status_susceptible_labels.clear();
    status_exposed.clear();
    status_exposed_labels.clear();
    status_removed.clear();
    status_removed_labels.clear();
    nstatus = 0u;

    baseline_status_susceptible = codes[0u];
    baseline_status_exposed     = codes[1u];
    baseline_status_removed     = codes[2u];

    add_status_susceptible(codes[0u], names[0u]);
    add_status_exposed(codes[1u], names[1u]);
    add_status_removed(codes[2u], names[2u]);

    if (verbose)
        print_status_codes();    


}

template<typename TSeq>
inline void Model<TSeq>::print_status_codes() const
{

    // Horizontal line
    std::string line = "";
    for (unsigned int i = 0u; i < 80u; ++i)
        line += "_";

    printf_epiworld("\n%s\nDEFAULT STATUS CODES\n\n", line.c_str());

    unsigned int nchar = 0u;
    for (auto & p : status_susceptible_labels)
        if (p.length() > nchar)
            nchar = p.length();
    
    for (auto & p : status_exposed_labels)
        if (p.length() > nchar)
            nchar = p.length();

    for (auto & p : status_removed_labels)
        if (p.length() > nchar)
            nchar = p.length();

    std::string fmt = " %2i = %-" + std::to_string(nchar + 1 + 4) + "s %s\n";
    for (unsigned int i = 0u; i < status_susceptible.size(); ++i)
    {

        printf_epiworld(
            fmt.c_str(),
            status_susceptible[i],
            (status_susceptible_labels[i] + " (S)").c_str(),
            (status_susceptible[i] == baseline_status_susceptible) ? " *" : ""
        );

    }

    for (unsigned int i = 0u; i < status_exposed.size(); ++i)
    {

        printf_epiworld(
            fmt.c_str(),
            status_exposed[i],
            (status_exposed_labels[i] + " (E)").c_str(),
            status_exposed[i] == baseline_status_exposed ? " *" : ""
        );

    }

    for (unsigned int i = 0u; i < status_removed.size(); ++i)
    {

        printf_epiworld(
            fmt.c_str(),
            status_removed[i],
            (status_removed_labels[i] + " (R)").c_str(),
            status_removed[i] == baseline_status_removed ? " *" : ""
        );

    }

    printf_epiworld(
        "\n(S): Susceptible, (E): Exposed, (R): Removed\n * : Baseline status (default)\n%s\n\n",
        line.c_str()
        );


}

template<typename TSeq>
inline epiworld_fast_uint Model<TSeq>::get_default_susceptible() const
{
    return baseline_status_susceptible;
}

template<typename TSeq>
inline epiworld_fast_uint Model<TSeq>::get_default_exposed() const
{
    return baseline_status_exposed;
}

template<typename TSeq>
inline epiworld_fast_uint Model<TSeq>::get_default_removed() const
{
    return baseline_status_removed;
}


#define CASE_PAR(a,b) case a: b = &(parameters[pname]);break;
#define CASES_PAR(a) \
    switch (a) \
    { \
    CASE_PAR(0u, p0) CASE_PAR(1u, p1) CASE_PAR(2u, p2) CASE_PAR(3u, p3) \
    CASE_PAR(4u, p4) CASE_PAR(5u, p5) CASE_PAR(6u, p6) CASE_PAR(7u, p7) \
    CASE_PAR(8u, p8) CASE_PAR(9u, p9) \
    CASE_PAR(10u, p10) CASE_PAR(11u, p11) CASE_PAR(12u, p12) CASE_PAR(13u, p13) \
    CASE_PAR(14u, p14) CASE_PAR(15u, p15) CASE_PAR(16u, p16) CASE_PAR(17u, p17) \
    CASE_PAR(18u, p18) CASE_PAR(19u, p19) \
    CASE_PAR(20u, p20) CASE_PAR(21u, p21) CASE_PAR(22u, p22) CASE_PAR(23u, p23) \
    CASE_PAR(24u, p24) CASE_PAR(25u, p25) CASE_PAR(26u, p26) CASE_PAR(27u, p27) \
    CASE_PAR(28u, p28) CASE_PAR(29u, p29) \
    CASE_PAR(30u, p30) CASE_PAR(31u, p31) CASE_PAR(32u, p22) CASE_PAR(33u, p23) \
    CASE_PAR(34u, p34) CASE_PAR(35u, p35) CASE_PAR(36u, p26) CASE_PAR(37u, p27) \
    CASE_PAR(38u, p38) CASE_PAR(39u, p39) \
    default: \
        break; \
    }

template<typename TSeq>
inline epiworld_double Model<TSeq>::add_param(
    epiworld_double initial_value,
    std::string pname
    ) {

    if (parameters.find(pname) == parameters.end())
        parameters[pname] = initial_value;

    CASES_PAR(npar_used++)
    
    return initial_value;

}

template<typename TSeq>
inline epiworld_double Model<TSeq>::set_param(
    std::string pname
    ) {

    if (parameters.find(pname) == parameters.end())
        throw std::logic_error("The parameter " + pname + " does not exists.");

    CASES_PAR(npar_used++)

    return parameters[pname];
    
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::get_param(std::string pname)
{
    if (parameters.find(pname) == parameters.end())
        throw std::logic_error("The parameter " + pname + " does not exists.");

    return parameters[pname];
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::par(std::string pname)
{
    return parameters[pname];
}

#define DURCAST(tunit,txtunit) {\
        elapsed       = std::chrono::duration_cast<std::chrono:: tunit>(\
            time_end - time_start).count(); \
        elapsed_total = std::chrono::duration_cast<std::chrono:: tunit>(time_elapsed).count(); \
        abbr_unit     = txtunit;}

template<typename TSeq>
inline void Model<TSeq>::get_elapsed(
    std::string unit,
    epiworld_double * last_elapsed,
    epiworld_double * total_elapsed,
    unsigned int * n_replicates,
    std::string * unit_abbr,
    bool print
) const {

    // Preparing the result
    epiworld_double elapsed, elapsed_total;
    std::string abbr_unit;

    // Figuring out the length
    if (unit == "auto")
    {

        size_t tlength = std::to_string(
            static_cast<int>(floor(time_elapsed.count()))
            ).length();
        
        if (tlength <= 1)
            unit = "nanoseconds";
        else if (tlength <= 3)
            unit = "microseconds";
        else if (tlength <= 6)
            unit = "milliseconds";
        else if (tlength <= 8)
            unit = "seconds";
        else if (tlength <= 9)
            unit = "minutes";
        else 
            unit = "hours";

    }

    if (unit == "nanoseconds")       DURCAST(nanoseconds,"ns")
    else if (unit == "microseconds") DURCAST(microseconds,"\xC2\xB5s")
    else if (unit == "milliseconds") DURCAST(milliseconds,"ms")
    else if (unit == "seconds")      DURCAST(seconds,"s")
    else if (unit == "minutes")      DURCAST(minutes,"m")
    else if (unit == "hours")        DURCAST(hours,"h")
    else
        throw std::range_error("The time unit " + unit + " is not supported.");


    if (last_elapsed != nullptr)
        *last_elapsed = elapsed;
    if (total_elapsed != nullptr)
        *total_elapsed = elapsed_total;
    if (n_replicates != nullptr)
        *n_replicates = time_n;
    if (unit_abbr != nullptr)
        *unit_abbr = abbr_unit;

    if (!print)
        return;

    if (time_n > 1u)
    {
        printf_epiworld("last run elapsed time : %.2f%s\n",
            elapsed, abbr_unit.c_str());
        printf_epiworld("total elapsed time    : %.2f%s\n",
            elapsed_total, abbr_unit.c_str());
        printf_epiworld("total runs            : %i\n",
            static_cast<int>(time_n));
        printf_epiworld("mean run elapsed time : %.2f%s\n",
            elapsed_total/static_cast<epiworld_double>(time_n), abbr_unit.c_str());

    } else {
        printf_epiworld("last run elapsed time : %.2f%s.\n", elapsed, abbr_unit.c_str());
    }
}

template<typename TSeq>
inline void Model<TSeq>::set_update_susceptible(UpdateFun<TSeq> fun) {
    
    update_susceptible = fun;

}

template<typename TSeq>
inline void Model<TSeq>::set_update_exposed(UpdateFun<TSeq> fun) {
    
    update_exposed = fun;

}

template<typename TSeq>
inline void Model<TSeq>::set_update_removed(UpdateFun<TSeq> fun) {
    
    update_removed = fun;

}


template<typename TSeq>
inline void Model<TSeq>::set_user_data(std::vector< std::string > names)
{
    db.set_user_data(names);
}

template<typename TSeq>
inline void Model<TSeq>::add_user_data(unsigned int j, epiworld_double x)
{
    db.add_user_data(j, x);
}

template<typename TSeq>
inline void Model<TSeq>::add_user_data(std::vector<epiworld_double> x)
{
    db.add_user_data(x);
}

template<typename TSeq>
inline UserData<TSeq> & Model<TSeq>::get_user_data()
{
    return db.get_user_data();
}

template<typename TSeq>
inline void Model<TSeq>::add_global_action(
    std::function<void(Model<TSeq>*)> fun,
    int date
)
{

    global_action_functions.push_back(fun);
    global_action_dates.push_back(date);

}

template<typename TSeq>
inline void Model<TSeq>::run_global_actions()
{

    for (unsigned int i = 0u; i < global_action_dates.size(); ++i)
    {

        if (global_action_dates[i] < 0)
        {

            global_action_functions[i](this);

        }
        else if (global_action_dates[i] == today())
        {

            global_action_functions[i](this);

        }

    }

}

template<typename TSeq>
inline void Model<TSeq>::toggle_visited()
{
    visited_model = !visited_model;
}

template<typename TSeq>
inline void Model<TSeq>::queuing_on()
{
    use_queuing = true;
}

template<typename TSeq>
inline void Model<TSeq>::queuing_off()
{
    use_queuing = false;
}

template<typename TSeq>
inline bool Model<TSeq>::is_queuing_on() const
{
    return use_queuing;
}

template<typename TSeq>
inline Queue<TSeq> & Model<TSeq>::get_queue()
{
    return queue;
}

#undef DURCAST

#undef CASES_PAR
#undef CASE_PAR

#undef EPIWORLD_CHECK_STATE
#undef EPIWORLD_CHECK_ALL_STATES
#undef EPIWORLD_COLLECT_STATUSES

#undef ADD_VIRUSES
#undef RM_VIRUSES
#undef UPDATE_QUEUE
#undef UPDATE_STATUS

#undef CHECK_INIT
#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/model-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/virus-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


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
    PostRecoveryFun<TSeq> post_recovery_fun                = nullptr;
    VirusFun<TSeq>        probability_of_infecting_fun = nullptr;
    VirusFun<TSeq>        probability_of_recovery_fun  = nullptr;
    VirusFun<TSeq>        probability_of_death_fun     = nullptr;

    // Setup parameters
    std::vector< epiworld_double * > params;
    std::vector< epiworld_double > data;

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
    bool is_active() const;
    void deactivate();

        /**
     * @brief Get and set the tool functions
     * 
     * @param v The virus over which to operate
     * @param fun the function to be used
     * 
     * @return epiworld_double 
     */
    ///@[
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
    ///@]


    void set_name(std::string name);
    std::string get_name() const;

    std::vector< epiworld_double > & get_data();

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/virus-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/virus-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_VIRUS_MEAT_HPP
#define EPIWORLD_VIRUS_MEAT_HPP

template<typename TSeq>
inline Virus<TSeq>::Virus(std::string name) {
    set_name(name);
}

// template<typename TSeq>
// inline Virus<TSeq>::Virus(TSeq sequence, std::string name) {
//     baseline_sequence = std::make_shared<TSeq>(sequence);
//     set_name(name);
// }

template<typename TSeq>
inline void Virus<TSeq>::mutate() {

    if (mutation_fun)
        if (mutation_fun(host, this, this->get_model()))
            host->get_model()->record_variant(this);
    

    return;
}

template<typename TSeq>
inline void Virus<TSeq>::set_mutation(
    MutFun<TSeq> fun
) {
    mutation_fun = MutFun<TSeq>(fun);
}

template<typename TSeq>
inline const TSeq * Virus<TSeq>::get_sequence() {
    return &(*baseline_sequence);
}

template<typename TSeq>
inline void Virus<TSeq>::set_sequence(TSeq sequence) {
    baseline_sequence = std::make_shared<TSeq>(sequence);
    return;
}

template<typename TSeq>
inline Person<TSeq> * Virus<TSeq>::get_host() {
    return host;
}

template<typename TSeq>
inline Model<TSeq> * Virus<TSeq>::get_model() {
    return host->get_model();
}

template<typename TSeq>
inline void Virus<TSeq>::set_id(int idx) {
    id = idx;
    return;
}

template<typename TSeq>
inline int Virus<TSeq>::get_id() const {
    
    return id;
}

template<typename TSeq>
inline void Virus<TSeq>::set_date(int d) {
    date = d;
    return;
}

template<typename TSeq>
inline int Virus<TSeq>::get_date() const {
    
    return date;
}

template<typename TSeq>
inline bool Virus<TSeq>::is_active() const {
    return active;
}

template<typename TSeq>
inline void Virus<TSeq>::deactivate()
{

    active = false;
    host->get_viruses().nactive--;

}



template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_prob_infecting()
{

    if (probability_of_infecting_fun)
        return probability_of_infecting_fun(host, this, host->get_model());
        
    return EPI_DEFAULT_VIRUS_PROB_INFECTION;

}



template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_prob_recovery()
{

    if (probability_of_recovery_fun)
        return probability_of_recovery_fun(host, this, host->get_model());
        
    return EPI_DEFAULT_VIRUS_PROB_RECOVERY;

}



template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_prob_death()
{

    if (probability_of_death_fun)
        return probability_of_death_fun(host, this, host->get_model());
        
    return EPI_DEFAULT_VIRUS_PROB_DEATH;

}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_infecting_fun(VirusFun<TSeq> fun)
{
    probability_of_infecting_fun = fun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_recovery_fun(VirusFun<TSeq> fun)
{
    probability_of_recovery_fun = fun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_death_fun(VirusFun<TSeq> fun)
{
    probability_of_death_fun = fun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_infecting(epiworld_double * prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return *prob;
        };
    
    probability_of_infecting_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_recovery(epiworld_double * prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return *prob;
        };
    
    probability_of_recovery_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_death(epiworld_double * prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return *prob;
        };
    
    probability_of_death_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_infecting(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return prob;
        };
    
    probability_of_infecting_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_recovery(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return prob;
        };
    
    probability_of_recovery_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_death(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return prob;
        };
    
    probability_of_death_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_post_recovery(PostRecoveryFun<TSeq> fun)
{
    if (post_recovery_fun)
    {
        printf_epiworld(
            "Warning: a PostRecoveryFun is alreay in place (overwriting)."
            );
    }

    post_recovery_fun = fun;
}

template<typename TSeq>
inline void Virus<TSeq>::post_recovery()
{

    if (post_recovery_fun)
        return post_recovery_fun(host, this, host->get_model());    

    return;
        
}

template<typename TSeq>
inline void Virus<TSeq>::set_post_immunity(
    epiworld_double prob
)
{

    if (post_recovery_fun)
    {

        std::string msg =
            std::string(
                "You cannot set post immunity when a post_recovery "
                ) +
            std::string(
                "function is already in place. Redesign the post_recovery function."
                );

        throw std::logic_error(msg);
        
    }

    PostRecoveryFun<TSeq> tmpfun = 
        [prob](Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            Tool<TSeq> no_reinfect(
                "No reinfect virus " +
                std::to_string(v->get_id())
                );
            
            no_reinfect.set_susceptibility_reduction(prob);
            no_reinfect.set_death_reduction(0.0);
            no_reinfect.set_transmission_reduction(0.0);
            no_reinfect.set_recovery_enhancer(0.0);

            p->add_tool(m->today(), no_reinfect);

            return;

        };

    post_recovery_fun = tmpfun;

}

template<typename TSeq>
inline void Virus<TSeq>::set_post_immunity(
    epiworld_double * prob
)
{

    if (post_recovery_fun)
    {

        std::string msg =
            std::string(
                "You cannot set post immunity when a post_recovery "
                ) +
            std::string(
                "function is already in place. Redesign the post_recovery function."
                );

        throw std::logic_error(msg);

    }

    PostRecoveryFun<TSeq> tmpfun = 
        [prob](Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            Tool<TSeq> no_reinfect(
                "No reinfect virus " +
                std::to_string(v->get_id())
                );
            
            no_reinfect.set_susceptibility_reduction(*prob);
            no_reinfect.set_death_reduction(0.0);
            no_reinfect.set_transmission_reduction(0.0);
            no_reinfect.set_recovery_enhancer(0.0);

            p->add_tool(m->today(), no_reinfect);

            return;

        };

    post_recovery_fun = tmpfun;

}

template<typename TSeq>
inline void Virus<TSeq>::set_name(std::string name)
{

    if (name == "")
        virus_name = nullptr;
    else
        virus_name = std::make_shared<std::string>(name);

}

template<typename TSeq>
inline std::string Virus<TSeq>::get_name() const
{

    if (virus_name)
        return *virus_name;
    
    return "unknown virus";

}

template<typename TSeq>
inline std::vector< epiworld_double > & Virus<TSeq>::get_data() {
    return data;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/virus-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/personviruses-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSONVIRUSES_BONES_HPP
#define EPIWORLD_PERSONVIRUSES_BONES_HPP

/**
 * @brief Set of viruses in host
 * 
 * @tparam TSeq 
 */
template<typename TSeq = bool>
class PersonViruses {
    friend class Person<TSeq>;
    friend class Model<TSeq>;

private:
    Person<TSeq> * host;
    std::vector< Virus<TSeq> > viruses;
    int nactive = 0;

public:
    void add_virus(epiworld_fast_uint new_status, Virus<TSeq> v);
    size_t size() const;
    int size_active() const;
    Virus<TSeq> & operator()(int i);
    void mutate();
    void reset();
    void deactivate(Virus<TSeq> & v);
    Person<TSeq> * get_host();
    bool has_virus(unsigned int v) const;
    bool has_virus(std::string vname) const;

};


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/personviruses-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/personviruses-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSONVIRUSES_MEAT_HPP
#define EPIWORLD_PERSONVIRUSES_MEAT_HPP

template<typename TSeq>
inline void PersonViruses<TSeq>::add_virus(
    epiworld_fast_uint new_status,
    Virus<TSeq> v
) {

    // This will make an independent copy of the virus.
    // Will keep the original sequence and will point to the
    // mutation and transmisibility functions.
    viruses.push_back(v);
    int vloc = viruses.size() - 1u;
    viruses[vloc].host = host;
    viruses[vloc].date = host->get_model()->today();

    nactive++;

}

template<typename TSeq>
inline size_t PersonViruses<TSeq>::size() const {
    return viruses.size();
}

template<typename TSeq>
inline int PersonViruses<TSeq>::size_active() const {
    return nactive;
}


template<typename TSeq>
inline Virus<TSeq> & PersonViruses<TSeq>::operator()(
    int i
) {

    return viruses.at(i);

}

template<typename TSeq>
inline void PersonViruses<TSeq>::mutate()
{
    for (auto & v : viruses)
        v.mutate();
}

template<typename TSeq>
inline void PersonViruses<TSeq>::reset()
{

    this->viruses.clear();

}

template<typename TSeq>
inline void PersonViruses<TSeq>::deactivate(Virus<TSeq> & v)
{

    if (v.get_host()->id != host->id)
        throw std::logic_error("A host cannot deactivate someone else's virus.");

    v.deactivate();

}

template<typename TSeq>
inline Person<TSeq> * PersonViruses<TSeq>::get_host() {
    return host;
}

template<typename TSeq>
inline bool PersonViruses<TSeq>::has_virus(unsigned int v) const
{
    int v2 = static_cast<int>(v);
    for (auto & virus : viruses)
        if (v2 == virus.get_id())
            return true;

    return false;
}

template<typename TSeq>
inline bool PersonViruses<TSeq>::has_virus(std::string vname) const
{
    for (auto & virus : viruses)
        if (vname == virus.get_name())
            return true;
            
    return false;
}


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/personviruses-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/tools-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



#ifndef EPIWORLD_TOOLS_BONES_HPP
#define EPIWORLD_TOOLS_BONES_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Person;

template<typename TSeq>
class Model;

template<typename TSeq>
class PersonTools;

template<typename TSeq>
class Tool;

/**
 * @brief Tools for defending the host against the virus
 * 
 * @tparam TSeq Type of sequence
 */
template<typename TSeq=bool> 
class Tool {
    friend class PersonTools<TSeq>;
    friend class Person<TSeq>;
    friend class Model<TSeq>;
private:

    Person<TSeq> * person;
    unsigned int id = 0u;
    std::shared_ptr<std::string> tool_name = nullptr;
    std::shared_ptr<TSeq> sequence = nullptr;
    TSeq sequence_unique  = default_sequence<TSeq>();
    ToolFun<TSeq> susceptibility_reduction_fun = nullptr;
    ToolFun<TSeq> transmission_reduction_fun   = nullptr;
    ToolFun<TSeq> recovery_enhancer_fun        = nullptr;
    ToolFun<TSeq> death_reduction_fun          = nullptr;

    // Setup parameters
    std::vector< epiworld_double * > params;  

public:
    Tool(std::string name = "unknown tool");
    // Tool(TSeq d, std::string name = "unknown tool");

    void set_sequence(TSeq d);
    void set_sequence_unique(TSeq d);
    void set_sequence(std::shared_ptr<TSeq> d);
    std::shared_ptr<TSeq> get_sequence();
    TSeq & get_sequence_unique();

    /**
     * @brief Get and set the tool functions
     * 
     * @param v The virus over which to operate
     * @param fun the function to be used
     * 
     * @return epiworld_double 
     */
    ///@[
    epiworld_double get_susceptibility_reduction(Virus<TSeq> * v);
    epiworld_double get_transmission_reduction(Virus<TSeq> * v);
    epiworld_double get_recovery_enhancer(Virus<TSeq> * v);
    epiworld_double get_death_reduction(Virus<TSeq> * v);
    void set_susceptibility_reduction_fun(ToolFun<TSeq> fun);
    void set_transmission_reduction_fun(ToolFun<TSeq> fun);
    void set_recovery_enhancer_fun(ToolFun<TSeq> fun);
    void set_death_reduction_fun(ToolFun<TSeq> fun);
    void set_susceptibility_reduction(epiworld_double * prob);
    void set_transmission_reduction(epiworld_double * prob);
    void set_recovery_enhancer(epiworld_double * prob);
    void set_death_reduction(epiworld_double * prob);
    void set_susceptibility_reduction(epiworld_double prob);
    void set_transmission_reduction(epiworld_double prob);
    void set_recovery_enhancer(epiworld_double prob);
    void set_death_reduction(epiworld_double prob);
    ///@]

    void set_name(std::string name);
    std::string get_name() const;

    Person<TSeq> * get_person();
    unsigned int get_id() const;


};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/tools-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/tools-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



#ifndef EPIWORLD_TOOLS_MEAT_HPP
#define EPIWORLD_TOOLS_MEAT_HPP

template<typename TSeq>
inline Tool<TSeq>::Tool(std::string name)
{
    set_name(name);
}

// template<typename TSeq>
// inline Tool<TSeq>::Tool(TSeq d, std::string name) {
//     sequence = std::make_shared<TSeq>(d);
//     tool_name = std::make_shared<std::string>(name);
// }

template<typename TSeq>
inline void Tool<TSeq>::set_sequence(TSeq d) {
    sequence = std::make_shared<TSeq>(d);
}

template<typename TSeq>
inline void Tool<TSeq>::set_sequence_unique(TSeq d) {
    sequence_unique = d;
}

template<typename TSeq>
inline void Tool<TSeq>::set_sequence(std::shared_ptr<TSeq> d) {
    sequence = d;
}

template<typename TSeq>
inline std::shared_ptr<TSeq> Tool<TSeq>::get_sequence() {
    return sequence;
}

template<typename TSeq>
inline TSeq & Tool<TSeq>::get_sequence_unique() {
    return sequence_unique;
}

template<typename TSeq>
inline epiworld_double Tool<TSeq>::get_susceptibility_reduction(
    Virus<TSeq> * v
)
{

    if (susceptibility_reduction_fun)
        return susceptibility_reduction_fun(this, this->person, v, person->get_model());

    return DEFAULT_TOOL_CONTAGION_REDUCTION;

}

template<typename TSeq>
inline epiworld_double Tool<TSeq>::get_transmission_reduction(
    Virus<TSeq> * v
)
{

    if (transmission_reduction_fun)
        return transmission_reduction_fun(this, this->person, v, person->get_model());

    return DEFAULT_TOOL_TRANSMISSION_REDUCTION;

}

template<typename TSeq>
inline epiworld_double Tool<TSeq>::get_recovery_enhancer(
    Virus<TSeq> * v
)
{

    if (recovery_enhancer_fun)
        return recovery_enhancer_fun(this, this->person, v, person->get_model());

    return DEFAULT_TOOL_RECOVERY_ENHANCER;

}

template<typename TSeq>
inline epiworld_double Tool<TSeq>::get_death_reduction(
    Virus<TSeq> * v
)
{

    if (death_reduction_fun)
        return death_reduction_fun(this, this->person, v, person->get_model());

    return DEFAULT_TOOL_DEATH_REDUCTION;

}

template<typename TSeq>
inline void Tool<TSeq>::set_susceptibility_reduction_fun(
    ToolFun<TSeq> fun
)
{
    susceptibility_reduction_fun = fun;
}

template<typename TSeq>
inline void Tool<TSeq>::set_transmission_reduction_fun(
    ToolFun<TSeq> fun
)
{
    transmission_reduction_fun = fun;
}

template<typename TSeq>
inline void Tool<TSeq>::set_recovery_enhancer_fun(
    ToolFun<TSeq> fun
)
{
    recovery_enhancer_fun = fun;
}

template<typename TSeq>
inline void Tool<TSeq>::set_death_reduction_fun(
    ToolFun<TSeq> fun
)
{
    death_reduction_fun = fun;
}

template<typename TSeq>
inline void Tool<TSeq>::set_susceptibility_reduction(epiworld_double * prob)
{

    ToolFun<TSeq> tmpfun =
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return *prob;
        };

    susceptibility_reduction_fun = tmpfun;

}

// EPIWORLD_SET_LAMBDA(susceptibility_reduction)
template<typename TSeq>
inline void Tool<TSeq>::set_transmission_reduction(epiworld_double * prob)
{
    
    ToolFun<TSeq> tmpfun =
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return *prob;
        };

    transmission_reduction_fun = tmpfun;

}

// EPIWORLD_SET_LAMBDA(transmission_reduction)
template<typename TSeq>
inline void Tool<TSeq>::set_recovery_enhancer(epiworld_double * prob)
{

    ToolFun<TSeq> tmpfun =
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return *prob;
        };

    recovery_enhancer_fun = tmpfun;

}

// EPIWORLD_SET_LAMBDA(recovery_enhancer)
template<typename TSeq>
inline void Tool<TSeq>::set_death_reduction(epiworld_double * prob)
{

    ToolFun<TSeq> tmpfun =
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return *prob;
        };

    death_reduction_fun = tmpfun;

}

// EPIWORLD_SET_LAMBDA(death_reduction)

// #undef EPIWORLD_SET_LAMBDA
template<typename TSeq>
inline void Tool<TSeq>::set_susceptibility_reduction(
    epiworld_double prob
)
{

    ToolFun<TSeq> tmpfun = 
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return prob;
        };

    susceptibility_reduction_fun = tmpfun;

}

template<typename TSeq>
inline void Tool<TSeq>::set_transmission_reduction(
    epiworld_double prob
)
{

    ToolFun<TSeq> tmpfun = 
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return prob;
        };

    transmission_reduction_fun = tmpfun;

}

template<typename TSeq>
inline void Tool<TSeq>::set_recovery_enhancer(
    epiworld_double prob
)
{

    ToolFun<TSeq> tmpfun = 
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return prob;
        };

    recovery_enhancer_fun = tmpfun;

}

template<typename TSeq>
inline void Tool<TSeq>::set_death_reduction(
    epiworld_double prob
)
{

    ToolFun<TSeq> tmpfun = 
        [prob](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)
        {
            return prob;
        };

    death_reduction_fun = tmpfun;

}

template<typename TSeq>
inline void Tool<TSeq>::set_name(std::string name)
{
    if (name != "")
        tool_name = std::make_shared<std::string>(name);
}

template<typename TSeq>
inline std::string Tool<TSeq>::get_name() const {

    if (tool_name)
        return *tool_name;

    return "unknown tool";

}

template<typename TSeq>
inline Person<TSeq> * Tool<TSeq>::get_person()
{
    return person;
}

template<typename TSeq>
inline unsigned int Tool<TSeq>::get_id() const {
    return id;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/tools-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/persontools-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSONTOOLS_BONES_HPP
#define EPIWORLD_PERSONTOOLS_BONES_HPP

/**
 * @brief List of tools available for the individual to 
 * 
 * @tparam TSeq 
 */
template<typename TSeq = bool>
class PersonTools {
    friend class Person<TSeq>;
    friend class Model<TSeq>;

private:
    Person<TSeq> * person; 
    std::vector<Tool<TSeq>> tools;
    std::vector< int > dates;
    MixerFun<TSeq> susceptibility_reduction_mixer;
    MixerFun<TSeq> transmission_reduction_mixer;
    MixerFun<TSeq> recovery_enhancer_mixer;
    MixerFun<TSeq> death_reduction_mixer;

public:
    PersonTools() {};
    void add_tool(int date, Tool<TSeq> tool);
    epiworld_double get_susceptibility_reduction(Virus<TSeq> * v);
    epiworld_double get_transmission_reduction(Virus<TSeq> * v);
    epiworld_double get_recovery_enhancer(Virus<TSeq> * v);
    epiworld_double get_death_reduction(Virus<TSeq> * v);

    void set_susceptibility_reduction_mixer(MixerFun<TSeq> fun);
    void set_transmission_reduction_mixer(MixerFun<TSeq> fun);
    void set_recovery_enhancer_mixer(MixerFun<TSeq> fun);
    void set_death_reduction_mixer(MixerFun<TSeq> fun);

    size_t size() const;
    Tool<TSeq> & operator()(int i);

    Person<TSeq> * get_person();
    Model<TSeq> * get_model();
    void reset();
    bool has_tool(unsigned int t) const;
    bool has_tool(std::string name) const;

};



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/persontools-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/persontools-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSONTOOLS_MEAT_HPP
#define EPIWORLD_PERSONTOOLS_MEAT_HPP

/**
 * @brief Default function for combining susceptibility_reduction levels
 * 
 * @tparam TSeq 
 * @param pt 
 * @return epiworld_double 
 */
///@[
template<typename TSeq>
inline epiworld_double susceptibility_reduction_mixer_default(
    PersonTools<TSeq>* pt,
    Person<TSeq>* p,
    Virus<TSeq>* v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
        total *= (1.0 - pt->operator()(i).get_susceptibility_reduction(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline epiworld_double transmission_reduction_mixer_default(
    PersonTools<TSeq>* pt,
    Person<TSeq>* p,
    Virus<TSeq>* v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
        total *= (1.0 - pt->operator()(i).get_transmission_reduction(v));

    return (1.0 - total);
    
}

template<typename TSeq>
inline epiworld_double recovery_enhancer_mixer_default(
    PersonTools<TSeq>* pt,
    Person<TSeq>* p,
    Virus<TSeq>* v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
        total *= (1.0 - pt->operator()(i).get_recovery_enhancer(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline epiworld_double death_reduction_mixer_default(
    PersonTools<TSeq>* pt,
    Person<TSeq>* p,
    Virus<TSeq>* v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (unsigned int i = 0; i < pt->size(); ++i)
    {
        total *= (1.0 - pt->operator()(i).get_death_reduction(v));
    } 

    return 1.0 - total;
    
}
///@]

template<typename TSeq>
inline void PersonTools<TSeq>::add_tool(
    int date,
    Tool<TSeq> tool
) {
    tools.push_back(tool);
    tools[tools.size() - 1].person = this->person;
    dates.push_back(date);
}

template<typename TSeq>
inline epiworld_double PersonTools<TSeq>::get_susceptibility_reduction(
    Virus<TSeq> * v
) {

    if (!susceptibility_reduction_mixer)
        set_susceptibility_reduction_mixer(susceptibility_reduction_mixer_default<TSeq>);

    return susceptibility_reduction_mixer(this, person, v, person->get_model());

}

template<typename TSeq>
inline epiworld_double PersonTools<TSeq>::get_transmission_reduction(
    Virus<TSeq> * v
) {

    if (!transmission_reduction_mixer)
        set_transmission_reduction_mixer(transmission_reduction_mixer_default<TSeq>);

    return transmission_reduction_mixer(this, person, v, person->get_model());

}

template<typename TSeq>
inline epiworld_double PersonTools<TSeq>::get_recovery_enhancer(
    Virus<TSeq> * v
) {

    if (!recovery_enhancer_mixer)
        set_recovery_enhancer_mixer(recovery_enhancer_mixer_default<TSeq>);

    return recovery_enhancer_mixer(this, person, v, person->get_model());

}


template<typename TSeq>
inline epiworld_double PersonTools<TSeq>::get_death_reduction(
    Virus<TSeq> * v
) {

    if (!death_reduction_mixer)
        set_death_reduction_mixer(death_reduction_mixer_default<TSeq>);

    return death_reduction_mixer(this, person, v, person->get_model());

}

template<typename TSeq>
inline void PersonTools<TSeq>::set_susceptibility_reduction_mixer(
    MixerFun<TSeq> fun
) {
    susceptibility_reduction_mixer = MixerFun<TSeq>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_transmission_reduction_mixer(
    MixerFun<TSeq> fun
) {
    transmission_reduction_mixer = MixerFun<TSeq>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_recovery_enhancer_mixer(
    MixerFun<TSeq> fun
) {
    recovery_enhancer_mixer = MixerFun<TSeq>(fun);
}

template<typename TSeq>
inline void PersonTools<TSeq>::set_death_reduction_mixer(
    MixerFun<TSeq> fun
) {
    death_reduction_mixer = MixerFun<TSeq>(fun);
}


template<typename TSeq>
inline size_t PersonTools<TSeq>::size() const {
    return tools.size();
}

template<typename TSeq>
inline Tool<TSeq> & PersonTools<TSeq>::operator()(int i) {
    return tools.at(i);
}

template<typename TSeq>
inline Person<TSeq> * PersonTools<TSeq>::get_person() {
    return person;
}

template<typename TSeq>
inline Model<TSeq> * PersonTools<TSeq>::get_model() {
    return person->get_model();
}

template<typename TSeq>
inline void PersonTools<TSeq>::reset()
{
    
    this->tools.clear();
    this->dates.clear();

}

template<typename TSeq>
inline bool PersonTools<TSeq>::has_tool(unsigned int t) const {

    for (auto & tool : tools)
        if (tool.get_id() == t)
            return true;

    return false;

}

template<typename TSeq>
inline bool PersonTools<TSeq>::has_tool(std::string name) const {

    for (auto * tool : tools)
        if (tool->get_name() == name)
            return true;

    return false;
    
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/persontools-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


    
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/person-meat-status.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSON_MEAT_STATUS_HPP 
#define EPIWORLD_PERSON_MEAT_STATUS_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Person;


#define EPIWORLD_UPDATE_SUSCEPTIBLE_CALC_PROBS(probs,variants) \
    /* Step 1: Compute the individual efficcacy */ \
    std::vector< epiworld_double > probs; \
    std::vector< epiworld::Virus<TSeq>* > variants; \
    /* Computing the susceptibility_reduction */ \
    for (unsigned int n = 0; n < p->get_neighbors().size(); ++n) \
    { \
        epiworld::Person<TSeq> * neighbor = p->get_neighbors()[n]; \
        /* Non-infected individuals make no difference */ \
        if (!epiworld::IN(neighbor->get_status(), m->get_status_exposed())) \
            continue; \
        epiworld::PersonViruses<TSeq> & nviruses = neighbor->get_viruses(); \
        /* Now over the neighbor's viruses */ \
        epiworld_double tmp_transmission; \
        for (unsigned int v = 0; v < nviruses.size(); ++v) \
        { \
            /* Computing the corresponding susceptibility_reduction */ \
            epiworld::Virus<TSeq> * tmp_v = &(nviruses(v)); \
            /* And it is a function of susceptibility_reduction as well */ \
            tmp_transmission = \
                (1.0 - p->get_susceptibility_reduction(tmp_v)) * \
                tmp_v->get_prob_infecting() * \
                (1.0 - neighbor->get_transmission_reduction(tmp_v)) \
                ; \
            probs.push_back(tmp_transmission); \
            variants.push_back(tmp_v); \
        } \
    }

template<typename TSeq>
inline epiworld_fast_uint default_update_susceptible(
    Person<TSeq> * p,
    Model<TSeq> * m
    )
{

    // This computes the prob of getting any neighbor variant
    unsigned int nvariants_tmp = 0u;
    for (unsigned int n = 0; n < p->get_neighbors().size(); ++n) 
    { 

        Person<TSeq> * neighbor = p->get_neighbors()[n]; 
        
        /* Non-infected individuals make no difference */ 
        if (!IN(neighbor->get_status(), m->get_status_exposed())) 
            continue; 
        
        PersonViruses<TSeq> & nviruses = neighbor->get_viruses(); 
        /* Now over the neighbor's viruses */ 
        
        epiworld_double tmp_transmission; 
        for (unsigned int v = 0; v < nviruses.size(); ++v) 
        { 
        
            /* Computing the corresponding susceptibility_reduction */ 
            Virus<TSeq> * tmp_v = &(nviruses(v)); 
        
            /* And it is a function of susceptibility_reduction as well */ 
            tmp_transmission = 
                (1.0 - p->get_susceptibility_reduction(tmp_v)) * 
                tmp_v->get_prob_infecting() * 
                (1.0 - neighbor->get_transmission_reduction(tmp_v)) 
                ; 
        
            m->array_double_tmp[nvariants_tmp]  = tmp_transmission;
            m->array_virus_tmp[nvariants_tmp++] = tmp_v;
            
        } 
    }

    // No virus to compute
    if (nvariants_tmp == 0u)
        return p->get_status();

    // Running the roulette
    int which = roulette(nvariants_tmp, m);

    if (which < 0)
        return p->get_status();

    p->add_virus(m->array_virus_tmp[which]); 

    return m->get_default_exposed();

}

#define EPIWORLD_UPDATE_EXPOSED_CALC_PROBS(prob_rec, prob_die) \
    epiworld::Virus<TSeq> * v = &(p->get_virus(0u)); \
    epiworld_double prob_rec = v->get_prob_recovery() * (1.0 - p->get_recovery_enhancer(v)); \
    epiworld_double prob_die = v->get_prob_death() * (1.0 - p->get_death_reduction(v)); 


template<typename TSeq>
inline epiworld_fast_uint default_update_exposed(Person<TSeq> * p, Model<TSeq> * m) {

    epiworld::Virus<TSeq> * v = &(p->get_virus(0u)); 
    epiworld_double p_rec = v->get_prob_recovery() * (1.0 - p->get_recovery_enhancer(v)); 
    epiworld_double p_die = v->get_prob_death() * (1.0 - p->get_death_reduction(v)); 
    
    epiworld_double r = EPI_RUNIF();
    epiworld_double cumsum = p_die * (1 - p_rec) / (1.0 - p_die * p_rec); 

    if (r < cumsum)
    {
        p->rm_virus(v);
        return m->get_default_removed();
    }
    
    cumsum += p_rec * (1 - p_die) / (1.0 - p_die * p_rec);
    
    if (r < cumsum)
    {
        p->rm_virus(v);
        return m->get_default_removed();
    }

    return p->get_status();

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/person-meat-status.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/person-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSON_BONES_HPP
#define EPIWORLD_PERSON_BONES_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Tool;

template<typename TSeq>
class PersonViruses;

template<typename TSeq>
class PersonTools;

template<typename TSeq>
class Queue;

template<typename TSeq = bool>
class Person {
    friend class Model<TSeq>;
    friend class Tool<TSeq>;
    friend class Queue<TSeq>;
private:
    Model<TSeq> * model;
    PersonViruses<TSeq> viruses;
    PersonTools<TSeq> tools;
    std::vector< Person<TSeq> * > neighbors;
    unsigned int index; ///< Location in the Model
    epiworld_fast_uint status_next; // Placeholder
    epiworld_fast_uint status;
    int id          = -1;
    UpdateFun<TSeq> update_susceptible = default_update_susceptible<TSeq>;
    UpdateFun<TSeq> update_exposed     = default_update_exposed<TSeq>;
    UpdateFun<TSeq> update_removed     = nullptr;

    bool visited_person = !EPI_DEFAULT_VISITED;
    bool in_queue       = false;

public:

    Person();
    void init(epiworld_fast_uint baseline_status);

    void add_tool(int d, Tool<TSeq> tool);
    void add_virus(Virus<TSeq> * virus);
    void rm_virus(Virus<TSeq> * virus);

    epiworld_double get_susceptibility_reduction(Virus<TSeq> * v);
    epiworld_double get_transmission_reduction(Virus<TSeq> * v);
    epiworld_double get_recovery_enhancer(Virus<TSeq> * v);
    epiworld_double get_death_reduction(Virus<TSeq> * v);
    int get_id() const;
    unsigned int get_index() const;
    
    std::mt19937 * get_rand_endgine();
    Model<TSeq> * get_model(); 

    Virus<TSeq> & get_virus(int i);
    PersonViruses<TSeq> & get_viruses();

    Tool<TSeq> & get_tool(int i);
    PersonTools<TSeq> & get_tools();

    void mutate_variant();
    void add_neighbor(
        Person<TSeq> * p,
        bool check_source = true,
        bool check_target = true
        );

    std::vector< Person<TSeq> * > & get_neighbors();

    void update_status();
    const epiworld_fast_uint & get_status() const;

    void reset();

    void set_update_susceptible(UpdateFun<TSeq> fun);
    void set_update_exposed(UpdateFun<TSeq> fun);
    void set_update_removed(UpdateFun<TSeq> fun);
    bool has_tool(unsigned int t) const;
    bool has_tool(std::string name) const;
    bool has_virus(unsigned int t) const;
    bool has_virus(std::string name) const;

    bool visited() const;
    void toggle_visited();

};



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/person-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/person-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSON_MEAT_HPP
#define EPIWORLD_PERSON_MEAT_HPP

// template<typename Ta>
// inline bool IN(Ta & a, std::vector< Ta > & b);

template<typename TSeq>
inline Person<TSeq>::Person()
{
    
}

template<typename TSeq>
inline void Person<TSeq>::init(epiworld_fast_uint baseline_status)
{
    tools.person = this;
    viruses.host = this;
    status       = baseline_status;
    status_next  = baseline_status;
}
    

template<typename TSeq>
inline void Person<TSeq>::add_tool(
    int d,
    Tool<TSeq> tool
) {
    tools.add_tool(d, tool);
}

template<typename TSeq>
inline void Person<TSeq>::add_virus(
    Virus<TSeq> * virus
)
{

    model->virus_to_add.push_back(virus);
    model->virus_to_add_person.push_back(this);

    if (model->is_queuing_on())
        model->get_queue() += this;

}

template<typename TSeq>
inline void Person<TSeq>::rm_virus(
    Virus<TSeq> * virus
)
{

    model->virus_to_remove.push_back(virus);

    if (model->is_queuing_on())
        model->get_queue() -= this;

}

template<typename TSeq>
inline epiworld_double Person<TSeq>::get_susceptibility_reduction(
    Virus<TSeq> * v
) {
    return tools.get_susceptibility_reduction(v);
}

template<typename TSeq>
inline epiworld_double Person<TSeq>::get_transmission_reduction(
    Virus<TSeq> * v
) {
    return tools.get_transmission_reduction(v);
}

template<typename TSeq>
inline epiworld_double Person<TSeq>::get_recovery_enhancer(
    Virus<TSeq> * v
) {
    return tools.get_recovery_enhancer(v);
}

template<typename TSeq>
inline epiworld_double Person<TSeq>::get_death_reduction(
    Virus<TSeq> * v
) {
    return tools.get_death_reduction(v);
}

template<typename TSeq>
inline int Person<TSeq>::get_id() const
{
    return id;
}

template<typename TSeq>
inline unsigned int Person<TSeq>::get_index() const
{
    return index;
}

template<typename TSeq>
inline std::mt19937 * Person<TSeq>::get_rand_endgine() {
    return model->get_rand_endgine();
}

template<typename TSeq>
inline Model<TSeq> * Person<TSeq>::get_model() {
    return model;
}

template<typename TSeq>
inline PersonViruses<TSeq> & Person<TSeq>::get_viruses() {
    return viruses;
}

template<typename TSeq>
inline Virus<TSeq> & Person<TSeq>::get_virus(int i) {
    return viruses(i);
}

template<typename TSeq>
inline PersonTools<TSeq> & Person<TSeq>::get_tools() {
    return tools;
}

template<typename TSeq>
inline Tool<TSeq> & Person<TSeq>::get_tool(int i) {
    return tools(i);
}

template<typename TSeq>
inline void Person<TSeq>::mutate_variant() {
    viruses.mutate();
}

template<typename TSeq>
inline void Person<TSeq>::add_neighbor(
    Person<TSeq> * p,
    bool check_source,
    bool check_target
) {
    // Can we find the neighbor?
    if (check_source)
    {

        bool found = false;
        for (auto & n: neighbors)    
            if (n->get_id() == p->get_id())
            {
                found = true;
                break;
            }

        if (!found)
            neighbors.push_back(p);

    } else 
        neighbors.push_back(p);

    if (check_target)
    {

        bool found = false;
        for (auto & n: p->neighbors)
            if (n->get_id() == id)
            {
                found = true;
                break;
            }

        if (!found)
            p->neighbors.push_back(this);
    
    } else 
        p->neighbors.push_back(this);
    

}

template<typename TSeq>
inline std::vector< Person<TSeq> *> & Person<TSeq>::get_neighbors()
{
    return neighbors;
}

template<typename TSeq>
inline void Person<TSeq>::update_status()
{

    // No change if removed
    if (IN(status, model->status_removed))
    {
        if (update_removed)
            status_next = update_removed(this, model);

    } else if (IN(status, model->status_susceptible)) {
        
        if (!update_susceptible)
            throw std::logic_error("No update_susceptible function?!");

        if (update_susceptible)
            status_next = update_susceptible(this, model);

    } else if (IN(status, model->status_exposed)) {

        if (update_exposed)
            status_next = update_exposed(this, model);

    } else
        throw std::range_error(
            "The reported status " + std::to_string(status) + " is not valid.");

    // Updating db
    if (status_next != status)
        model->get_db().state_change(status, status_next);


    return;

}

template<typename TSeq>
inline const epiworld_fast_uint & Person<TSeq>::get_status() const {
    return status;
}

template<typename TSeq>
inline void Person<TSeq>::reset() {

    this->viruses.reset();
    this->tools.reset();
    this->status      = model->baseline_status_susceptible;
    this->status_next = model->baseline_status_susceptible;

}

template<typename TSeq>
inline void Person<TSeq>::set_update_susceptible(UpdateFun<TSeq> fun) {
    update_susceptible = fun;
}

template<typename TSeq>
inline void Person<TSeq>::set_update_exposed(UpdateFun<TSeq> fun) {
    update_exposed = fun;
}

template<typename TSeq>
inline void Person<TSeq>::set_update_removed(UpdateFun<TSeq> fun) {
    update_removed = fun;
}

template<typename TSeq>
inline bool Person<TSeq>::has_tool(unsigned int t) const {
    return tools.has_tool(t);
}

template<typename TSeq>
inline bool Person<TSeq>::has_tool(std::string name) const {
    return tools.has_tool(name);
}

template<typename TSeq>
inline bool Person<TSeq>::has_virus(unsigned int t) const {
    return viruses.has_virus(t);
}

template<typename TSeq>
inline bool Person<TSeq>::has_virus(std::string name) const {
    return viruses.has_virus(name);
}

template<typename TSeq>
inline bool Person<TSeq>::visited() const
{
    
    return visited_person == model->visited_model;

}

template<typename TSeq>
inline void Person<TSeq>::toggle_visited()
{
    visited_person = !visited_person;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/person-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



}

#endif 
