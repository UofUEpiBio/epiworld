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
#include <algorithm>
#include <regex>
#include <iomanip>
#include <set>

#ifndef EPIWORLD_HPP
#define EPIWORLD_HPP

/* Versioning */
#define EPIWORLD_VERSION_MAJOR 0
#define EPIWORLD_VERSION_MINOR 7
#define EPIWORLD_VERSION_PATCH 2

static const int epiworld_version_major = EPIWORLD_VERSION_MAJOR;
static const int epiworld_version_minor = EPIWORLD_VERSION_MINOR;
static const int epiworld_version_patch = EPIWORLD_VERSION_PATCH;

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
    #define EPIWORLD_MAXNEIGHBORS 1048576
#endif

#if defined(_OPENMP) || defined(__OPENMP)
    #include <omp.h>
// #else
//     #define omp_get_thread_num() 0
//     #define omp_set_num_threads() 1
#endif

#ifndef epiworld_double
    #define epiworld_double float
#endif

#ifndef epiworld_fast_int
    #define epiworld_fast_int long long int
#endif

#ifndef epiworld_fast_uint
    #define epiworld_fast_uint unsigned long long int
#endif

#define EPI_DEFAULT_TSEQ int

template<typename TSeq = EPI_DEFAULT_TSEQ>
class Model;

template<typename TSeq = EPI_DEFAULT_TSEQ>
class Agent;

template<typename TSeq = EPI_DEFAULT_TSEQ>
class PersonTools;

template<typename TSeq = EPI_DEFAULT_TSEQ>
class Virus;

template<typename TSeq = EPI_DEFAULT_TSEQ>
class Viruses;

template<typename TSeq = EPI_DEFAULT_TSEQ>
class Viruses_const;

template<typename TSeq = EPI_DEFAULT_TSEQ>
class Tool;

template<typename TSeq = EPI_DEFAULT_TSEQ>
class Tools;

template<typename TSeq = EPI_DEFAULT_TSEQ>
class Tools_const;

template<typename TSeq = EPI_DEFAULT_TSEQ>
class Entity;

template<typename TSeq = EPI_DEFAULT_TSEQ>
using VirusPtr = std::shared_ptr< Virus< TSeq > >;

template<typename TSeq = EPI_DEFAULT_TSEQ>
using ToolPtr = std::shared_ptr< Tool< TSeq > >;

template<typename TSeq = EPI_DEFAULT_TSEQ>
using ToolFun = std::function<epiworld_double(Tool<TSeq>&,Agent<TSeq>*,VirusPtr<TSeq>,Model<TSeq>*)>;

template<typename TSeq = EPI_DEFAULT_TSEQ>
using MixerFun = std::function<epiworld_double(Agent<TSeq>*,VirusPtr<TSeq>,Model<TSeq>*)>;

template<typename TSeq = EPI_DEFAULT_TSEQ>
using MutFun = std::function<bool(Agent<TSeq>*,Virus<TSeq>&,Model<TSeq>*)>;

template<typename TSeq = EPI_DEFAULT_TSEQ>
using PostRecoveryFun = std::function<void(Agent<TSeq>*,Virus<TSeq>&,Model<TSeq>*)>;

template<typename TSeq = EPI_DEFAULT_TSEQ>
using VirusFun = std::function<epiworld_double(Agent<TSeq>*,Virus<TSeq>&,Model<TSeq>*)>;

template<typename TSeq = EPI_DEFAULT_TSEQ>
using UpdateFun = std::function<void(Agent<TSeq>*,Model<TSeq>*)>;

template<typename TSeq = EPI_DEFAULT_TSEQ>
using GlobalFun = std::function<void(Model<TSeq>*)>;

template<typename TSeq>
struct Event;

template<typename TSeq = EPI_DEFAULT_TSEQ>
using EventFun = std::function<void(Event<TSeq>&,Model<TSeq>*)>;

/**
 * @brief Decides how to distribute viruses at initialization
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
using VirusToAgentFun = std::function<void(Virus<TSeq>&,Model<TSeq>*)>;

/**
 * @brief Decides how to distribute tools at initialization
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
using ToolToAgentFun = std::function<void(Tool<TSeq>&,Model<TSeq>*)>;

/**
 * @brief Decides how to distribute entities at initialization
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
using EntityToAgentFun = std::function<void(Entity<TSeq>&,Model<TSeq>*)>;

/**
 * @brief Event data for update an agent
 * 
 * @tparam TSeq 
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
struct Event {
    Agent<TSeq> * agent;
    VirusPtr<TSeq> virus;
    ToolPtr<TSeq> tool;
    Entity<TSeq> * entity;
    epiworld_fast_int new_state;
    epiworld_fast_int queue;
    EventFun<TSeq> call;
    int idx_agent;
    int idx_object;
public:
/**
     * @brief Construct a new Event object
     * 
     * All the parameters are rather optional.
     * 
     * @param agent_ Agent over who the action will happen
     * @param virus_ Virus to add
     * @param tool_ Tool to add
     * @param virus_idx Index of virus to be removed (if needed)
     * @param tool_idx Index of tool to be removed (if needed)
     * @param new_state_ Next state
     * @param queue_ Efect on the queue
     * @param call_ The action call (if needed)
     * @param idx_agent_ Location of agent in object.
     * @param idx_object_ Location of object in agent.
     */
    Event(
        Agent<TSeq> * agent_,
        VirusPtr<TSeq> virus_,
        ToolPtr<TSeq> tool_,
        Entity<TSeq> * entity_,
        epiworld_fast_int new_state_,
        epiworld_fast_int queue_,
        EventFun<TSeq> call_,
        int idx_agent_,
        int idx_object_
    ) : agent(agent_), virus(virus_), tool(tool_), entity(entity_),
        new_state(new_state_),
        queue(queue_), call(call_), idx_agent(idx_agent_), idx_object(idx_object_) {
            return;
        };
};

/**
 * @name Constants in epiworld 
 * 
 * @details The following are the default values some probabilities and
 * rates take when no value has been specified in the model.
 */
///@{
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
    #define EPI_DEFAULT_VIRUS_PROB_RECOVERY     0.1428
#endif

#ifndef EPI_DEFAULT_VIRUS_PROB_DEATH
    #define EPI_DEFAULT_VIRUS_PROB_DEATH        0.0
#endif

#ifndef EPI_DEFAULT_INCUBATION_DAYS
    #define EPI_DEFAULT_INCUBATION_DAYS         7.0
#endif
///@}

#ifdef EPI_DEBUG
    #define EPI_DEBUG_PRINTF printf_epiworld

    #define EPI_DEBUG_ERROR(etype, msg) \
        (etype)("[[epi-debug]] (error) " + std::string(msg));

    #define EPI_DEBUG_NOTIFY_ACTIVE() \
        EPI_DEBUG_PRINTF("DEBUGGING ON (compiled with EPI_DEBUG defined)%s\n", "");

    #define EPI_DEBUG_ALL_NON_NEGATIVE(vect) \
        for (auto & v : vect) \
            if (static_cast<double>(v) < 0.0) \
                throw EPI_DEBUG_ERROR(std::logic_error, "A negative value not allowed.");

    #define EPI_DEBUG_SUM_DBL(vect, num) \
        double _epi_debug_sum = 0.0; \
        for (auto & v : vect) \
        {   \
            _epi_debug_sum += static_cast<double>(v);\
            if (_epi_debug_sum > static_cast<double>(num)) \
                throw EPI_DEBUG_ERROR(std::logic_error, "The sum of elements not reached."); \
        }

    #define EPI_DEBUG_SUM_INT(vect, num) \
        int _epi_debug_sum = 0; \
        for (auto & v : vect) \
        {   \
            _epi_debug_sum += static_cast<int>(v);\
            if (_epi_debug_sum > static_cast<int>(num)) \
                throw EPI_DEBUG_ERROR(std::logic_error, "The sum of elements not reached."); \
        }

    #define EPI_DEBUG_VECTOR_MATCH_INT(a, b, c) \
        if (a.size() != b.size())  {\
            EPI_DEBUG_PRINTF("In '%s'", std::string(c).c_str()); \
            EPI_DEBUG_PRINTF("Size of vector a: %lu\n", (a).size());\
            EPI_DEBUG_PRINTF("Size of vector b: %lu\n", (b).size());\
            throw EPI_DEBUG_ERROR(std::length_error, "The vectors do not match size."); \
        }\
        for (int _i = 0; _i < static_cast<int>(a.size()); ++_i) \
            if (a[_i] != b[_i]) {\
                EPI_DEBUG_PRINTF("In '%s'", std::string(c).c_str()); \
                EPI_DEBUG_PRINTF("Iterating the last 5 values%s:\n", ""); \
                for (int _j = std::max(0, static_cast<int>(_i) - 4); _j <= _i; ++_j) \
                { \
                    EPI_DEBUG_PRINTF( \
                        "a[%i]: %i; b[%i]: %i\n", \
                        _j, \
                        static_cast<int>(a[_j]), \
                        _j, static_cast<int>(b[_j])); \
                } \
                throw EPI_DEBUG_ERROR(std::logic_error, "The vectors do not match."); \
            }

    #define EPI_DEBUG_FAIL_AT_TRUE(a,b) \
        if (a) \
        {\
            throw EPI_DEBUG_ERROR(std::logic_error, b); \
        }

    #define epiexception(a) std::logic_error
#else
    #define EPI_DEBUG_PRINTF(fmt, ...)
    #define EPI_DEBUG_ERROR(fmt, ...)
    #define EPI_DEBUG_NOTIFY_ACTIVE()
    #define EPI_DEBUG_ALL_NON_NEGATIVE(vect)
    #define EPI_DEBUG_SUM_DBL(vect, num)
    #define EPI_DEBUG_SUM_INT(vect, num)
    #define EPI_DEBUG_VECTOR_MATCH_INT(a, b, c)
    #define EPI_DEBUG_FAIL_AT_TRUE(a, b) \
        if (a) \
            return false;
    #define epiexception(a) a
#endif

#if defined(EPI_DEBUG_NO_THREAD_ID) || (!defined(__OPENMP) && !defined(_OPENMP))
    #define EPI_GET_THREAD_ID() 0
#else
    #define EPI_GET_THREAD_ID() omp_get_thread_num()
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
    epiworld::Tool< tseq > & t, \
    epiworld::Agent< tseq > * p, \
    std::shared_ptr<epiworld::Virus< tseq >> v, \
    epiworld::Model< tseq > * m\
    )

/**
 * @brief Create a Tool within a function
 * 
 */
#define EPI_NEW_TOOL_LAMBDA(funname,tseq) \
    epiworld::ToolFun<tseq> funname = \
    [](epiworld::Tool<tseq> & t, \
    epiworld::Agent<tseq> * p, \
    std::shared_ptr<epiworld::Virus<tseq>> v, \
    epiworld::Model<tseq> * m) -> epiworld_double

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
    epiworld::Agent<tseq> * p, \
    epiworld::Virus<tseq> & v, \
    epiworld::Model<tseq> * m )

#define EPI_NEW_MUTFUN_LAMBDA(funname,tseq) \
    epiworld::MutFun<tseq> funname = \
    [](epiworld::Agent<tseq> * p, \
    epiworld::Virus<tseq> & v, \
    epiworld::Model<tseq> * m) -> void

#define EPI_NEW_POSTRECOVERYFUN(funname,tseq) inline void \
    (funname)( \
    epiworld::Agent<tseq> * p, \
    epiworld::Virus<tseq> & v, \
    epiworld::Model<tseq> * m\
    )

#define EPI_NEW_POSTRECOVERYFUN_LAMBDA(funname,tseq) \
    epiworld::PostRecoveryFun<tseq> funname = \
    [](epiworld::Agent<tseq> * p, \
    epiworld::Virus<tseq> & v , \
    epiworld::Model<tseq> * m) -> void

#define EPI_NEW_VIRUSFUN(funname,tseq) inline epiworld_double \
    (funname)( \
    epiworld::Agent<tseq> * p, \
    epiworld::Virus<tseq> & v, \
    epiworld::Model<tseq> * m\
    )

#define EPI_NEW_VIRUSFUN_LAMBDA(funname,TSeq) \
    epiworld::VirusFun<TSeq> funname = \
    [](epiworld::Agent<TSeq> * p, \
    epiworld::Virus<TSeq> & v, \
    epiworld::Model<TSeq> * m) -> epiworld_double

#define EPI_RUNIF() m->runif()

#define EPIWORLD_RUN(a) \
    if (a.get_verbose()) \
    { \
        printf_epiworld("Running the model...\n");\
    } \
    for (epiworld_fast_uint niter = 0; niter < a.get_ndays(); ++niter)

#define EPI_TOKENPASTE(a,b) a ## b
#define MPAR(num) *(m->EPI_TOKENPASTE(p,num))

#define EPI_NEW_UPDATEFUN(funname,tseq) inline void \
    (funname)(epiworld::Agent<tseq> * p, epiworld::Model<tseq> * m)

#define EPI_NEW_UPDATEFUN_LAMBDA(funname,tseq) \
    epiworld::UpdateFun<tseq> funname = \
    [](epiworld::Agent<tseq> * p, epiworld::Model<tseq> * m) -> void

#define EPI_NEW_GLOBALFUN(funname,tseq) inline void \
    (funname)(epiworld::Model<tseq>* m)

#define EPI_NEW_GLOBALFUN_LAMBDA(funname,tseq) \
    epiworld::GlobalFun<tseq> funname = \
    [](epiworld::Model<tseq>* m) -> void


#define EPI_NEW_ENTITYTOAGENTFUN(funname,tseq) inline void \
    (funname)(epiworld::Entity<tseq> & e, epiworld::Model<tseq> * m)

#define EPI_NEW_ENTITYTOAGENTFUN_LAMBDA(funname,tseq) \
    epiworld::EntityToAgentFun<tseq> funname = \
    [](epiworld::Entity<tseq> & e, epiworld::Model<tseq> * m) -> void

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
class Agent;

// Relevant for anything using vecHasher function ------------------------------
/**
 * @brief Vector hasher
 * @tparam T 
 */
template <typename T>
struct vecHasher {
    std::size_t operator()(std::vector< T > const&  dat) const noexcept {
        
        std::hash< T > hasher;
        std::size_t hash = hasher(dat[0u]);
        
        // ^ makes bitwise XOR
        // 0x9e3779b9 is a 32 bit constant (comes from the golden ratio)
        // << is a shift operator, something like lhs * 2^(rhs)
        if (dat.size() > 1u)
            for (epiworld_fast_uint i = 1u; i < dat.size(); ++i)
                hash ^= hasher(dat[i]) + 0x9e3779b9 + (hash<<6) + (hash>>2);
        
        return hash;
        
    }
};

template<typename Ta = epiworld_double, typename Tb = epiworld_fast_uint> 
using MapVec_type = std::unordered_map< std::vector< Ta >, Tb, vecHasher<Ta>>;

/**
 * @name Default sequence initializers
 * 
 * @details 
 * If the user does not provide a default sequence, this function is used when
 * a sequence needs to be initialized. Some examples: `Agent`, `Virus`, and
 * `Tool` need a default sequence.
 * 
 * @tparam TSeq 
 * @return TSeq 
 */
///@{
template<typename TSeq = int>
inline TSeq default_sequence(int seq_count);

// Making it 'static' so that we don't have problems when including the
// header. This is important during the linkage, e.g., in R.
// See https://en.cppreference.com/w/cpp/language/storage_duration#Linkage
// static int _n_sequences_created = 0;

template<>
inline bool default_sequence(int seq_count) {

    if (seq_count == 2)
        throw std::logic_error("Maximum number of sequence created.");

    return seq_count++ ? false : true;
}

template<>
inline int default_sequence(int seq_count) {
    return seq_count++;
}

template<>
inline epiworld_double default_sequence(int seq_count) {
    return static_cast<epiworld_double>(seq_count++);
}

template<>
inline std::vector<bool> default_sequence(int seq_count) {

    if (seq_count == 2)
        throw std::logic_error("Maximum number of sequence created.");

    return {seq_count++ ? false : true};
}

template<>
inline std::vector<int> default_sequence(int seq_count) {
    return {seq_count++};
}

template<>
inline std::vector<epiworld_double> default_sequence(int seq_count) {
    return {static_cast<epiworld_double>(seq_count++)};
}
///@}

/**
 * @brief Check whether `a` is included in `b`
 * 
 * @tparam Ta Type of `a`. Could be int, epiworld_double, etc.
 * @param a Scalar of class `Ta`.
 * @param b Vector `std::vector` of class `Ta`.
 * @return `true` if `a in b`, and `false` otherwise.
 */
template<typename Ta>
inline bool IN(const Ta & a, const std::vector< Ta > & b) noexcept
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
template<typename TSeq = EPI_DEFAULT_TSEQ, typename TDbl = epiworld_double >
inline int roulette(
    const std::vector< TDbl > & probs,
    Model<TSeq> * m
    )
{

    // Step 1: Computing the prob on none 
    TDbl p_none = 1.0;
    std::vector< int > certain_infection;
    certain_infection.reserve(probs.size());

    for (epiworld_fast_uint p = 0u; p < probs.size(); ++p)
    {
        p_none *= (1.0 - probs[p]);

        if (probs[p] > (1 - 1e-100))
            certain_infection.push_back(p);
        
    }

    TDbl r = static_cast<TDbl>(m->runif());
    // If there are one or more probs that go close to 1, sample
    // uniformly
    if (certain_infection.size() > 0)
        return certain_infection[std::floor(r * certain_infection.size())];

    // Step 2: Calculating the prob of none or single
    std::vector< TDbl > probs_only_p(probs.size());
    TDbl p_none_or_single = p_none;
    for (epiworld_fast_uint p = 0u; p < probs.size(); ++p)
    {
        probs_only_p[p] = probs[p] * (p_none / (1.0 - probs[p]));
        p_none_or_single += probs_only_p[p];
    }

    // Step 3: Roulette
    TDbl cumsum = p_none/p_none_or_single;
    if (r < cumsum)
    {
        return -1;
    }

    for (epiworld_fast_uint p = 0u; p < probs.size(); ++p)
    {
        // If it yield here, then bingo, the individual will acquire the disease
        cumsum += probs_only_p[p]/(p_none_or_single);
        if (r < cumsum)
            return static_cast<int>(p);
        
    }


    #ifdef EPI_DEBUG
    printf_epiworld("[epi-debug] roulette::cumsum = %.4f\n", cumsum);
    #endif

    return static_cast<int>(probs.size() - 1u);

}

template<typename TSeq>
inline int roulette(std::vector< double > & probs, Model<TSeq> * m)
{
    return roulette<TSeq, double>(probs, m);
}

template<typename TSeq>
inline int roulette(std::vector< float > & probs, Model<TSeq> * m)
{
    return roulette<TSeq, float>(probs, m);
}


template<typename TSeq>
inline int roulette(
    epiworld_fast_uint nelements,
    Model<TSeq> * m
    )
{

    if ((nelements * 2) > m->array_double_tmp.size())
    {
        throw std::logic_error(
            "Trying to sample from more data than there is in roulette!" +
            std::to_string(nelements) + " vs " + 
            std::to_string(m->array_double_tmp.size())
            );
    }

    // Step 1: Computing the prob on none 
    epiworld_double p_none = 1.0;
    epiworld_fast_uint ncertain = 0u;
    // std::vector< int > certain_infection;
    for (epiworld_fast_uint p = 0u; p < nelements; ++p)
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
    for (epiworld_fast_uint p = 0u; p < nelements; ++p)
    {
        m->array_double_tmp[nelements + p] = 
            m->array_double_tmp[p] * (p_none / (1.0 - m->array_double_tmp[p]));
        p_none_or_single += m->array_double_tmp[nelements + p];
    }

    // Step 3: Roulette
    epiworld_double cumsum = p_none/p_none_or_single;
    if (r < cumsum)
        return -1;

    for (epiworld_fast_uint p = 0u; p < nelements; ++p)
    {
        // If it yield here, then bingo, the individual will acquire the disease
        cumsum += m->array_double_tmp[nelements + p]/(p_none_or_single);
        if (r < cumsum)
            return static_cast<int>(p);
        
    }

    return static_cast<int>(nelements - 1u);

}

/**
 * @brief Read parameters from a yaml file
 * 
 * @details
 * The file should have the following structure:
 * ```yaml
 * # Comment
 * [name of parameter 1]: [value in T]
 * [name of parameter 2]: [value in T]
 * ...
 * ```
 * 
 * @tparam T Type of the parameter
 * @param fn Path to the file containing the parameters
 * @return std::map<std::string, T> 
 */
template <typename T>
inline std::map< std::string, T > read_yaml(std::string fn)
{

    std::ifstream paramsfile(fn);

    if (!paramsfile)
        throw std::logic_error("The file " + fn + " was not found.");

    std::regex pattern("^([^:]+)\\s*[:]\\s*([-]?[0-9]+|[-]?[0-9]*\\.[0-9]+)?\\s*$");

    std::string line;
    std::smatch match;
    auto empty = std::sregex_iterator();

    // Making room
    std::map<std::string, T> parameters;

    while (std::getline(paramsfile, line))
    {

        // Is it a comment or an empty line?
        if (std::regex_match(line, std::regex("^([*].+|//.+|#.+|\\s*)$")))
            continue;

        // Finding the patter, if it doesn't match, then error
        std::regex_match(line, match, pattern);

        if (match.empty())
            throw std::logic_error("The line does not match parameters:\n" + line);

        // Capturing the number
        std::string anumber = match[2u].str() + match[3u].str();
        T tmp_num = static_cast<T>(
            std::strtod(anumber.c_str(), nullptr)
            );

        std::string pname = std::regex_replace(
            match[1u].str(),
            std::regex("^\\s+|\\s+$"),
            "");

        // Adding the parameter to the map
        parameters[pname] = tmp_num;

    }

    return parameters;

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


    if (n_ < 0)
        throw std::invalid_argument("n must be greater or equal than 0.");

    if (width_ <= 0)
        throw std::invalid_argument("width must be greater than 0");

    width     = std::max(7, width_ - 7);
    n         = n_;
    step_size = n == 0? width : static_cast<epiworld_double>(width)/
        static_cast<epiworld_double>(n);
    last_loc  = 0;
    i         = 0;

}

inline void Progress::start()
{

    #ifndef EPI_DEBUG
    for (int j = 0; j < (width); ++j)
    {
        printf_epiworld("_");
    }
    printf_epiworld("\n");
    #endif
}

inline void Progress::next() {

    if (i == 0)
        start();

    cur_loc = std::floor((++i) * step_size);

    #ifndef EPI_DEBUG
    for (int j = 0; j < (cur_loc - last_loc); ++j)
    { 
        printf_epiworld("|");
    }
    #endif

    last_loc = cur_loc;

}

inline void Progress::end() {

    #ifndef EPI_DEBUG
    printf_epiworld(" done.\n");
    #endif

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/progress.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/modeldiagram-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELDIAGRAM_HPP
#define EPIWORLD_MODELDIAGRAM_HPP

class ModelDiagram {
private:

    std::map< std::pair< std::string, std::string >, int > data;

    std::vector< std::string > states;
    std::vector< epiworld_double > tprob;

    void draw_mermaid(
        std::string fn_output = "",
        bool self = false
    );


    int n_runs = 0; ///< The number of runs included in the diagram.

    /**
     * @brief Reads the transitions from a file.
     * @details
     * The file is assumed to come from the output of [Model::write_data()],
     * and should be in the following format:
     * ```
     * 0 "S" "I" 10
     * 1 "I" "R" 5
     * 2 "R" "S" 3
     * ...
     * ```
     * The first column is the step, the second column is the state from which
     * the transition is made, the third column is the state to which the
     * transition is made, and the fourth column is the number of transitions.
     * 
     * @param fn_transition The name of the file to read.
     * @return void
     * @throws std::runtime_error if the file cannot be opened.
     */
    void read_transitions(
        const std::string & fn_transition
    );

    /**
     * @brief In the case of multiple files (like from run_multiple)
     * @param fns_transition The names of the files to read.
     * @details
     * It will read the transitions from multiple files and concatenate them.
     * into the same object.
     */
    void read_transitions(
        const std::vector< std::string > & fns_transition
    );
    
    /**
     * @brief Computes the transition probability matrix.
     * @param normalize Whether to compute only the counts,
     * otherwise the probabilities will be computed (row-stochastic).
     * @return void
     */
    void transition_probability(bool normalize = true);

    void clear();

public:

    ModelDiagram() {};

    void draw_from_data(
        const std::vector< std::string > & states,
        const std::vector< epiworld_double > & tprob,
        const std::string & fn_output = "",
        bool self = false
    );

    void draw_from_file(
        const std::string & fn_transition,
        const std::string & fn_output = "",
        bool self = false
    );

    void draw_from_files(
        const std::vector< std::string > & fns_transition,
        const std::string & fn_output = "",
        bool self = false
    );

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/modeldiagram-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/modeldiagram-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELDIAGRAM_MEAT_HPP
#define EPIWORLD_MODELDIAGRAM_MEAT_HPP

inline void ModelDiagram::read_transitions(
    const std::string & fn_transition
) {

    // Checking if the file exists
    std::ifstream file(fn_transition);

    if (!file.is_open())
        throw std::runtime_error(
            "Could not open the file '" + 
            fn_transition + "' for reading."
        );

    // Reading the data
    std::string line;
    int i = 0;
    while (std::getline(file, line))
    {

        // Skipping the first line
        if (i++ == 0)
            continue;

        std::istringstream iss(line);
        #ifdef EPI_DEBUG
        int t;
        iss >> t;
        #endif

        int i_;
        std::string from_, to_;
        int counts_;

        iss >> i_;

        // Read the quoted strings
        iss >> std::quoted(from_) >> std::quoted(to_);

        // Read the integer
        iss >> counts_;

        if (counts_ > 0)
        {
            auto idx = std::make_pair(from_, to_);
            if (data.find(idx) == data.end())
                data[idx] = counts_;
            else
                data[idx] += counts_;
        }

    }

    // Incrementing the number of runs
    this->n_runs++;

}

inline void ModelDiagram::read_transitions(
    const std::vector< std::string > & fns_transition
)
{

    for (const auto & fn: fns_transition)
        this->read_transitions(fn);

    return;

}

inline void ModelDiagram::transition_probability(
    bool normalize
)
{

    // Generating the map of states
    std::set< std::string > states_set;

    for (const auto & d: data)
    {
        states_set.insert(d.first.first);
        states_set.insert(d.first.second);
    }

    // Generating the transition matrix
    states = std::vector< std::string >(states_set.begin(), states_set.end());
    size_t n_states = states.size();
    tprob.resize(n_states * n_states);
    std::fill(tprob.begin(), tprob.end(), 0.0);

    std::vector< epiworld_double > rowsum(n_states, 0.0);
    for (size_t i = 0; i < n_states; ++i)
    {

        for (size_t j = 0; j < n_states; ++j)
        {

            auto key = std::make_pair(states[i], states[j]);
            if (data.find(key) != data.end())
                tprob[i + j * n_states] = static_cast<epiworld_double>(
                    data[key]
                );

            if (normalize)
                rowsum[i] += tprob[i + j * n_states];

        }

        if (normalize)
        {
            for (size_t j = 0; j < n_states; ++j)
                tprob[i + j * n_states] /= rowsum[i];
        }

    }

    return;
    

}

inline void ModelDiagram::clear()
{
    data.clear();
    states.clear();
    tprob.clear();
    n_runs = 0;
    return;
}

inline void ModelDiagram::draw_mermaid(
    std::string fn_output,
    bool self
)
{

    // Getting a sorting vector of indices from the states
    // string vector
    std::vector< size_t > idx(states.size());
    std::iota(idx.begin(), idx.end(), 0u);

    std::sort(
        idx.begin(),
        idx.end(),
        [&states = this->states](size_t i, size_t j) {
            return states[i] < states[j];
        }
    );

    std::vector< std::string > states_ids;
    for (size_t i = 0u; i < states.size(); ++i)
        states_ids.push_back("s" + std::to_string(i));

    std::string graph = "flowchart LR\n";

    // Declaring the states
    for (size_t i = 0u; i < states.size(); ++i)
    {
        graph += "\t" + states_ids[i] + "[" + states[idx[i]] + "]\n";
    }

    // Adding the transitions
    size_t n_states = states.size();
    for (size_t i = 0u; i < states.size(); ++i)
    {
        for (size_t j = 0u; j < states.size(); ++j)
        {
            if (!self && i == j)
                continue;

            if (tprob[idx[i] + idx[j] * n_states] > 0.0)
            {
                graph += "\t" + states_ids[i] + " -->|" + 
                    std::to_string(tprob[idx[i] + idx[j] * n_states]) + 
                    "| " + states_ids[j] + "\n";
            }
        }
    }

    if (fn_output != "")
    {
        std::ofstream file(fn_output);

        if (!file.is_open())
            throw std::runtime_error(
                "Could not open the file " +
                fn_output + 
                " for writing."
            );

        file << graph;
        file.close();
        
    } else {
        printf_epiworld("%s\n", graph.c_str());
    }

    return;

}

inline void ModelDiagram::draw_from_file(
    const std::string & fn_transition,
    const std::string & fn_output,
    bool self
) {

    this->clear();

    // Loading the transition file
    this->read_transitions(fn_transition);

    // Computing the transition probability
    this->transition_probability();

    // Actually drawing the diagram
    this->draw_mermaid(fn_output, self);

    return;
  
}

inline void ModelDiagram::draw_from_files(
    const std::vector< std::string > & fns_transition,
    const std::string & fn_output,
    bool self
) {

    this->clear();

    // Loading the transition files
    this->read_transitions(fns_transition);

    // Computing the transition probability
    this->transition_probability();

    // Actually drawing the diagram
    this->draw_mermaid(fn_output, self);

    return;
}

inline void ModelDiagram::draw_from_data(
    const std::vector< std::string > & states,
    const std::vector< epiworld_double > & tprob,
    const std::string & fn_output,
    bool self
) {

    this->clear();

    this->states = states;
    this->tprob = tprob;

    this->draw_mermaid(fn_output, self);

    return;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/modeldiagram-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/math/distributions.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MATH_DISTRIBUTIONS_HPP
#define EPIWORLD_MATH_DISTRIBUTIONS_HPP

// Implementing the factorial function
/**
 * @brief Compute the log of the factorial
 * 
 * @param n Number
 * 
 * @return The log of the factorial
 */
inline double log_factorial(int n)
{
    if (n == 0)
        return 0.0;
    return std::log(static_cast<double>(n)) + log_factorial(n-1);
}

/**
 * @brief Compute the Poisson probability
 * 
 * @param k Number of events
 * @param lambda Rate
 * @param max_n Maximum number of events
 * @param as_log Return the log of the probability
 * 
 * @return The Poisson probability
 */
inline double dpois(
    int k,
    double lambda,
    int max_n = 100,
    bool as_log = false
    )
{

    if (max_n < k)
        throw std::runtime_error("max_n must be greater than k");

    double res = k * std::log(lambda) - lambda - log_factorial(
        std::min(k, max_n)
        );
    
    return as_log ? res : std::exp(res);
}

/**
 * @brief Compute the probability of the generation interval
 * 
 * @details
 * If `p_0_approx` is negative, it will be computed using the Poisson
 * distribution. If `normalizing` is negative, it will be computed on the fly
 * 
 * @param g Generation interval
 * @param S Population size
 * @param p_c Probability of contact
 * @param p_i Probability of infection
 * @param p_r Probability of recovery
 * @param p_0_approx Approximation of the probability of not being infected
 * @param normalizing Normalizing constant
 * @param max_contacts Maximum number of contacts
 * @param max_days Maximum number of days
 * 
 * @return The probability of the generation interval
 * 
 */
inline double dgenint(
    int g,
    double S,
    double p_c,
    double p_i,
    double p_r,
    double & p_0_approx,
    double & normalizing,
    int max_contacts = 200,
    int max_days = 200
    ) {

    if ((g < 1) || (g > max_days))
        return 0.0;

    if (p_0_approx < 0.0)
    {

        p_0_approx = 0.0;
        for (int i = 0; i < max_contacts; ++i)
        {

            p_0_approx += std::exp(
                dpois(i, S * p_c, max_contacts, true) +
                std::log(1.0 - p_i) * static_cast<double>(i)
                );

        }
    }

    double g_dbl = static_cast<double>(g);

    if (normalizing < 0.0)
    {

        normalizing = 1.0;
        double log1_p_r = std::log(1.0 - p_r);
        double log_p_r = std::log(p_r);
        double log_p_0_approx = std::log(p_0_approx);
        for (int i = 1; i <= max_days; ++i)
        {

            double i_dbl = static_cast<double>(i);

            normalizing -= std::exp(
                log1_p_r * (i_dbl - 1.0) +
                log_p_r +
                log_p_0_approx * (i_dbl - 1.0)
                );
        }

    }


    return std::exp(
        std::log(1 - p_r) * (g_dbl)+
        std::log(p_0_approx) * (g_dbl - 1.0) +
        std::log(1.0 - p_0_approx) -
        std::log(normalizing)
        );

}

// Mean of the generation interval
/**
 * @brief Compute the mean of the generation interval
 * @param S Population size.
 * @param p_c Probability of contact.
 * @param p_i Probability of infection.
 * @param p_r Probability of recovery.
 * @param max_days Maximum number of days.
 * @param max_n Maximum number of contacts.
 * 
 * @return The mean of the generation interval
 */
inline double gen_int_mean(
    double S,
    double p_c,
    double p_i,
    double p_r,
    int max_days = 200,
    int max_n = 200
    ) {

    double mean = 0.0;
    double p_0_approx = -1.0;
    double normalizing = -1.0;
    for (int i = 1; i < max_days; ++i)
    {
        mean += 
            static_cast<double>(i) *
            dgenint(
                i, S, p_c, p_i, p_r, p_0_approx, normalizing, max_n, max_days
                );

    }

    return mean;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/math/distributions.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/math/lfmcmc.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_LFMCMC_HPP
#define EPIWORLD_LFMCMC_HPP

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//math/lfmcmc/lfmcmc-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_LFMCMC_BONES_HPP
#define EPIWORLD_LFMCMC_BONES_HPP

#ifndef epiworld_double
    #define epiworld_double float
#endif


template<typename TData>
class LFMCMC;

template<typename TData>
using LFMCMCSimFun = std::function<TData(const std::vector< epiworld_double >&,LFMCMC<TData>*)>;

template<typename TData>
using LFMCMCSummaryFun = std::function<void(std::vector< epiworld_double >&,const TData&,LFMCMC<TData>*)>;

template<typename TData>
using LFMCMCProposalFun = std::function<void(std::vector< epiworld_double >&,const std::vector< epiworld_double >&,LFMCMC<TData>*)>;

template<typename TData>
using LFMCMCKernelFun = std::function<epiworld_double(const std::vector< epiworld_double >&,const std::vector< epiworld_double >&,epiworld_double,LFMCMC<TData>*)>;

/**
 * @brief Proposal function
 * @param new_params Vector where to save the new parameters.
 * @param old_params Vector of reference parameters.
 * @param m LFMCMC model.
 * @tparam TData 
 */
template<typename TData>
inline void proposal_fun_normal(
    std::vector< epiworld_double >& new_params,
    const std::vector< epiworld_double >& old_params,
    LFMCMC<TData>* m
);

/**
 * @brief Factory for a reflective normal kernel
 * 
 * @details Reflective kernel corrects proposals by forcing them to be
 * within prespecified boundaries. 
 * 
 * @tparam TData 
 * @param scale Scale of the normal kernel
 * @param lb Lower bound (applies the same to all parameters)
 * @param ub Upper bound (applies the same to all parameters)
 * @return LFMCMCProposalFun<TData> 
 */
template<typename TData>
inline LFMCMCProposalFun<TData> make_proposal_norm_reflective(
    epiworld_double scale,
    epiworld_double lb = std::numeric_limits<epiworld_double>::min(),
    epiworld_double ub = std::numeric_limits<epiworld_double>::max()
);

/**
 * @brief Uniform proposal kernel
 * 
 * Proposals are made within a radious 1 of the current
 * state of the parameters.
 * 
 * @param new_params Where to write the new parameters
 * @param old_params Reference parameters
 * @tparam TData 
 * @param m LFMCMC model.
 */
template<typename TData>
inline void proposal_fun_unif(
    std::vector< epiworld_double >& new_params,
    const std::vector< epiworld_double >& old_params,
    LFMCMC<TData>* m
);

/**
 * @brief Uses the uniform kernel with euclidean distance
 * 
 * @param simulated_stats Vector of statistics based on 
 * simulated data
 * @param observed_stats Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_uniform(
    const std::vector< epiworld_double >& simulated_stats,
    const std::vector< epiworld_double >& observed_stats,
    epiworld_double epsilon,
    LFMCMC<TData>* m
);

/**
 * @brief Gaussian kernel
 * 
 * @tparam TData 
 * @param simulated_stats Vector of statistics based on 
 * simulated data
 * @param observed_stats Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_gaussian(
    const std::vector< epiworld_double >& simulated_stats,
    const std::vector< epiworld_double >& observed_stats,
    epiworld_double epsilon,
    LFMCMC<TData>* m
);

/**
 * @brief Likelihood-Free Markov Chain Monte Carlo
 * 
 * @tparam TData Type of data that is generated
 */
template<typename TData>
class LFMCMC {
private:

    // Random number sampling
    std::shared_ptr< std::mt19937 > m_engine = nullptr;
    
    std::shared_ptr< std::uniform_real_distribution<> > runifd =
        std::make_shared< std::uniform_real_distribution<> >(0.0, 1.0);

    std::shared_ptr< std::normal_distribution<> > rnormd =
        std::make_shared< std::normal_distribution<> >(0.0);

    std::shared_ptr< std::gamma_distribution<> > rgammad = 
        std::make_shared< std::gamma_distribution<> >();

    // Process data
    TData m_observed_data;
    std::vector< TData > * m_simulated_data = nullptr;

    // Information about the size of the process
    size_t m_n_samples;
    size_t m_n_stats;
    size_t m_n_params;

    epiworld_double m_epsilon;

    std::vector< epiworld_double > m_initial_params;             ///< Initial parameters
    std::vector< epiworld_double > m_current_proposed_params;    ///< Proposed parameters for the next sample
    std::vector< epiworld_double > m_current_accepted_params;    ///< Most recently accepted parameters (current state of MCMC)
    std::vector< epiworld_double > m_current_proposed_stats;     ///< Statistics from simulation run with proposed parameters
    std::vector< epiworld_double > m_current_accepted_stats;     ///< Statistics from simulation run with most recently accepted params

    std::vector< epiworld_double > m_observed_stats;             ///< Observed statistics

    std::vector< epiworld_double > m_all_sample_params;          ///< Parameter samples
    std::vector< epiworld_double > m_all_sample_stats;           ///< Statistic samples
    std::vector< bool >            m_all_sample_acceptance;      ///< Indicator if sample was accepted
    std::vector< epiworld_double > m_all_sample_drawn_prob;      ///< Drawn probabilities (runif()) for each sample
    std::vector< epiworld_double > m_all_sample_kernel_scores;   ///< Kernel scores for each sample

    std::vector< epiworld_double > m_all_accepted_params;        ///< Posterior distribution of parameters from accepted samples
    std::vector< epiworld_double > m_all_accepted_stats;         ///< Posterior distribution of statistics from accepted samples
    std::vector< epiworld_double > m_all_accepted_kernel_scores; ///< Kernel scores for each accepted sample

    // Functions
    LFMCMCSimFun<TData> m_simulation_fun;
    LFMCMCSummaryFun<TData> m_summary_fun;
    LFMCMCProposalFun<TData> m_proposal_fun = proposal_fun_normal<TData>;
    LFMCMCKernelFun<TData> m_kernel_fun     = kernel_fun_uniform<TData>;

    // Misc
    std::vector< std::string > m_param_names;
    std::vector< std::string > m_stat_names;

    std::chrono::time_point<std::chrono::steady_clock> m_start_time;
    std::chrono::time_point<std::chrono::steady_clock> m_end_time;

    // Timing
    // std::chrono::milliseconds
    std::chrono::duration<epiworld_double,std::micro> m_elapsed_time = 
        std::chrono::duration<epiworld_double,std::micro>::zero();

    inline void get_elapsed_time(
        std::string unit,
        epiworld_double * last_elapsed,
        std::string * unit_abbr,
        bool print
    ) const;

    void chrono_start();
    void chrono_end();

    // Progress
    bool verbose = true;
    Progress progress_bar;
    
public:

    void run(
        std::vector< epiworld_double > params_init_,
        size_t n_samples_,
        epiworld_double epsilon_,
        int seed = -1
        );

    LFMCMC() {};
    LFMCMC(const TData & observed_data_) : m_observed_data(observed_data_) {};
    ~LFMCMC() {};

    // Setting LFMCMC variables
    void set_observed_data(const TData & observed_data_) {m_observed_data = observed_data_;};
    
    void set_proposal_fun(LFMCMCProposalFun<TData> fun);
    void set_simulation_fun(LFMCMCSimFun<TData> fun);
    void set_summary_fun(LFMCMCSummaryFun<TData> fun);
    void set_kernel_fun(LFMCMCKernelFun<TData> fun);

    void set_params_names(std::vector< std::string > names);
    void set_stats_names(std::vector< std::string > names);
    
    /**
     * @name Random number generation
     * 
     * @param eng 
     */
    ///@{
    void set_rand_engine(std::shared_ptr< std::mt19937 > & eng);
    std::shared_ptr< std::mt19937 > & get_rand_endgine();
    void seed(epiworld_fast_uint s);
    void set_rand_gamma(epiworld_double alpha, epiworld_double beta);
    epiworld_double runif();
    epiworld_double rnorm();
    epiworld_double rgamma();
    epiworld_double runif(epiworld_double lb, epiworld_double ub);
    epiworld_double rnorm(epiworld_double mean, epiworld_double sd);
    epiworld_double rgamma(epiworld_double alpha, epiworld_double beta);
    ///@}

    // Accessing parameters of the function
    size_t get_n_samples() const {return m_n_samples;};
    size_t get_n_stats() const {return m_n_stats;};
    size_t get_n_params() const {return m_n_params;};
    epiworld_double get_epsilon() const {return m_epsilon;};

    const std::vector< epiworld_double > & get_initial_params() const {return m_initial_params;};
    const std::vector< epiworld_double > & get_current_proposed_params() const {return m_current_proposed_params;};
    const std::vector< epiworld_double > & get_current_accepted_params() const {return m_current_accepted_params;};
    const std::vector< epiworld_double > & get_current_proposed_stats() const {return m_current_proposed_stats;};
    const std::vector< epiworld_double > & get_current_accepted_stats() const {return m_current_accepted_stats;};

    const std::vector< epiworld_double > & get_observed_stats() const {return m_observed_stats;};

    const std::vector< epiworld_double > & get_all_sample_params() const {return m_all_sample_params;};
    const std::vector< epiworld_double > & get_all_sample_stats() const {return m_all_sample_stats;};
    const std::vector< bool >            & get_all_sample_acceptance() const {return m_all_sample_acceptance;};
    const std::vector< epiworld_double > & get_all_sample_drawn_prob() const {return m_all_sample_drawn_prob;};
    const std::vector< epiworld_double > & get_all_sample_kernel_scores() const {return m_all_sample_kernel_scores;};

    const std::vector< epiworld_double > & get_all_accepted_params() const {return m_all_accepted_params;};
    const std::vector< epiworld_double > & get_all_accepted_stats() const {return m_all_accepted_stats;};
    const std::vector< epiworld_double > & get_all_accepted_kernel_scores() const {return m_all_accepted_kernel_scores;};
    
    std::vector< TData > * get_simulated_data() const {return m_simulated_data;};

    std::vector< epiworld_double > get_mean_params();
    std::vector< epiworld_double > get_mean_stats();

    // Printing
    LFMCMC<TData> & verbose_off();
    LFMCMC<TData> & verbose_on();
    void print(size_t burnin = 0u) const;

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//math/lfmcmc/lfmcmc-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//math/lfmcmc/lfmcmc-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_LFMCMC_MEAT_HPP
#define EPIWORLD_LFMCMC_MEAT_HPP

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//math//lfmcmc/lfmcmc-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_LFMCMC_BONES_HPP
#define EPIWORLD_LFMCMC_BONES_HPP

#ifndef epiworld_double
    #define epiworld_double float
#endif


template<typename TData>
class LFMCMC;

template<typename TData>
using LFMCMCSimFun = std::function<TData(const std::vector< epiworld_double >&,LFMCMC<TData>*)>;

template<typename TData>
using LFMCMCSummaryFun = std::function<void(std::vector< epiworld_double >&,const TData&,LFMCMC<TData>*)>;

template<typename TData>
using LFMCMCProposalFun = std::function<void(std::vector< epiworld_double >&,const std::vector< epiworld_double >&,LFMCMC<TData>*)>;

template<typename TData>
using LFMCMCKernelFun = std::function<epiworld_double(const std::vector< epiworld_double >&,const std::vector< epiworld_double >&,epiworld_double,LFMCMC<TData>*)>;

/**
 * @brief Proposal function
 * @param new_params Vector where to save the new parameters.
 * @param old_params Vector of reference parameters.
 * @param m LFMCMC model.
 * @tparam TData 
 */
template<typename TData>
inline void proposal_fun_normal(
    std::vector< epiworld_double >& new_params,
    const std::vector< epiworld_double >& old_params,
    LFMCMC<TData>* m
);

/**
 * @brief Factory for a reflective normal kernel
 * 
 * @details Reflective kernel corrects proposals by forcing them to be
 * within prespecified boundaries. 
 * 
 * @tparam TData 
 * @param scale Scale of the normal kernel
 * @param lb Lower bound (applies the same to all parameters)
 * @param ub Upper bound (applies the same to all parameters)
 * @return LFMCMCProposalFun<TData> 
 */
template<typename TData>
inline LFMCMCProposalFun<TData> make_proposal_norm_reflective(
    epiworld_double scale,
    epiworld_double lb = std::numeric_limits<epiworld_double>::min(),
    epiworld_double ub = std::numeric_limits<epiworld_double>::max()
);

/**
 * @brief Uniform proposal kernel
 * 
 * Proposals are made within a radious 1 of the current
 * state of the parameters.
 * 
 * @param new_params Where to write the new parameters
 * @param old_params Reference parameters
 * @tparam TData 
 * @param m LFMCMC model.
 */
template<typename TData>
inline void proposal_fun_unif(
    std::vector< epiworld_double >& new_params,
    const std::vector< epiworld_double >& old_params,
    LFMCMC<TData>* m
);

/**
 * @brief Uses the uniform kernel with euclidean distance
 * 
 * @param simulated_stats Vector of statistics based on 
 * simulated data
 * @param observed_stats Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_uniform(
    const std::vector< epiworld_double >& simulated_stats,
    const std::vector< epiworld_double >& observed_stats,
    epiworld_double epsilon,
    LFMCMC<TData>* m
);

/**
 * @brief Gaussian kernel
 * 
 * @tparam TData 
 * @param simulated_stats Vector of statistics based on 
 * simulated data
 * @param observed_stats Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_gaussian(
    const std::vector< epiworld_double >& simulated_stats,
    const std::vector< epiworld_double >& observed_stats,
    epiworld_double epsilon,
    LFMCMC<TData>* m
);

/**
 * @brief Likelihood-Free Markov Chain Monte Carlo
 * 
 * @tparam TData Type of data that is generated
 */
template<typename TData>
class LFMCMC {
private:

    // Random number sampling
    std::shared_ptr< std::mt19937 > m_engine = nullptr;
    
    std::shared_ptr< std::uniform_real_distribution<> > runifd =
        std::make_shared< std::uniform_real_distribution<> >(0.0, 1.0);

    std::shared_ptr< std::normal_distribution<> > rnormd =
        std::make_shared< std::normal_distribution<> >(0.0);

    std::shared_ptr< std::gamma_distribution<> > rgammad = 
        std::make_shared< std::gamma_distribution<> >();

    // Process data
    TData m_observed_data;
    std::vector< TData > * m_simulated_data = nullptr;

    // Information about the size of the process
    size_t m_n_samples;
    size_t m_n_stats;
    size_t m_n_params;

    epiworld_double m_epsilon;

    std::vector< epiworld_double > m_initial_params;             ///< Initial parameters
    std::vector< epiworld_double > m_current_proposed_params;    ///< Proposed parameters for the next sample
    std::vector< epiworld_double > m_current_accepted_params;    ///< Most recently accepted parameters (current state of MCMC)
    std::vector< epiworld_double > m_current_proposed_stats;     ///< Statistics from simulation run with proposed parameters
    std::vector< epiworld_double > m_current_accepted_stats;     ///< Statistics from simulation run with most recently accepted params

    std::vector< epiworld_double > m_observed_stats;             ///< Observed statistics

    std::vector< epiworld_double > m_all_sample_params;          ///< Parameter samples
    std::vector< epiworld_double > m_all_sample_stats;           ///< Statistic samples
    std::vector< bool >            m_all_sample_acceptance;      ///< Indicator if sample was accepted
    std::vector< epiworld_double > m_all_sample_drawn_prob;      ///< Drawn probabilities (runif()) for each sample
    std::vector< epiworld_double > m_all_sample_kernel_scores;   ///< Kernel scores for each sample

    std::vector< epiworld_double > m_all_accepted_params;        ///< Posterior distribution of parameters from accepted samples
    std::vector< epiworld_double > m_all_accepted_stats;         ///< Posterior distribution of statistics from accepted samples
    std::vector< epiworld_double > m_all_accepted_kernel_scores; ///< Kernel scores for each accepted sample

    // Functions
    LFMCMCSimFun<TData> m_simulation_fun;
    LFMCMCSummaryFun<TData> m_summary_fun;
    LFMCMCProposalFun<TData> m_proposal_fun = proposal_fun_normal<TData>;
    LFMCMCKernelFun<TData> m_kernel_fun     = kernel_fun_uniform<TData>;

    // Misc
    std::vector< std::string > m_param_names;
    std::vector< std::string > m_stat_names;

    std::chrono::time_point<std::chrono::steady_clock> m_start_time;
    std::chrono::time_point<std::chrono::steady_clock> m_end_time;

    // Timing
    // std::chrono::milliseconds
    std::chrono::duration<epiworld_double,std::micro> m_elapsed_time = 
        std::chrono::duration<epiworld_double,std::micro>::zero();

    inline void get_elapsed_time(
        std::string unit,
        epiworld_double * last_elapsed,
        std::string * unit_abbr,
        bool print
    ) const;

    void chrono_start();
    void chrono_end();

    // Progress
    bool verbose = true;
    Progress progress_bar;
    
public:

    void run(
        std::vector< epiworld_double > params_init_,
        size_t n_samples_,
        epiworld_double epsilon_,
        int seed = -1
        );

    LFMCMC() {};
    LFMCMC(const TData & observed_data_) : m_observed_data(observed_data_) {};
    ~LFMCMC() {};

    // Setting LFMCMC variables
    void set_observed_data(const TData & observed_data_) {m_observed_data = observed_data_;};
    
    void set_proposal_fun(LFMCMCProposalFun<TData> fun);
    void set_simulation_fun(LFMCMCSimFun<TData> fun);
    void set_summary_fun(LFMCMCSummaryFun<TData> fun);
    void set_kernel_fun(LFMCMCKernelFun<TData> fun);

    void set_params_names(std::vector< std::string > names);
    void set_stats_names(std::vector< std::string > names);
    
    /**
     * @name Random number generation
     * 
     * @param eng 
     */
    ///@{
    void set_rand_engine(std::shared_ptr< std::mt19937 > & eng);
    std::shared_ptr< std::mt19937 > & get_rand_endgine();
    void seed(epiworld_fast_uint s);
    void set_rand_gamma(epiworld_double alpha, epiworld_double beta);
    epiworld_double runif();
    epiworld_double rnorm();
    epiworld_double rgamma();
    epiworld_double runif(epiworld_double lb, epiworld_double ub);
    epiworld_double rnorm(epiworld_double mean, epiworld_double sd);
    epiworld_double rgamma(epiworld_double alpha, epiworld_double beta);
    ///@}

    // Accessing parameters of the function
    size_t get_n_samples() const {return m_n_samples;};
    size_t get_n_stats() const {return m_n_stats;};
    size_t get_n_params() const {return m_n_params;};
    epiworld_double get_epsilon() const {return m_epsilon;};

    const std::vector< epiworld_double > & get_initial_params() const {return m_initial_params;};
    const std::vector< epiworld_double > & get_current_proposed_params() const {return m_current_proposed_params;};
    const std::vector< epiworld_double > & get_current_accepted_params() const {return m_current_accepted_params;};
    const std::vector< epiworld_double > & get_current_proposed_stats() const {return m_current_proposed_stats;};
    const std::vector< epiworld_double > & get_current_accepted_stats() const {return m_current_accepted_stats;};

    const std::vector< epiworld_double > & get_observed_stats() const {return m_observed_stats;};

    const std::vector< epiworld_double > & get_all_sample_params() const {return m_all_sample_params;};
    const std::vector< epiworld_double > & get_all_sample_stats() const {return m_all_sample_stats;};
    const std::vector< bool >            & get_all_sample_acceptance() const {return m_all_sample_acceptance;};
    const std::vector< epiworld_double > & get_all_sample_drawn_prob() const {return m_all_sample_drawn_prob;};
    const std::vector< epiworld_double > & get_all_sample_kernel_scores() const {return m_all_sample_kernel_scores;};

    const std::vector< epiworld_double > & get_all_accepted_params() const {return m_all_accepted_params;};
    const std::vector< epiworld_double > & get_all_accepted_stats() const {return m_all_accepted_stats;};
    const std::vector< epiworld_double > & get_all_accepted_kernel_scores() const {return m_all_accepted_kernel_scores;};
    
    std::vector< TData > * get_simulated_data() const {return m_simulated_data;};

    std::vector< epiworld_double > get_mean_params();
    std::vector< epiworld_double > get_mean_stats();

    // Printing
    LFMCMC<TData> & verbose_off();
    LFMCMC<TData> & verbose_on();
    void print(size_t burnin = 0u) const;

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//math//lfmcmc/lfmcmc-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/**
 * @brief Proposal function
 * @param new_params Vector where to save the new parameters.
 * @param old_params Vector of reference parameters.
 * @param m LFMCMC model.
 * @tparam TData 
 */
template<typename TData>
inline void proposal_fun_normal(
    std::vector< epiworld_double >& new_params,
    const std::vector< epiworld_double >& old_params,
    LFMCMC<TData>* m
) {

    for (size_t p = 0u; p < m->get_n_params(); ++p)
        new_params[p] = old_params[p] + m->rnorm();

    return;
}

/**
 * @brief Factory for a reflective normal kernel
 * 
 * @details Reflective kernel corrects proposals by forcing them to be
 * within prespecified boundaries. 
 * 
 * @tparam TData 
 * @param scale Scale of the normal kernel
 * @param lb Lower bound (applies the same to all parameters)
 * @param ub Upper bound (applies the same to all parameters)
 * @return LFMCMCProposalFun<TData> 
 */
template<typename TData>
inline LFMCMCProposalFun<TData> make_proposal_norm_reflective(
    epiworld_double scale,
    epiworld_double lb,
    epiworld_double ub
) {

    LFMCMCProposalFun<TData> fun =
        [scale,lb,ub](
            std::vector< epiworld_double >& new_params,
            const std::vector< epiworld_double >& old_params,
            LFMCMC<TData>* m
        ) {

        // Making the proposal
        for (size_t p = 0u; p < m->get_n_params(); ++p)
            new_params[p] = old_params[p] + m->rnorm() * scale;

        // Checking boundaries
        epiworld_double d = ub - lb;
        int odd;
        epiworld_double d_above, d_below;
        for (auto & p : new_params)
        {

            // Correcting if parameter goes above the upper bound
            if (p > ub)
            {
                d_above = p - ub;
                odd     = static_cast<int>(std::floor(d_above / d)) % 2;
                d_above = d_above - std::floor(d_above / d) * d;

                p = (lb + d_above) * odd +
                    (ub - d_above) * (1 - odd);

            // Correcting if parameter goes below upper bound
            } else if (p < lb)
            {
                d_below = lb - p;
                int odd = static_cast<int>(std::floor(d_below / d)) % 2;
                d_below = d_below - std::floor(d_below / d) * d;

                p = (ub - d_below) * odd +
                    (lb + d_below) * (1 - odd);
            }

        }

        #ifdef EPI_DEBUG
        for (auto & p : new_params)
            if (p < lb || p > ub)
                throw std::range_error("The parameter is out of bounds.");
        #endif


        return;

    };

    return fun;
}

/**
 * @brief Uniform proposal kernel
 * 
 * Proposals are made within a radious 1 of the current
 * state of the parameters.
 * 
 * @param new_params Where to write the new parameters
 * @param old_params Reference parameters
 * @tparam TData 
 * @param m LFMCMC model.
 */
template<typename TData>
inline void proposal_fun_unif(
    std::vector< epiworld_double >& new_params,
    const std::vector< epiworld_double >& old_params,
    LFMCMC<TData>* m
) {

    for (size_t p = 0u; p < m->get_n_params(); ++p)
        new_params[p] = (old_params[p] + m->runif(-1.0, 1.0));

    return;
}

/**
 * @brief Uses the uniform kernel with euclidean distance
 * 
 * @param simulated_stats Vector of statistics based on 
 * simulated data
 * @param observed_stats Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_uniform(
    const std::vector< epiworld_double >& simulated_stats,
    const std::vector< epiworld_double >& observed_stats,
    epiworld_double epsilon,
    LFMCMC<TData>* m
) {

    epiworld_double ans = 0.0;
    for (size_t p = 0u; p < m->get_n_params(); ++p)
        ans += std::pow(observed_stats[p] - simulated_stats[p], 2.0);

    return std::sqrt(ans) < epsilon ? 1.0 : 0.0;

}

#define sqrt2pi() 2.5066282746310002416

/**
 * @brief Gaussian kernel
 * 
 * @tparam TData 
 * @param simulated_stats Vector of statistics based on 
 * simulated data
 * @param observed_stats Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_gaussian(
    const std::vector< epiworld_double >& simulated_stats,
    const std::vector< epiworld_double >& observed_stats,
    epiworld_double epsilon,
    LFMCMC<TData>* m
) {

    epiworld_double ans = 0.0;
    for (size_t p = 0u; p < m->get_n_params(); ++p)
        ans += std::pow(observed_stats[p] - simulated_stats[p], 2.0);

    return std::exp(
        -.5 * (ans/std::pow(1 + std::pow(epsilon, 2.0)/3.0, 2.0))
        ) / sqrt2pi() ;

}


template<typename TData>
inline void LFMCMC<TData>::set_proposal_fun(LFMCMCProposalFun<TData> fun)
{
    m_proposal_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_simulation_fun(LFMCMCSimFun<TData> fun)
{
    m_simulation_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_summary_fun(LFMCMCSummaryFun<TData> fun)
{
    m_summary_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_kernel_fun(LFMCMCKernelFun<TData> fun)
{
    m_kernel_fun = fun;
}


template<typename TData>
inline void LFMCMC<TData>::run(
    std::vector< epiworld_double > params_init_,
    size_t n_samples_,
    epiworld_double epsilon_,
    int seed
    )
{

    // Starting timing
    chrono_start();

    // Setting the baseline parameters of the model
    m_n_samples    = n_samples_;
    m_epsilon      = epsilon_;
    m_initial_params  = params_init_;
    m_n_params = params_init_.size();

    if (seed >= 0)
        this->seed(seed);

    m_current_proposed_params.resize(m_n_params);
    m_current_accepted_params.resize(m_n_params);

    if (m_simulated_data != nullptr)
        m_simulated_data->resize(m_n_samples);

    m_current_accepted_params = m_initial_params;
    m_current_proposed_params  = m_initial_params;

    // Computing the baseline sufficient statistics
    m_summary_fun(m_observed_stats, m_observed_data, this);
    m_n_stats = m_observed_stats.size();

    // Reserving size
    m_current_proposed_stats.resize(m_n_stats);
    m_current_accepted_stats.resize(m_n_stats);
    m_all_sample_drawn_prob.resize(m_n_samples);
    m_all_sample_acceptance.resize(m_n_samples, false);
    m_all_sample_params.resize(m_n_samples * m_n_params);
    m_all_sample_stats.resize(m_n_samples * m_n_stats);
    m_all_sample_kernel_scores.resize(m_n_samples);

    m_all_accepted_params.resize(m_n_samples * m_n_params);
    m_all_accepted_stats.resize(m_n_samples * m_n_stats);
    m_all_accepted_kernel_scores.resize(m_n_samples);

    TData data_i = m_simulation_fun(m_initial_params, this);

    m_summary_fun(m_current_proposed_stats, data_i, this);
    m_all_accepted_kernel_scores[0u] = m_kernel_fun(
        m_current_proposed_stats, m_observed_stats, m_epsilon, this
        );

    // Recording statistics
    for (size_t i = 0u; i < m_n_stats; ++i)
        m_all_sample_stats[i] = m_current_proposed_stats[i];
    
    m_current_accepted_stats = m_current_proposed_stats;

    for (size_t k = 0u; k < m_n_params; ++k)
        m_all_accepted_params[k] = m_initial_params[k];
    
    for (size_t k = 0u; k < m_n_params; ++k)
        m_all_sample_params[k] = m_initial_params[k];
   
    // Init progress bar
    progress_bar = Progress(m_n_samples, 80);
    if (verbose) { 
        progress_bar.next(); 
    }

    // Run LFMCMC
    for (size_t i = 1u; i < m_n_samples; ++i)
    {
        // Step 1: Generate a proposal and store it in m_current_proposed_params
        m_proposal_fun(m_current_proposed_params, m_current_accepted_params, this);

        // Step 2: Using m_current_proposed_params, simulate data
        TData data_i = m_simulation_fun(m_current_proposed_params, this);

        // Are we storing the data?
        if (m_simulated_data != nullptr)
            m_simulated_data->operator[](i) = data_i;

        // Step 3: Generate the summary statistics of the data
        m_summary_fun(m_current_proposed_stats, data_i, this);

        // Step 4: Compute the hastings ratio using the kernel function
        epiworld_double hr = m_kernel_fun(
            m_current_proposed_stats, m_observed_stats, m_epsilon, this
            );

        m_all_sample_kernel_scores[i] = hr;

        // Storing data
        for (size_t k = 0u; k < m_n_params; ++k)
            m_all_sample_params[i * m_n_params + k] = m_current_proposed_params[k];

        for (size_t k = 0u; k < m_n_stats; ++k)
            m_all_sample_stats[i * m_n_stats + k] = m_current_proposed_stats[k];
        
        // Running Hastings ratio
        epiworld_double r = runif();
        m_all_sample_drawn_prob[i] = r;

        // Step 5: Update if likely
        if (r < std::min(static_cast<epiworld_double>(1.0), hr / m_all_accepted_kernel_scores[i - 1u]))
        {
            m_all_accepted_kernel_scores[i] = hr;
            m_all_sample_acceptance[i]     = true;
            
            for (size_t k = 0u; k < m_n_stats; ++k)
                m_all_accepted_stats[i * m_n_stats + k] =
                    m_current_proposed_stats[k];

            m_current_accepted_params = m_current_proposed_params;
            m_current_accepted_stats = m_current_proposed_stats;
        } else
        {

            for (size_t k = 0u; k < m_n_stats; ++k)
                m_all_accepted_stats[i * m_n_stats + k] =
                    m_all_accepted_stats[(i - 1) * m_n_stats + k];

            m_all_accepted_kernel_scores[i] = m_all_accepted_kernel_scores[i - 1u];
        }
            

        for (size_t k = 0u; k < m_n_params; ++k)
            m_all_accepted_params[i * m_n_params + k] = m_current_accepted_params[k];

        if (verbose) { 
            progress_bar.next(); 
        }
    }

    // End timing
    chrono_end();

}


template<typename TData>
inline epiworld_double LFMCMC<TData>::runif()
{
    return runifd->operator()(*m_engine);
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::runif(
    epiworld_double lb,
    epiworld_double ub
)
{
    return runifd->operator()(*m_engine) * (ub - lb) + lb;
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rnorm()
{
    return rnormd->operator()(*m_engine);
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rnorm(
    epiworld_double mean,
    epiworld_double sd
    )
{
    return (rnormd->operator()(*m_engine)) * sd + mean;
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rgamma()
{
    return rgammad->operator()(*m_engine);
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rgamma(
    epiworld_double alpha,
    epiworld_double beta
    )
{

    auto old_param = rgammad->param();

    rgammad->param(std::gamma_distribution<>::param_type(alpha, beta));

    epiworld_double ans = rgammad->operator()(*m_engine);

    rgammad->param(old_param);

    return ans;

}

template<typename TData>
inline void LFMCMC<TData>::seed(epiworld_fast_uint s) {

    this->m_engine->seed(s);

}

template<typename TData>
inline void LFMCMC<TData>::set_rand_engine(std::shared_ptr< std::mt19937 > & eng)
{
    m_engine = eng;
}

template<typename TData>
inline void LFMCMC<TData>::set_rand_gamma(epiworld_double alpha, epiworld_double beta)
{
    rgammad = std::make_shared<std::gamma_distribution<>>(alpha,beta);
}

template<typename TData>
inline std::shared_ptr< std::mt19937 > & LFMCMC<TData>::get_rand_endgine()
{
    return m_engine;
}

// Step 1: Simulate data

// Step 2: Compute the sufficient statistics

// Step 3: Compute the hastings-ratio

// Step 4: Accept/reject, and go back to step 1

#define DURCAST(tunit,txtunit) {\
        elapsed       = std::chrono::duration_cast<std::chrono:: tunit>(\
            m_end_time - m_start_time).count(); \
        abbr_unit     = txtunit;}

template<typename TData>
inline void LFMCMC<TData>::get_elapsed_time(
    std::string unit,
    epiworld_double * last_elapsed,
    std::string * unit_abbr,
    bool print
) const {

    // Preparing the result
    epiworld_double elapsed;
    std::string abbr_unit;

    // Figuring out the length
    if (unit == "auto")
    {

        size_t tlength = std::to_string(
            static_cast<int>(floor(m_elapsed_time.count()))
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
    if (unit_abbr != nullptr)
        *unit_abbr = abbr_unit;

    if (!print)
        return;

    printf_epiworld("Elapsed time : %.2f%s.\n", elapsed, abbr_unit.c_str());
}

#undef DURCAST

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//math//lfmcmc/lfmcmc-meat-print.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef LFMCMC_MEAT_PRINT_HPP
#define LFMCMC_MEAT_PRINT_HPP

template<typename TData>
inline void LFMCMC<TData>::print(size_t burnin) const
{

    // For each statistic or parameter in the model, we print three values: 
    // - mean, the 2.5% quantile, and the 97.5% quantile
    std::vector< epiworld_double > summ_params(m_n_params * 3, 0.0);
    std::vector< epiworld_double > summ_stats(m_n_stats * 3, 0.0);

    // Compute the number of samples to use based on burnin rate
    size_t n_samples_print = m_n_samples;
    if (burnin > 0)
    {
        if (burnin >= m_n_samples)
            throw std::length_error(
                "The burnin is greater than or equal to the number of samples."
                );

        n_samples_print = m_n_samples - burnin;

    }

    epiworld_double n_samples_dbl = static_cast< epiworld_double >(
        n_samples_print
        );

    // Compute parameter summary values
    for (size_t k = 0u; k < m_n_params; ++k)
    {

        // Retrieving the relevant parameter
        std::vector< epiworld_double > par_i(n_samples_print);
        for (size_t i = burnin; i < m_n_samples; ++i)
        {
            par_i[i-burnin] = m_all_accepted_params[i * m_n_params + k];
            summ_params[k * 3] += par_i[i-burnin]/n_samples_dbl;
        }

        // Computing the 95% Credible interval
        std::sort(par_i.begin(), par_i.end());

        summ_params[k * 3 + 1u] = 
            par_i[std::floor(.025 * n_samples_dbl)];
        summ_params[k * 3 + 2u] = 
            par_i[std::floor(.975 * n_samples_dbl)];

    }

    // Compute statistics summary values
    for (size_t k = 0u; k < m_n_stats; ++k)
    {

        // Retrieving the relevant parameter
        std::vector< epiworld_double > stat_k(n_samples_print);
        for (size_t i = burnin; i < m_n_samples; ++i)
        {
            stat_k[i-burnin] = m_all_accepted_stats[i * m_n_stats + k];
            summ_stats[k * 3] += stat_k[i-burnin]/n_samples_dbl;
        }

        // Computing the 95% Credible interval
        std::sort(stat_k.begin(), stat_k.end());
        summ_stats[k * 3 + 1u] = 
            stat_k[std::floor(.025 * n_samples_dbl)];
        summ_stats[k * 3 + 2u] = 
            stat_k[std::floor(.975 * n_samples_dbl)];

    }

    printf_epiworld("___________________________________________\n\n");
    printf_epiworld("LIKELIHOOD-FREE MARKOV CHAIN MONTE CARLO\n\n");

    printf_epiworld("N Samples (total) : %zu\n", m_n_samples);
    printf_epiworld("N Samples (after burn-in period) : %zu\n", m_n_samples - burnin);

    std::string abbr;
    epiworld_double elapsed;
    get_elapsed_time("auto", &elapsed, &abbr, false);
    printf_epiworld("Elapsed t : %.2f%s\n\n", elapsed, abbr.c_str());
    
    ////////////////////////////////////////////////////////////////////////////
    // PARAMETERS
    ////////////////////////////////////////////////////////////////////////////
    printf_epiworld("Parameters:\n");

    // Figuring out format
    std::string fmt_params;
    
    int nchar_par_num = 0;
    for (auto & n : summ_params)
    {
        
        int tmp_nchar;
        
        if (std::abs(n) < 1) {
            // std::log10(<1) will return negative number
            // std::log10(0) will return -inf and throw a runtime error
            tmp_nchar = 0;
        } else {
            tmp_nchar = std::floor(std::log10(std::abs(n)));
        }

        if (nchar_par_num < tmp_nchar)
            nchar_par_num = tmp_nchar;
    }
    nchar_par_num += 5; // 1 for neg padd, 2 for decimals, 1 the decimal point, and one b/c log(<10) < 1.
    std::string charlen = std::to_string(nchar_par_num);

    if (m_param_names.size() != 0u)
    {
        int nchar_par = 0;
        for (auto & n : m_param_names)
        {
            int tmp_nchar = n.length();
            if (nchar_par < tmp_nchar)
                nchar_par = tmp_nchar;
        }

        fmt_params = std::string("  -%-") +
            std::to_string(nchar_par) +
            std::string("s : % ") + charlen  + 
            std::string(".2f [% ") + charlen + 
            std::string(".2f, % ") + charlen +
            std::string(".2f] (initial : % ") +
            charlen + std::string(".2f)\n");

        for (size_t k = 0u; k < m_n_params; ++k)
        {
            printf_epiworld(
                fmt_params.c_str(),
                m_param_names[k].c_str(),
                summ_params[k * 3],
                summ_params[k * 3 + 1u],
                summ_params[k * 3 + 2u],
                m_initial_params[k]
                );
        }

        
    } else {

        fmt_params = std::string("  [%-2ld]: % ") + charlen + 
            std::string(".2f [% ") + charlen +
            std::string(".2f, % ") + charlen + 
            std::string(".2f] (initial : % ") + charlen +
            std::string(".2f)\n");

        for (size_t k = 0u; k < m_n_params; ++k)
        {
            
            printf_epiworld(
                fmt_params.c_str(),
                k,
                summ_params[k * 3],
                summ_params[k * 3 + 1u],
                summ_params[k * 3 + 2u],
                m_initial_params[k]
                );
        }

    }    

    ////////////////////////////////////////////////////////////////////////////
    // Statistics
    ////////////////////////////////////////////////////////////////////////////
    printf_epiworld("\nStatistics:\n");
    int nchar = 0;
    for (auto & s : summ_stats)
    {
        int tmp_nchar;
        if (std::abs(s) < 1) {
            // std::log10(<1) will return negative number
            // std::log10(0) will return -inf and throw a runtime error
            tmp_nchar = 0;
        } else {
            tmp_nchar = std::floor(std::log10(std::abs(s)));
        }
    
        if (nchar < tmp_nchar)
            nchar = tmp_nchar;
    }

    nchar += 5; // See above

    std::string nchar_char = std::to_string(nchar);

    // Figuring out format
    std::string fmt_stats;
    if (m_stat_names.size() != 0u)
    {
        int nchar_stats = 0;
        for (auto & n : m_stat_names)
        {
            int tmp_nchar = n.length();
            if (nchar_stats < tmp_nchar)
                nchar_stats = tmp_nchar;
        }

        fmt_stats = std::string("  -%-") +
            std::to_string(nchar_stats) +
            std::string("s : % ") + nchar_char +
            std::string(".2f [% ") + nchar_char +
            std::string(".2f, % ") + nchar_char +
            std::string(".2f] (Observed: % ") + nchar_char +
            std::string(".2f)\n");

        for (size_t k = 0u; k < m_n_stats; ++k)
        {
            printf_epiworld(
                fmt_stats.c_str(),
                m_stat_names[k].c_str(),
                summ_stats[k * 3],
                summ_stats[k * 3 + 1u],
                summ_stats[k * 3 + 2u],
                m_observed_stats[k]
                );
        }

        
    } else {

        fmt_stats = std::string("  [%-2ld] : % ") + 
            nchar_char +
            std::string(".2f [% ") + nchar_char +
            std::string(".2f, % ") + nchar_char +
            std::string(".2f] (Observed: % ") + nchar_char +
            std::string(".2f)\n");

        for (size_t k = 0u; k < m_n_stats; ++k)
        {
            printf_epiworld(
                fmt_stats.c_str(),
                k,
                summ_stats[k * 3],
                summ_stats[k * 3 + 1u],
                summ_stats[k * 3 + 2u],
                m_observed_stats[k]
                );
        }

    }

    printf_epiworld("___________________________________________\n\n");
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//math//lfmcmc/lfmcmc-meat-print.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



template<typename TData>
inline void LFMCMC<TData>::chrono_start() {
    m_start_time = std::chrono::steady_clock::now();
}

template<typename TData>
inline void LFMCMC<TData>::chrono_end() {
    m_end_time = std::chrono::steady_clock::now();
    m_elapsed_time += (m_end_time - m_start_time);
}

template<typename TData>
inline void LFMCMC<TData>::set_params_names(std::vector< std::string > names)
{

    if (names.size() != m_n_params)
        throw std::length_error("The number of names to add differs from the number of parameters in the model.");

    m_param_names = names;

}
template<typename TData>
inline void LFMCMC<TData>::set_stats_names(std::vector< std::string > names)
{

    if (names.size() != m_n_stats)
        throw std::length_error("The number of names to add differs from the number of statistics in the model.");

    m_stat_names = names;

}

template<typename TData>
inline std::vector< epiworld_double > LFMCMC<TData>::get_mean_params()
{
    std::vector< epiworld_double > res(this->m_n_params, 0.0);
    
    for (size_t k = 0u; k < m_n_params; ++k)
    {
        for (size_t i = 0u; i < m_n_samples; ++i)
            res[k] += (this->m_all_accepted_params[k + m_n_params * i])/
                static_cast< epiworld_double >(m_n_samples);
    }

    return res;

}

template<typename TData>
inline std::vector< epiworld_double > LFMCMC<TData>::get_mean_stats()
{
    std::vector< epiworld_double > res(this->m_n_stats, 0.0);
    
    for (size_t k = 0u; k < m_n_stats; ++k)
    {
        for (size_t i = 0u; i < m_n_samples; ++i)
            res[k] += (this->m_all_accepted_stats[k + m_n_stats * i])/
                static_cast< epiworld_double >(m_n_samples);
    }

    return res;

}

template<typename TData>
inline LFMCMC<TData> & LFMCMC<TData>::verbose_off()
{
    verbose = false;
    return *this;
}

template<typename TData>
inline LFMCMC<TData> & LFMCMC<TData>::verbose_on()
{
    verbose = true;
    return *this;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//math/lfmcmc/lfmcmc-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/math/lfmcmc.hpp-

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

/**
 * @brief Personalized data by the user
 * 
 * @tparam TSeq 
 */
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

    epiworld_fast_uint k = 0u;
    epiworld_fast_uint n = 0u;

    int last_day = -1;

public:

    UserData() = delete;
    UserData(Model<TSeq> & m) : model(&m) {};
    UserData(Model<TSeq> * m) : model(m) {};

    /**
     * @brief Construct a new User Data object
     * 
     * @param names A vector of names. The length of the vector sets
     * the number of columns to record.
     */
    UserData(std::vector< std::string > names);

    /**
     * @name Append data 
     * 
     * @param x A vector of length `ncol()` (if vector), otherwise a `epiworld_double`.
     * @param j Index of the data point, from 0 to `ncol() - 1`.
     */
    ///@{
    void add(std::vector<epiworld_double> x);
    void add(
        epiworld_fast_uint j,
        epiworld_double x
        );
    ///@}

    /**
     * @name Access data 
     * 
     * @param i Row (0 through ndays - 1.)
     * @param j Column (0 through `ncols()`).
     * @return epiworld_double& 
     */
    ///@{
    epiworld_double & operator()(
        epiworld_fast_uint i,
        epiworld_fast_uint j
        );

    epiworld_double & operator()(
        epiworld_fast_uint i,
        std::string name
        );
    ///@}

    std::vector< std::string > & get_names();

    std::vector< int > & get_dates();

    std::vector< epiworld_double > & get_data();

    void get_all(
        std::vector< std::string > * names    = nullptr,
        std::vector< int > * date             = nullptr,
        std::vector< epiworld_double > * data = nullptr
    );

    epiworld_fast_uint nrow() const;
    epiworld_fast_uint ncol() const;

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
inline void UserData<TSeq>::add(epiworld_fast_uint j, epiworld_double x)
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
    std::vector< int > * date,
    std::vector< epiworld_double > * data
) 
{
    
    if (names != nullptr)
        names = &this->data_names;

    if (date != nullptr)
        date = &this->data_dates;

    if (data != nullptr)
        data = &this->data_data;

}

template<typename TSeq>
inline epiworld_double & UserData<TSeq>::operator()(
    epiworld_fast_uint i,
    epiworld_fast_uint j
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
    epiworld_fast_uint i,
    std::string name
)
{
    int loc = -1;
    for (epiworld_fast_uint l = 0u; l < k; ++l)
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

    return operator()(i, static_cast<epiworld_fast_uint>(loc));

}

template<typename TSeq>
inline epiworld_fast_uint UserData<TSeq>::nrow() const
{
    return n;
}

template<typename TSeq>
inline epiworld_fast_uint UserData<TSeq>::ncol() const
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
    
    epiworld_fast_uint ndata = 0u;
    for (epiworld_fast_uint i = 0u; i < n; ++i)
    {
        file_ud << data_dates[i];

        for (epiworld_fast_uint j = 0u; j < k; ++j)
            file_ud << " " << data_data[ndata++];

        file_ud << "\n";
    }

    return;
}

template<typename TSeq>
inline void UserData<TSeq>::print() const
{
    // File header
    printf_epiworld("Total records: %llu\n", n);
    printf_epiworld("date");

    for (auto & cn : data_names)
    {

        printf_epiworld(" %s", cn.c_str());

    }

    printf_epiworld("\n");
    
    epiworld_fast_uint ndata = 0u;
    
    for (epiworld_fast_uint i = 0u; i < n; ++i)
    {

        printf_epiworld("%i", data_dates[i]);

        for (epiworld_fast_uint j = 0u; j < k; ++j)
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
    std::vector<int> ans(x.size());
    size_t j = 0;
    for (const auto & i : x)
        ans[j++] = i? 1 : 0;
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
template<typename TSeq = int>
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

template<typename TSeq>
inline void default_add_virus(Event<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_add_tool(Event<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_virus(Event<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_tool(Event<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_change_state(Event<TSeq> & a, Model<TSeq> * m);

/**
 * @brief Statistical data about the process
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class DataBase {
    friend class Model<TSeq>;
    friend void default_add_virus<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_add_tool<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_virus<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_tool<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_change_state<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
private:
    Model<TSeq> * model;

    // Variants information 
    MapVec_type<int,int> virus_id; ///< The squence is the key
    std::vector< std::string > virus_name;
    std::vector< TSeq> virus_sequence;
    std::vector< int > virus_origin_date;
    std::vector< int > virus_parent_id;

    MapVec_type<int,int> tool_id; ///< The squence is the key
    std::vector< std::string > tool_name;
    std::vector< TSeq> tool_sequence;
    std::vector< int > tool_origin_date;

    std::function<std::vector<int>(const TSeq&)> seq_hasher = default_seq_hasher<TSeq>;
    std::function<std::string(const TSeq &)> seq_writer = default_seq_writer<TSeq>;

    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    std::vector< std::vector<int> > today_virus;

    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    std::vector< std::vector<int> > today_tool;

    // {Susceptible, Infected, etc.}
    std::vector< int > today_total;

    // Totals
    int today_total_nviruses_active = 0;
    
    int sampling_freq = 1;

    // Variants history
    std::vector< int > hist_virus_date;
    std::vector< int > hist_virus_id;
    std::vector< epiworld_fast_uint > hist_virus_state;
    std::vector< int > hist_virus_counts;

    // Tools history
    std::vector< int > hist_tool_date;
    std::vector< int > hist_tool_id;
    std::vector< epiworld_fast_uint > hist_tool_state;
    std::vector< int > hist_tool_counts;

    // Overall hist
    std::vector< int > hist_total_date;
    std::vector< int > hist_total_nviruses_active;
    std::vector< epiworld_fast_uint > hist_total_state;
    std::vector< int > hist_total_counts;
    std::vector< int > hist_transition_matrix;

    // Transmission network
    std::vector< int > transmission_date;                 ///< Date of the transmission event
    std::vector< int > transmission_source;               ///< Id of the source
    std::vector< int > transmission_target;               ///< Id of the target
    std::vector< int > transmission_virus;              ///< Id of the variant
    std::vector< int > transmission_source_exposure_date; ///< Date when the source acquired the variant

    std::vector< int > transition_matrix;

    UserData<TSeq> user_data;

    void update_state(
        epiworld_fast_uint prev_state,
        epiworld_fast_uint new_state,
        bool undo = false
    );

    void update_virus(
        epiworld_fast_uint virus_id,
        epiworld_fast_uint prev_state,
        epiworld_fast_uint new_state
    );

    void update_tool(
        epiworld_fast_uint tool_id,
        epiworld_fast_uint prev_state,
        epiworld_fast_uint new_state
    );

    void record_transition(epiworld_fast_uint from, epiworld_fast_uint to, bool undo);


public:

    #ifdef EPI_DEBUG
    int n_transmissions_potential = 0;
    int n_transmissions_today     = 0;
    #endif

    DataBase() = delete;
    DataBase(Model<TSeq> & m) : model(&m), user_data(m) {};
    DataBase(const DataBase<TSeq> & db);
    // DataBase<TSeq> & operator=(const DataBase<TSeq> & m);

    /**
     * @brief Registering a new variant
     * 
     * @param v Pointer to the new virus.
     * Since viruses are originated in the agent, the numbers simply move around.
     * From the parent virus to the new virus. And the total number of infected
     * does not change.
     */
    void record_virus(Virus<TSeq> & v); 
    void record_tool(Tool<TSeq> & t); 
    void set_seq_hasher(std::function<std::vector<int>(TSeq)> fun);
    void reset();
    Model<TSeq> * get_model();
    void record();

    const std::vector< TSeq > & get_sequence() const;
    const std::vector< int > & get_nexposed() const;
    size_t size() const;

    /**
     * @name Get recorded information from the model
     * 
     * @param what std::string, The state, e.g., 0, 1, 2, ...
     * @return In `get_today_total`, the current counts of `what`.
     * @return In `get_today_virus`, the current counts of `what` for
     * each virus.
     * @return In `get_hist_total`, the time series of `what`
     * @return In `get_hist_virus`, the time series of what for each virus.
     * @return In `get_hist_total_date` and `get_hist_virus_date` the
     * corresponding date
     */
    ///@{
    int get_today_total(std::string what) const;
    int get_today_total(epiworld_fast_uint what) const;
    void get_today_total(
        std::vector< std::string > * state = nullptr,
        std::vector< int > * counts = nullptr
    ) const;

    void get_today_virus(
        std::vector< std::string > & state,
        std::vector< int > & id,
        std::vector< int > & counts
    ) const;

    void get_today_transition_matrix(
        std::vector< int > & counts
    ) const;

    void get_hist_total(
        std::vector< int > * date,
        std::vector< std::string > * state,
        std::vector< int > * counts
    ) const;

    void get_hist_virus(
        std::vector< int > & date,
        std::vector< int > & id,
        std::vector< std::string > & state,
        std::vector< int > & counts
    ) const;

    void get_hist_tool(
        std::vector< int > & date,
        std::vector< int > & id,
        std::vector< std::string > & state,
        std::vector< int > & counts
    ) const;

    void get_hist_transition_matrix(
        std::vector< std::string > & state_from,
        std::vector< std::string > & state_to,
        std::vector< int > & date,
        std::vector< int > & counts,
        bool skip_zeros
    ) const;
    ///@}

    /**
     * @brief Get the transmissions object
     * 
     * @param date 
     * @param source 
     * @param target 
     * @param virus 
     * @param source_exposure_date 
     */
    ///@{
    void get_transmissions(
        std::vector<int> & date,
        std::vector<int> & source,
        std::vector<int> & target,
        std::vector<int> & virus,
        std::vector<int> & source_exposure_date
    ) const;

    void get_transmissions(
        int * date,
        int * source,
        int * target,
        int * virus,
        int * source_exposure_date
    ) const;
    ///@}

    void write_data(
        std::string fn_virus_info,
        std::string fn_virus_hist,
        std::string fn_tool_info,
        std::string fn_tool_hist,
        std::string fn_total_hist,
        std::string fn_transmission,
        std::string fn_transition,
        std::string fn_reproductive_number,
        std::string fn_generation_time
        ) const;
    
    /***
     * @brief Record a transmission event
     */
    void record_transmission(int i, int j, int virus, int i_expo_date);

    size_t get_n_viruses() const; ///< Get the number of viruses
    size_t get_n_tools() const; ///< Get the number of tools
    
    void set_user_data(std::vector< std::string > names);
    void add_user_data(std::vector< epiworld_double > x);
    void add_user_data(epiworld_fast_uint j, epiworld_double x);
    UserData<TSeq> & get_user_data();


    /**
     * @brief Computes the reproductive number of each case
     * 
     * @details By definition, whereas it computes R0 (basic reproductive number)
     * or Rt/R (the effective reproductive number) will depend on whether the
     * virus is allowed to circulate naïvely or not, respectively.
     * 
     * @param fn File where to write out the reproductive number.
     */
    ///@{
    MapVec_type<int,int> reproductive_number() const;

    void reproductive_number(
        std::string fn
        ) const;
    ///@}

    /**
     * @brief Calculates the transition probabilities
     * @param print Print the transition matrix.
     * @param normalize Normalize the transition matrix. Otherwise, 
     * it returns raw counts.
     * @details
     * The transition matrix is the matrix of the counts of transitions
     * from one state to another. So the ij-th element of the matrix is
     * the number of transitions from state i to state j (when not normalized),
     * or the probability of transitioning from state i to state j 
     * (when normalized).
     * @return std::vector< epiworld_double > 
     */
    std::vector< epiworld_double > transition_probability(
        bool print = true,
        bool normalize = true
    ) const;

    bool operator==(const DataBase<TSeq> & other) const;
    bool operator!=(const DataBase<TSeq> & other) const {return !operator==(other);};

    /**
     * Calculates the generating time
     * @param agent_id,virus_id,time,gentime vectors where to save the values 
     * 
     * @details
     * The generation time is the time between the infection of the source and 
     * the infection of the target.
    */
   ///@{
    void generation_time(
        std::vector< int > & agent_id,
        std::vector< int > & virus_id,
        std::vector< int > & time,
        std::vector< int > & gentime
    ) const; ///< Get the generation time

    void generation_time(
        std::string fn
    ) const; ///< Write the generation time to a file
    ///@}

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
inline void DataBase<TSeq>::reset()
{

    // Initializing the counts
    today_total.resize(model->nstates);
    std::fill(today_total.begin(), today_total.end(), 0);
    for (auto & p : model->get_agents())
        ++today_total[p.get_state()];

    #ifdef EPI_DEBUG
    // Only the first should be different from zero
    {
        int n = static_cast<int>(model->size());
        if (today_total[0] != n)
            throw std::runtime_error("The number of susceptible agents is not equal to the total number of agents.");

        if (std::accumulate(today_total.begin(), today_total.end(), 0) != n)
            throw std::runtime_error("The total number of agents is not equal to the sum of the number of agents in each state.");
            
    }
    #endif

    
    transition_matrix.resize(model->nstates * model->nstates);
    std::fill(transition_matrix.begin(), transition_matrix.end(), 0);
    for (size_t s = 0u; s < model->nstates; ++s)
        transition_matrix[s + s * model->nstates] = today_total[s];

    hist_virus_date.clear();
    hist_virus_id.clear();
    hist_virus_state.clear();
    hist_virus_counts.clear();

    hist_tool_date.clear();
    hist_tool_id.clear();
    hist_tool_state.clear();
    hist_tool_counts.clear();    

    today_virus.resize(get_n_viruses());
    std::fill(today_virus.begin(), today_virus.begin(), std::vector<int>(model->nstates, 0));

    today_tool.resize(get_n_tools());
    std::fill(today_tool.begin(), today_tool.begin(), std::vector<int>(model->nstates, 0));

    hist_total_date.clear();
    hist_total_state.clear();
    hist_total_nviruses_active.clear();
    hist_total_counts.clear();
    hist_transition_matrix.clear();

    transmission_date.clear();
    transmission_virus.clear();
    transmission_source.clear();
    transmission_target.clear();
    transmission_source_exposure_date.clear();

    return;

}

template<typename TSeq>
inline DataBase<TSeq>::DataBase(const DataBase<TSeq> & db) :
    virus_id(db.virus_id),
    virus_name(db.virus_name),
    virus_sequence(db.virus_sequence),
    virus_origin_date(db.virus_origin_date),
    virus_parent_id(db.virus_parent_id),
    tool_id(db.tool_id),
    tool_name(db.tool_name),
    tool_sequence(db.tool_sequence),
    tool_origin_date(db.tool_origin_date),
    seq_hasher(db.seq_hasher),
    seq_writer(db.seq_writer),
    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    today_virus(db.today_virus),
    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    today_tool(db.today_tool),
    // {Susceptible, Infected, etc.}
    today_total(db.today_total),
    // Totals
    today_total_nviruses_active(db.today_total_nviruses_active),
    sampling_freq(db.sampling_freq),
    // Variants history
    hist_virus_date(db.hist_virus_date),
    hist_virus_id(db.hist_virus_id),
    hist_virus_state(db.hist_virus_state),
    hist_virus_counts(db.hist_virus_counts),
    // Tools history
    hist_tool_date(db.hist_tool_date),
    hist_tool_id(db.hist_tool_id),
    hist_tool_state(db.hist_tool_state),
    hist_tool_counts(db.hist_tool_counts),
    // Overall hist
    hist_total_date(db.hist_total_date),
    hist_total_nviruses_active(db.hist_total_nviruses_active),
    hist_total_state(db.hist_total_state),
    hist_total_counts(db.hist_total_counts),
    hist_transition_matrix(db.hist_transition_matrix),
    // Transmission network
    transmission_date(db.transmission_date),
    transmission_source(db.transmission_source),
    transmission_target(db.transmission_target),
    transmission_virus(db.transmission_virus),
    transmission_source_exposure_date(db.transmission_source_exposure_date),
    transition_matrix(db.transition_matrix),
    user_data(nullptr)
{}

// DataBase<TSeq> & DataBase<TSeq>::operator=(const DataBase<TSeq> & m)
// {

// }

template<typename TSeq>
inline Model<TSeq> * DataBase<TSeq>::get_model() {
    return model;
}

template<typename TSeq>
inline const std::vector< TSeq > & DataBase<TSeq>::get_sequence() const {
    return virus_sequence;
}

template<typename TSeq>
inline void DataBase<TSeq>::record() 
{

    ////////////////////////////////////////////////////////////////////////////
    // DEBUGGING BLOCK
    ////////////////////////////////////////////////////////////////////////////
    EPI_DEBUG_SUM_INT(today_total, model->size())
    EPI_DEBUG_ALL_NON_NEGATIVE(today_total)

    #ifdef EPI_DEBUG
    // Checking whether the sums correspond
    std::vector< int > _today_total_cp(today_total.size(), 0);
    for (auto & p : model->population)
        _today_total_cp[p.get_state()]++;
    
    EPI_DEBUG_VECTOR_MATCH_INT(
        _today_total_cp, today_total,
        "Sums of __today_total_cp in database-meat.hpp"
        )

    if (model->today() == 0)
    {
        if (hist_total_date.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_total_date should be of length 0.")
        if (hist_total_nviruses_active.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_total_nviruses_active should be of length 0.")
        if (hist_total_state.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_total_state should be of length 0.")
        if (hist_total_counts.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_total_counts should be of length 0.")
        if (hist_virus_date.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_virus_date should be of length 0.")
        if (hist_virus_id.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_virus_id should be of length 0.")
        if (hist_virus_state.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_virus_state should be of length 0.")
        if (hist_virus_counts.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_virus_counts should be of length 0.")
        if (hist_tool_date.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_tool_date should be of length 0.")
        if (hist_tool_id.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_tool_id should be of length 0.")
        if (hist_tool_state.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_tool_state should be of length 0.")
        if (hist_tool_counts.size() != 0)
            EPI_DEBUG_ERROR(std::logic_error, "DataBase::record hist_tool_counts should be of length 0.")
    }
    #endif
    ////////////////////////////////////////////////////////////////////////////

    // Only store every now and then
    if ((model->today() % sampling_freq) == 0)
    {

        // Recording virus's history
        for (auto & p : virus_id)
        {

            for (epiworld_fast_uint s = 0u; s < model->nstates; ++s)
            {

                hist_virus_date.push_back(model->today());
                hist_virus_id.push_back(p.second);
                hist_virus_state.push_back(s);
                hist_virus_counts.push_back(today_virus[p.second][s]);

            }

        }

        // Recording tool's history
        for (auto & p : tool_id)
        {

            for (epiworld_fast_uint s = 0u; s < model->nstates; ++s)
            {

                hist_tool_date.push_back(model->today());
                hist_tool_id.push_back(p.second);
                hist_tool_state.push_back(s);
                hist_tool_counts.push_back(today_tool[p.second][s]);

            }

        }

        // Recording the overall history
        for (epiworld_fast_uint s = 0u; s < model->nstates; ++s)
        {
            hist_total_date.push_back(model->today());
            hist_total_nviruses_active.push_back(today_total_nviruses_active);
            hist_total_state.push_back(s);
            hist_total_counts.push_back(today_total[s]);
        }

        for (auto cell : transition_matrix)
            hist_transition_matrix.push_back(cell);

        // Now the diagonal must reflect the state
        for (size_t s_i = 0u; s_i < model->nstates; ++s_i)
        {

            for (size_t s_j = 0u; s_j < model->nstates; ++s_j)
            {
                
                if ((s_i != s_j) && (transition_matrix[s_i + s_j * model->nstates] > 0))
                {
                    transition_matrix[s_j + s_j * model->nstates] +=
                        transition_matrix[s_i + s_j * model->nstates];

                    transition_matrix[s_i + s_j * model->nstates] = 0;
                }
         
            }

        }

        #ifdef EPI_DEBUG
        for (size_t s_i = 0u; s_i < model->nstates; ++s_i)
        {
            if (transition_matrix[s_i + s_i * model->nstates] != 
                today_total[s_i])
                throw std::logic_error(
                    "The diagonal of the updated transition Matrix should match the daily totals"
                    );
        }
        #endif

    }

}

template<typename TSeq>
inline void DataBase<TSeq>::record_virus(Virus<TSeq> & v)
{

    // If no sequence, then need to add one. This is regardless of the case
    if (v.get_sequence() == nullptr)
        v.set_sequence(default_sequence<TSeq>(
            static_cast<int>(virus_name.size())
            ));

    // Negative id -> virus hasn't been recorded
    if (v.get_id() < 0)
    {


        // Generating the hash
        std::vector< int > hash = seq_hasher(*v.get_sequence());

        epiworld_fast_uint new_id = virus_id.size();
        virus_id[hash] = new_id;
        virus_name.push_back(v.get_name());
        virus_sequence.push_back(*v.get_sequence());
        virus_origin_date.push_back(model->today());
        
        virus_parent_id.push_back(v.get_id()); // Must be -99
        
        today_virus.push_back({});
        today_virus[new_id].resize(model->nstates, 0);
       
        // Updating the variant
        v.set_id(new_id);
        v.set_date(model->today());

        today_total_nviruses_active++;

    } else { // In this case, the virus is already on record, need to make sure
             // The new sequence is new.

        // Updating registry
        std::vector< int > hash = seq_hasher(*v.get_sequence());
        epiworld_fast_uint old_id = v.get_id();
        epiworld_fast_uint new_id;

        // If the sequence is new, then it means that the
        if (virus_id.find(hash) == virus_id.end())
        {

            new_id = virus_id.size();
            virus_id[hash] = new_id;
            virus_name.push_back(v.get_name());
            virus_sequence.push_back(*v.get_sequence());
            virus_origin_date.push_back(model->today());
            
            virus_parent_id.push_back(old_id);
            
            today_virus.push_back({});
            today_virus[new_id].resize(model->nstates, 0);
        
            // Updating the variant
            v.set_id(new_id);
            v.set_date(model->today());

            today_total_nviruses_active++;

        } else {

            // Finding the id
            new_id = virus_id[hash];

            // Reflecting the change
            v.set_id(new_id);
            v.set_date(virus_origin_date[new_id]);

        }

        // Moving statistics (only if we are affecting an individual)
        if (v.get_agent() != nullptr)
        {
            // Correcting math
            epiworld_fast_uint tmp_state = v.get_agent()->get_state();
            today_virus[old_id][tmp_state]--;
            today_virus[new_id][tmp_state]++;

        }

    }
    
    return;

} 

template<typename TSeq>
inline void DataBase<TSeq>::record_tool(Tool<TSeq> & t)
{

    if (t.get_sequence() == nullptr)
        t.set_sequence(default_sequence<TSeq>(
            static_cast<int>(tool_name.size())
        ));

    if (t.get_id() < 0) 
    {

        std::vector< int > hash = seq_hasher(*t.get_sequence());
        epiworld_fast_uint new_id = tool_id.size();
        tool_id[hash] = new_id;
        tool_name.push_back(t.get_name());
        tool_sequence.push_back(*t.get_sequence());
        tool_origin_date.push_back(model->today());
                
        today_tool.push_back({});
        today_tool[new_id].resize(model->nstates, 0);

        // Updating the tool
        t.set_id(new_id);
        t.set_date(model->today());

    } else {

        // Updating registry
        std::vector< int > hash = seq_hasher(*t.get_sequence());
        epiworld_fast_uint old_id = t.get_id();
        epiworld_fast_uint new_id;
        
        if (tool_id.find(hash) == tool_id.end())
        {

            new_id = tool_id.size();
            tool_id[hash] = new_id;
            tool_name.push_back(t.get_name());
            tool_sequence.push_back(*t.get_sequence());
            tool_origin_date.push_back(model->today());
                    
            today_tool.push_back({});
            today_tool[new_id].resize(model->nstates, 0);

            // Updating the tool
            t.set_id(new_id);
            t.set_date(model->today());

        } else {

            // Finding the id
            new_id = tool_id[hash];

            // Reflecting the change
            t.set_id(new_id);
            t.set_date(tool_origin_date[new_id]);

        }

        // Moving statistics (only if we are affecting an individual)
        if (t.get_agent() != nullptr)
        {
            // Correcting math
            epiworld_fast_uint tmp_state = t.get_agent()->get_state();
            today_tool[old_id][tmp_state]--;
            today_tool[new_id][tmp_state]++;

        }

    }

    
    
    return;
} 

template<typename TSeq>
inline size_t DataBase<TSeq>::size() const
{
    return virus_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::update_state(
        epiworld_fast_uint prev_state,
        epiworld_fast_uint new_state,
        bool undo
) {

    if (undo)
    {

        today_total[prev_state]++;
        today_total[new_state]--;
        
    } else {

        today_total[prev_state]--;
        today_total[new_state]++;

    }

    record_transition(prev_state, new_state, undo);
    
    return;
}

template<typename TSeq>
inline void DataBase<TSeq>::update_virus(
        epiworld_fast_uint virus_id,
        epiworld_fast_uint prev_state,
        epiworld_fast_uint new_state
) {

    today_virus[virus_id][prev_state]--;
    today_virus[virus_id][new_state]++;

    return;
    
}

template<typename TSeq>
inline void DataBase<TSeq>::update_tool(
        epiworld_fast_uint tool_id,
        epiworld_fast_uint prev_state,
        epiworld_fast_uint new_state
) {


    today_tool[tool_id][prev_state]--;    
    today_tool[tool_id][new_state]++;

    return;

}

template<typename TSeq>
inline void DataBase<TSeq>::record_transition(
    epiworld_fast_uint from,
    epiworld_fast_uint to,
    bool undo
) {

    if (undo)
    {   

        transition_matrix[to * model->nstates + from]--;
        transition_matrix[from * model->nstates + from]++;

    } else {

        transition_matrix[to * model->nstates + from]++;
        transition_matrix[from * model->nstates + from]--;

    }

    #ifdef EPI_DEBUG
    if (transition_matrix[from * model->nstates + from] < 0)
        throw std::logic_error("An entry in transition matrix is negative.");
    #endif

}

template<typename TSeq>
inline int DataBase<TSeq>::get_today_total(
    std::string what
) const
{

    for (auto i = 0u; i < model->states_labels.size(); ++i)
    {
        if (model->states_labels[i] == what)
            return today_total[i];
    }

    throw std::range_error("The value '" + what + "' is not in the model.");

}

template<typename TSeq>
inline void DataBase<TSeq>::get_today_total(
    std::vector< std::string > * state,
    std::vector< int > * counts
) const
{
    if (state != nullptr)
        (*state) = model->states_labels;

    if (counts != nullptr)
        *counts = today_total;

}

template<typename TSeq>
inline void DataBase<TSeq>::get_today_virus(
    std::vector< std::string > & state,
    std::vector< int > & id,
    std::vector< int > & counts
    ) const
{
      
    state.resize(today_virus.size(), "");
    id.resize(today_virus.size(), 0);
    counts.resize(today_virus.size(),0);

    int n = 0u;
    for (epiworld_fast_uint v = 0u; v < today_virus.size(); ++v)
        for (epiworld_fast_uint s = 0u; s < model->states_labels.size(); ++s)
        {
            state[n]   = model->states_labels[s];
            id[n]       = static_cast<int>(v);
            counts[n++] = today_virus[v][s];

        }

}

template<typename TSeq>
inline void DataBase<TSeq>::get_hist_total(
    std::vector< int > * date,
    std::vector< std::string > * state,
    std::vector< int > * counts
) const
{

    if (date != nullptr)
        *date = hist_total_date;

    if (state != nullptr)
    {
        state->resize(hist_total_state.size(), "");
        for (epiworld_fast_uint i = 0u; i < hist_total_state.size(); ++i)
            state->operator[](i) = model->states_labels[hist_total_state[i]];
    }

    if (counts != nullptr)
        *counts = hist_total_counts;

    return;

}

template<typename TSeq>
inline void DataBase<TSeq>::get_hist_virus(
    std::vector< int > & date,
    std::vector< int > & id,
    std::vector< std::string > & state,
    std::vector< int > & counts
) const {

    date = hist_virus_date;
    std::vector< std::string > labels;
    labels = model->states_labels;
    
    id = hist_virus_id;
    state.resize(hist_virus_state.size(), "");
    for (epiworld_fast_uint i = 0u; i < hist_virus_state.size(); ++i)
        state[i] = labels[hist_virus_state[i]];

    counts = hist_virus_counts;

    return;

}


template<typename TSeq>
inline void DataBase<TSeq>::get_hist_tool(
    std::vector< int > & date,
    std::vector< int > & id,
    std::vector< std::string > & state,
    std::vector< int > & counts
) const {

    date = hist_tool_date;
    std::vector< std::string > labels;
    labels = model->states_labels;
    
    id = hist_tool_id;
    state.resize(hist_tool_state.size(), "");
    for (size_t i = 0u; i < hist_tool_state.size(); ++i)
        state[i] = labels[hist_tool_state[i]];

    counts = hist_tool_counts;

    return;

}

template<typename TSeq>
inline void DataBase<TSeq>::get_today_transition_matrix(
    std::vector< int > & counts
) const
{

    counts = transition_matrix;

    return;

}

template<typename TSeq>
inline void DataBase<TSeq>::get_hist_transition_matrix(
    std::vector< std::string > & state_from,
    std::vector< std::string > & state_to,
    std::vector< int > & date,
    std::vector< int > & counts,
    bool skip_zeros
) const
{

    size_t n = this->hist_transition_matrix.size();
    
    // Clearing the previous vectors
    state_from.clear();
    state_to.clear();
    date.clear();
    counts.clear();

    // Reserving space
    state_from.reserve(n);
    state_to.reserve(n);
    date.reserve(n);
    counts.reserve(n);

    size_t n_states = model->nstates;
    size_t n_steps  = model->get_ndays();

    // If n is zero, then we are done
    if (n == 0u)
        return;

    for (size_t step = 0u; step <= n_steps; ++step) // The final step counts
    {
        for (size_t j = 0u; j < n_states; ++j) // Column major storage
        {
            for (size_t i = 0u; i < n_states; ++i)
            {
                // Retrieving the value of the day
                int v = hist_transition_matrix[
                    step * n_states * n_states + // Day of the data
                    j * n_states +               // Column (to)
                    i                            // Row (from)
                    ];

                // If we are skipping the zeros and it is zero, then don't save
                if (skip_zeros && v == 0)
                    continue;
                                
                state_from.push_back(model->states_labels[i]);
                state_to.push_back(model->states_labels[j]);
                date.push_back(hist_total_date[step * n_states]);
                counts.push_back(v);

            }

        }
    }

    return;


}

template<typename TSeq>
inline void DataBase<TSeq>::get_transmissions(
    std::vector<int> & date,
    std::vector<int> & source,
    std::vector<int> & target,
    std::vector<int> & virus,
    std::vector<int> & source_exposure_date
) const 
{

    size_t nevents = transmission_date.size();

    date.resize(nevents);
    source.resize(nevents);
    target.resize(nevents);
    virus.resize(nevents);
    source_exposure_date.resize(nevents);

    get_transmissions(
        &date[0u],
        &source[0u],
        &target[0u],
        &virus[0u],
        &source_exposure_date[0u]
    );

}

template<typename TSeq>
inline void DataBase<TSeq>::get_transmissions(
    int * date,
    int * source,
    int * target,
    int * virus,
    int * source_exposure_date
) const 
{

    size_t nevents = transmission_date.size();

    for (size_t i = 0u; i < nevents; ++i)
    {

        *(date + i) = transmission_date.at(i);
        *(source + i) = transmission_source.at(i);
        *(target + i) = transmission_target.at(i);
        *(virus + i) = transmission_virus.at(i);
        *(source_exposure_date + i) = transmission_source_exposure_date.at(i);

    }

}

template<typename TSeq>
inline void DataBase<TSeq>::write_data(
    std::string fn_virus_info,
    std::string fn_virus_hist,
    std::string fn_tool_info,
    std::string fn_tool_hist,
    std::string fn_total_hist,
    std::string fn_transmission,
    std::string fn_transition,
    std::string fn_reproductive_number,
    std::string fn_generation_time
) const
{

    if (fn_virus_info != "")
    {
        std::ofstream file_virus_info(fn_virus_info, std::ios_base::out);

        // Check if the file exists and throw an error if it doesn't
        if (!file_virus_info)
        {
            throw std::runtime_error(
                "Could not open file \"" + fn_virus_info +
                "\" for writing.")
                ;
        }


        file_virus_info <<
        #ifdef EPI_DEBUG
            "thread" << "virus_id " << "virus " << "virus_sequence " << "date_recorded " << "parent\n";
        #else
            "virus_id " << "virus " << "virus_sequence " << "date_recorded " << "parent\n";
        #endif

        for (const auto & v : virus_id)
        {
            int id = v.second;
            file_virus_info <<
                #ifdef EPI_DEBUG
                EPI_GET_THREAD_ID() << " " <<
                #endif
                id << " \"" <<
                virus_name[id] << "\" " <<
                seq_writer(virus_sequence[id]) << " " <<
                virus_origin_date[id] << " " <<
                virus_parent_id[id] << "\n";
        }

    }

    if (fn_virus_hist != "")
    {
        std::ofstream file_virus(fn_virus_hist, std::ios_base::out);
        
        // Repeat the same error if the file doesn't exists
        if (!file_virus)
        {
            throw std::runtime_error(
                "Could not open file \"" + fn_virus_hist +
                "\" for writing.")
                ;
        }

        file_virus <<
            #ifdef EPI_DEBUG
            "thread "<< "date " << "virus_id " << "virus " << "state " << "n\n";
            #else
            "date " << "virus_id " << "virus " << "state " << "n\n";
            #endif

        for (epiworld_fast_uint i = 0; i < hist_virus_id.size(); ++i)
            file_virus <<
                #ifdef EPI_DEBUG
                EPI_GET_THREAD_ID() << " " <<
                #endif
                hist_virus_date[i] << " " <<
                hist_virus_id[i] << " \"" <<
                virus_name[hist_virus_id[i]] << "\" " <<
                model->states_labels[hist_virus_state[i]] << " " <<
                hist_virus_counts[i] << "\n";
    }

    if (fn_tool_info != "")
    {
        std::ofstream file_tool_info(fn_tool_info, std::ios_base::out);

        // Repeat the same error if the file doesn't exists
        if (!file_tool_info)
        {
            throw std::runtime_error(
                "Could not open file \"" + fn_tool_info +
                "\" for writing.")
                ;
        }

        file_tool_info <<
            #ifdef EPI_DEBUG
            "thread " << 
            #endif
            "id " << "tool_name " << "tool_sequence " << "date_recorded\n";

        for (const auto & t : tool_id)
        {
            int id = t.second;
            file_tool_info <<
                #ifdef EPI_DEBUG
                EPI_GET_THREAD_ID() << " " <<
                #endif
                id << " \"" <<
                tool_name[id] << "\" " <<
                seq_writer(tool_sequence[id]) << " " <<
                tool_origin_date[id] << "\n";
        }

    }

    if (fn_tool_hist != "")
    {
        std::ofstream file_tool_hist(fn_tool_hist, std::ios_base::out);

        // Repeat the same error if the file doesn't exists
        if (!file_tool_hist)
        {
            throw std::runtime_error(
                "Could not open file \"" + fn_tool_hist +
                "\" for writing.")
                ;
        }
        
        file_tool_hist <<
            #ifdef EPI_DEBUG
            "thread " << 
            #endif
            "date " << "id " << "state " << "n\n";

        for (epiworld_fast_uint i = 0; i < hist_tool_id.size(); ++i)
            file_tool_hist <<
                #ifdef EPI_DEBUG
                EPI_GET_THREAD_ID() << " " <<
                #endif
                hist_tool_date[i] << " " <<
                hist_tool_id[i] << " " <<
                model->states_labels[hist_tool_state[i]] << " " <<
                hist_tool_counts[i] << "\n";
    }

    if (fn_total_hist != "")
    {
        std::ofstream file_total(fn_total_hist, std::ios_base::out);

        // Repeat the same error if the file doesn't exists
        if (!file_total)
        {
            throw std::runtime_error(
                "Could not open file \"" + fn_total_hist +
                "\" for writing.")
                ;
        }

        file_total <<
            #ifdef EPI_DEBUG
            "thread " << 
            #endif
            "date " << "nviruses " << "state " << "counts\n";

        for (epiworld_fast_uint i = 0; i < hist_total_date.size(); ++i)
            file_total <<
                #ifdef EPI_DEBUG
                EPI_GET_THREAD_ID() << " " <<
                #endif
                hist_total_date[i] << " " <<
                hist_total_nviruses_active[i] << " \"" <<
                model->states_labels[hist_total_state[i]] << "\" " << 
                hist_total_counts[i] << "\n";
    }

    if (fn_transmission != "")
    {
        std::ofstream file_transmission(fn_transmission, std::ios_base::out);

        // Repeat the same error if the file doesn't exists
        if (!file_transmission)
        {
            throw std::runtime_error(
                "Could not open file \"" + fn_transmission +
                "\" for writing.")
                ;
        }

        file_transmission <<
            #ifdef EPI_DEBUG
            "thread " << 
            #endif
            "date " << "virus_id virus " << "source_exposure_date " << "source " << "target\n";

        for (epiworld_fast_uint i = 0; i < transmission_target.size(); ++i)
            file_transmission <<
                #ifdef EPI_DEBUG
                EPI_GET_THREAD_ID() << " " <<
                #endif
                transmission_date[i] << " " <<
                transmission_virus[i] << " \"" <<
                virus_name[transmission_virus[i]] << "\" " <<
                transmission_source_exposure_date[i] << " " <<
                transmission_source[i] << " " <<
                transmission_target[i] << "\n";
                
    }

    if (fn_transition != "")
    {
        std::ofstream file_transition(fn_transition, std::ios_base::out);

        // Repeat the same error if the file doesn't exists
        if (!file_transition)
        {
            throw std::runtime_error(
                "Could not open file \"" + fn_transition +
                "\" for writing.")
                ;
        }

        file_transition <<
            #ifdef EPI_DEBUG
            "thread " << 
            #endif
            "date " << "from " << "to " << "counts\n";

        int ns = model->nstates;

        for (int i = 0; i <= model->today(); ++i)
        {

            for (int from = 0u; from < ns; ++from)
                for (int to = 0u; to < ns; ++to)
                    file_transition <<
                        #ifdef EPI_DEBUG
                        EPI_GET_THREAD_ID() << " " <<
                        #endif
                        i << " \"" <<
                        model->states_labels[from] << "\" \"" <<
                        model->states_labels[to] << "\" " <<
                        hist_transition_matrix[i * (ns * ns) + to * ns + from] << "\n";
                
        }
                
    }

    if (fn_reproductive_number != "")
        reproductive_number(fn_reproductive_number);

    if (fn_generation_time != "")
        generation_time(fn_generation_time);

}

template<typename TSeq>
inline void DataBase<TSeq>::record_transmission(
    int i,
    int j,
    int virus,
    int i_expo_date
) {

    transmission_date.push_back(model->today());
    transmission_source.push_back(i);
    transmission_target.push_back(j);
    transmission_virus.push_back(virus);
    transmission_source_exposure_date.push_back(i_expo_date);

}

template<typename TSeq>
inline size_t DataBase<TSeq>::get_n_viruses() const
{
    return virus_id.size();
}

template<typename TSeq>
inline size_t DataBase<TSeq>::get_n_tools() const
{
    return tool_id.size();
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
    epiworld_fast_uint k,
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

template<typename TSeq>
inline MapVec_type<int,int> DataBase<TSeq>::reproductive_number()
const {

    // Checking size
    MapVec_type<int,int> map;

    // Number of digits of maxid
    for (size_t i = 0u; i < transmission_date.size(); ++i)
    {
        // Fabricating id
        std::vector< int > h = {
            transmission_virus[i],
            transmission_source[i],
            transmission_source_exposure_date[i]
        };

        // Adding to counter
        if (map.find(h) == map.end())
            map[h] = 1;
        else
            map[h]++;

        // The target is added
        std::vector< int > h_target = {
            transmission_virus[i],
            transmission_target[i],
            transmission_date[i]
        };

        map[h_target] = 0;
        
    }

    return map;

}

template<typename TSeq>
inline void DataBase<TSeq>::reproductive_number(
    std::string fn
) const {


    auto map = reproductive_number();

    std::ofstream fn_file(fn, std::ios_base::out);

    // Repeat the same error if the file doesn't exists
    if (!fn_file)
    {
        throw std::runtime_error(
            "Could not open file \"" + fn +
            "\" for writing.")
            ;
    }

    fn_file << 
        #ifdef EPI_DEBUG
        "thread " <<
        #endif
        "virus_id virus source source_exposure_date rt\n";


    for (auto & m : map)
        fn_file <<
            #ifdef EPI_DEBUG
            EPI_GET_THREAD_ID() << " " <<
            #endif
            m.first[0u] << " \"" <<
            virus_name[m.first[0u]] << "\" " <<
            m.first[1u] << " " <<
            m.first[2u] << " " <<
            m.second << "\n";

    return;

}

template<typename TSeq>
inline std::vector< epiworld_double > DataBase<TSeq>::transition_probability(
    bool print,
    bool normalize
) const {

    auto states_labels = model->get_states();
    size_t n_state = states_labels.size();
    size_t n_days   = model->get_ndays();
    std::vector< epiworld_double > res(n_state * n_state, 0.0);
    std::vector< epiworld_double > rowsums(n_state, 0.0);

    for (size_t t = 0; t < n_days; ++t)
    {

        for (size_t s_i = 0; s_i < n_state; ++s_i)
        {

            for (size_t s_j = 0u; s_j < n_state; ++s_j)
            {
                res[s_i + s_j * n_state] += (
                    hist_transition_matrix[
                        s_i + s_j * n_state +
                        t * (n_state * n_state)
                    ]
                );
                
                rowsums[s_i] += hist_transition_matrix[
                    s_i + s_j * n_state +
                    t * (n_state * n_state)
                ];
            
            }

        }

    }

    if (normalize)
    {
        for (size_t s_i = 0; s_i < n_state; ++s_i)
        {
            for (size_t s_j = 0; s_j < n_state; ++s_j)
                res[s_i + s_j * n_state] /= rowsums[s_i];
        }
    }

    if (print)
    {   

        size_t nchar = 0u;
        for (auto & l : states_labels)
            if (l.length() > nchar)
                nchar = l.length();

        std::string fmt = " - %-" + std::to_string(nchar) + "s";

        std::string fmt_entry = " % 4.2f";
        if (!normalize)
        {
            nchar = 0u;
            for (auto & l: res)
            {
                std::string tmp = std::to_string(l);
                if (tmp.length() > nchar)
                    nchar = tmp.length();
            }

            fmt_entry = " % " + std::to_string(nchar) + ".0f";
        } 

        
        printf_epiworld("\nTransition Probabilities:\n");
        for (size_t s_i = 0u; s_i < n_state; ++s_i)
        {
            printf_epiworld(fmt.c_str(), states_labels[s_i].c_str());
            for (size_t s_j = 0u; s_j < n_state; ++s_j)
            {
                if (std::isnan(res[s_i + s_j * n_state]))
                {
                    printf_epiworld("     -");
                } else {
                    printf_epiworld(
                        fmt_entry.c_str(), res[s_i + s_j * n_state]
                    );
                }
            }
            printf_epiworld("\n");
        }

        printf_epiworld("\n");

    }

    return res;


} 

#define VECT_MATCH(a, b, c) \
    EPI_DEBUG_FAIL_AT_TRUE(a.size() != b.size(), c) \
    for (size_t __i = 0u; __i < a.size(); ++__i) \
    {\
        EPI_DEBUG_FAIL_AT_TRUE(a[__i] != b[__i], c) \
    }

template<>
inline bool DataBase<std::vector<int>>::operator==(const DataBase<std::vector<int>> & other) const
{
    VECT_MATCH(
        virus_name, other.virus_name,
        "DataBase:: virus_name don't match"
        )

    EPI_DEBUG_FAIL_AT_TRUE(
        virus_sequence.size() != other.virus_sequence.size(),
        "DataBase:: virus_sequence don't match."
        )

    for (size_t i = 0u; i < virus_sequence.size(); ++i)
    {
        VECT_MATCH(
            virus_sequence[i], other.virus_sequence[i],
            "DataBase:: virus_sequence[i] don't match"
            )
    }

    VECT_MATCH(
        virus_origin_date,
        other.virus_origin_date,
        "DataBase:: virus_origin_date[i] don't match"
    )

    VECT_MATCH(
        virus_parent_id,
        other.virus_parent_id,
        "DataBase:: virus_parent_id[i] don't match"
    )

    VECT_MATCH(
        tool_name,
        other.tool_name,
        "DataBase:: tool_name[i] don't match"
    )

    VECT_MATCH(
        tool_sequence,
        other.tool_sequence,
        "DataBase:: tool_sequence[i] don't match"
    )

    VECT_MATCH(
        tool_origin_date,
        other.tool_origin_date,
        "DataBase:: tool_origin_date[i] don't match"
    )


    EPI_DEBUG_FAIL_AT_TRUE(
        sampling_freq != other.sampling_freq,
        "DataBase:: sampling_freq don't match."
        )

    // Variants history
    VECT_MATCH(
        hist_virus_date,
        other.hist_virus_date,
        "DataBase:: hist_virus_date[i] don't match"
        )

    VECT_MATCH(
        hist_virus_id,
        other.hist_virus_id,
        "DataBase:: hist_virus_id[i] don't match"
        )

    VECT_MATCH(
        hist_virus_state,
        other.hist_virus_state,
        "DataBase:: hist_virus_state[i] don't match"
        )

    VECT_MATCH(
        hist_virus_counts,
        other.hist_virus_counts,
        "DataBase:: hist_virus_counts[i] don't match"
        )

    // Tools history
    VECT_MATCH(
        hist_tool_date,
        other.hist_tool_date,
        "DataBase:: hist_tool_date[i] don't match"
        )

    VECT_MATCH(
        hist_tool_id,
        other.hist_tool_id,
        "DataBase:: hist_tool_id[i] don't match"
        )

    VECT_MATCH(
        hist_tool_state,
        other.hist_tool_state,
        "DataBase:: hist_tool_state[i] don't match"
        )

    VECT_MATCH(
        hist_tool_counts,
        other.hist_tool_counts,
        "DataBase:: hist_tool_counts[i] don't match"
        )

    // Overall hist
    VECT_MATCH(
        hist_total_date,
        other.hist_total_date,
        "DataBase:: hist_total_date[i] don't match"
        )

    VECT_MATCH(
        hist_total_nviruses_active,
        other.hist_total_nviruses_active,
        "DataBase:: hist_total_nviruses_active[i] don't match"
        )

    VECT_MATCH(
        hist_total_state,
        other.hist_total_state,
        "DataBase:: hist_total_state[i] don't match"
        )

    VECT_MATCH(
        hist_total_counts,
        other.hist_total_counts,
        "DataBase:: hist_total_counts[i] don't match"
        )

    VECT_MATCH(
        hist_transition_matrix,
        other.hist_transition_matrix,
        "DataBase:: hist_transition_matrix[i] don't match"
        )

    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    EPI_DEBUG_FAIL_AT_TRUE(
        today_virus.size() != other.today_virus.size(),
        "DataBase:: today_virus don't match."
        )
    
    for (size_t i = 0u; i < today_virus.size(); ++i)
    {
        VECT_MATCH(
            today_virus[i], other.today_virus[i],
            "DataBase:: today_virus[i] don't match"
            )
    }

    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    if (today_tool.size() != other.today_tool.size())
        return false;
    
    for (size_t i = 0u; i < today_tool.size(); ++i)
    {
        VECT_MATCH(
            today_tool[i], other.today_tool[i],
            "DataBase:: today_tool[i] don't match"
            )
    }

    // {Susceptible, Infected, etc.}
    VECT_MATCH(
        today_total, other.today_total,
        "DataBase:: today_total don't match"
        )

    // Totals
    EPI_DEBUG_FAIL_AT_TRUE(
        today_total_nviruses_active != other.today_total_nviruses_active,
        "DataBase:: today_total_nviruses_active don't match."
        )

    // Transmission network
    VECT_MATCH(
        transmission_date,
        other.transmission_date,                 ///< Date of the transmission eve,
        "DataBase:: transmission_date[i] don't match"
        )

    VECT_MATCH(
        transmission_source,
        other.transmission_source,               ///< Id of the sour,
        "DataBase:: transmission_source[i] don't match"
        )

    VECT_MATCH(
        transmission_target,
        other.transmission_target,               ///< Id of the targ,
        "DataBase:: transmission_target[i] don't match"
        )

    VECT_MATCH(
        transmission_virus,
        other.transmission_virus,              ///< Id of the varia,
        "DataBase:: transmission_virus[i] don't match"
        )

    VECT_MATCH(
        transmission_source_exposure_date,
        other.transmission_source_exposure_date, ///< Date when the source acquired the varia,
        "DataBase:: transmission_source_exposure_date[i] don't match"
        )


    VECT_MATCH(
        transition_matrix,
        other.transition_matrix,
        "DataBase:: transition_matrix[i] don't match"
        )


    return true;

}

template<typename TSeq>
inline bool DataBase<TSeq>::operator==(const DataBase<TSeq> & other) const
{
    VECT_MATCH(
        virus_name,
        other.virus_name,
        "DataBase:: virus_name[i] don't match"
    )

    VECT_MATCH(
        virus_sequence,
        other.virus_sequence,
        "DataBase:: virus_sequence[i] don't match"
    )

    VECT_MATCH(
        virus_origin_date,
        other.virus_origin_date,
        "DataBase:: virus_origin_date[i] don't match"
    )

    VECT_MATCH(
        virus_parent_id,
        other.virus_parent_id,
        "DataBase:: virus_parent_id[i] don't match"
    )

    VECT_MATCH(
        tool_name,
        other.tool_name,
        "DataBase:: tool_name[i] don't match"
    )

    VECT_MATCH(
        tool_sequence,
        other.tool_sequence,
        "DataBase:: tool_sequence[i] don't match"
    )

    VECT_MATCH(
        tool_origin_date,
        other.tool_origin_date,
        "DataBase:: tool_origin_date[i] don't match"
    )

    
    EPI_DEBUG_FAIL_AT_TRUE(
        sampling_freq != other.sampling_freq,
        "DataBase:: sampling_freq don't match."
    )

    // Variants history
    VECT_MATCH(
        hist_virus_date,
        other.hist_virus_date,
        "DataBase:: hist_virus_date[i] don't match"
    )

    VECT_MATCH(
        hist_virus_id,
        other.hist_virus_id,
        "DataBase:: hist_virus_id[i] don't match"
    )

    VECT_MATCH(
        hist_virus_state,
        other.hist_virus_state,
        "DataBase:: hist_virus_state[i] don't match"
    )

    VECT_MATCH(
        hist_virus_counts,
        other.hist_virus_counts,
        "DataBase:: hist_virus_counts[i] don't match"
    )

    // Tools history
    VECT_MATCH(
        hist_tool_date,
        other.hist_tool_date,
        "DataBase:: hist_tool_date[i] don't match"
    )

    VECT_MATCH(
        hist_tool_id,
        other.hist_tool_id,
        "DataBase:: hist_tool_id[i] don't match"
    )

    VECT_MATCH(
        hist_tool_state,
        other.hist_tool_state,
        "DataBase:: hist_tool_state[i] don't match"
    )

    VECT_MATCH(
        hist_tool_counts,
        other.hist_tool_counts,
        "DataBase:: hist_tool_counts[i] don't match"
    )

    // Overall hist
    VECT_MATCH(
        hist_total_date,
        other.hist_total_date,
        "DataBase:: hist_total_date[i] don't match"
    )

    VECT_MATCH(
        hist_total_nviruses_active,
        other.hist_total_nviruses_active,
        "DataBase:: hist_total_nviruses_active[i] don't match"
    )

    VECT_MATCH(
        hist_total_state,
        other.hist_total_state,
        "DataBase:: hist_total_state[i] don't match"
    )

    VECT_MATCH(
        hist_total_counts,
        other.hist_total_counts,
        "DataBase:: hist_total_counts[i] don't match"
    )

    VECT_MATCH(
        hist_transition_matrix,
        other.hist_transition_matrix,
        "DataBase:: hist_transition_matrix[i] don't match"
    )

    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    EPI_DEBUG_FAIL_AT_TRUE(
        today_virus.size() != other.today_virus.size(),
        "DataBase:: today_virus.size() don't match."
    )
    
    for (size_t i = 0u; i < today_virus.size(); ++i)
    {
        VECT_MATCH(
            today_virus[i], other.today_virus[i],
            "DataBase:: today_virus[i] don't match"
            )
    }

    // {Variant 1: {state 1, state 2, etc.}, Variant 2: {...}, ...}
    EPI_DEBUG_FAIL_AT_TRUE(
        today_tool.size() != other.today_tool.size(),
        "DataBase:: today_tool.size() don't match."
    )
    
    for (size_t i = 0u; i < today_tool.size(); ++i)
    {
        VECT_MATCH(
            today_tool[i], other.today_tool[i],
            "DataBase:: today_tool[i] don't match"
            )
    }

    // {Susceptible, Infected, etc.}
    VECT_MATCH(
        today_total, other.today_total,
        "DataBase:: today_total[i] don't match"
        )

    // Totals
    EPI_DEBUG_FAIL_AT_TRUE(
        today_total_nviruses_active != other.today_total_nviruses_active,
        "DataBase:: today_total_nviruses_active don't match."
    )

    // Transmission network
    VECT_MATCH( ///< Date of the transmission eve
        transmission_date,
        other.transmission_date,
        "DataBase:: transmission_date[i] don't match"
    )

    VECT_MATCH( ///< Id of the sour
        transmission_source,
        other.transmission_source,
        "DataBase:: transmission_source[i] don't match"
    )

    VECT_MATCH( ///< Id of the targ
        transmission_target,
        other.transmission_target,
        "DataBase:: transmission_target[i] don't match"
    )

    VECT_MATCH( ///< Id of the varia
        transmission_virus,
        other.transmission_virus,
        "DataBase:: transmission_virus[i] don't match"
    )

    VECT_MATCH( ///< Date when the source acquired the varia
        transmission_source_exposure_date,
        other.transmission_source_exposure_date,
        "DataBase:: transmission_source_exposure_date[i] don't match"
    )

    VECT_MATCH(
        transition_matrix,
        other.transition_matrix,
        "DataBase:: transition_matrix[i] don't match"
    )

    return true;

}

template<typename TSeq>
inline void DataBase<TSeq>::generation_time(
    std::vector< int > & agent_id,
    std::vector< int > & virus_id,
    std::vector< int > & time,
    std::vector< int > & gentime
) const {
    
    size_t nevents = transmission_date.size();

    agent_id.reserve(nevents);
    virus_id.reserve(nevents);
    time.reserve(nevents);
    gentime.reserve(nevents);

    // Iterating through the individuals
    for (size_t i = 0u; i < nevents; ++i)
    {
        int agent_id_i = transmission_target[i];
        agent_id.push_back(agent_id_i);
        virus_id.push_back(transmission_virus[i]);
        time.push_back(transmission_date[i]);

        bool found = false;
        for (size_t j = i; j < nevents; ++j)
        {

            if (transmission_source[j] == agent_id_i)
            {
                gentime.push_back(transmission_date[j] - time[i]);
                found = true;
                break;
            }

        }

        // If there's no transmission, we set the generation time to
        // minus 1;
        if (!found)
            gentime.push_back(-1);

    }

    agent_id.shrink_to_fit();
    virus_id.shrink_to_fit();
    time.shrink_to_fit();
    gentime.shrink_to_fit();

    return;

}

template<typename TSeq>
inline void DataBase<TSeq>::generation_time(
    std::string fn
) const
{

    std::vector< int > agent_id;
    std::vector< int > virus_id;
    std::vector< int > time;
    std::vector< int > gentime;

    generation_time(agent_id, virus_id, time, gentime);

    std::ofstream fn_file(fn, std::ios_base::out);

    // Throw an error if the file doesn't exists using throw
    if (!fn_file)
    {
        throw std::runtime_error(
            "DataBase::generation_time: "
            "Cannot open file " + fn + "."
        );
    }



    fn_file << 
        #ifdef EPI_DEBUG
        "thread " <<
        #endif
        "virus source source_exposure_date gentime\n";

    size_t n = agent_id.size();
    for (size_t i = 0u; i < n; ++i)
        fn_file <<
            #ifdef EPI_DEBUG
            EPI_GET_THREAD_ID() << " " <<
            #endif
            virus_id[i] << " " <<
            agent_id[i] << " " <<
            time[i] << " " <<
            gentime[i] << "\n";

    return;

}

#undef VECT_MATCH

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

    std::vector<std::map<int, int>> dat;
    bool directed;
    epiworld_fast_uint N = 0;
    epiworld_fast_uint E = 0;

public:

    AdjList() {};

    /**
     * @brief Construct a new Adj List object
     * 
     * @details 
     * Ids in the network are assume to range from `0` to `size - 1`.
     * 
     * @param source Unsigned int vector with the source
     * @param target Unsigned int vector with the target
     * @param size Number of vertices in the network.
     * @param directed Bool true if the network is directed
     */
    AdjList(
        const std::vector< int > & source,
        const std::vector< int > & target,
        int size,
        bool directed
        );

    AdjList(AdjList && a); // Move constructor
    AdjList(const AdjList & a); // Copy constructor
    AdjList& operator=(const AdjList& a);


    /**
     * @brief Read an edgelist
     * 
     * Ids in the network are assume to range from `0` to `size - 1`.
     * 
     * @param fn Path to the file
     * @param skip Number of lines to skip (e.g., 1 if there's a header)
     * @param directed `true` if the network is directed
     * @param size Number of vertices in the network.
     */
    void read_edgelist(
        std::string fn,
        int size,
        int skip = 0,
        bool directed = true
        );

    std::map<int, int> operator()(
        epiworld_fast_uint i
        ) const;
        
    void print(epiworld_fast_uint limit = 20u) const;
    size_t vcount() const; ///< Number of vertices/nodes in the network.
    size_t ecount() const; ///< Number of edges/arcs/ties in the network.
    
    std::vector<std::map<int,int>> & get_dat() {
        return dat;
    };

    bool is_directed() const; ///< `true` if the network is directed.

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
    const std::vector< int > & source,
    const std::vector< int > & target,
    int size,
    bool directed
) : directed(directed) {


    dat.resize(size, std::map<int,int>({}));
    int max_id = size - 1;

    int i,j;
    for (int m = 0; m < static_cast<int>(source.size()); ++m)
    {

        i = source[m];
        j = target[m];

        if (i > max_id)
            throw std::range_error(
                "The source["+std::to_string(m)+"] = " + std::to_string(i) +
                " is above the max_id " + std::to_string(max_id)
                );

        if (j > max_id)
            throw std::range_error(
                "The target["+std::to_string(m)+"] = " + std::to_string(j) +
                " is above the max_id " + std::to_string(max_id)
                );

        // Adding nodes
        if (dat[i].find(j) == dat[i].end())
            dat[i].insert(std::pair<int, int>(j, 1u));
        else
            dat[i][j]++; 
        
        if (!directed)
        {

            if (dat[j].find(i) == dat[j].end())
                dat[j].insert(std::pair<int, int>(i, 1u));
            else
                dat[j][i]++;

        }

        E++;

    }

    N = size;

    return;

}


inline AdjList::AdjList(AdjList && a) :
    dat(std::move(a.dat)),
    directed(a.directed),
    N(a.N),
    E(a.E)
{

}

inline AdjList::AdjList(const AdjList & a) :
    dat(a.dat),
    directed(a.directed),
    N(a.N),
    E(a.E)
{

}

inline AdjList& AdjList::operator=(const AdjList& a)
{
    if (this == &a)
        return *this;

    this->dat = a.dat;
    this->directed = a.directed;
    this->N = a.N;
    this->E = a.E;

    return *this;
}

inline void AdjList::read_edgelist(
    std::string fn,
    int size,
    int skip,
    bool directed
) {

    int i,j;
    std::ifstream filei(fn);

    if (!filei)
        throw std::logic_error("The file " + fn + " was not found.");

    int linenum = 0;
    std::vector< int > source_;
    std::vector< int > target_;

    source_.reserve(1e5);
    target_.reserve(1e5);

    int max_id = size - 1;

    while (!filei.eof())
    {

        if (linenum++ < skip)
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

        if (i > max_id)
            throw std::range_error(
                "The source["+std::to_string(linenum)+"] = " + std::to_string(i) +
                " is above the max_id " + std::to_string(max_id)
                );

        if (j > max_id)
            throw std::range_error(
                "The target["+std::to_string(linenum)+"] = " + std::to_string(j) +
                " is above the max_id " + std::to_string(max_id)
                );

        source_.push_back(i);
        target_.push_back(j);

    }

    // Now using the right constructor
    *this = AdjList(source_, target_, size, directed);

    return;

}

inline std::map<int,int> AdjList::operator()(
    epiworld_fast_uint i
    ) const {

    if (i >= N)
        throw std::range_error(
            "The vertex id " + std::to_string(i) + " is not in the network."
            );

    return dat[i];

}

inline void AdjList::print(epiworld_fast_uint limit) const {


    epiworld_fast_uint counter = 0;
    printf_epiworld("Nodeset:\n");
    int i = -1;
    for (auto & n : dat)
    {

        if (counter++ > limit)
            break;

        printf_epiworld("  % 3i: {", ++i);
        int niter = 0;
        for (auto n_n : n)
            if (++niter < static_cast<int>(n.size()))
            {    
                printf_epiworld("%i, ", static_cast<int>(n_n.first));
            }
            else {
                printf_epiworld("%i}\n", static_cast<int>(n_n.first));
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
class Agent;

class AdjList;


template<typename TSeq, typename TDat>
inline void rewire_degseq(
    TDat * agents,
    Model<TSeq> * model,
    epiworld_double proportion
    );

template<typename TSeq = int>
inline void rewire_degseq(
    std::vector< Agent<TSeq> > * agents,
    Model<TSeq> * model,
    epiworld_double proportion
    )
{

    #ifdef EPI_DEBUG
    std::vector< int > _degree0(agents->size(), 0);
    for (size_t i = 0u; i < _degree0.size(); ++i)
        _degree0[i] = model->get_agents()[i].get_neighbors().size();
    #endif

    // Identifying individuals with degree > 0
    std::vector< epiworld_fast_uint > non_isolates;
    std::vector< epiworld_double > weights;
    epiworld_double nedges = 0.0;
    
    for (epiworld_fast_uint i = 0u; i < agents->size(); ++i)
    {
        if (agents->operator[](i).get_neighbors().size() > 0u)
        {
            non_isolates.push_back(i);
            epiworld_double wtemp = static_cast<epiworld_double>(
                agents->operator[](i).get_neighbors().size()
                );
            weights.push_back(wtemp);
            nedges += wtemp;
        }
    }

    if (non_isolates.size() == 0u)
        throw std::logic_error("The graph is completely disconnected.");

    // Cumulative probs
    weights[0u] /= nedges;
    for (epiworld_fast_uint i = 1u; i < non_isolates.size(); ++i)
    {
         weights[i] /= nedges;
         weights[i] += weights[i - 1u];
    }

    // Only swap if needed
    epiworld_fast_uint N = non_isolates.size();
    epiworld_double prob;
    int nrewires = floor(proportion * nedges);
    while (nrewires-- > 0)
    {

        // Picking egos
        prob = model->runif();
        int id0 = N - 1;
        for (epiworld_fast_uint i = 0u; i < N; ++i)
            if (prob <= weights[i])
            {
                id0 = i;
                break;
            }

        prob = model->runif();
        int id1 = N - 1;
        for (epiworld_fast_uint i = 0u; i < N; ++i)
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

        Agent<TSeq> & p0 = agents->operator[](non_isolates[id0]);
        Agent<TSeq> & p1 = agents->operator[](non_isolates[id1]);

        // Picking alters (relative location in their lists)
        // In this case, these are uniformly distributed within the list
        int id01 = std::floor(p0.get_n_neighbors() * model->runif());
        int id11 = std::floor(p1.get_n_neighbors() * model->runif());

        // When rewiring, we need to flip the individuals from the other
        // end as well, since we are dealing withi an undirected graph
        
        // Finding what neighbour is id0
        model->get_agents()[id0].swap_neighbors(
            model->get_agents()[id1],
            id01,
            id11
            );
        

    }

    #ifdef EPI_DEBUG
    for (size_t _i = 0u; _i < _degree0.size(); ++_i)
    {
        if (_degree0[_i] != static_cast<int>(model->get_agents()[_i].get_n_neighbors()))
            throw std::logic_error("[epi-debug] Degree does not match afted rewire_degseq.");
    }
    #endif

    return;

}

template<typename TSeq>
inline void rewire_degseq(
    AdjList * agents,
    Model<TSeq> * model,
    epiworld_double proportion
    )
{

    // Identifying individuals with degree > 0
    std::vector< epiworld_fast_int > nties(agents->vcount(), 0); 

    #ifdef EPI_DEBUG
    std::vector< int > _degree0(agents->vcount(), 0);
    for (size_t i = 0u; i < _degree0.size(); ++i)
        _degree0[i] = agents->get_dat()[i].size();
    #endif
    
    std::vector< epiworld_fast_uint > non_isolates;
    non_isolates.reserve(nties.size());

    std::vector< epiworld_double > weights;
    weights.reserve(nties.size());

    epiworld_double nedges = 0.0;
    auto & dat = agents->get_dat();

    for (size_t i = 0u; i < dat.size(); ++i)
        nties[i] += dat[i].size();
    
    bool directed = agents->is_directed();
    for (size_t i = 0u; i < dat.size(); ++i)
    {
        if (nties[i] > 0)
        {
            non_isolates.push_back(i);
            if (directed)
            {
                weights.push_back( 
                    static_cast<epiworld_double>(nties[i])
                );
                nedges += static_cast<epiworld_double>(nties[i]);
            }
            else {
                weights.push_back( 
                    static_cast<epiworld_double>(nties[i])/2.0
                );
                nedges += static_cast<epiworld_double>(nties[i]) / 2.0;
            }
        }
    }

    if (non_isolates.size() == 0u)
        throw std::logic_error("The graph is completely disconnected.");

    // Cumulative probs
    weights[0u] /= nedges;
    for (epiworld_fast_uint i = 1u; i < non_isolates.size(); ++i)
    {
         weights[i] /= nedges;
         weights[i] += weights[i - 1u];
    }

    // Only swap if needed
    epiworld_fast_uint N = non_isolates.size();
    epiworld_double prob;
    int nrewires = floor(proportion * nedges / (
        agents->is_directed() ? 1.0 : 2.0
    ));

    while (nrewires-- > 0)
    {

        // Picking egos
        prob = model->runif();
        int id0 = N - 1;
        for (epiworld_fast_uint i = 0u; i < N; ++i)
            if (prob <= weights[i])
            {
                id0 = i;
                break;
            }

        prob = model->runif();
        int id1 = N - 1;
        for (epiworld_fast_uint i = 0u; i < N; ++i)
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

        std::map<int,int> & p0 = agents->get_dat()[non_isolates[id0]];
        std::map<int,int> & p1 = agents->get_dat()[non_isolates[id1]];

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
        if (!agents->is_directed())
        {

            std::map<int,int> & p01 = agents->get_dat()[id01];
            std::map<int,int> & p11 = agents->get_dat()[id11];

            std::swap(p01[id0], p11[id1]);
            
        }

        // Moving alter first
        std::swap(p0[id01], p1[id11]);

    }

    #ifdef EPI_DEBUG
    for (size_t _i = 0u; _i < _degree0.size(); ++_i)
    {
        if (_degree0[_i] != static_cast<int>(agents->get_dat()[_i].size()))
            throw std::logic_error(
                "[epi-debug] Degree does not match afted rewire_degseq. " +
                std::string("Expected: ") + 
                std::to_string(_degree0[_i]) + 
                std::string(", observed: ") +
                std::to_string(agents->get_dat()[_i].size())
                );
    }
    #endif


    return;

}

template<typename TSeq>
inline AdjList rgraph_bernoulli(
    epiworld_fast_uint n,
    epiworld_double p,
    bool directed,
    Model<TSeq> & model
) {

    std::vector< int > source;
    std::vector< int > target;

    // Checking the density (how many)
    std::binomial_distribution<> d(
        n * (n - 1.0) / (directed ? 1.0 : 2.0),
        p
    );

    epiworld_fast_uint m = d(model.get_rand_endgine());

    source.resize(m);
    target.resize(m);

    epiworld_fast_uint a,b;
    for (epiworld_fast_uint i = 0u; i < m; ++i)
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

        source[i] = static_cast<int>(a);
        target[i] = static_cast<int>(b);

    }

    AdjList al(source, target, static_cast<int>(n), directed);

    return al;
    
}

template<typename TSeq>
inline AdjList rgraph_bernoulli2(
    epiworld_fast_uint n,
    epiworld_double p,
    bool directed,
    Model<TSeq> & model
) {

    std::vector< int > source;
    std::vector< int > target;

    // Checking the density (how many)
    std::binomial_distribution<> d(
        n * (n - 1.0) / (directed ? 1.0 : 2.0),
        p
    );

    // Need to compensate for the possible number of diagonal
    // elements sampled. If n * n, then each diag element has
    // 1/(n^2) chance of sampling

    epiworld_fast_uint m = d(model.get_rand_endgine());

    source.resize(m);
    target.resize(m);

    double n2 = static_cast<double>(n * n);

    int loc,row,col;
    for (epiworld_fast_uint i = 0u; i < m; ++i)
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

    AdjList al(source, target, static_cast<int>(n), directed);

    return al;
    
}

inline AdjList rgraph_ring_lattice(
    epiworld_fast_uint n,
    epiworld_fast_uint k,
    bool directed = false
) {

    if ((n - 1u) < k)
        throw std::logic_error("k can be at most n - 1.");

    std::vector< int > source;
    std::vector< int > target;

    if (!directed)
        if (k > 1u) k = static_cast< size_t >(floor(k / 2.0));

    for (size_t i = 0; i < n; ++i)
    {

        for (size_t j = 1u; j <= k; ++j)
        {

            // Next neighbor
            size_t l = i + j;
            if (l >= n) l = l - n;

            source.push_back(i);
            target.push_back(l);

        }

    }

    return AdjList(source, target, n, directed);

}

/**
 * @brief Smallworld network (Watts-Strogatz)
 * 
 * @tparam TSeq 
 * @param n 
 * @param k 
 * @param p 
 * @param directed 
 * @param model 
 * @return AdjList 
 */
template<typename TSeq>
inline AdjList rgraph_smallworld(
    epiworld_fast_uint n,
    epiworld_fast_uint k,
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

/**
 * @brief Generates a blocked network
 * 
 * Since block sizes and number of connections between blocks are fixed,
 * this routine is fully deterministic.
 * 
 * @tparam TSeq 
 * @param n Size of the network
 * @param blocksize Size of the block.
 * @param ncons Number of connections between blocks
 * @param model A model
 * @return AdjList 
 */
template<typename TSeq>
inline AdjList rgraph_blocked(
    epiworld_fast_uint n,
    epiworld_fast_uint blocksize,
    epiworld_fast_uint ncons,
    Model<TSeq>&
) {

    std::vector< int > source_;
    std::vector< int > target_;

    size_t i = 0u;
    size_t cum_node_count = 0u;
    while (i < n)
    {

        for (size_t j = 0; j < blocksize; ++j)
        {

            for (size_t k = 0; k < j; ++k)
            {
                // No loops
                if (k == j)
                    continue;

                // Exists the loop in case there are no more 
                // nodes available
                if ((i + k) >= n)
                    break;

                source_.push_back(static_cast<int>(j + i));
                target_.push_back(static_cast<int>(k + i));
            }

            // No more nodes left to build connections
            if (++cum_node_count >= n)
                break;
            
        }

        // Connections between this and the previou sone
        if (i != 0)
        {

            size_t max_cons = std::min(ncons, n - cum_node_count);

            // Generating the connections
            for (size_t j = 0u; j < max_cons; ++j)
            {

                source_.push_back(static_cast<int>(i + j - blocksize));
                target_.push_back(static_cast<int>(i + j));

            }
        }

        i += blocksize;
        
    }
        
    return AdjList(source_, target_, n, false);

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
template<typename TSeq>
class Queue
{
    friend class Model<TSeq>;

private:

    /**
     * @brief Count of ego's neighbors in queue (including ego)
     */
    std::vector< epiworld_fast_int > active;
    Model<TSeq> * model = nullptr;
    int n_in_queue = 0;

    // Auxiliary variable that checks how many steps
    // left are there
    // int n_steps_left;
    // bool queuing_started   = false;

public:

    void operator+=(Agent<TSeq> * p);
    void operator-=(Agent<TSeq> * p);
    epiworld_fast_int & operator[](epiworld_fast_uint i);

    // void initialize(Model<TSeq> * m, Agent<TSeq> * p);
    void reset();

    bool operator==(const Queue<TSeq> & other) const;
    bool operator!=(const Queue<TSeq> & other) const {return !operator==(other);};

    static const int NoOne    = 0;
    static const int OnlySelf = 1;
    static const int Everyone = 2;

};

template<typename TSeq>
inline void Queue<TSeq>::operator+=(Agent<TSeq> * p)
{

    if (++active[p->id] == 1)
        n_in_queue++;

    for (auto n : p->neighbors)
    {

        if (++active[n] == 1)
            n_in_queue++;

    }

}

template<typename TSeq>
inline void Queue<TSeq>::operator-=(Agent<TSeq> * p)
{

    if (--active[p->id] == 0)
        n_in_queue--;

    for (auto n : p->neighbors)
    {
        if (--active[n] == 0)
            n_in_queue--;
    }

}

template<typename TSeq>
inline epiworld_fast_int & Queue<TSeq>::operator[](epiworld_fast_uint i)
{
    return active[i];
}

template<typename TSeq>
inline void Queue<TSeq>::reset()
{

    if (n_in_queue)
    {

        for (auto & q : this->active)
            q = 0;

        n_in_queue = 0;
        
    }

    active.resize(model->size(), 0);

}

template<typename TSeq>
inline bool Queue<TSeq>::operator==(const Queue<TSeq> & other) const 
{
    if (active.size() != other.active.size())
        return false;

    for (size_t i = 0u; i < active.size(); ++i)
    {
        if (active[i] != other.active[i])
            return false;
    }

    return true;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/queue-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/globalevent-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_GLOBALEVENT_BONES_HPP
#define EPIWORLD_GLOBALEVENT_BONES_HPP

// template<typename TSeq = EPI_DEFAULT_TSEQ>
// using GlobalFun = std::function<void(Model<TSeq>*)>;

/**
 * @brief Template for a Global Event
 * @details Global events are functions that Model<TSeq> executes
 * at the end of a day.
 * 
 */
template<typename TSeq>
class GlobalEvent
{
private:
    GlobalFun<TSeq> fun = nullptr;
    std::string name = "A global action";
    int day = -99;
public:

    GlobalEvent() {};

    /**
     * @brief Construct a new Global Event object
     * 
     * @param fun A function that takes a Model<TSeq> * as argument and returns void.
     * @param name A descriptive name for the action.
     * @param day The day when the action will be executed. If negative, it will be executed every day.
     */
    GlobalEvent(GlobalFun<TSeq> fun, std::string name, int day = -99);
    
    ~GlobalEvent() {};

    void operator()(Model<TSeq> * m, int day);

    void set_name(std::string name);
    std::string get_name() const;

    void set_day(int day);
    int get_day() const;
    
    void print() const;

    // Comparison operators
    bool operator==(const GlobalEvent<TSeq> & other) const;
    bool operator!=(const GlobalEvent<TSeq> & other) const;

};



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/globalevent-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/globalevent-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_GLOBALEVENT_MEAT_HPP
#define EPIWORLD_GLOBALEVENT_MEAT_HPP

template<typename TSeq>
inline GlobalEvent<TSeq>::GlobalEvent(
    GlobalFun<TSeq> fun,
    std::string name,
    int day
    )
{
    this->fun = fun;
    this->name = name;
    this->day = day;
}

template<typename TSeq>
inline void GlobalEvent<TSeq>::operator()(Model<TSeq> * m, int day)
{   
    
    if (this->fun == nullptr)
        return;

    // events apply if day is negative or if day is equal to the day of the action
    if (this->day < 0 || this->day == day)
        this->fun(m);
    
    return;

}

template<typename TSeq>
inline void GlobalEvent<TSeq>::set_name(std::string name)
{
    this->name = name;
}

template<typename TSeq>
inline std::string GlobalEvent<TSeq>::get_name() const
{
    return this->name;
}

template<typename TSeq>
inline void GlobalEvent<TSeq>::set_day(int day)
{
    this->day = day;
}

template<typename TSeq>
inline int GlobalEvent<TSeq>::get_day() const
{
    return this->day;
}

template<typename TSeq>
inline void GlobalEvent<TSeq>::print() const
{
    printf_epiworld(
        "Global action: %s\n"
        "  - Day: %i\n",
        this->name.c_str(),
        this->day
        );
}

template<typename TSeq>
inline bool GlobalEvent<TSeq>::operator==(const GlobalEvent<TSeq> & other) const
{
    return (this->name == other.name) && (this->day == other.day);
}

template<typename TSeq>
inline bool GlobalEvent<TSeq>::operator!=(const GlobalEvent<TSeq> & other) const
{
    return !(*this == other);
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/globalevent-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/model-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_BONES_HPP
#define EPIWORLD_MODEL_BONES_HPP

template<typename TSeq>
class Agent;

template<typename TSeq>
class AgentsSample;

template<typename TSeq>
class Virus;

template<typename TSeq>
class Viruses;

template<typename TSeq> 
class Viruses_const;

template<typename TSeq>
class Tool;

class AdjList;

template<typename TSeq>
class DataBase;

template<typename TSeq>
class Queue;

template<typename TSeq>
struct Event;

template<typename TSeq>
class GlobalEvent;

template<typename TSeq>
inline epiworld_double susceptibility_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
    );
template<typename TSeq>
inline epiworld_double transmission_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
    );
template<typename TSeq>
inline epiworld_double recovery_enhancer_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
    );
template<typename TSeq>
inline epiworld_double death_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
    );

template<typename TSeq = EPI_DEFAULT_TSEQ>
inline std::function<void(size_t,Model<TSeq>*)> make_save_run(
    std::string fmt = "%03lu-episimulation.csv",
    bool total_hist = true,
    bool virus_info = false,
    bool virus_hist = false,
    bool tool_info = false,
    bool tool_hist = false,
    bool transmission = false,
    bool transition = false,
    bool reproductive = false,
    bool generation = false
    );

// template<typename TSeq>
// class VirusPtr;

// template<typename TSeq>
// class ToolPtr;

/**
 * @brief Core class of epiworld.
 * 
 * The model class provides the wrapper that puts together `Agent`, `Virus`, and
 * `Tools`.
 * 
 * @tparam TSeq Type of sequence. In principle, users can build models in which
 * virus and human sequence is represented as numeric vectors (if needed.)
 */
template<typename TSeq>
class Model {
    friend class Agent<TSeq>;
    friend class AgentsSample<TSeq>;
    friend class DataBase<TSeq>;
    friend class Queue<TSeq>;
protected:

    std::string name = ""; ///< Name of the model

    DataBase<TSeq> db = DataBase<TSeq>(*this);

    std::vector< Agent<TSeq> > population = {};

    bool using_backup = true;
    std::vector< Agent<TSeq> > population_backup = {};

    /**
     * @name Auxiliary variables for AgentsSample<TSeq> iterators
     * 
     * @details These variables+objects are used by the AgentsSample<TSeq>
     * class for building efficient iterators over agents. The idea is to
     * reduce the memory allocation, so only during the first call of
     * AgentsSample<TSeq>::AgentsSample(Model<TSeq>) these vectors are allocated.
     */
    ///@{
    std::vector< Agent<TSeq> * > sampled_population;
    size_t sampled_population_n = 0u;
    std::vector< size_t > population_left;
    size_t population_left_n = 0u;
    ///@}

    /**
     * @name Agents features
     * 
     * @details Optionally, a model can include an external data source
     * pointing to agents information. The data can then be access through
     * the `Agent::operator()` method.
     * 
     */
    ///@{
    double * agents_data = nullptr;
    size_t agents_data_ncols = 0u;
    ///@}

    bool directed = false;
    
    std::vector< VirusPtr<TSeq> > viruses = {};
    std::vector< ToolPtr<TSeq> > tools = {};

    std::vector< Entity<TSeq> > entities = {}; 
    std::vector< Entity<TSeq> > entities_backup = {};

    std::shared_ptr< std::mt19937 > engine = std::make_shared< std::mt19937 >();
    
    std::uniform_real_distribution<> runifd      =
        std::uniform_real_distribution<> (0.0, 1.0);
    std::normal_distribution<>       rnormd      =
        std::normal_distribution<>(0.0);
    std::gamma_distribution<>        rgammad     =
        std::gamma_distribution<>();
    std::lognormal_distribution<>    rlognormald =
        std::lognormal_distribution<>();
    std::exponential_distribution<>  rexpd       =
        std::exponential_distribution<>();
    std::binomial_distribution<> rbinomd         =
        std::binomial_distribution<>();
    std::negative_binomial_distribution<> rnbinomd =
        std::negative_binomial_distribution<>();
    std::geometric_distribution<> rgeomd          =
        std::geometric_distribution<>();
    std::poisson_distribution<> rpoissd           =
        std::poisson_distribution<>();

    std::function<void(std::vector<Agent<TSeq>>*,Model<TSeq>*,epiworld_double)> rewire_fun;
    epiworld_double rewire_prop = 0.0;
        
    std::map<std::string, epiworld_double > parameters;
    epiworld_fast_uint ndays = 0;
    Progress pb;

    std::vector< UpdateFun<TSeq> >    state_fun = {};                  ///< Functions to update states
    std::vector< std::string >        states_labels = {};              ///< Labels of the states
    
    /** Function to distribute states. Goes along with the function  */
    std::function<void(Model<TSeq>*)> initial_states_fun = [](Model<TSeq> * /**/)
    -> void {};

    epiworld_fast_uint nstates = 0u;
    
    bool verbose     = true;
    int current_date = 0;

    void dist_tools();
    void dist_virus();
    void dist_entities();

    std::chrono::time_point<std::chrono::steady_clock> time_start;
    std::chrono::time_point<std::chrono::steady_clock> time_end;

    // std::chrono::milliseconds
    std::chrono::duration<epiworld_double,std::micro> time_elapsed = 
        std::chrono::duration<epiworld_double,std::micro>::zero();
    epiworld_fast_uint n_replicates = 0u;
    void chrono_start();
    void chrono_end();

    std::vector<GlobalEvent<TSeq>> globalevents;

    Queue<TSeq> queue;
    bool use_queuing   = true;

    /**
     * @brief Variables used to keep track of the events
     * to be made regarding viruses.
     */
    std::vector< Event<TSeq> > events = {};
    epiworld_fast_uint nactions = 0u;

    /**
     * @brief Construct a new Event object
     * 
     * @param agent_ Agent over which the action will be called
     * @param virus_ Virus pointer included in the action
     * @param tool_ Tool pointer included in the action
     * @param entity_ Entity pointer included in the action
     * @param new_state_ New state of the agent
     * @param call_ Function the action will call
     * @param queue_ Change in the queue
     * @param idx_agent_ Location of agent in object.
     * @param idx_object_ Location of object in agent.
     */
    void events_add(
        Agent<TSeq> * agent_,
        VirusPtr<TSeq> virus_,
        ToolPtr<TSeq> tool_,
        Entity<TSeq> * entity_,
        epiworld_fast_int new_state_,
        epiworld_fast_int queue_,
        EventFun<TSeq> call_,
        int idx_agent_,
        int idx_object_
        );

    /**
     * @name Tool Mixers
     * 
     * These functions combine the effects tools have to deliver
     * a single effect. For example, wearing a mask, been vaccinated,
     * and the immune system combine together to jointly reduce
     * the susceptibility for a given virus.
     * 
     */
    MixerFun<TSeq> susceptibility_reduction_mixer = susceptibility_reduction_mixer_default<TSeq>;
    MixerFun<TSeq> transmission_reduction_mixer = transmission_reduction_mixer_default<TSeq>;
    MixerFun<TSeq> recovery_enhancer_mixer = recovery_enhancer_mixer_default<TSeq>;
    MixerFun<TSeq> death_reduction_mixer = death_reduction_mixer_default<TSeq>;

    /**
     * @brief Advanced usage: Makes a copy of data and returns it as undeleted pointer
     * 
     * @param copy 
     */
    virtual Model<TSeq> * clone_ptr();

public:

    
    std::vector<epiworld_double> array_double_tmp;
    std::vector<Virus<TSeq> * > array_virus_tmp;
    std::vector< int > array_int_tmp;

    Model();
    Model(const Model<TSeq> & m);
    Model(Model<TSeq> & m);
    Model(Model<TSeq> && m);
    Model<TSeq> & operator=(const Model<TSeq> & m);

    virtual ~Model() {};

    /**
     * @name Set the backup object
     * @details `backup` can be used to restore the entire object
     * after a run. This can be useful if the user wishes to have
     * individuals start with the same network from the beginning.
     * 
     */
    ///@{
    void set_backup();
    // void restore_backup();
    ///@}

    DataBase<TSeq> & get_db();
    const DataBase<TSeq> & get_db() const;
    epiworld_double & operator()(std::string pname);

    size_t size() const;

    /**
     * @name Random number generation
     * 
     * @param eng Random number generator
     * @param s Seed
     */
    ///@{
    void set_rand_engine(std::shared_ptr< std::mt19937 > & eng);
    std::shared_ptr< std::mt19937 > & get_rand_endgine();
    void seed(size_t s);
    void set_rand_norm(epiworld_double mean, epiworld_double sd);
    void set_rand_unif(epiworld_double a, epiworld_double b);
    void set_rand_exp(epiworld_double lambda);
    void set_rand_gamma(epiworld_double alpha, epiworld_double beta);
    void set_rand_lognormal(epiworld_double mean, epiworld_double shape);
    void set_rand_binom(int n, epiworld_double p);
    void set_rand_nbinom(int n, epiworld_double p);
    void set_rand_geom(epiworld_double p);
    void set_rand_poiss(epiworld_double lambda);
    epiworld_double runif();
    epiworld_double runif(epiworld_double a, epiworld_double b);
    epiworld_double rnorm();
    epiworld_double rnorm(epiworld_double mean, epiworld_double sd);
    epiworld_double rgamma();
    epiworld_double rgamma(epiworld_double alpha, epiworld_double beta);
    epiworld_double rexp();
    epiworld_double rexp(epiworld_double lambda);
    epiworld_double rlognormal();
    epiworld_double rlognormal(epiworld_double mean, epiworld_double shape);
    int rbinom();
    int rbinom(int n, epiworld_double p);
    int rnbinom();
    int rnbinom(int n, epiworld_double p);
    int rgeom();
    int rgeom(epiworld_double p);
    int rpoiss();
    int rpoiss(epiworld_double lambda);
    ///@}

    /**
     * @name Add Virus/Tool to the model
     * 
     * This is done before the model has been initialized.
     * 
     * @param v Virus to be added
     * @param t Tool to be added
     * @param preval Initial prevalence (initial state.) It can be
     * specified as a proportion (between zero and one,) or an integer
     * indicating number of individuals.
     */
    ///@{
    void add_virus(Virus<TSeq> & v);
    void add_tool(Tool<TSeq> & t);
    void add_entity(Entity<TSeq> e);
    void rm_virus(size_t virus_pos);
    void rm_tool(size_t tool_pos);
    void rm_entity(size_t entity_id);
    ///@}

    /**
     * @brief Associate agents-entities from a file
     * 
     * The structure of the file should be two columns separated by 
     * space. The first column indexing between 0 and nagents-1, and the
     * second column between 0 and nentities - 1.
     * 
     * @param fn Path to the file.
     * @param skip How many rows to skip.
     */
    void load_agents_entities_ties(std::string fn, int skip);
    
    /**
     * @brief Associate agents-entities from data
    */
    void load_agents_entities_ties(
        const std::vector<int> & agents_ids,
        const std::vector<int> & entities_ids
        );

    void load_agents_entities_ties(
        const int * agents_id,
        const int * entities_id,
        size_t n
        );

    /**
     * @name Accessing population of the model
     * 
     * @param fn std::string Filename of the edgelist file.
     * @param skip int Number of lines to skip in `fn`.
     * @param directed bool Whether the graph is directed or not.
     * @param size Size of the network.
     * @param al AdjList to read into the model.
     */
    ///@{
    void agents_from_adjlist(
        std::string fn,
        int size,
        int skip = 0,
        bool directed = false
        );

    void agents_from_edgelist(
        const std::vector< int > & source,
        const std::vector< int > & target,
        int size,
        bool directed
    );

    void agents_from_adjlist(AdjList al);

    bool is_directed() const;

    std::vector< Agent<TSeq> > & get_agents(); ///< Returns a reference to the vector of agents.

    Agent<TSeq> & get_agent(size_t i);

    std::vector< epiworld_fast_uint > get_agents_states() const; ///< Returns a vector with the states of the agents.

    std::vector< Viruses_const<TSeq> > get_agents_viruses() const; ///< Returns a const vector with the viruses of the agents.

    std::vector< Viruses<TSeq> > get_agents_viruses(); ///< Returns a vector with the viruses of the agents.

    std::vector< Entity<TSeq> > & get_entities();

    Entity<TSeq> & get_entity(size_t entity_id, int * entity_pos = nullptr);

    Model<TSeq> & agents_smallworld(
        epiworld_fast_uint n = 1000,
        epiworld_fast_uint k = 5,
        bool d = false,
        epiworld_double p = .01
        );
    void agents_empty_graph(epiworld_fast_uint n = 1000);
    ///@}

    /**
     * @name Functions to run the model
     * 
     * @param seed Seed to be used for Pseudo-RNG.
     * @param ndays Number of days (steps) of the simulation.
     * @param fun In the case of `run_multiple`, a function that is called
     * after each experiment.
     * 
     */
    ///@{
    void update_state();
    void mutate_virus();
    void next();
    virtual Model<TSeq> & run(
        epiworld_fast_uint ndays,
        int seed = -1
    ); ///< Runs the simulation (after initialization)
    void run_multiple( ///< Multiple runs of the simulation
        epiworld_fast_uint ndays,
        epiworld_fast_uint nexperiments,
        int seed_ = -1,
        std::function<void(size_t,Model<TSeq>*)> fun = make_save_run<TSeq>(),
        bool reset = true,
        bool verbose = true,
        int nthreads = 1
        );
    ///@}

    size_t get_n_viruses() const; ///< Number of viruses in the model
    size_t get_n_tools() const; ///< Number of tools in the model
    epiworld_fast_uint get_ndays() const;
    epiworld_fast_uint get_n_replicates() const;
    void set_ndays(epiworld_fast_uint ndays);
    bool get_verbose() const;
    Model<TSeq> & verbose_off();
    Model<TSeq> & verbose_on();
    int today() const; ///< The current time of the model

    /**
     * @name Rewire the network preserving the degree sequence.
     *
     * @details This implementation assumes an undirected network,
     * thus if {(i,j), (k,l)} -> {(i,l), (k,j)}, the reciprocal
     * is also true, i.e., {(j,i), (l,k)} -> {(j,k), (l,i)}.
     * 
     * @param proportion Proportion of ties to be rewired.
     * 
     * @result A rewired version of the network.
     */
    ///@{
    void set_rewire_fun(std::function<void(std::vector<Agent<TSeq>>*,Model<TSeq>*,epiworld_double)> fun);
    void set_rewire_prop(epiworld_double prop);
    epiworld_double get_rewire_prop() const;
    void rewire();
    ///@}

    /**
     * @brief Wrapper of `DataBase::write_data`
     * 
     * @param fn_virus_info Filename. Information about the virus.
     * @param fn_virus_hist Filename. History of the virus.
     * @param fn_tool_info Filename. Information about the tool.
     * @param fn_tool_hist Filename. History of the tool.
     * @param fn_total_hist   Filename. Aggregated history (state)
     * @param fn_transmission Filename. Transmission history.
     * @param fn_transition   Filename. Markov transition history.
     * @param fn_reproductive_number Filename. Case by case reproductive number
     */
    void write_data(
        std::string fn_virus_info,
        std::string fn_virus_hist,
        std::string fn_tool_info,
        std::string fn_tool_hist,
        std::string fn_total_hist,
        std::string fn_transmission,
        std::string fn_transition,
        std::string fn_reproductive_number,
        std::string fn_generation_time
        ) const;

    /**
     * @name Export the network data in edgelist form
     * 
     * @param fn std::string. File name.
     * @param source Integer vector
     * @param target Integer vector
     * 
     * @details When passing the source and target, the function will
     * write the edgelist on those.
     */
    ///@{
    void write_edgelist(
        std::string fn
        ) const;

    void write_edgelist(
        std::vector< int > & source,
        std::vector< int > & target
        ) const;
    ///@}

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
    virtual void reset();
    const Model<TSeq> & print(bool lite = false) const;

    /**
     * @name Manage state (states) in the model
     * 
     * @details
     * 
     * The functions `get_state` return the current values for the 
     * states included in the model.
     * 
     * @param lab `std::string` Name of the state.
     * 
     * @return `add_state*` returns nothing.
     * @return `get_state_*` returns a vector of pairs with the 
     * states and their labels.
     */
    ///@{
    void add_state(std::string lab, UpdateFun<TSeq> fun = nullptr);
    const std::vector< std::string > & get_states() const;
    const std::vector< UpdateFun<TSeq> > & get_state_fun() const;
    void print_state_codes() const;
    ///@}

    /**
     * @name Initial states
     * 
     * @details These functions are called before the simulation starts.
     * 
     * @param proportions_ Vector of proportions for each state.
     * @param queue_ Vector of queue for each state.
     */
    virtual Model<TSeq> & initial_states(
        std::vector< double > /*proportions_*/,
        std::vector< int > /*queue_*/
    ) {return *this;};

    /**
     * @name Setting and accessing parameters from the model
     * 
     * @details Tools can incorporate parameters included in the model.
     * Internally, parameters in the tool are stored as pointers to
     * an std::map<> of parameters in the model. Using the `epiworld_fast_uint`
     * method directly fetches the parameters in the order these were
     * added to the tool. Accessing parameters via the `std::string` method
     * involves searching the parameter directly in the std::map<> member
     * of the model (so it is not recommended.)
     * 
     * The `par()` function members are aliases for `get_param()`.
     * 
     * In the case of the function `read_params`, users can pass a file
     * listing parameters to be included in the model. Each line in the
     * file should have the following structure:
     * 
     * ```
     * [name of parameter 1]: [value in double]
     * [name of parameter 2]: [value in double]
     * ...
     * ```
     * 
     * The only condition for parameter names is that these do not include
     * a colon.
     * 
     * 
     * @param initial_val 
     * @param pname Name of the parameter to add or to fetch
     * @param fn Path to the file containing parameters
     * @return The current value of the parameter
     * in the model.
     * 
     */
    ///@{
    epiworld_double add_param(
        epiworld_double initial_val, std::string pname, bool overwrite = false
    );
    Model<TSeq> & read_params(std::string fn, bool overwrite = false);
    epiworld_double get_param(epiworld_fast_uint k);
    epiworld_double get_param(std::string pname);
    // void set_param(size_t k, epiworld_double val);
    void set_param(std::string pname, epiworld_double val);
    // epiworld_double par(epiworld_fast_uint k);
    epiworld_double par(std::string pname) const;
    ///@}

    void get_elapsed(
        std::string unit = "auto",
        epiworld_double * last_elapsed = nullptr,
        epiworld_double * total_elapsed = nullptr,
        std::string * unit_abbr = nullptr,
        bool print = true
    ) const;

    /**
     * @name Set the user data object
     * 
     * @param names string vector with the names of the variables.
     */
    ///[@
    void set_user_data(std::vector< std::string > names);
    void add_user_data(epiworld_fast_uint j, epiworld_double x);
    void add_user_data(std::vector< epiworld_double > x);
    UserData<TSeq> & get_user_data();
    ///@}

    /**
     * @brief Set a global action
     * 
     * @param fun A function to be called on the prescribed date
     * @param name Name of the action.
     * @param date Integer indicating when the function is called (see details)
     * 
     * @details When date is less than zero, then the function is called
     * at the end of every day. Otherwise, the function will be called only
     * at the end of the indicated date.
     */
    void add_globalevent(
        std::function<void(Model<TSeq>*)> fun,
        std::string name = "A global action",
        int date = -99
        );

    void add_globalevent(
        GlobalEvent<TSeq> action
    );

    GlobalEvent<TSeq> & get_globalevent(std::string name); ///< Retrieve a global action by name
    GlobalEvent<TSeq> & get_globalevent(size_t i); ///< Retrieve a global action by index

    void rm_globalevent(std::string name); ///< Remove a global action by name
    void rm_globalevent(size_t i); ///< Remove a global action by index

    void run_globalevents();

    void clear_state_set();

    /**
     * @name Queuing system
     * @details When queueing is on, the model will keep track of which agents
     * are either in risk of exposure or exposed. This then is used at each 
     * step to act only on the aforementioned agents.
     * 
     */
    ////@{
    void queuing_on(); ///< Activates the queuing system (default.)
    Model<TSeq> & queuing_off(); ///< Deactivates the queuing system.
    bool is_queuing_on() const; ///< Query if the queuing system is on.
    Queue<TSeq> & get_queue(); ///< Retrieve the `Queue` object.
    ///@}

    /**
     * @name Get the susceptibility reduction object
     * 
     * @param v 
     * @return epiworld_double 
     */
    ///@{
    void set_susceptibility_reduction_mixer(MixerFun<TSeq> fun);
    void set_transmission_reduction_mixer(MixerFun<TSeq> fun);
    void set_recovery_enhancer_mixer(MixerFun<TSeq> fun);
    void set_death_reduction_mixer(MixerFun<TSeq> fun);
    ///@}

    const std::vector< VirusPtr<TSeq> > & get_viruses() const;
    const std::vector< ToolPtr<TSeq> > & get_tools() const;
    Virus<TSeq> & get_virus(size_t id);
    Tool<TSeq> & get_tool(size_t id);

    /**
     * @brief Set the agents data object
     * 
     * @details The data should be an array with the data stored in a
     * column major order, i.e., by column.
     * 
     * @param data_ Pointer to the first element of an array of size
     * `size() * ncols_`.
     * @param ncols_ Number of features included in the data.
     * 
     */
    void set_agents_data(double * data_, size_t ncols_);
    double * get_agents_data();
    size_t get_agents_data_ncols() const;

    /**
     * @brief Set the name object
     * 
     * @param name 
     */
    void set_name(std::string name);
    std::string get_name() const;

    bool operator==(const Model<TSeq> & other) const;
    bool operator!=(const Model<TSeq> & other) const {return !operator==(other);};

    /**
     * @brief Executes the stored action
     * 
     * @param model_ Model over which it will be executed.
     */
    void events_run();

    /**
     * @brief Draws a mermaid diagram of the model.
     * @param model The model to draw.
     * @param fn_output The name of the file to write the diagram.
     * If empty, the diagram will be printed to the standard output.
     * @param self Whether to allow self-transitions.
     */
    void draw(
        const std::string & fn_output = "",
        bool self = false
    );


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

/**
 * @brief Function factory for saving model runs
 * 
 * @details This function is the default behavior of the `run_multiple`
 * member of `Model<TSeq>`. By default only the total history (
 * case counts by unit of time.)
 * 
 * @tparam TSeq 
 * @param fmt 
 * @param total_hist 
 * @param virus_info 
 * @param virus_hist 
 * @param tool_info 
 * @param tool_hist 
 * @param transmission 
 * @param transition 
 * @return std::function<void(size_t,Model<TSeq>*)> 
 */
template<typename TSeq>
inline std::function<void(size_t,Model<TSeq>*)> make_save_run(
    std::string fmt,
    bool total_hist,
    bool virus_info,
    bool virus_hist,
    bool tool_info,
    bool tool_hist,
    bool transmission,
    bool transition,
    bool reproductive,
    bool generation
    )
{

    // Counting number of %
    int n_fmt = 0;
    for (auto & f : fmt)
        if (f == '%')
            n_fmt++;

    if (n_fmt != 1)
        throw std::logic_error("The -fmt- argument must have only one \"%\" symbol.");

    // Listting things to save
    std::vector< bool > what_to_save = {
        virus_info,
        virus_hist,
        tool_info,
        tool_hist,
        total_hist,
        transmission,
        transition,
        reproductive,
        generation
    };

    std::function<void(size_t,Model<TSeq>*)> saver = [fmt,what_to_save](
        size_t niter, Model<TSeq> * m
    ) -> void {

        std::string virus_info = "";
        std::string virus_hist = "";
        std::string tool_info = "";
        std::string tool_hist = "";
        std::string total_hist = "";
        std::string transmission = "";
        std::string transition = "";
        std::string reproductive = "";
        std::string generation = "";

        char buff[1024u];
        if (what_to_save[0u])
        {
            virus_info = fmt + std::string("_virus_info.csv");
            snprintf(buff, sizeof(buff), virus_info.c_str(), niter);
            virus_info = buff;
        } 
        if (what_to_save[1u])
        {
            virus_hist = fmt + std::string("_virus_hist.csv");
            snprintf(buff, sizeof(buff), virus_hist.c_str(), niter);
            virus_hist = buff;
        } 
        if (what_to_save[2u])
        {
            tool_info = fmt + std::string("_tool_info.csv");
            snprintf(buff, sizeof(buff), tool_info.c_str(), niter);
            tool_info = buff;
        } 
        if (what_to_save[3u])
        {
            tool_hist = fmt + std::string("_tool_hist.csv");
            snprintf(buff, sizeof(buff), tool_hist.c_str(), niter);
            tool_hist = buff;
        } 
        if (what_to_save[4u])
        {
            total_hist = fmt + std::string("_total_hist.csv");
            snprintf(buff, sizeof(buff), total_hist.c_str(), niter);
            total_hist = buff;
        } 
        if (what_to_save[5u])
        {
            transmission = fmt + std::string("_transmission.csv");
            snprintf(buff, sizeof(buff), transmission.c_str(), niter);
            transmission = buff;
        } 
        if (what_to_save[6u])
        {
            transition = fmt + std::string("_transition.csv");
            snprintf(buff, sizeof(buff), transition.c_str(), niter);
            transition = buff;
        } 
        if (what_to_save[7u])
        {

            reproductive = fmt + std::string("_reproductive.csv");
            snprintf(buff, sizeof(buff), reproductive.c_str(), niter);
            reproductive = buff;

        }
        if (what_to_save[8u])
        {

            generation = fmt + std::string("_generation.csv");
            snprintf(buff, sizeof(buff), generation.c_str(), niter);
            generation = buff;

        }
        
    
        m->write_data(
            virus_info,
            virus_hist,
            tool_info,
            tool_hist,
            total_hist,
            transmission,
            transition,
            reproductive,
            generation
        );

    };

    return saver;
}


template<typename TSeq>
inline void Model<TSeq>::events_add(
    Agent<TSeq> * agent_,
    VirusPtr<TSeq> virus_,
    ToolPtr<TSeq> tool_,
    Entity<TSeq> * entity_,
    epiworld_fast_int new_state_,
    epiworld_fast_int queue_,
    EventFun<TSeq> call_,
    int idx_agent_,
    int idx_object_
) {

    ++nactions;

    #ifdef EPI_DEBUG
    if (nactions == 0)
        throw std::logic_error("Events cannot be zero!!");
    #endif

    if (nactions > events.size())
    {

        events.emplace_back(
            Event<TSeq>(
                agent_, virus_, tool_, entity_, new_state_, queue_, call_,
                idx_agent_, idx_object_
            ));

    }
    else 
    {

        Event<TSeq> & A = events.at(nactions - 1u);

        A.agent      = agent_;
        A.virus      = virus_;
        A.tool       = tool_;
        A.entity     = entity_;
        A.new_state  = new_state_;
        A.queue      = queue_;
        A.call       = call_;
        A.idx_agent  = idx_agent_;
        A.idx_object = idx_object_;

    }

    return;

}

template<typename TSeq>
inline void Model<TSeq>::events_run()
{
    // Making the call
    size_t nevents_tmp = 0;
    while (nevents_tmp < nactions)
    {

        Event<TSeq> & a = events[nevents_tmp++];
        Agent<TSeq> * p  = a.agent;

        #ifdef EPI_DEBUG
        if (a.new_state >= static_cast<epiworld_fast_int>(nstates))
            throw std::range_error(
                "The proposed state " + std::to_string(a.new_state) + " is out of range. " +
                "The model currently has " + std::to_string(nstates - 1) + " states.");

        if (a.new_state < 0)
            throw std::range_error(
                "The proposed state " + std::to_string(a.new_state) + " is out of range. " +
                "The state cannot be negative.");
        #endif

        // Undoing the change in the transition matrix
        if ((p->state_last_changed == today()) && (static_cast<int>(p->state) != a.new_state))
        {
            // Undoing state change in the transition matrix
            // The previous state is already recorded
            db.update_state(p->state_prev, p->state, true);

        } else 
            p->state_prev = p->state; // Recording the previous state

        // Applying function after the fact. This way, if there were
        // updates, they can be recorded properly, before losing the information
        p->state = a.new_state;
        if (a.call)
        {
            a.call(a, this);
        }

        // Registering that the last change was today
        p->state_last_changed = today();

        #ifdef EPI_DEBUG
        if (static_cast<int>(p->state) >= static_cast<int>(nstates))
                throw std::range_error(
                    "The new state " + std::to_string(p->state) + " is out of range. " +
                    "The model currently has " + std::to_string(nstates - 1) + " states.");
        #endif

        // Updating queue
        if (use_queuing)
        {

            if (a.queue == Queue<TSeq>::Everyone)
                queue += p;
            else if (a.queue == -Queue<TSeq>::Everyone)
                queue -= p;
            else if (a.queue == Queue<TSeq>::OnlySelf)
                queue[p->get_id()]++;
            else if (a.queue == -Queue<TSeq>::OnlySelf)
                queue[p->get_id()]--;
            else if (a.queue != Queue<TSeq>::NoOne)
                throw std::logic_error(
                    "The proposed queue change is not valid. Queue values can be {-2, -1, 0, 1, 2}."
                    );
                    
        }

    }

    // Go back to square 1
    nactions = 0u;

    return;
    
}

/**
 * @name Default function for combining susceptibility_reduction levels
 * 
 * @tparam TSeq 
 * @param pt 
 * @return epiworld_double 
 */
///@{
template<typename TSeq>
inline epiworld_double susceptibility_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq> * m
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
        total *= (1.0 - tool->get_susceptibility_reduction(v, m));

    return 1.0 - total;
    
}

template<typename TSeq>
inline epiworld_double transmission_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
        total *= (1.0 - tool->get_transmission_reduction(v, m));

    return (1.0 - total);
    
}

template<typename TSeq>
inline epiworld_double recovery_enhancer_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
        total *= (1.0 - tool->get_recovery_enhancer(v, m));

    return 1.0 - total;
    
}

template<typename TSeq>
inline epiworld_double death_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
) {

    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
    {
        total *= (1.0 - tool->get_death_reduction(v, m));
    } 

    return 1.0 - total;
    
}
///@}

template<typename TSeq>
inline Model<TSeq> * Model<TSeq>::clone_ptr()
{
    Model<TSeq> * ptr = new Model<TSeq>(*dynamic_cast<const Model<TSeq>*>(this));

    #ifdef EPI_DEBUG
    if (*this != *ptr)
        throw std::logic_error("Model::clone_ptr The copies of the model don't match.");
    #endif

    return ptr;
}

template<typename TSeq>
inline Model<TSeq>::Model()
{
    db.model = this;
    db.user_data = this;
    if (use_queuing)
        queue.model = this;
}

template<typename TSeq>
inline Model<TSeq>::Model(const Model<TSeq> & model) :
    name(model.name),
    db(model.db),
    population(model.population),
    population_backup(model.population_backup),
    directed(model.directed),
    viruses(model.viruses),
    tools(model.tools),
    entities(model.entities),
    entities_backup(model.entities_backup),
    rewire_fun(model.rewire_fun),
    rewire_prop(model.rewire_prop),
    parameters(model.parameters),
    ndays(model.ndays),
    pb(model.pb),
    state_fun(model.state_fun),
    states_labels(model.states_labels),
    initial_states_fun(model.initial_states_fun),
    nstates(model.nstates),
    verbose(model.verbose),
    current_date(model.current_date),
    globalevents(model.globalevents),
    queue(model.queue),
    use_queuing(model.use_queuing),
    array_double_tmp(model.array_double_tmp.size()),
    array_virus_tmp(model.array_virus_tmp.size()),
    array_int_tmp(model.array_int_tmp.size())
{


    // Removing old neighbors
    for (auto & p : population)
        p.model = this;

    if (population_backup.size() != 0u)
        for (auto & p : population_backup)
            p.model = this;

    // Pointing to the right place. This needs
    // to be done afterwards since the state zero is set as a function
    // of the population.
    db.model = this;
    db.user_data.model = this;

    if (use_queuing)
        queue.model = this;

    agents_data = model.agents_data;
    agents_data_ncols = model.agents_data_ncols;

}

template<typename TSeq>
inline Model<TSeq>::Model(Model<TSeq> & model) :
    Model(dynamic_cast< const Model<TSeq> & >(model)) {}

template<typename TSeq>
inline Model<TSeq>::Model(Model<TSeq> && model) :
    name(std::move(model.name)),
    db(std::move(model.db)),
    population(std::move(model.population)),
    agents_data(std::move(model.agents_data)),
    agents_data_ncols(std::move(model.agents_data_ncols)),
    directed(std::move(model.directed)),
    // Virus
    viruses(std::move(model.viruses)),
    // Tools
    tools(std::move(model.tools)),
    // Entities
    entities(std::move(model.entities)),
    entities_backup(std::move(model.entities_backup)),
    // Pseudo-RNG
    engine(std::move(model.engine)),
    runifd(std::move(model.runifd)),
    rnormd(std::move(model.rnormd)),
    rgammad(std::move(model.rgammad)),
    rlognormald(std::move(model.rlognormald)),
    rexpd(std::move(model.rexpd)),
    // Rewiring
    rewire_fun(std::move(model.rewire_fun)),
    rewire_prop(std::move(model.rewire_prop)),
    parameters(std::move(model.parameters)),
    // Others
    ndays(model.ndays),
    pb(std::move(model.pb)),
    state_fun(std::move(model.state_fun)),
    states_labels(std::move(model.states_labels)),
    initial_states_fun(std::move(model.initial_states_fun)),
    nstates(model.nstates),
    verbose(model.verbose),
    current_date(std::move(model.current_date)),
    globalevents(std::move(model.globalevents)),
    queue(std::move(model.queue)),
    use_queuing(model.use_queuing),
    array_double_tmp(model.array_double_tmp.size()),
    array_virus_tmp(model.array_virus_tmp.size()),
    array_int_tmp(model.array_int_tmp.size())
{

    db.model = this;
    db.user_data.model = this;

    if (use_queuing)
        queue.model = this;

}

template<typename TSeq>
inline Model<TSeq> & Model<TSeq>::operator=(const Model<TSeq> & m)
{
    name = m.name;

    population        = m.population;
    population_backup = m.population_backup;

    for (auto & p : population)
        p.model = this;

    if (population_backup.size() != 0)
        for (auto & p : population_backup)
            p.model = this;

    db = m.db;
    db.model = this;
    db.user_data.model = this;

    directed = m.directed;
    
    viruses                        = m.viruses;

    tools                         = m.tools;
    
    entities        = m.entities;
    entities_backup = m.entities_backup;
    
    rewire_fun  = m.rewire_fun;
    rewire_prop = m.rewire_prop;

    parameters = m.parameters;
    ndays      = m.ndays;
    pb         = m.pb;

    state_fun    = m.state_fun;
    states_labels = m.states_labels;
    initial_states_fun = m.initial_states_fun;
    nstates       = m.nstates;

    verbose     = m.verbose;

    current_date = m.current_date;

    globalevents = m.globalevents;

    queue       = m.queue;
    use_queuing = m.use_queuing;

    // Making sure population is passed correctly
    // Pointing to the right place
    db.model = this;
    db.user_data.model = this;

    agents_data            = m.agents_data;
    agents_data_ncols = m.agents_data_ncols;

    // Figure out the queuing
    if (use_queuing)
        queue.model = this;

    array_double_tmp.resize(std::max(
        size(),
        static_cast<size_t>(1024 * 1024)
    ));

    array_virus_tmp.resize(1024u);
    array_int_tmp.resize(1024u * 1024);

    return *this;

}

template<typename TSeq>
inline DataBase<TSeq> & Model<TSeq>::get_db()
{
    return db;
}

template<typename TSeq>
inline const DataBase<TSeq> & Model<TSeq>::get_db() const
{
    return db;
}


template<typename TSeq>
inline std::vector<Agent<TSeq>> & Model<TSeq>::get_agents()
{
    return population;
}

template<typename TSeq>
inline Agent<TSeq> & Model<TSeq>::get_agent(size_t i)
{
    return population[i];
}

template<typename TSeq>
inline std::vector< epiworld_fast_uint > Model<TSeq>::get_agents_states() const
{
    std::vector< epiworld_fast_uint > states(population.size());
    for (size_t i = 0u; i < population.size(); ++i)
        states[i] = population[i].get_state();

    return states;
}

template<typename TSeq>
inline std::vector< Viruses_const<TSeq> > Model<TSeq>::get_agents_viruses() const
{

    std::vector< Viruses_const<TSeq> > viruses(population.size());
    for (size_t i = 0u; i < population.size(); ++i)
        viruses[i] = population[i].get_virus();

    return viruses;

}

// Same as before, but the non const version
template<typename TSeq>
inline std::vector< Viruses<TSeq> > Model<TSeq>::get_agents_viruses()
{

    std::vector< Viruses<TSeq> > viruses(population.size());
    for (size_t i = 0u; i < population.size(); ++i)
        viruses[i] = population[i].get_virus();

    return viruses;

}

template<typename TSeq>
inline std::vector<Entity<TSeq>> & Model<TSeq>::get_entities()
{
    return entities;
}

template<typename TSeq>
inline Entity<TSeq> & Model<TSeq>::get_entity(size_t i, int * entity_pos)
{
    
    for (size_t j = 0u; j < entities.size(); ++j)
        if (entities[j].get_id() == static_cast<int>(i))
        {

            if (entity_pos)
                *entity_pos = j;

            return entities[j];

        }

    throw std::range_error("The entity with id " + std::to_string(i) + " was not found.");

}

template<typename TSeq>
inline Model<TSeq> & Model<TSeq>::agents_smallworld(
    epiworld_fast_uint n,
    epiworld_fast_uint k,
    bool d,
    epiworld_double p
)
{
    agents_from_adjlist(
        rgraph_smallworld(n, k, p, d, *this)
    );

    return *this;
}

template<typename TSeq>
inline void Model<TSeq>::agents_empty_graph(
    epiworld_fast_uint n
) 
{

    // Resizing the people
    population.clear();
    population.resize(n, Agent<TSeq>());

    // Filling the model and ids
    size_t i = 0u;
    for (auto & p : population)
    {
        p.id = i++;
        p.model = this;
    }
    

}

template<typename TSeq>
inline void Model<TSeq>::set_rand_gamma(epiworld_double alpha, epiworld_double beta)
{
    rgammad = std::gamma_distribution<>(alpha,beta);
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_norm(epiworld_double mean, epiworld_double sd)
{ 
    rnormd  = std::normal_distribution<>(mean, sd);
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_unif(epiworld_double a, epiworld_double b)
{ 
    runifd  = std::uniform_real_distribution<>(a, b);
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_lognormal(epiworld_double mean, epiworld_double shape)
{ 
    rlognormald  = std::lognormal_distribution<>(mean, shape);
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_exp(epiworld_double lambda)
{ 
    rexpd  = std::exponential_distribution<>(lambda);
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_binom(int n, epiworld_double p)
{ 
    rbinomd  = std::binomial_distribution<>(n, p);
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_nbinom(int n, epiworld_double p)
{ 
    rnbinomd  = std::negative_binomial_distribution<>(n, p);
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_geom(epiworld_double p)
{ 
    rgeomd  = std::geometric_distribution<>(p);
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_poiss(epiworld_double lambda)
{ 
    rpoissd  = std::poisson_distribution<>(lambda);
}

template<typename TSeq>
inline epiworld_double & Model<TSeq>::operator()(std::string pname) {

    if (parameters.find(pname) == parameters.end())
        throw std::range_error("The parameter '"+ pname + "' is not in the model.");

    return parameters[pname];

}

template<typename TSeq>
inline size_t Model<TSeq>::size() const {
    return population.size();
}

template<typename TSeq>
inline void Model<TSeq>::dist_virus()
{

    for (auto & v: viruses)
    {

        v->distribute(this);

        // Apply the events
        events_run();
    }

}

template<typename TSeq>
inline void Model<TSeq>::dist_tools()
{

    for (auto & tool: tools)
    {

        tool->distribute(this);

        // Apply the events
        events_run();

    }

}

template<typename TSeq>
inline void Model<TSeq>::dist_entities()
{

    for (auto & entity: entities)
    {

        entity.distribute(this);

        // Apply the events
        events_run();

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
    n_replicates++;
}

template<typename TSeq>
inline void Model<TSeq>::set_backup()
{

    if (population_backup.size() == 0u)
        population_backup = population;

    if (entities_backup.size() == 0u)
        entities_backup = entities;

}

// template<typename TSeq>
// inline void Model<TSeq>::restore_backup()
// {

//     // Restoring the data
//     population = *population_backup;
//     entities   = *entities_backup;

//     // And correcting the pointer
//     for (auto & p : population)
//         p.model = this;

//     for (auto & e : entities)
//         e.model = this;

// }

template<typename TSeq>
inline std::shared_ptr< std::mt19937 > & Model<TSeq>::get_rand_endgine()
{
    return engine;
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::runif() {
    // CHECK_INIT()
    return runifd(*engine);
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::runif(epiworld_double a, epiworld_double b) {
    // CHECK_INIT()
    return runifd(*engine) * (b - a) + a;
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rnorm() {
    // CHECK_INIT()
    return rnormd(*engine);
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rnorm(epiworld_double mean, epiworld_double sd) {
    // CHECK_INIT()
    return rnormd(*engine) * sd + mean;
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rgamma() {
    return rgammad(*engine);
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rgamma(epiworld_double alpha, epiworld_double beta) {
    auto old_param = rgammad.param();
    rgammad.param(std::gamma_distribution<>::param_type(alpha, beta));
    epiworld_double ans = rgammad(*engine);
    rgammad.param(old_param);
    return ans;
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rexp() {
    return rexpd(*engine);
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rexp(epiworld_double lambda) {
    auto old_param = rexpd.param();
    rexpd.param(std::exponential_distribution<>::param_type(lambda));
    epiworld_double ans = rexpd(*engine);
    rexpd.param(old_param);
    return ans;
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rlognormal() {
    return rlognormald(*engine);
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rlognormal(epiworld_double mean, epiworld_double shape) {
    auto old_param = rlognormald.param();
    rlognormald.param(std::lognormal_distribution<>::param_type(mean, shape));
    epiworld_double ans = rlognormald(*engine);
    rlognormald.param(old_param);
    return ans;
}

template<typename TSeq>
inline int Model<TSeq>::rbinom() {
    return rbinomd(*engine);
}

template<typename TSeq>
inline int Model<TSeq>::rbinom(int n, epiworld_double p) {
    auto old_param = rbinomd.param();
    rbinomd.param(std::binomial_distribution<>::param_type(n, p));
    epiworld_double ans = rbinomd(*engine);
    rbinomd.param(old_param);
    return ans;
}

template<typename TSeq>
inline int Model<TSeq>::rnbinom() {
    return rnbinomd(*engine);
}

template<typename TSeq>
inline int Model<TSeq>::rnbinom(int n, epiworld_double p) {
    auto old_param = rnbinomd.param();
    rnbinomd.param(std::negative_binomial_distribution<>::param_type(n, p));
    int ans = rnbinomd(*engine);
    rnbinomd.param(old_param);
    return ans;
}

template<typename TSeq>
inline int Model<TSeq>::rgeom() {
    return rgeomd(*engine);
}

template<typename TSeq>
inline int Model<TSeq>::rgeom(epiworld_double p) {
    auto old_param = rgeomd.param();
    rgeomd.param(std::geometric_distribution<>::param_type(p));
    int ans = rgeomd(*engine);
    rgeomd.param(old_param);
    return ans;
}

template<typename TSeq>
inline int Model<TSeq>::rpoiss() {
    return rpoissd(*engine);
}

template<typename TSeq>
inline int Model<TSeq>::rpoiss(epiworld_double lambda) {
    auto old_param = rpoissd.param();
    rpoissd.param(std::poisson_distribution<>::param_type(lambda));
    int ans = rpoissd(*engine);
    rpoissd.param(old_param);
    return ans;
}

template<typename TSeq>
inline void Model<TSeq>::seed(size_t s) {
    this->engine->seed(s);
}

template<typename TSeq>
inline void Model<TSeq>::add_virus(
    Virus<TSeq> & v
    )
{

    // Checking the state
    epiworld_fast_int init_, post_, rm_;
    v.get_state(&init_, &post_, &rm_);

    if (init_ == -99)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no -init- state."
            );
    else if (post_ == -99)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no -post- state."
            );
    
    // Recording the variant
    db.record_virus(v);

    // Adding new virus
    viruses.push_back(std::make_shared< Virus<TSeq> >(v));

}

template<typename TSeq>
inline void Model<TSeq>::add_tool(Tool<TSeq> & t)
{

    
    db.record_tool(t);

    // Adding the tool to the model (and database.)
    tools.push_back(std::make_shared< Tool<TSeq> >(t));

}

template<typename TSeq>
inline void Model<TSeq>::add_entity(Entity<TSeq> e)
{

    e.id = entities.size();
    entities.push_back(e);

}

template<typename TSeq>
inline void Model<TSeq>::rm_entity(size_t entity_id)
{

    int entity_pos = 0;
    auto & entity = this->get_entity(entity_id, &entity_pos);

    // First, resetting the entity
    entity.reset();

    // How should
    if (entity_pos != (static_cast<int>(entities.size()) - 1))
        std::swap(entities[entity_pos], entities[entities.size() - 1]);

    entities.pop_back();
}

template<typename TSeq>
inline void Model<TSeq>::rm_virus(size_t virus_pos)
{

    if (viruses.size() <= virus_pos)
        throw std::range_error(
            std::string("The specified virus (") +
            std::to_string(virus_pos) +
            std::string(") is out of range. ") +
            std::string("There are only ") +
            std::to_string(viruses.size()) +
            std::string(" viruses.")
            );

    // Flipping with the last one
    std::swap(viruses[virus_pos], viruses[viruses.size() - 1]);
    viruses.pop_back();

    return;

}

template<typename TSeq>
inline void Model<TSeq>::rm_tool(size_t tool_pos)
{

    if (tools.size() <= tool_pos)
        throw std::range_error(
            std::string("The specified tool (") +
            std::to_string(tool_pos) +
            std::string(") is out of range. ") +
            std::string("There are only ") +
            std::to_string(tools.size()) +
            std::string(" tools.")
            );

    // Flipping with the last one
    std::swap(tools[tool_pos], tools[tools.size() - 1]);
    
    /* There's an error on windows:
    https://github.com/UofUEpiBio/epiworldR/actions/runs/4801482395/jobs/8543744180#step:6:84

    More clear here:
    https://stackoverflow.com/questions/58660207/why-doesnt-stdswap-work-on-vectorbool-elements-under-clang-win
    */

    tools.pop_back();

    return;

}

template<typename TSeq>
inline void Model<TSeq>::load_agents_entities_ties(
    std::string fn,
    int skip
    )
{

    int i,j;
    std::ifstream filei(fn);

    if (!filei)
        throw std::logic_error("The file " + fn + " was not found.");

    int linenum = 0;
    std::vector< std::vector< epiworld_fast_uint > > target_(entities.size());

    target_.reserve(1e5);

    while (!filei.eof())
    {

        if (linenum++ < skip)
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

        if (i >= static_cast<int>(this->size()))
            throw std::range_error(
                "The agent["+std::to_string(linenum)+"] = " + std::to_string(i) +
                " is above the max id " + std::to_string(this->size() - 1)
                );

        if (j >= static_cast<int>(this->entities.size()))
            throw std::range_error(
                "The entity["+std::to_string(linenum)+"] = " + std::to_string(j) +
                " is above the max id " + std::to_string(this->entities.size() - 1)
                );

        target_[j].push_back(i);

        population[i].add_entity(entities[j], nullptr);

    }

    return;

}

template<typename TSeq>
inline void Model<TSeq>::load_agents_entities_ties(
    const std::vector< int > & agents_ids,
    const std::vector< int > & entities_ids
) {

    // Checking the size
    if (agents_ids.size() != entities_ids.size())
        throw std::length_error(
            std::string("The size of agents_ids (") +
            std::to_string(agents_ids.size()) +
            std::string(") and entities_ids (") +
            std::to_string(entities_ids.size()) +
            std::string(") must be the same.")
            );

    return this->load_agents_entities_ties(
        agents_ids.data(),
        entities_ids.data(),
        agents_ids.size()
    );

}

template<typename TSeq>
inline void Model<TSeq>::load_agents_entities_ties(
    const int * agents_ids,
    const int * entities_ids,
    size_t n
) {

    auto get_agent = [agents_ids](int i) -> int {
        return *(agents_ids + i);
        };

    auto get_entity = [entities_ids](int i) -> int {
        return *(entities_ids + i);
        };

    for (size_t i = 0u; i < n; ++i)
    {

        if (get_agent(i) < 0)
            throw std::length_error(
                std::string("agents_ids[") +
                std::to_string(i) +
                std::string("] = ") +
                std::to_string(get_agent(i)) +
                std::string(" is negative.")
                );

        if (get_entity(i) < 0)
            throw std::length_error(
                std::string("entities_ids[") +
                std::to_string(i) +
                std::string("] = ") +
                std::to_string(get_entity(i)) +
                std::string(" is negative.")
                );

        int pop_size = static_cast<int>(this->population.size());
        if (get_agent(i) >= pop_size)
            throw std::length_error(
                std::string("agents_ids[") +
                std::to_string(i) +
                std::string("] = ") +
                std::to_string(get_agent(i)) +
                std::string(" is out of range (population size: ") +
                std::to_string(pop_size) +
                std::string(").")
                );

        int ent_size = static_cast<int>(this->entities.size());
        if (get_entity(i) >= ent_size)
            throw std::length_error(
                std::string("entities_ids[") +
                std::to_string(i) +
                std::string("] = ") +
                std::to_string(get_entity(i)) +
                std::string(" is out of range (entities size: ") +
                std::to_string(ent_size) +
                std::string(").")
                );

        // Adding the entity to the agent
        this->population[get_agent(i)].add_entity(
            this->entities[get_entity(i)],
            nullptr /* Immediately add it to the agent */
        );

    }

    return;


}

template<typename TSeq>
inline void Model<TSeq>::agents_from_adjlist(
    std::string fn,
    int size,
    int skip,
    bool directed
    ) {

    AdjList al;
    al.read_edgelist(fn, size, skip, directed);
    this->agents_from_adjlist(al);

}

template<typename TSeq>
inline void Model<TSeq>::agents_from_edgelist(
    const std::vector< int > & source,
    const std::vector< int > & target,
    int size,
    bool directed
) {

    AdjList al(source, target, size, directed);
    agents_from_adjlist(al);

}

template<typename TSeq>
inline void Model<TSeq>::agents_from_adjlist(AdjList al) {

    // Resizing the people
    agents_empty_graph(al.vcount());
    
    const auto & tmpdat = al.get_dat();
    
    for (size_t i = 0u; i < tmpdat.size(); ++i)
    {

        // population[i].id    = i;
        population[i].model = this;

        for (const auto & link: tmpdat[i])
        {

            population[i].add_neighbor(
                population[link.first],
                true, true
                );

        }

    }

    #ifdef EPI_DEBUG
    for (auto & p: population)
    {
        if (p.id >= static_cast<int>(al.vcount()))
            throw std::logic_error(
                "Agent's id cannot be negative above or equal to the number of agents!");
    }
    #endif

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

    if (ndays == 0)
      return 0;

    return this->current_date;
}

template<typename TSeq>
inline void Model<TSeq>::next() {

    db.record();
    ++this->current_date;
    
    // Advancing the progress bar
    if ((this->current_date >= 1) && verbose)
        pb.next();

    return ;
}

template<typename TSeq>
inline Model<TSeq> & Model<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
) 
{

    if (size() == 0u)
        throw std::logic_error("There are no agents in this model!");

    if (nstates == 0u)
        throw std::logic_error(
            std::string("No states registered in this model. ") +
            std::string("At least one state should be included. See the function -Model::add_state()-")
            );

    // Setting up the number of steps
    this->ndays = ndays;

    if (seed >= 0)
        engine->seed(seed);

    array_double_tmp.resize(std::max(
        size(),
        static_cast<size_t>(1024 * 1024)
    ));


    array_virus_tmp.resize(1024);
    array_int_tmp.resize(1024 * 1024);

    // Checking whether the proposed state in/out/removed
    // are valid
    epiworld_fast_int _init, _end, _removed;
    int nstate_int = static_cast<int>(nstates);
    for (auto & v : viruses)
    {
        v->get_state(&_init, &_end, &_removed);
        
        // Negative unspecified state
        if (((_init != -99) && (_init < 0)) || (_init >= nstate_int))
            throw std::range_error("States must be between 0 and " +
                std::to_string(nstates - 1));

        // Negative unspecified state
        if (((_end != -99) && (_end < 0)) || (_end >= nstate_int))
            throw std::range_error("States must be between 0 and " +
                std::to_string(nstates - 1));

        if (((_removed != -99) && (_removed < 0)) || (_removed >= nstate_int))
            throw std::range_error("States must be between 0 and " +
                std::to_string(nstates - 1));

    }

    for (auto & t : tools)
    {
        t->get_state(&_init, &_end);
        
        // Negative unspecified state
        if (((_init != -99) && (_init < 0)) || (_init >= nstate_int))
            throw std::range_error("States must be between 0 and " +
                std::to_string(nstates - 1));

        // Negative unspecified state
        if (((_end != -99) && (_end < 0)) || (_end >= nstate_int))
            throw std::range_error("States must be between 0 and " +
                std::to_string(nstates - 1));

    }

    // Starting first infection and tools
    reset();

    // Initializing the simulation
    chrono_start();
    EPIWORLD_RUN((*this))
    {

        #ifdef EPI_DEBUG
        db.n_transmissions_potential = 0;
        db.n_transmissions_today = 0;
        #endif

        // We can execute these components in whatever order the
        // user needs.
        this->update_state();
    
        // We start with the Global events
        this->run_globalevents();

        // In this case we are applying degree sequence rewiring
        // to change the network just a bit.
        this->rewire();

        // This locks all the changes
        this->next();

        // Mutation must happen at the very end of all
        this->mutate_virus();

    }

    // The last reaches the end...
    this->current_date--;

    chrono_end();

    return *this;

}

template<typename TSeq>
inline void Model<TSeq>::run_multiple(
    epiworld_fast_uint ndays,
    epiworld_fast_uint nexperiments,
    int seed_,
    std::function<void(size_t,Model<TSeq>*)> fun,
    bool reset,
    bool verbose,
    #ifdef _OPENMP
    int nthreads
    #else
    int
    #endif
)
{

    if (seed_ >= 0)
        this->seed(seed_);

    // Seeds will be reproducible by default
    std::vector< int > seeds_n(nexperiments);
    for (auto & s : seeds_n)
    {
        s = static_cast<int>(
            std::floor(
                runif() * static_cast<double>(std::numeric_limits<int>::max())
                )
        );
    }
    // #endif

    EPI_DEBUG_NOTIFY_ACTIVE()

    bool old_verb = this->verbose;
    verbose_off();

    // Setting up backup
    if (reset)
        set_backup();

    #ifdef _OPENMP

    omp_set_num_threads(nthreads);

    // Generating copies of the model
    std::vector< Model<TSeq> * > these;
    for (size_t i = 0; i < static_cast<size_t>(std::max(nthreads - 1, 0)); ++i)
        these.push_back(clone_ptr());

    // Figuring out how many replicates
    std::vector< size_t > nreplicates(nthreads, 0);
    std::vector< size_t > nreplicates_csum(nthreads, 0);
    size_t sums = 0u;
    for (int i = 0; i < nthreads; ++i)
    {
        nreplicates[i] = static_cast<epiworld_fast_uint>(
            std::floor(nexperiments/nthreads)
            );
        
        // This takes the cumsum
        nreplicates_csum[i] = sums;

        sums += nreplicates[i];

    }

    if (sums < nexperiments)
        nreplicates[nthreads - 1] += (nexperiments - sums);

    Progress pb_multiple(
        nreplicates[0u],
        EPIWORLD_PROGRESS_BAR_WIDTH
        );

    if (verbose)
    {

        printf_epiworld(
            "Starting multiple runs (%i) using %i thread(s)\n", 
            static_cast<int>(nexperiments),
            static_cast<int>(nthreads)
        );

        pb_multiple.start();

    }

    #ifdef EPI_DEBUG
    // Checking the initial state of all the models. Throw an
    // exception if they are not the same.
    for (size_t i = 1; i < static_cast<size_t>(std::max(nthreads - 1, 0)); ++i)
    {

        if (db != these[i]->db)
        {
            throw std::runtime_error(
                "The initial state of the models is not the same"
            );
        }
    }
    #endif

    #pragma omp parallel shared(these, nreplicates, nreplicates_csum, seeds_n) \
        firstprivate(nexperiments, nthreads, fun, reset, verbose, pb_multiple, ndays) \
        default(shared)
    {

        auto iam = omp_get_thread_num();

        for (size_t n = 0u; n < nreplicates[iam]; ++n)
        {
            size_t sim_id = nreplicates_csum[iam] + n;
            if (iam == 0)
            {

                // Initializing the seed
                run(ndays, seeds_n[sim_id]);

                if (fun)
                    fun(n, this);

                // Only the first one prints
                if (verbose)
                    pb_multiple.next();

            } else {

                // Initializing the seed
                these[iam - 1]->run(ndays, seeds_n[sim_id]);

                if (fun)
                    fun(sim_id, these[iam - 1]);

            }

        }
        
    }

    // Adjusting the number of replicates
    n_replicates += (nexperiments - nreplicates[0u]);

    for (auto & ptr : these)
        delete ptr;

    #else
    // if (reset)
    //     set_backup();

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

    for (size_t n = 0u; n < nexperiments; ++n)
    {

        run(ndays, seeds_n[n]);

        if (fun)
            fun(n, this);

        if (verbose)
            pb_multiple.next();
    
    }
    #endif

    if (verbose)
        pb_multiple.end();

    if (old_verb)
        verbose_on();

    return;

}

template<typename TSeq>
inline void Model<TSeq>::update_state() {

    // Next state
    if (use_queuing)
    {
        int i = -1;
        for (auto & p: population)
            if (queue[++i] > 0)
            {
                if (state_fun[p.state])
                    state_fun[p.state](&p, this);
            }

    }
    else
    {

        for (auto & p: population)
            if (state_fun[p.state])
                    state_fun[p.state](&p, this);

    }

    events_run();
    
}



template<typename TSeq>
inline void Model<TSeq>::mutate_virus() {

    // Checking if any virus has mutation
    size_t nmutates = 0u;
    for (const auto & v: viruses)
        if (v->mutation_fun)
            nmutates++;

    if (nmutates == 0u)
        return;

    if (use_queuing)
    {

        int i = -1;
        for (auto & p: population)
        {

            if (queue[++i] == 0)
                continue;

            if (p.virus != nullptr)
                p.virus->mutate(this);

        }

    }
    else 
    {

        for (auto & p: population)
        {

            if (p.virus != nullptr)
                p.virus->mutate(this);

        }

    }
    

}

template<typename TSeq>
inline size_t Model<TSeq>::get_n_viruses() const {
    return db.size();
}

template<typename TSeq>
inline size_t Model<TSeq>::get_n_tools() const {
    return tools.size();
}

template<typename TSeq>
inline epiworld_fast_uint Model<TSeq>::get_ndays() const {
    return ndays;
}

template<typename TSeq>
inline epiworld_fast_uint Model<TSeq>::get_n_replicates() const
{
    return n_replicates;
}

template<typename TSeq>
inline void Model<TSeq>::set_ndays(epiworld_fast_uint ndays) {
    this->ndays = ndays;
}

template<typename TSeq>
inline bool Model<TSeq>::get_verbose() const {
    return verbose;
}

template<typename TSeq>
inline Model<TSeq> & Model<TSeq>::verbose_on() {
    verbose = true;
    return *this;
}

template<typename TSeq>
inline Model<TSeq> & Model<TSeq>::verbose_off() {
    verbose = false;
    return *this;
}

template<typename TSeq>
inline void Model<TSeq>::set_rewire_fun(
    std::function<void(std::vector<Agent<TSeq>>*,Model<TSeq>*,epiworld_double)> fun
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
    std::string fn_virus_info,
    std::string fn_virus_hist,
    std::string fn_tool_info,
    std::string fn_tool_hist,
    std::string fn_total_hist,
    std::string fn_transmission,
    std::string fn_transition,
    std::string fn_reproductive_number,
    std::string fn_generation_time
    ) const
{

    db.write_data(
        fn_virus_info, fn_virus_hist,
        fn_tool_info, fn_tool_hist,
        fn_total_hist, fn_transmission, fn_transition,
        fn_reproductive_number, fn_generation_time
        );

}

template<typename TSeq>
inline void Model<TSeq>::write_edgelist(
    std::string fn
    ) const
{

    // Figuring out the writing sequence
    std::vector< const Agent<TSeq> * > wseq(size());
    for (const auto & p: population)
        wseq[p.id] = &p;

    std::ofstream efile(fn, std::ios_base::out);
    efile << "source target\n";
    if (this->is_directed())
    {

        for (const auto & p : wseq)
        {
            for (auto & n : p->neighbors)
                efile << p->id << " " << n << "\n";
        }

    } else {

        for (const auto & p : wseq)
        {
            for (auto & n : p->neighbors)
                if (static_cast<int>(p->id) <= static_cast<int>(n))
                    efile << p->id << " " << n << "\n";
        }

    }

}

template<typename TSeq>
inline void Model<TSeq>::write_edgelist(
std::vector< int > & source,
std::vector< int > & target
) const {

    // Figuring out the writing sequence
    std::vector< const Agent<TSeq> * > wseq(size());
    for (const auto & p: population)
        wseq[p.id] = &p;

    if (this->is_directed())
    {

        for (const auto & p : wseq)
        {
            for (auto & n : p->neighbors)
            {
                source.push_back(static_cast<int>(p->id));
                target.push_back(static_cast<int>(n));
            }
        }

    } else {

        for (const auto & p : wseq)
        {
            for (auto & n : p->neighbors) {
                if (static_cast<int>(p->id) <= static_cast<int>(n)) {
                    source.push_back(static_cast<int>(p->id));
                    target.push_back(static_cast<int>(n));
                }
            }
        }

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

    if (population_backup.size() != 0u)
    {
        population = population_backup;

        #ifdef EPI_DEBUG
        for (size_t i = 0; i < population.size(); ++i)
        {

            if (population[i] != population_backup[i])
                throw std::logic_error("Model::reset population doesn't match.");

        }
        #endif

    }

    for (auto & p : population)
        p.reset();

    #ifdef EPI_DEBUG
    for (auto & a: population)
    {
        if (a.get_state() != 0u)
            throw std::logic_error("Model::reset population doesn't match."
                "Some agents are not in the baseline state.");
    }
    #endif
        
    if (entities_backup.size() != 0)
    {
        entities = entities_backup;

        #ifdef EPI_DEBUG
        for (size_t i = 0; i < entities.size(); ++i)
        {

            if (entities[i] != entities_backup[i])
                throw std::logic_error("Model::reset entities don't match.");

        }
        #endif
        
    }

    for (auto & e: entities)
        e.reset();
    
    current_date = 0;

    db.reset();

    // This also clears the queue
    if (use_queuing)
        queue.reset();

    // Re distributing tools and virus
    dist_virus();
    dist_tools();
    dist_entities();

    // Distributing initial state, if specified
    initial_states_fun(this);

    // Recording the original state (at time 0) and advancing
    // to time 1
    next();


}

// Too big to keep here
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//model-meat-print.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_MEAT_PRINT_HPP
#define EPIWORLD_MODEL_MEAT_PRINT_HPP

template<typename TSeq>
inline const Model<TSeq> & Model<TSeq>::print(bool lite) const
{

    // Horizontal line
    std::string line = "";
    for (epiworld_fast_uint i = 0u; i < 80u; ++i)
        line += "_";

    // Prints a message if debugging is on
    EPI_DEBUG_NOTIFY_ACTIVE()

    printf_epiworld("%s\n",line.c_str());

    if (lite)
    {
        // Printing the name of the model
        printf_epiworld("%s", name.c_str());

        // Printing the number of agents, viruses, and tools
        printf_epiworld(
            "\nIt features %i agents, %i virus(es), and %i tool(s).\n",
            static_cast<int>(size()),
            static_cast<int>(get_n_viruses()),
            static_cast<int>(get_n_tools())
            );

        printf_epiworld(
            "The model has %i states.",
            static_cast<int>(nstates)
            );

        if (today() != 0)
        {
            printf_epiworld(
                "\nThe final distribution is: "
            );

            int nstate_int = static_cast<int>(nstates);

            for (int i = 0u; i < nstate_int; ++i)
            {
                printf_epiworld(
                    "%i %s%s",
                    static_cast<int>(db.today_total[ i ]),
                    states_labels[i].c_str(),
                    (
                        i == (nstate_int - 2)
                        ) ? ", and " : (
                            (i == (nstate_int - 1)) ? ".\n" : ", "
                            )
                );
            }
        } else {
            printf_epiworld(" The model hasn't been run yet.\n");
        }

        return *this;
    }

    printf_epiworld("%s\n%s\n\n",line.c_str(), "SIMULATION STUDY");

    printf_epiworld("Name of the model   : %s\n", (this->name == "") ? std::string("(none)").c_str() : name.c_str());
    printf_epiworld("Population size     : %i\n", static_cast<int>(size()));

    auto ncols = get_agents_data_ncols();

    if (ncols > 0)
    {
        printf_epiworld("Agents' data loaded : yes (%i columns/features)\n", static_cast<int>(ncols));
    }
    else
    {
        printf_epiworld("Agents' data        : (none)\n");
    }

    printf_epiworld("Number of entities  : %i\n", static_cast<int>(entities.size()));
    printf_epiworld("Days (duration)     : %i (of %i)\n", today(), static_cast<int>(ndays));
    printf_epiworld("Number of viruses   : %i\n", static_cast<int>(db.get_n_viruses()));
    if (n_replicates > 0u)
    {
        std::string abbr;
        epiworld_double elapsed;
        epiworld_double total;
        get_elapsed("auto", &elapsed, &total, &abbr, false);
        printf_epiworld("Last run elapsed t  : %.2f%s\n", elapsed, abbr.c_str());
        if (n_replicates > 1u)
        {
            printf_epiworld("Total elapsed t     : %.2f%s (%i runs)\n", total, abbr.c_str(), static_cast<int>(n_replicates));
        }

        // Elapsed time in speed
        get_elapsed("microseconds", &elapsed, &total, &abbr, false);
        printf_epiworld("Last run speed      : %.2f million agents x day / second\n",
            static_cast<double>(this->size()) *
            static_cast<double>(this->get_ndays()) /
            static_cast<double>(elapsed)
            );
        if (n_replicates > 1u)
        {
            printf_epiworld("Average run speed   : %.2f million agents x day / second\n",
                static_cast<double>(this->size()) *
                static_cast<double>(this->get_ndays()) *
                static_cast<double>(n_replicates) /
                static_cast<double>(total)
            );
        }

    } else {
        printf_epiworld("Last run elapsed t  : -\n");
    }
    
    
    if (rewire_fun)
    {
        printf_epiworld("Rewiring            : on (%.2f)\n\n", rewire_prop);
    } else {
        printf_epiworld("Rewiring            : off\n\n");
    }
    
    // Printing Global events
    printf_epiworld("Global events:\n");
    for (auto & a : globalevents)
    {
        if (a.get_day() < 0)
        {
            printf_epiworld(" - %s (runs daily)\n", a.get_name().c_str());
        } else {
            printf_epiworld(" - %s (day %i)\n", a.get_name().c_str(), a.get_day());
        }
    }

    if (globalevents.size() == 0u)
    {
        printf_epiworld(" (none)\n");
    }

    printf_epiworld("\nVirus(es):\n");
    size_t n_viruses_model = viruses.size();
    for (size_t i = 0u; i < n_viruses_model; ++i)
    {    

        
        const auto & virus = viruses[i];
        if ((n_viruses_model > 10) && (i >= 10))
        {
            printf_epiworld(" ...and %i more viruses...\n",
                static_cast<int>(n_viruses_model) - 
                static_cast<int>(i)
                );
            break;
        }

        if (i < n_viruses_model)
        {

            printf_epiworld(
                " - %s\n",
                virus->get_name().c_str()
            );

        } else {

            printf_epiworld(
                " - %s (originated in the model...)\n",
                virus->get_name().c_str()
            );

        }

    }

    auto nvariants = db.get_n_viruses() - n_viruses_model;
    if (nvariants > 0)
    {

        printf_epiworld(" ...and %i more variants...\n", static_cast<int>(nvariants));

    }

    if (viruses.size() == 0u)
    {
        printf_epiworld(" (none)\n");
    }

    printf_epiworld("\nTool(s):\n");
    size_t n_tools_model = tools.size();
    for (size_t i = 0u; i < tools.size(); ++i)
    {   
        const auto & tool = tools[i];

        if ((n_tools_model > 10) && (i >= 10))
        {
            printf_epiworld(
                " ...and %i more tools...\n",
                static_cast<int>(n_tools_model) - static_cast<int>(i)
                );
            break;
        }

        if (i < n_tools_model)
        {
            printf_epiworld(
                " - %s\n",
                tool->get_name().c_str()
                );


        } else {

            printf_epiworld(
                " - %s (originated in the model...)\n",
                tool->get_name().c_str()
            );

        }
        

    }

    if (tools.size() == 0u)
    {
        printf_epiworld(" (none)\n");
    }

    // Information about the parameters included
    printf_epiworld("\nModel parameters:\n");
    epiworld_fast_uint nchar = 0u;
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

    if (parameters.size() == 0u)
    {
        printf_epiworld(" (none)\n");
    }

    nchar = 0u;
    for (auto & p : states_labels)
        if (p.length() > nchar)
            nchar = p.length();

    

    if (today() != 0)
    {
        fmt =
            std::string("  - (%") +
                std::to_string(std::to_string(nstates).length()) +
            std::string("d) %-") + std::to_string(nchar) +
            std::string("s : %") +
            std::to_string(std::to_string(size()).length()) +
            std::string("i -> %i\n");
    } else {
        fmt =
            std::string("  - (%") +
                std::to_string(std::to_string(nstates).length()) +
            std::string("d) %-") + std::to_string(nchar) +
            std::string("s : %i\n");
    }
        
    if (today() != 0)
    {
        printf_epiworld("\nDistribution of the population at time %i:\n", today());
        for (size_t s = 0u; s < nstates; ++s)
        {

                printf_epiworld(
                    fmt.c_str(),
                    s,
                    states_labels[s].c_str(),
                    db.hist_total_counts[s],
                    db.today_total[ s ]
                    );

        }
            // else
            // {

            //     printf_epiworld(
            //         fmt.c_str(),
            //         s,
            //         states_labels[s].c_str(),
            //         db.today_total[ s ]
            //         );

            // }
    }

    if (today() != 0)
        (void) db.transition_probability(true);

    return *this;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//model-meat-print.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/




template<typename TSeq>
inline void Model<TSeq>::add_state(
    std::string lab, 
    UpdateFun<TSeq> fun
)
{

    // Checking it doesn't match
    for (auto & s : states_labels)
        if (s == lab)
            throw std::logic_error("state \"" + s + "\" already registered.");

    states_labels.push_back(lab);
    state_fun.push_back(fun);
    nstates++;

}


template<typename TSeq>
inline const std::vector< std::string > &
Model<TSeq>::get_states() const
{
    return states_labels;
}

template<typename TSeq>
inline const std::vector< UpdateFun<TSeq> > &
Model<TSeq>::get_state_fun() const
{
    return state_fun;
}

template<typename TSeq>
inline void Model<TSeq>::print_state_codes() const
{

    // Horizontal line
    std::string line = "";
    for (epiworld_fast_uint i = 0u; i < 80u; ++i)
        line += "_";

    printf_epiworld("\n%s\nstates CODES\n\n", line.c_str());

    epiworld_fast_uint nchar = 0u;
    for (auto & p : states_labels)
        if (p.length() > nchar)
            nchar = p.length();
    
    std::string fmt = " %2i = %-" + std::to_string(nchar + 1 + 4) + "s\n";
    for (epiworld_fast_uint i = 0u; i < nstates; ++i)
    {

        printf_epiworld(
            fmt.c_str(),
            i,
            (states_labels[i] + " (S)").c_str()
        );

    }

}



template<typename TSeq>
inline epiworld_double Model<TSeq>::add_param(
    epiworld_double initial_value,
    std::string pname,
    bool overwrite
    ) {

    if (parameters.find(pname) == parameters.end())
        parameters[pname] = initial_value;
    else if (!overwrite)
        throw std::logic_error("The parameter " + pname + " already exists.");
    else
        parameters[pname] = initial_value;
    
    return initial_value;

}

template<typename TSeq>
inline Model<TSeq> & Model<TSeq>::read_params(std::string fn, bool overwrite)
{

    auto params_map = read_yaml<epiworld_double>(fn);

    for (auto & p : params_map)
        add_param(p.second, p.first, overwrite);

    return *this;

}

template<typename TSeq>
inline epiworld_double Model<TSeq>::get_param(std::string pname)
{
    if (parameters.find(pname) == parameters.end())
        throw std::logic_error("The parameter " + pname + " does not exists.");

    return parameters[pname];
}

template<typename TSeq>
inline void Model<TSeq>::set_param(std::string pname, epiworld_double value)
{
    if (parameters.find(pname) == parameters.end())
        throw std::logic_error("The parameter " + pname + " does not exists.");

    parameters[pname] = value;

    return;

}

// // Same as before but using the size_t method
// template<typename TSeq>
// inline void Model<TSeq>::set_param(size_t k, epiworld_double value)
// {
//     if (k >= parameters.size())
//         throw std::logic_error("The parameter index " + std::to_string(k) + " does not exists.");

//     // Access the k-th element of the std::unordered_map parameters


//     *(parameters.begin() + k) = value;

//     return;
// }

template<typename TSeq>
inline epiworld_double Model<TSeq>::par(std::string pname) const
{
    const auto iter = parameters.find(pname);
    if (iter == parameters.end())
        throw std::logic_error("The parameter " + pname + " does not exists.");
    return iter->second;
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
    if (unit_abbr != nullptr)
        *unit_abbr = abbr_unit;

    if (!print)
        return;

    if (n_replicates > 1u)
    {
        printf_epiworld("last run elapsed time : %.2f%s\n",
            elapsed, abbr_unit.c_str());
        printf_epiworld("total elapsed time    : %.2f%s\n",
            elapsed_total, abbr_unit.c_str());
        printf_epiworld("total runs            : %i\n",
            static_cast<int>(n_replicates));
        printf_epiworld("mean run elapsed time : %.2f%s\n",
            elapsed_total/static_cast<epiworld_double>(n_replicates), abbr_unit.c_str());

    } else {
        printf_epiworld("last run elapsed time : %.2f%s.\n", elapsed, abbr_unit.c_str());
    }
}

template<typename TSeq>
inline void Model<TSeq>::set_user_data(std::vector< std::string > names)
{
    db.set_user_data(names);
}

template<typename TSeq>
inline void Model<TSeq>::add_user_data(epiworld_fast_uint j, epiworld_double x)
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
inline void Model<TSeq>::add_globalevent(
    std::function<void(Model<TSeq>*)> fun,
    std::string name,
    int date
)
{

    globalevents.push_back(
        GlobalEvent<TSeq>(
            fun,
            name,
            date
            )
    );

}

template<typename TSeq>
inline void Model<TSeq>::add_globalevent(
    GlobalEvent<TSeq> action
)
{
    globalevents.push_back(action);
}

template<typename TSeq>
GlobalEvent<TSeq> & Model<TSeq>::get_globalevent(
    std::string name
)
{

    for (auto & a : globalevents)
        if (a.name == name)
            return a;

    throw std::logic_error("The global action " + name + " was not found.");

}

template<typename TSeq>
GlobalEvent<TSeq> & Model<TSeq>::get_globalevent(
    size_t index
)
{

    if (index >= globalevents.size())
        throw std::range_error("The index " + std::to_string(index) + " is out of range.");

    return globalevents[index];

}

// Remove implementation
template<typename TSeq>
inline void Model<TSeq>::rm_globalevent(
    std::string name
)
{

    for (auto it = globalevents.begin(); it != globalevents.end(); ++it)
    {
        if (it->get_name() == name)
        {
            globalevents.erase(it);
            return;
        }
    }

    throw std::logic_error("The global action " + name + " was not found.");

}

// Same as above, but the index implementation
template<typename TSeq>
inline void Model<TSeq>::rm_globalevent(
    size_t index
)
{

    if (index >= globalevents.size())
        throw std::range_error("The index " + std::to_string(index) + " is out of range.");

    globalevents.erase(globalevents.begin() + index);

}

template<typename TSeq>
inline void Model<TSeq>::run_globalevents()
{

    for (auto & action: globalevents)
    {
        action(this, today());
        events_run();
    }

}

template<typename TSeq>
inline void Model<TSeq>::queuing_on()
{
    use_queuing = true;
}

template<typename TSeq>
inline Model<TSeq> & Model<TSeq>::queuing_off()
{
    use_queuing = false;
    return *this;
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

template<typename TSeq>
inline const std::vector< VirusPtr<TSeq> > & Model<TSeq>::get_viruses() const
{
    return viruses;
}

template<typename TSeq>
const std::vector< ToolPtr<TSeq> > & Model<TSeq>::get_tools() const
{
    return tools;
}

template<typename TSeq>
inline Virus<TSeq> & Model<TSeq>::get_virus(size_t id)
{

    if (viruses.size() <= id)
        throw std::length_error("The specified id for the virus is out of range");

    return *viruses[id];

}

template<typename TSeq>
inline Tool<TSeq> & Model<TSeq>::get_tool(size_t id)
{

    if (tools.size() <= id)
        throw std::length_error("The specified id for the tools is out of range");

    return *tools[id];

}


template<typename TSeq>
inline void Model<TSeq>::set_agents_data(double * data_, size_t ncols_)
{
    agents_data = data_;
    agents_data_ncols = ncols_;
}

template<typename TSeq>
inline double * Model<TSeq>::get_agents_data() {
    return this->agents_data;
}

template<typename TSeq>
inline size_t Model<TSeq>::get_agents_data_ncols() const {
    return this->agents_data_ncols;
}


template<typename TSeq>
inline void Model<TSeq>::set_name(std::string name)
{
    this->name = name;
}

template<typename TSeq>
inline std::string Model<TSeq>::get_name() const 
{
    return this->name;
}

#define VECT_MATCH(a, b, c) \
    EPI_DEBUG_FAIL_AT_TRUE(a.size() != b.size(), c) \
    for (size_t __i = 0u; __i < a.size(); ++__i) \
    {\
        EPI_DEBUG_FAIL_AT_TRUE(a[__i] != b[__i], c) \
    }

template<typename TSeq>
inline bool Model<TSeq>::operator==(const Model<TSeq> & other) const
{
    EPI_DEBUG_FAIL_AT_TRUE(name != other.name, "names don't match")
    EPI_DEBUG_FAIL_AT_TRUE(db != other.db, "database don't match")

    VECT_MATCH(population, other.population, "population doesn't match")

    EPI_DEBUG_FAIL_AT_TRUE(
        using_backup != other.using_backup,
        "Model:: using_backup don't match"
        )
    
    if ((population_backup.size() != 0) & (other.population_backup.size() != 0))
    {

        // False is population_backup.size() != other.population_backup.size()
        if (population_backup.size() != other.population_backup.size())
            return false;

        for (size_t i = 0u; i < population_backup.size(); ++i)
        {
            if (population_backup[i] != other.population_backup[i])
                return false;
        }
        
    } else if ((population_backup.size() == 0) & (other.population_backup.size() != 0)) {
        return false;
    } else if ((population_backup.size() != 0) & (other.population_backup.size() == 0))
    {
        return false;
    }

    EPI_DEBUG_FAIL_AT_TRUE(
        agents_data != other.agents_data,
        "Model:: agents_data don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        agents_data_ncols != other.agents_data_ncols,
        "Model:: agents_data_ncols don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        directed != other.directed,
        "Model:: directed don't match"
    )
    
    // Viruses -----------------------------------------------------------------
    EPI_DEBUG_FAIL_AT_TRUE(
        viruses.size() != other.viruses.size(),
        "Model:: viruses.size() don't match"
        )

    for (size_t i = 0u; i < viruses.size(); ++i)
    {
        EPI_DEBUG_FAIL_AT_TRUE(
            *viruses[i] != *other.viruses[i],
            "Model:: *viruses[i] don't match"
        )
            
    }
    
    // Tools -------------------------------------------------------------------
    EPI_DEBUG_FAIL_AT_TRUE(
        tools.size() != other.tools.size(),
        "Model:: tools.size() don't match"
        )
        
    for (size_t i = 0u; i < tools.size(); ++i)
    {
        EPI_DEBUG_FAIL_AT_TRUE(
            *tools[i] != *other.tools[i],
            "Model:: *tools[i] don't match"
        )
            
    }
    
    VECT_MATCH(
        entities,
        other.entities,
        "entities don't match"
    )

    if ((entities_backup.size() != 0) & (other.entities_backup.size() != 0))
    {
        
        for (size_t i = 0u; i < entities_backup.size(); ++i)
        {

            EPI_DEBUG_FAIL_AT_TRUE(
                entities_backup[i] != other.entities_backup[i],
                "Model:: entities_backup[i] don't match"
            )

        }
        
    } else if ((entities_backup.size() == 0) & (other.entities_backup.size() != 0)) {
        EPI_DEBUG_FAIL_AT_TRUE(true, "entities_backup don't match")
    } else if ((entities_backup.size() != 0) & (other.entities_backup.size() == 0))
    {
        EPI_DEBUG_FAIL_AT_TRUE(true, "entities_backup don't match")
    }

    EPI_DEBUG_FAIL_AT_TRUE(
        rewire_prop != other.rewire_prop,
        "Model:: rewire_prop don't match"
    )
        
    EPI_DEBUG_FAIL_AT_TRUE(
        parameters.size() != other.parameters.size(),
        "Model:: () don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        parameters != other.parameters,
        "Model:: parameters don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        ndays != other.ndays,
        "Model:: ndays don't match"
    )
    
    VECT_MATCH(
        states_labels,
        other.states_labels,
        "state labels don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        nstates != other.nstates,
        "Model:: nstates don't match"
    )
    
    EPI_DEBUG_FAIL_AT_TRUE(
        verbose != other.verbose,
        "Model:: verbose don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        current_date != other.current_date,
        "Model:: current_date don't match"
    )

    VECT_MATCH(globalevents, other.globalevents, "global action don't match");

    EPI_DEBUG_FAIL_AT_TRUE(
        queue != other.queue,
        "Model:: queue don't match"
    )
    

    EPI_DEBUG_FAIL_AT_TRUE(
        use_queuing != other.use_queuing,
        "Model:: use_queuing don't match"
    )
    
    return true;

}

template<typename TSeq>
inline void Model<TSeq>::draw(
    const std::string & fn_output,
    bool self
) {

    ModelDiagram diagram;

    diagram.draw_from_data(
        this->get_states(),
        this->get_db().transition_probability(false),
        fn_output,
        self
    );

    return;

}

#undef VECT_MATCH
#undef DURCAST
#undef CASES_PAR
#undef CASE_PAR
#undef CHECK_INIT
#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/model-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/viruses-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_VIRUSES_BONES_HPP
#define EPIWORLD_VIRUSES_BONES_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Agent;

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
    std::vector< VirusPtr<TSeq> > * dat;
    const epiworld_fast_uint * n_viruses;

public:

    Viruses() = delete;
    Viruses(Agent<TSeq> & p) : dat(&p.viruses), n_viruses(&p.n_viruses) {};

    typename std::vector< VirusPtr<TSeq> >::iterator begin();
    typename std::vector< VirusPtr<TSeq> >::iterator end();

    VirusPtr<TSeq> & operator()(size_t i);
    VirusPtr<TSeq> & operator[](size_t i);

    size_t size() const noexcept;

    void print() const noexcept;

};

template<typename TSeq>
inline typename std::vector< VirusPtr<TSeq> >::iterator Viruses<TSeq>::begin()
{

    if (*n_viruses == 0u)
        return dat->end();
    
    return dat->begin();
}

template<typename TSeq>
inline typename std::vector< VirusPtr<TSeq> >::iterator Viruses<TSeq>::end()
{
    
    #ifdef EPI_DEBUG
    if (dat->size() < *n_viruses)
        throw EPI_DEBUG_ERROR(std::logic_error, "Viruses:: The end of the virus is out of range");
    #endif 

    return begin() + *n_viruses;
}

template<typename TSeq>
inline VirusPtr<TSeq> & Viruses<TSeq>::operator()(size_t i)
{

    if (i >= *n_viruses)
        throw std::range_error("Virus index out of range.");

    return dat->operator[](i);

}

template<typename TSeq>
inline VirusPtr<TSeq> & Viruses<TSeq>::operator[](size_t i)
{

    return dat->operator[](i);

}

template<typename TSeq>
inline size_t Viruses<TSeq>::size() const noexcept 
{
    return *n_viruses;
}

template<typename TSeq>
inline void Viruses<TSeq>::print() const noexcept
{

    if (*n_viruses == 0u)
    {
        printf_epiworld("List of viruses (none)\n");
        return;
    }

    printf_epiworld("List of viruses (%i): ", *n_viruses);

    // Printing the name of each virus separated by a comma
    for (size_t i = 0u; i < *n_viruses; ++i)
    {
        if (i == *n_viruses - 1u)
        {
            printf_epiworld("%s", dat->operator[](i)->get_name().c_str());
        } else 
        {
            printf_epiworld("%s, ", dat->operator[](i)->get_name().c_str());
        }
    }
    
    printf_epiworld("\n");

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
    const std::vector< VirusPtr<TSeq> > * dat;
    const epiworld_fast_uint * n_viruses;

public:

    Viruses_const() = delete;
    Viruses_const(const Agent<TSeq> & p) : dat(&p.viruses), n_viruses(&p.n_viruses) {};

    typename std::vector< VirusPtr<TSeq> >::const_iterator begin() const;
    typename std::vector< VirusPtr<TSeq> >::const_iterator end() const;

    const VirusPtr<TSeq> & operator()(size_t i);
    const VirusPtr<TSeq> & operator[](size_t i);

    size_t size() const noexcept;

    void print() const noexcept;

};

template<typename TSeq>
inline typename std::vector< VirusPtr<TSeq> >::const_iterator Viruses_const<TSeq>::begin() const {

    if (*n_viruses == 0u)
        return dat->end();
    
    return dat->begin();
}

template<typename TSeq>
inline typename std::vector< VirusPtr<TSeq> >::const_iterator Viruses_const<TSeq>::end() const {

    #ifdef EPI_DEBUG
    if (dat->size() < *n_viruses)
        throw EPI_DEBUG_ERROR(std::logic_error, "Viruses_const:: The end of the virus is out of range");
    #endif 
    return begin() + *n_viruses;
}

template<typename TSeq>
inline const VirusPtr<TSeq> & Viruses_const<TSeq>::operator()(size_t i)
{

    if (i >= *n_viruses)
        throw std::range_error("Virus index out of range.");

    return dat->operator[](i);

}

template<typename TSeq>
inline const VirusPtr<TSeq> & Viruses_const<TSeq>::operator[](size_t i)
{

    return dat->operator[](i);

}

template<typename TSeq>
inline size_t Viruses_const<TSeq>::size() const noexcept 
{
    return *n_viruses;
}

template<typename TSeq>
inline void Viruses_const<TSeq>::print() const noexcept
{

    if (*n_viruses == 0u)
    {
        printf_epiworld("List of viruses (none)\n");
        return;
    }

    printf_epiworld("List of viruses (%i): ", *n_viruses);

    // Printing the name of each virus separated by a comma
    for (size_t i = 0u; i < *n_viruses; ++i)
    {
        if (i == *n_viruses - 1u)
        {
            printf_epiworld("%s", dat->operator[](i)->get_name().c_str());
        } else
        {
            printf_epiworld("%s, ", dat->operator[](i)->get_name().c_str());
        }
    }
    
    printf_epiworld("\n");

}


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/viruses-bones.hpp-

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
class Agent;

template<typename TSeq>
class Virus;

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
 * should be a function of the agent.
 */
template<typename TSeq>
class Virus {
    friend class Agent<TSeq>;
    friend class Model<TSeq>;
    friend class DataBase<TSeq>;
    friend void default_add_virus<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_virus<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
private:
    
    Agent<TSeq> * agent       = nullptr;

    std::shared_ptr<TSeq> baseline_sequence = nullptr;
    std::shared_ptr<std::string> virus_name = nullptr;
    int date = -99;
    int id   = -99;
    bool active = true;
    MutFun<TSeq>          mutation_fun                 = nullptr;
    PostRecoveryFun<TSeq> post_recovery_fun            = nullptr;
    VirusFun<TSeq>        probability_of_infecting_fun = nullptr;
    VirusFun<TSeq>        probability_of_recovery_fun  = nullptr;
    VirusFun<TSeq>        probability_of_death_fun     = nullptr;
    VirusFun<TSeq>        incubation_fun               = nullptr;

    // Setup parameters
    std::vector< epiworld_double > data = {};

    epiworld_fast_int state_init    = -99; ///< Change of state when added to agent.
    epiworld_fast_int state_post    = -99; ///< Change of state when removed from agent.
    epiworld_fast_int state_removed = -99; ///< Change of state when agent is removed

    epiworld_fast_int queue_init    = Queue<TSeq>::Everyone; ///< Change of state when added to agent.
    epiworld_fast_int queue_post    = -Queue<TSeq>::Everyone; ///< Change of state when removed from agent.
    epiworld_fast_int queue_removed = -99; ///< Change of state when agent is removed

    // Information about how distribution works
    VirusToAgentFun<TSeq> dist_fun = nullptr;

public:
    Virus(std::string name = "unknown virus");

    Virus(
        std::string name,
        epiworld_double prevalence,
        bool as_proportion
        );

    void mutate(Model<TSeq> * model);
    void set_mutation(MutFun<TSeq> fun);
    
    std::shared_ptr<TSeq> get_sequence();
    void set_sequence(TSeq sequence);
    
    Agent<TSeq> * get_agent();
    void set_agent(Agent<TSeq> * p);
    
    void set_date(int d);
    int get_date() const;

    void set_id(int idx);
    int get_id() const;

    /**
     * @name Get and set the tool functions
     * 
     * @param v The virus over which to operate
     * @param fun the function to be used
     * 
     * @return epiworld_double 
     */
    ///@{
    epiworld_double get_prob_infecting(Model<TSeq> * model);
    epiworld_double get_prob_recovery(Model<TSeq> * model);
    epiworld_double get_prob_death(Model<TSeq> * model);
    epiworld_double get_incubation(Model<TSeq> * model);
    
    void post_recovery(Model<TSeq> * model);
    void set_post_recovery(PostRecoveryFun<TSeq> fun);
    void set_post_immunity(epiworld_double prob);
    void set_post_immunity(epiworld_double * prob);

    void set_prob_infecting_fun(VirusFun<TSeq> fun);
    void set_prob_recovery_fun(VirusFun<TSeq> fun);
    void set_prob_death_fun(VirusFun<TSeq> fun);
    void set_incubation_fun(VirusFun<TSeq> fun);
    
    void set_prob_infecting(const epiworld_double * prob);
    void set_prob_recovery(const epiworld_double * prob);
    void set_prob_death(const epiworld_double * prob);
    void set_incubation(const epiworld_double * prob);
    
    void set_prob_infecting(epiworld_double prob);
    void set_prob_recovery(epiworld_double prob);
    void set_prob_death(epiworld_double prob);
    void set_incubation(epiworld_double prob);
    ///@}


    void set_name(std::string name);
    std::string get_name() const;

    std::vector< epiworld_double > & get_data();

    /**
     * @name Get and set the state and queue
     * 
     * After applied, viruses can change the state and affect
     * the queue of agents. These function sets the default values,
     * which are retrieved when adding or removing a virus does not
     * specify a change in state or in queue.
     * 
     * @param init After the virus/tool is added to the agent.
     * @param end After the virus/tool is removed.
     * @param removed After the agent (Agent) is removed.
     */
    ///@{
    void set_state(
        epiworld_fast_int init,
        epiworld_fast_int end,
        epiworld_fast_int removed = -99
        );
        
    void set_queue(
        epiworld_fast_int init,
        epiworld_fast_int end,
        epiworld_fast_int removed = -99
        );

    void get_state(
        epiworld_fast_int * init,
        epiworld_fast_int * end,
        epiworld_fast_int * removed = nullptr
        );

    void get_queue(
        epiworld_fast_int * init,
        epiworld_fast_int * end,
        epiworld_fast_int * removed = nullptr
        );
    ///@}

    bool operator==(const Virus<TSeq> & other) const;
    bool operator!=(const Virus<TSeq> & other) const {return !operator==(other);};

    void print() const;

    /**
     * @brief Get information about the prevalence of the virus
     */
    ///@{
    void distribute(Model<TSeq> * model);
    void set_distribution(VirusToAgentFun<TSeq> fun);
    ///@}


};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/virus-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/virus-distribute-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_VIRUS_DISTRIBUTE_MEAT_HPP
#define EPIWORLD_VIRUS_DISTRIBUTE_MEAT_HPP

/**
 * Distributes a virus to a set of agents.
 *
 * This function takes a vector of agent IDs and returns a lambda function that
 * can be used to distribute a virus to the specified agents.
 *
 * @param agents_ids A vector of agent IDs representing the set of agents to
 * distribute the virus to.
 * 
 * @return A lambda function that takes a Virus object and a Model object and
 * distributes the virus to the specified agents.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline VirusToAgentFun<TSeq> distribute_virus_to_set(
    std::vector< size_t > agents_ids
) {

    return [agents_ids](
        Virus<TSeq> & virus, Model<TSeq> * model
    ) -> void 
    { 
        // Adding action
        for (auto i: agents_ids)
        {
            model->get_agent(i).set_virus(
                virus,
                const_cast<Model<TSeq> * >(model)
                );
        }
    };

}

/**
 * @brief Distributes a virus randomly to agents.
 * 
 * This function takes a sequence of agents and randomly assigns a virus to
 * each agent.
 * 
 * @tparam TSeq The type of the sequence of agents.
 * @param agents The sequence of agents to distribute the virus to.
 * @return A function object that assigns a virus to each agent randomly.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline VirusToAgentFun<TSeq> distribute_virus_randomly(
    epiworld_double prevalence,
    bool prevalence_as_proportion = true
) {

    return [prevalence,prevalence_as_proportion](
        Virus<TSeq> & virus, Model<TSeq> * model
    ) -> void 
    { 
        
        // Figuring out how what agents are available
        std::vector< size_t > idx;
        for (const auto & agent: model->get_agents())
            if (agent.get_virus() == nullptr)
                idx.push_back(agent.get_id());

        // Picking how many
        int n = static_cast<int>(model->size());
        int n_available = static_cast<int>(idx.size());
        int n_to_sample;
        if (prevalence_as_proportion)
        {
            n_to_sample = static_cast<int>(std::floor(
                prevalence * static_cast< epiworld_double >(n)
            ));

            // Correcting for possible overflow
            if (n_to_sample > n)
                n_to_sample = n;
        }
        else
        {
            n_to_sample = static_cast<int>(prevalence);
        }

        if (n_to_sample > n_available)
            throw std::range_error(
                "There are only " + std::to_string(n_available) + 
                " individuals with no virus in the population. " +
                "Cannot add the virus to " +
                std::to_string(n_to_sample)
            );
        
        auto & population = model->get_agents();
        for (int i = 0; i < n_to_sample; ++i)
        {

            int loc = static_cast<epiworld_fast_uint>(
                floor(model->runif() * (n_available--))
                );

            // Correcting for possible overflow
            if ((loc > 0) && (loc >= n_available))
                loc = n_available - 1;

            Agent<TSeq> & agent = population[idx[loc]];
            
            // Adding action
            agent.set_virus(
                virus,
                const_cast<Model<TSeq> * >(model)
                );

            // Adjusting sample
            std::swap(idx[loc], idx[n_available]);

        }

    };

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/virus-distribute-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/virus-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_VIRUS_MEAT_HPP
#define EPIWORLD_VIRUS_MEAT_HPP

/**
 * @brief Factory function of VirusFun base on logit
 * 
 * @tparam TSeq 
 * @param vars Vector indicating the position of the variables to use.
 * @param coefs Vector of coefficients.
 * @return VirusFun<TSeq> 
 */
template<typename TSeq>
inline VirusFun<TSeq> virus_fun_logit(
    std::vector< int > vars,
    std::vector< double > coefs,
    Model<TSeq> * model,
    bool logit = true
) {

    // Checking that there are features
    if (coefs.size() == 0u)
        throw std::logic_error(
            "The -coefs- argument should feature at least one element."
            );

    if (coefs.size() != vars.size())
        throw std::length_error(
            std::string("The length of -coef- (") +
            std::to_string(coefs.size()) + 
            std::string(") and -vars- (") +
            std::to_string(vars.size()) +
            std::string(") should match. ")            
            );

    // Checking that there are variables in the model
    if (model != nullptr)
    {

        size_t K = model->get_agents_data_ncols();
        for (const auto & var: vars)
        {
            if ((var >= static_cast<int>(K)) | (var < 0))
                throw std::range_error(
                    std::string("The variable ") +
                    std::to_string(var) +
                    std::string(" is out of range.") +
                    std::string(" The agents only feature ") +
                    std::to_string(K) + 
                    std::string("variables (features).")
                );
        }
        
    }

    std::vector< epiworld_double > coefs_f;
    for (auto c: coefs)
        coefs_f.push_back(static_cast<epiworld_double>(c));

    VirusFun<TSeq> fun_infect = [coefs_f,vars](
        Agent<TSeq> * agent,
        Virus<TSeq> &,
        Model<TSeq> *
        ) -> epiworld_double {

        size_t K = coefs_f.size();
        epiworld_double res = 0.0;

        #if defined(__OPENMP) || defined(_OPENMP)
        #pragma omp simd reduction(+:res)
        #endif
        for (size_t i = 0u; i < K; ++i)
            res += agent->operator[](vars.at(i)) * coefs_f.at(i);

        return 1.0/(1.0 + std::exp(-res));

    };

    return fun_infect;

}

template<typename TSeq>
inline Virus<TSeq>::Virus(
    std::string name
    ) {
    set_name(name);
}

template<typename TSeq>
inline Virus<TSeq>::Virus(
    std::string name,
    epiworld_double prevalence,
    bool prevalence_as_proportion
    ) {
    set_name(name);
    set_distribution(
        distribute_virus_randomly<TSeq>(
            prevalence,
            prevalence_as_proportion
        )
    );
}

template<typename TSeq>
inline void Virus<TSeq>::mutate(
    Model<TSeq> * model
) {

    if (mutation_fun)
        if (mutation_fun(agent, *this, model))
            model->get_db().record_virus(*this);

    return;
    
}

template<typename TSeq>
inline void Virus<TSeq>::set_mutation(
    MutFun<TSeq> fun
) {
    mutation_fun = MutFun<TSeq>(fun);
}

template<typename TSeq>
inline std::shared_ptr<TSeq> Virus<TSeq>::get_sequence()
{

    return baseline_sequence;

}

template<typename TSeq>
inline void Virus<TSeq>::set_sequence(TSeq sequence)
{

    baseline_sequence = std::make_shared<TSeq>(sequence);
    return;

}

template<typename TSeq>
inline Agent<TSeq> * Virus<TSeq>::get_agent()
{

    return agent;

}

template<typename TSeq>
inline void Virus<TSeq>::set_agent(Agent<TSeq> * p)
{
    agent = p;
}

template<typename TSeq>
inline void Virus<TSeq>::set_id(int idx)
{

    id = idx;
    return;

}

template<typename TSeq>
inline int Virus<TSeq>::get_id() const
{
    
    return id;

}

template<typename TSeq>
inline void Virus<TSeq>::set_date(int d) 
{

    date = d;
    return;

}

template<typename TSeq>
inline int Virus<TSeq>::get_date() const
{
    
    return date;
    
}

template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_prob_infecting(
    Model<TSeq> * model
)
{

    if (probability_of_infecting_fun)
        return probability_of_infecting_fun(agent, *this, model);
        
    return EPI_DEFAULT_VIRUS_PROB_INFECTION;

}



template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_prob_recovery(
    Model<TSeq> * model
)
{

    if (probability_of_recovery_fun)
        return probability_of_recovery_fun(agent, *this, model);
        
    return EPI_DEFAULT_VIRUS_PROB_RECOVERY;

}



template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_prob_death(
    Model<TSeq> * model
)
{

    if (probability_of_death_fun)
        return probability_of_death_fun(agent, *this, model);
        
    return EPI_DEFAULT_VIRUS_PROB_DEATH;

}

template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_incubation(
    Model<TSeq> * model
)
{

    if (incubation_fun)
        return incubation_fun(agent, *this, model);
        
    return EPI_DEFAULT_INCUBATION_DAYS;

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
inline void Virus<TSeq>::set_incubation_fun(VirusFun<TSeq> fun)
{
    incubation_fun = fun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_infecting(const epiworld_double * prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return *prob;
        };
    
    probability_of_infecting_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_recovery(const epiworld_double * prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return *prob;
        };
    
    probability_of_recovery_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_death(const epiworld_double * prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return *prob;
        };
    
    probability_of_death_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_incubation(const epiworld_double * prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return *prob;
        };
    
    incubation_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_infecting(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return prob;
        };
    
    probability_of_infecting_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_recovery(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return prob;
        };
    
    probability_of_recovery_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_death(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return prob;
        };
    
    probability_of_death_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_incubation(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return prob;
        };
    
    incubation_fun = tmpfun;
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
inline void Virus<TSeq>::post_recovery(
    Model<TSeq> * model
)
{

    if (post_recovery_fun)
        post_recovery_fun(agent, *this, model);    

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

    // To make sure that we keep registering the virus
    ToolPtr<TSeq> __no_reinfect = std::make_shared<Tool<TSeq>>(
        "Immunity (" + *virus_name + ")"
    );

    __no_reinfect->set_susceptibility_reduction(prob);
    __no_reinfect->set_death_reduction(0.0);
    __no_reinfect->set_transmission_reduction(0.0);
    __no_reinfect->set_recovery_enhancer(0.0);

    PostRecoveryFun<TSeq> tmpfun = 
        [__no_reinfect](
            Agent<TSeq> * p, Virus<TSeq> &, Model<TSeq> * m
            )
        {
            
            // Have we registered the tool?
            if (__no_reinfect->get_id() == -99)
                m->get_db().record_tool(*__no_reinfect);

            p->add_tool(__no_reinfect, m);

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

    // To make sure that we keep registering the virus
    ToolPtr<TSeq> __no_reinfect = std::make_shared<Tool<TSeq>>(
        "Immunity (" + *virus_name + ")"
    );

    __no_reinfect->set_susceptibility_reduction(prob);
    __no_reinfect->set_death_reduction(0.0);
    __no_reinfect->set_transmission_reduction(0.0);
    __no_reinfect->set_recovery_enhancer(0.0);

    PostRecoveryFun<TSeq> tmpfun = 
        [__no_reinfect](Agent<TSeq> * p, Virus<TSeq> &, Model<TSeq> * m)
        {

            // Have we registered the tool?
            if (__no_reinfect->get_id() == -99)
                m->get_db().record_tool(*__no_reinfect);

            p->add_tool(__no_reinfect, m);

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

template<typename TSeq>
inline void Virus<TSeq>::set_state(
    epiworld_fast_int init,
    epiworld_fast_int end,
    epiworld_fast_int removed
)
{
    state_init    = init;
    state_post    = end;
    state_removed = removed;
}

template<typename TSeq>
inline void Virus<TSeq>::set_queue(
    epiworld_fast_int init,
    epiworld_fast_int end,
    epiworld_fast_int removed
)
{

    queue_init    = init;
    queue_post     = end;
    queue_removed = removed;

}

template<typename TSeq>
inline void Virus<TSeq>::get_state(
    epiworld_fast_int * init,
    epiworld_fast_int * end,
    epiworld_fast_int * removed
)
{

    if (init != nullptr)
        *init = state_init;

    if (end != nullptr)
        *end = state_post;

    if (removed != nullptr)
        *removed = state_removed;

}

template<typename TSeq>
inline void Virus<TSeq>::get_queue(
    epiworld_fast_int * init,
    epiworld_fast_int * end,
    epiworld_fast_int * removed
)
{

    if (init != nullptr)
        *init = queue_init;

    if (end != nullptr)
        *end = queue_post;

    if (removed != nullptr)
        *removed = queue_removed;
        
}

template<>
inline bool Virus<std::vector<int>>::operator==(
    const Virus<std::vector<int>> & other
    ) const
{
    

    EPI_DEBUG_FAIL_AT_TRUE(
        baseline_sequence->size() != other.baseline_sequence->size(),
        "Virus:: baseline_sequence don't match"
        )

    for (size_t i = 0u; i < baseline_sequence->size(); ++i)
    {

        EPI_DEBUG_FAIL_AT_TRUE(
            baseline_sequence->operator[](i) != other.baseline_sequence->operator[](i),
            "Virus:: baseline_sequence[i] don't match"
            )

    }

    EPI_DEBUG_FAIL_AT_TRUE(
        virus_name != other.virus_name,
        "Virus:: virus_name don't match"
        )
    
    EPI_DEBUG_FAIL_AT_TRUE(
        state_init != other.state_init,
        "Virus:: state_init don't match"
        )

    EPI_DEBUG_FAIL_AT_TRUE(
        state_post != other.state_post,
        "Virus:: state_post don't match"
        )

    EPI_DEBUG_FAIL_AT_TRUE(
        state_removed != other.state_removed,
        "Virus:: state_removed don't match"
        )

    EPI_DEBUG_FAIL_AT_TRUE(
        queue_init != other.queue_init,
        "Virus:: queue_init don't match"
        )

    EPI_DEBUG_FAIL_AT_TRUE(
        queue_post != other.queue_post,
        "Virus:: queue_post don't match"
        )

    EPI_DEBUG_FAIL_AT_TRUE(
        queue_removed != other.queue_removed,
        "Virus:: queue_removed don't match"
        )

    return true;

}

template<typename TSeq>
inline bool Virus<TSeq>::operator==(const Virus<TSeq> & other) const
{
    
    EPI_DEBUG_FAIL_AT_TRUE(
        *baseline_sequence != *other.baseline_sequence,
        "Virus:: baseline_sequence don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        virus_name != other.virus_name,
        "Virus:: virus_name don't match"
    )
    
    EPI_DEBUG_FAIL_AT_TRUE(
        state_init != other.state_init,
        "Virus:: state_init don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        state_post != other.state_post,
        "Virus:: state_post don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        state_removed != other.state_removed,
        "Virus:: state_removed don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        queue_init != other.queue_init,
        "Virus:: queue_init don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        queue_post != other.queue_post,
        "Virus:: queue_post don't match"
    )

    EPI_DEBUG_FAIL_AT_TRUE(
        queue_removed != other.queue_removed,
        "Virus:: queue_removed don't match"
    )

    return true;

}

template<typename TSeq>
inline void Virus<TSeq>::print() const
{

    printf_epiworld("Virus         : %s\n", virus_name->c_str());
    printf_epiworld("Id            : %s\n", (id < 0)? std::string("(empty)").c_str() : std::to_string(id).c_str());
    printf_epiworld("state_init    : %i\n", static_cast<int>(state_init));
    printf_epiworld("state_post    : %i\n", static_cast<int>(state_post));
    printf_epiworld("state_removed : %i\n", static_cast<int>(state_removed));
    printf_epiworld("queue_init    : %i\n", static_cast<int>(queue_init));
    printf_epiworld("queue_post    : %i\n", static_cast<int>(queue_post));
    printf_epiworld("queue_removed : %i\n", static_cast<int>(queue_removed));

}

template<typename TSeq>
inline void Virus<TSeq>::distribute(Model<TSeq> * model)
{

    if (dist_fun)
    {

        dist_fun(*this, model);

    }

}

template<typename TSeq>
inline void Virus<TSeq>::set_distribution(VirusToAgentFun<TSeq> fun)
{
    dist_fun = fun;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/virus-meat.hpp-

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
class Tool;

template<typename TSeq>
class Agent;

// #define ToolPtr<TSeq> std::shared_ptr< Tool<TSeq> >

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
    std::vector< ToolPtr<TSeq> > * dat;
    const epiworld_fast_uint * n_tools;

public:

    Tools() = delete;
    Tools(Agent<TSeq> & p) : dat(&p.tools), n_tools(&p.n_tools) {};

    typename std::vector< ToolPtr<TSeq> >::iterator begin();
    typename std::vector< ToolPtr<TSeq> >::iterator end();

    ToolPtr<TSeq> & operator()(size_t i);
    ToolPtr<TSeq> & operator[](size_t i);

    size_t size() const noexcept;

    void print() const noexcept;

};

template<typename TSeq>
inline typename std::vector< ToolPtr<TSeq> >::iterator Tools<TSeq>::begin()
{

    if (*n_tools == 0u)
        return dat->end();
    
    return dat->begin();
}

template<typename TSeq>
inline typename std::vector< ToolPtr<TSeq> >::iterator Tools<TSeq>::end()
{
     
    return begin() + *n_tools;
}

template<typename TSeq>
inline ToolPtr<TSeq> & Tools<TSeq>::operator()(size_t i)
{

    if (i >= *n_tools)
        throw std::range_error("Tool index out of range.");

    return dat->operator[](i);

}

template<typename TSeq>
inline ToolPtr<TSeq> & Tools<TSeq>::operator[](size_t i)
{

    return dat->operator[](i);

}

template<typename TSeq>
inline size_t Tools<TSeq>::size() const noexcept 
{
    return *n_tools;
}

template<typename TSeq>
inline void Tools<TSeq>::print() const noexcept 
{
    if (*n_tools == 0u)
    {
        printf_epiworld("List of tools (none)\n");
        return;
    }

    printf_epiworld("List of tools (%i): ", static_cast<int>(*n_tools));

    // Printing the name of each virus separated by a comma
    for (size_t i = 0u; i < *n_tools; ++i)
    {
        if (i == *n_tools - 1u)
        {
            printf_epiworld("%s", dat->operator[](i)->get_name().c_str());
        } else
        {
            printf_epiworld("%s, ", dat->operator[](i)->get_name().c_str());
        }
    }
    
    printf_epiworld("\n");

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
    const std::vector< ToolPtr<TSeq> > * dat;
    const epiworld_fast_uint * n_tools;

public:

    Tools_const() = delete;
    Tools_const(const Agent<TSeq> & p) : dat(&p.tools), n_tools(&p.n_tools) {};

    typename std::vector< ToolPtr<TSeq> >::const_iterator begin() const;
    typename std::vector< ToolPtr<TSeq> >::const_iterator end() const;

    const ToolPtr<TSeq> & operator()(size_t i);
    const ToolPtr<TSeq> & operator[](size_t i);

    size_t size() const noexcept;

    void print() const noexcept;

};

template<typename TSeq>
inline typename std::vector< ToolPtr<TSeq> >::const_iterator Tools_const<TSeq>::begin() const {

    if (*n_tools == 0u)
        return dat->end();
    
    return dat->begin();
}

template<typename TSeq>
inline typename std::vector< ToolPtr<TSeq> >::const_iterator Tools_const<TSeq>::end() const {
     
    return begin() + *n_tools;
}

template<typename TSeq>
inline const ToolPtr<TSeq> & Tools_const<TSeq>::operator()(size_t i)
{

    if (i >= *n_tools)
        throw std::range_error("Tool index out of range.");

    return dat->operator[](i);

}

template<typename TSeq>
inline const ToolPtr<TSeq> & Tools_const<TSeq>::operator[](size_t i)
{

    return dat->operator[](i);

}

template<typename TSeq>
inline size_t Tools_const<TSeq>::size() const noexcept 
{
    return *n_tools;
}

template<typename TSeq>
inline void Tools_const<TSeq>::print() const noexcept 
{
    if (*n_tools == 0u)
    {
        printf_epiworld("List of tools (none)\n");
        return;
    }

    printf_epiworld("List of tools (%i): ", *n_tools);

    // Printing the name of each virus separated by a comma
    for (size_t i = 0u; i < *n_tools; ++i)
    {
        if (i == *n_tools - 1u)
        {
            printf_epiworld("%s", dat->operator[](i)->get_name().c_str());
        } else
        {
            printf_epiworld("%s, ", dat->operator[](i)->get_name().c_str());
        }
    }
    
    printf_epiworld("\n");

}



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/tools-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/tool-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



#ifndef EPIWORLD_TOOL_BONES_HPP
#define EPIWORLD_TOOL_BONES_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Agent;

template<typename TSeq>
class Model;

template<typename TSeq>
class Tool;

/**
 * @brief Tools for defending the agent against the virus
 * 
 * @tparam TSeq Type of sequence
 */
template<typename TSeq> 
class Tool {
    friend class Agent<TSeq>;
    friend class Model<TSeq>;
    friend void default_add_tool<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_tool<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
private:

    Agent<TSeq> * agent = nullptr;
    int pos_in_agent        = -99; ///< Location in the agent

    int date = -99;
    int id   = -99;
    std::shared_ptr<std::string> tool_name     = nullptr;
    std::shared_ptr<TSeq> sequence             = nullptr;
    ToolFun<TSeq> susceptibility_reduction_fun = nullptr;
    ToolFun<TSeq> transmission_reduction_fun   = nullptr;
    ToolFun<TSeq> recovery_enhancer_fun        = nullptr;
    ToolFun<TSeq> death_reduction_fun          = nullptr;

    // Setup parameters
    std::vector< epiworld_double * > params;  

    epiworld_fast_int state_init = -99;
    epiworld_fast_int state_post = -99;

    epiworld_fast_int queue_init = Queue<TSeq>::NoOne; ///< Change of state when added to agent.
    epiworld_fast_int queue_post = Queue<TSeq>::NoOne; ///< Change of state when removed from agent.

    void set_agent(Agent<TSeq> * p, size_t idx);

    ToolToAgentFun<TSeq> dist_fun = nullptr;

public:
    Tool(std::string name = "unknown tool");
    Tool(
        std::string name,
        epiworld_double prevalence,
        bool as_proportion
    );

    void set_sequence(TSeq d);
    void set_sequence(std::shared_ptr<TSeq> d);
    std::shared_ptr<TSeq> get_sequence();

    /**
     * @name Get and set the tool functions
     * 
     * @param v The virus over which to operate
     * @param fun the function to be used
     * 
     * @return epiworld_double 
     */
    ///@{
    epiworld_double get_susceptibility_reduction(VirusPtr<TSeq> v, Model<TSeq> * model);
    epiworld_double get_transmission_reduction(VirusPtr<TSeq> v, Model<TSeq> * model);
    epiworld_double get_recovery_enhancer(VirusPtr<TSeq> v, Model<TSeq> * model);
    epiworld_double get_death_reduction(VirusPtr<TSeq> v, Model<TSeq> * model);
    
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
    ///@}

    void set_name(std::string name);
    std::string get_name() const;

    Agent<TSeq> * get_agent();
    int get_id() const;
    void set_id(int id);
    void set_date(int d);
    int get_date() const;

    void set_state(epiworld_fast_int init, epiworld_fast_int post);
    void set_queue(epiworld_fast_int init, epiworld_fast_int post);
    void get_state(epiworld_fast_int * init, epiworld_fast_int * post);
    void get_queue(epiworld_fast_int * init, epiworld_fast_int * post);

    bool operator==(const Tool<TSeq> & other) const;
    bool operator!=(const Tool<TSeq> & other) const {return !operator==(other);};

    void print() const;

    void distribute(Model<TSeq> * model);
    void set_distribution(ToolToAgentFun<TSeq> fun);

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/tool-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/tool-distribute-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef TOOL_DISTRIBUTE_MEAT_HPP
#define TOOL_DISTRIBUTE_MEAT_HPP

/**
 * @brief Distributes a tool to a set of agents.
 * 
 * This function takes a vector of agent IDs and returns a lambda function that
 * distributes a tool to each agent in the set.
 * 
 * The lambda function takes a reference to a Tool object and a pointer to a
 * Model object as parameters. It iterates over the agent IDs and adds the tool
 * to each agent using the add_tool() method of the Model object.
 * 
 * @tparam TSeq The sequence type used in the Tool and Model objects.
 * @param agents_ids A vector of agent IDs representing the set of agents to
 * distribute the tool to.
 * @return A lambda function that distributes the tool to the set of agents.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline ToolToAgentFun<TSeq> distribute_tool_to_set(
    std::vector< size_t > agents_ids
) {

    return [agents_ids](
        Tool<TSeq> & tool, Model<TSeq> * model
    ) -> void 
    { 
        // Adding action
        for (auto i: agents_ids)
        {
            model->get_agent(i).add_tool(
                tool,
                const_cast<Model<TSeq> * >(model)
                );
        }
    };

}

/**
 * Function template to distribute a tool randomly to agents in a model.
 * 
 * @tparam TSeq The sequence type used in the model.
 * @param prevalence The prevalence of the tool in the population.
 * @param as_proportion Flag indicating whether the prevalence is given as a
 * proportion or an absolute value.
 * @return A lambda function that distributes the tool randomly to agents in
 * the model.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline ToolToAgentFun<TSeq> distribute_tool_randomly(
    epiworld_double prevalence,
    bool as_proportion = true
) {

    return [prevalence, as_proportion](
        Tool<TSeq> & tool, Model<TSeq> * model
        ) -> void {

            // Picking how many
            int n_to_distribute;
            int n = model->size();
            if (as_proportion)
            {
                n_to_distribute = static_cast<int>(std::floor(prevalence * n));

                // Correcting for possible rounding errors
                if (n_to_distribute > n)
                    n_to_distribute = n;

            }
            else
            {
                n_to_distribute = static_cast<int>(prevalence);
            }

            if (n_to_distribute > n)
                throw std::range_error("There are only " + std::to_string(n) + 
                " individuals in the population. Cannot add the tool to " + std::to_string(n_to_distribute));
            
            std::vector< int > idx(n);
            std::iota(idx.begin(), idx.end(), 0);
            auto & population = model->get_agents();
            for (int i = 0u; i < n_to_distribute; ++i)
            {
                int loc = static_cast<epiworld_fast_uint>(
                    floor(model->runif() * n--)
                    );

                if ((loc > 0) && (loc == n))
                    loc--;
                
                population[idx[loc]].add_tool(
                    tool,
                    const_cast< Model<TSeq> * >(model)
                    );
                
                std::swap(idx[loc], idx[n]);

            }

        };

}
#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/tool-distribute-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/tool-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



#ifndef EPIWORLD_TOOLS_MEAT_HPP
#define EPIWORLD_TOOLS_MEAT_HPP

/**
 * @brief Factory function of ToolFun base on logit
 * 
 * @tparam TSeq 
 * @param vars Vector indicating the position of the variables to use.
 * @param coefs Vector of coefficients.
 * @return ToolFun<TSeq> 
 */
template<typename TSeq>
inline ToolFun<TSeq> tool_fun_logit(
    std::vector< int > vars,
    std::vector< double > coefs,
    Model<TSeq> * model
) {

    // Checking that there are features
    if (coefs.size() == 0u)
        throw std::logic_error(
            "The -coefs- argument should feature at least one element."
            );

    if (coefs.size() != vars.size())
        throw std::length_error(
            std::string("The length of -coef- (") +
            std::to_string(coefs.size()) + 
            std::string(") and -vars- (") +
            std::to_string(vars.size()) +
            std::string(") should match. ")            
            );

    // Checking that there are variables in the model
    if (model != nullptr)
    {

        size_t K = model->get_agents_data_ncols();
        for (const auto & var: vars)
        {
            if ((var >= static_cast<int>(K)) | (var < 0))
                throw std::range_error(
                    std::string("The variable ") +
                    std::to_string(var) +
                    std::string(" is out of range.") +
                    std::string(" The agents only feature ") +
                    std::to_string(K) + 
                    std::string("variables (features).")
                );
        }
        
    }

    std::vector< epiworld_double > coefs_f;
    for (auto c: coefs)
        coefs_f.push_back(static_cast<epiworld_double>(c));

    ToolFun<TSeq> fun_ = [coefs_f,vars](
        Tool<TSeq>& tool,
        Agent<TSeq> * agent,
        VirusPtr<TSeq> virus,
        Model<TSeq> * model
        ) -> epiworld_double {

        size_t K = coefs_f.size();
        epiworld_double res = 0.0;

        #if defined(__OPENMP) || defined(_OPENMP)
        #pragma omp simd reduction(+:res)
        #endif
        for (size_t i = 0u; i < K; ++i)
            res += agent->operator[](vars.at(i)) * coefs_f.at(i);

        return 1.0/(1.0 + std::exp(-res));

    };

    return fun_;

}

template<typename TSeq>
inline Tool<TSeq>::Tool(std::string name)
{
    set_name(name);
}

template<typename TSeq>
inline Tool<TSeq>::Tool(
    std::string name,
    epiworld_double prevalence,
    bool as_proportion
    )
{
    set_name(name);

    set_distribution(
        distribute_tool_randomly<TSeq>(prevalence, as_proportion)
    );
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
inline void Tool<TSeq>::set_sequence(std::shared_ptr<TSeq> d) {
    sequence = d;
}

template<typename TSeq>
inline std::shared_ptr<TSeq> Tool<TSeq>::get_sequence() {
    return sequence;
}

template<typename TSeq>
inline epiworld_double Tool<TSeq>::get_susceptibility_reduction(
    VirusPtr<TSeq> v,
    Model<TSeq> * model
)
{

    if (susceptibility_reduction_fun)
        return susceptibility_reduction_fun(*this, this->agent, v, model);

    return DEFAULT_TOOL_CONTAGION_REDUCTION;

}

template<typename TSeq>
inline epiworld_double Tool<TSeq>::get_transmission_reduction(
    VirusPtr<TSeq> v,
    Model<TSeq> * model
)
{

    if (transmission_reduction_fun)
        return transmission_reduction_fun(*this, this->agent, v, model);

    return DEFAULT_TOOL_TRANSMISSION_REDUCTION;

}

template<typename TSeq>
inline epiworld_double Tool<TSeq>::get_recovery_enhancer(
    VirusPtr<TSeq> v,
    Model<TSeq> * model
)
{

    if (recovery_enhancer_fun)
        return recovery_enhancer_fun(*this, this->agent, v, model);

    return DEFAULT_TOOL_RECOVERY_ENHANCER;

}

template<typename TSeq>
inline epiworld_double Tool<TSeq>::get_death_reduction(
    VirusPtr<TSeq> v,
    Model<TSeq> * model
)
{

    if (death_reduction_fun)
        return death_reduction_fun(*this, this->agent, v, model);

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
        [prob](Tool<TSeq> &, Agent<TSeq> *, VirusPtr<TSeq>, Model<TSeq> *)
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
        [prob](Tool<TSeq> &, Agent<TSeq> *, VirusPtr<TSeq>, Model<TSeq> *)
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
        [prob](Tool<TSeq> &, Agent<TSeq> *, VirusPtr<TSeq>, Model<TSeq> *)
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
        [prob](Tool<TSeq> &, Agent<TSeq> *, VirusPtr<TSeq>, Model<TSeq> *)
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
        [prob](Tool<TSeq> &, Agent<TSeq> *, VirusPtr<TSeq>, Model<TSeq> *)
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
        [prob](Tool<TSeq> &, Agent<TSeq> *, VirusPtr<TSeq>, Model<TSeq> *)
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
        [prob](Tool<TSeq> &, Agent<TSeq> *, VirusPtr<TSeq>, Model<TSeq> *)
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
        [prob](Tool<TSeq> &, Agent<TSeq> *, VirusPtr<TSeq>, Model<TSeq> *)
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
inline Agent<TSeq> * Tool<TSeq>::get_agent()
{
    return this->agent;
}

template<typename TSeq>
inline void Tool<TSeq>::set_agent(Agent<TSeq> * p, size_t idx)
{
    agent        = p;
    pos_in_agent = static_cast<int>(idx);
}

template<typename TSeq>
inline int Tool<TSeq>::get_id() const {
    return id;
}


template<typename TSeq>
inline void Tool<TSeq>::set_id(int id)
{
    this->id = id;
}

template<typename TSeq>
inline void Tool<TSeq>::set_date(int d)
{
    this->date = d;
}

template<typename TSeq>
inline int Tool<TSeq>::get_date() const
{
    return date;
}

template<typename TSeq>
inline void Tool<TSeq>::set_state(
    epiworld_fast_int init,
    epiworld_fast_int end
)
{
    state_init = init;
    state_post = end;
}

template<typename TSeq>
inline void Tool<TSeq>::set_queue(
    epiworld_fast_int init,
    epiworld_fast_int end
)
{
    queue_init = init;
    queue_post = end;
}

template<typename TSeq>
inline void Tool<TSeq>::get_state(
    epiworld_fast_int * init,
    epiworld_fast_int * post
)
{
    if (init != nullptr)
        *init = state_init;

    if (post != nullptr)
        *post = state_post;

}

template<typename TSeq>
inline void Tool<TSeq>::get_queue(
    epiworld_fast_int * init,
    epiworld_fast_int * post
)
{
    if (init != nullptr)
        *init = queue_init;

    if (post != nullptr)
        *post = queue_post;

}

template<>
inline bool Tool<std::vector<int>>::operator==(
    const Tool<std::vector<int>> & other
    ) const
{
    
    if (sequence->size() != other.sequence->size())
        return false;

    for (size_t i = 0u; i < sequence->size(); ++i)
    {
        if (sequence->operator[](i) != other.sequence->operator[](i))
            return false;
    }

    if (tool_name != other.tool_name)
        return false;
    
    if (state_init != other.state_init)
        return false;

    if (state_post != other.state_post)
        return false;

    if (queue_init != other.queue_init)
        return false;

    if (queue_post != other.queue_post)
        return false;


    return true;

}

template<typename TSeq>
inline bool Tool<TSeq>::operator==(const Tool<TSeq> & other) const
{
    if (*sequence != *other.sequence)
        return false;

    if (tool_name != other.tool_name)
        return false;
    
    if (state_init != other.state_init)
        return false;

    if (state_post != other.state_post)
        return false;

    if (queue_init != other.queue_init)
        return false;

    if (queue_post != other.queue_post)
        return false;

    return true;

}


template<typename TSeq>
inline void Tool<TSeq>::print() const
{

    printf_epiworld("Tool       : %s\n", tool_name->c_str());
    printf_epiworld("Id         : %s\n", (id < 0)? std::string("(empty)").c_str() : std::to_string(id).c_str());
    printf_epiworld("state_init : %i\n", static_cast<int>(state_init));
    printf_epiworld("state_post : %i\n", static_cast<int>(state_post));
    printf_epiworld("queue_init : %i\n", static_cast<int>(queue_init));
    printf_epiworld("queue_post : %i\n", static_cast<int>(queue_post));

}

template<typename TSeq>
inline void Tool<TSeq>::distribute(Model<TSeq> * model)
{

    if (dist_fun)
    {

        dist_fun(*this, model);

    }

}

template<typename TSeq>
inline void Tool<TSeq>::set_distribution(ToolToAgentFun<TSeq> fun)
{
    dist_fun = fun;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/tool-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/entity-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_ENTITY_BONES_HPP
#define EPIWORLD_ENTITY_BONES_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Agent;

template<typename TSeq>
class AgentsSample;

template<typename TSeq>
inline void default_add_entity(Event<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_entity(Event<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
class Entity {
    friend class Agent<TSeq>;
    friend class AgentsSample<TSeq>;
    friend class Model<TSeq>;
    friend void default_add_entity<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_entity<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
private:
    
    int id = -1;
    std::vector< size_t > agents;   ///< Vector of agents
    std::vector< size_t > agents_location; ///< Location where the entity is stored in the agent
    size_t n_agents = 0u;

    /**
     * @name Auxiliary variables for AgentsSample<TSeq> iterators
     * 
     * @details These variables+objects are used by the AgentsSample<TSeq>
     * class for building efficient iterators over agents. The idea is to
     * reduce the memory allocation, so only during the first call of
     * AgentsSample<TSeq>::AgentsSample(Entity<TSeq>) these vectors are allocated.
     */
    ///@{
    std::vector< Agent<TSeq> * > sampled_agents;
    size_t sampled_agents_n = 0u;
    std::vector< size_t > sampled_agents_left;
    size_t sampled_agents_left_n = 0u;
    // int date_last_add_or_remove = -99; ///< Last time the entity added or removed an agent
    ///@}

    int max_capacity = -1;
    std::string entity_name = "Unnamed entity";

    std::vector< epiworld_double > location = {0.0}; ///< An arbitrary vector for location

    epiworld_fast_int state_init = -99;
    epiworld_fast_int state_post = -99;

    epiworld_fast_int queue_init = 0; ///< Change of state when added to agent.
    epiworld_fast_int queue_post = 0; ///< Change of state when removed from agent.

    EntityToAgentFun<TSeq> dist_fun = nullptr;

public:


    /**
     * @brief Constructs an Entity object.
     *
     * This constructor initializes an Entity object with the specified parameters.
     *
     * @param name The name of the entity.
     * @param fun A function pointer to a function that maps the entity to an agent.
     */
    Entity(
        std::string name,
        EntityToAgentFun<TSeq> fun = nullptr
        ) :
            entity_name(name),
            dist_fun(fun)
        {};
    
    void add_agent(Agent<TSeq> & p, Model<TSeq> * model);
    void add_agent(Agent<TSeq> * p, Model<TSeq> * model);
    void rm_agent(size_t idx, Model<TSeq> * model);
    size_t size() const noexcept;
    void set_location(std::vector< epiworld_double > loc);
    std::vector< epiworld_double > & get_location();

    typename std::vector< Agent<TSeq> * >::iterator begin();
    typename std::vector< Agent<TSeq> * >::iterator end();

    typename std::vector< Agent<TSeq> * >::const_iterator begin() const;
    typename std::vector< Agent<TSeq> * >::const_iterator end() const;

    size_t operator[](size_t i);

    int get_id() const noexcept;
    const std::string & get_name() const noexcept;

    void set_state(epiworld_fast_int init, epiworld_fast_int post);
    void set_queue(epiworld_fast_int init, epiworld_fast_int post);
    void get_state(epiworld_fast_int * init, epiworld_fast_int * post);
    void get_queue(epiworld_fast_int * init, epiworld_fast_int * post);

    void reset();

    bool operator==(const Entity<TSeq> & other) const;
    bool operator!=(const Entity<TSeq> & other) const {return !operator==(other);};

    /** 
     * @name Entity distribution
     * 
     * @details These functions are used for distributing agents among entities.
     * The idea is to have a flexible way of distributing agents among entities.
     
     */
    void distribute(Model<TSeq> * model);

    std::vector< size_t > & get_agents();

    void print() const;
    void set_distribution(EntityToAgentFun<TSeq> fun);

};


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/entity-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/entity-distribute-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_ENTITY_DISTRIBUTE_MEAT_HPP
#define EPIWORLD_ENTITY_DISTRIBUTE_MEAT_HPP


template <typename TSeq = EPI_DEFAULT_TSEQ>
/**
 * Distributes an entity to unassigned agents in the model.
 * 
 * @param prevalence The proportion of agents to distribute the entity to.
 * @param as_proportion Flag indicating whether the prevalence is a proportion
 * @param to_unassigned Flag indicating whether to distribute the entity only
 * to unassigned agents.
 * @return An EntityToAgentFun object that distributes the entity to unassigned
 * agents.
 */
inline EntityToAgentFun<TSeq> distribute_entity_randomly(
    epiworld_double prevalence,
    bool as_proportion,
    bool to_unassigned
)
{

    return [prevalence, as_proportion, to_unassigned](
        Entity<TSeq> & e, Model<TSeq> * m
        ) -> void {

        
        // Preparing the sampling space
        std::vector< size_t > idx;
        if (to_unassigned)
        {
            for (const auto & a: m->get_agents())
                if (a.get_n_entities() == 0)
                    idx.push_back(a.get_id());
        } 
        else
        {

            for (const auto & a: m->get_agents())
                idx.push_back(a.get_id());

        }
        
        size_t n = idx.size();

        // Figuring out how many to sample
        int n_to_sample;
        if (as_proportion)
        {
            n_to_sample = static_cast<int>(std::floor(prevalence * n));

            // Correcting for possible overflow
            if (n_to_sample > static_cast<int>(n))
                n_to_sample = static_cast<int>(n);

        } else
        {
            n_to_sample = static_cast<int>(prevalence);
            if (n_to_sample > static_cast<int>(n))
                throw std::range_error("There are only " + std::to_string(n) + 
                " individuals in the population. Cannot add the entity to " +
                    std::to_string(n_to_sample));
        }

        int n_left = n;
        for (int i = 0; i < n_to_sample; ++i)
        {
            int loc = static_cast<epiworld_fast_uint>(
                floor(m->runif() * n_left--)
                );

            // Correcting for possible overflow
            if ((loc > 0) && (loc >= n_left))
                loc = n_left - 1;

            m->get_agent(idx[loc]).add_entity(e, m);

            std::swap(idx[loc], idx[n_left]);

        }

    };

}

template<typename TSeq = EPI_DEFAULT_TSEQ>
/**
 * Distributes an entity to a range of agents.
 *
 * @param from The starting index of the range.
 * @param to The ending index of the range.
 * @param to_unassigned Flag indicating whether to distribute the entity only
 * to unassigned agents.
 * @return A lambda function that distributes the entity to the specified range
 * of agents.
 */
inline EntityToAgentFun<TSeq> distribute_entity_to_range(
    int from,
    int to,
    bool to_unassigned = false
    ) {

    if (to_unassigned)
    {

        return [from, to](Entity<TSeq> & e, Model<TSeq> * m) -> void {

            auto & agents = m->get_agents();
            for (size_t i = from; i < to; ++i)
            {
                if (agents[i].get_n_entities() == 0)
                    e.add_agent(&agents[i], m);
                else
                    throw std::logic_error(
                        "Agent " + std::to_string(i) + " already has an entity."
                    );
            }
            
            return;

        };

    }
    else
    {

        return [from, to](Entity<TSeq> & e, Model<TSeq> * m) -> void {

            auto & agents = m->get_agents();
            for (size_t i = from; i < to; ++i)
            {
                e.add_agent(&agents[i], m);
            }
            
            return;

        };

    }
}


template<typename TSeq = EPI_DEFAULT_TSEQ>
inline EntityToAgentFun<TSeq> distribute_entity_to_set(
    std::vector< size_t > & idx
    ) {

    return [idx](Entity<TSeq> & e, Model<TSeq> * m) -> void {

        for (const auto & i: idx)
        {
            e.add_agent(&m->get_agent(i), m);
        }

    };

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/entity-distribute-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/entity-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_ENTITY_MEAT_HPP
#define EPIWORLD_ENTITY_MEAT_HPP

template<typename TSeq>
inline void Entity<TSeq>::add_agent(
    Agent<TSeq> & p,
    Model<TSeq> * model
    )
{

    // Need to add it to the events, through the individual
    p.add_entity(*this, model);    

}

template<typename TSeq>
inline void Entity<TSeq>::add_agent(
    Agent<TSeq> * p,
    Model<TSeq> * model
    )
{
    p->add_entity(*this, model);
}

template<typename TSeq>
inline void Entity<TSeq>::rm_agent(size_t idx, Model<TSeq> * model)
{
    if (idx >= n_agents)
        throw std::out_of_range(
            "Trying to remove agent "+ std::to_string(idx) +
            " out of " + std::to_string(n_agents)
            );

    model->get_agents()[agents[idx]].rm_entity(*this, model);

    return;
}

template<typename TSeq>
inline size_t Entity<TSeq>::size() const noexcept
{
    return n_agents;
}

template<typename TSeq>
inline void Entity<TSeq>::set_location(std::vector< epiworld_double > loc)
{
    location = loc;
}

template<typename TSeq>
inline std::vector< epiworld_double > & Entity<TSeq>::get_location()
{
    return location;
}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator Entity<TSeq>::begin()
{

    if (n_agents == 0)
        return agents.end();

    return agents.begin();

}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator Entity<TSeq>::end()
{
    return agents.begin() + n_agents;
}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::const_iterator Entity<TSeq>::begin() const
{

    if (n_agents == 0)
        return agents.end();

    return agents.begin();

}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::const_iterator Entity<TSeq>::end() const
{
    return agents.begin() + n_agents;
}

template<typename TSeq>
size_t Entity<TSeq>::operator[](size_t i)
{
    if (n_agents <= i)
        throw std::logic_error(
            "There are not that many agents in this entity. " +
            std::to_string(n_agents) + " <= " + std::to_string(i)
            );

    return i;
}

template<typename TSeq>
inline int Entity<TSeq>::get_id() const noexcept
{
    return id;
}

template<typename TSeq>
inline const std::string & Entity<TSeq>::get_name() const noexcept
{
    return entity_name;
}

template<typename TSeq>
inline void Entity<TSeq>::set_state(
    epiworld_fast_int init,
    epiworld_fast_int end
)
{
    state_init = init;
    state_post = end;
}

template<typename TSeq>
inline void Entity<TSeq>::set_queue(
    epiworld_fast_int init,
    epiworld_fast_int end
)
{
    queue_init = init;
    queue_post = end;
}

template<typename TSeq>
inline void Entity<TSeq>::get_state(
    epiworld_fast_int * init,
    epiworld_fast_int * post
)
{
    if (init != nullptr)
        *init = state_init;

    if (post != nullptr)
        *post = state_post;

}

template<typename TSeq>
inline void Entity<TSeq>::get_queue(
    epiworld_fast_int * init,
    epiworld_fast_int * post
)
{
    if (init != nullptr)
        *init = queue_init;

    if (post != nullptr)
        *post = queue_post;

}

template<typename TSeq>
inline void Entity<TSeq>::reset()
{
    sampled_agents.clear();
    sampled_agents_n = 0u;
    sampled_agents_left.clear();
    sampled_agents_left_n = 0u;

    this->agents.clear();
    this->n_agents = 0u;
    this->agents_location.clear();

    return;

}

template<typename TSeq>
inline bool Entity<TSeq>::operator==(const Entity<TSeq> & other) const
{

    if (id != other.id)
        return false;

    if (n_agents != other.n_agents)
        return false;

    for (size_t i = 0u; i < n_agents; ++i)
    {
        if (agents[i] != other.agents[i])
            return false;
    }


    if (max_capacity != other.max_capacity)
        return false;

    if (entity_name != other.entity_name)
        return false;

    if (location.size() != other.location.size())
        return false;

    for (size_t i = 0u; i < location.size(); ++i)
    {

        if (location[i] != other.location[i])
            return false;

    }

    if (state_init != other.state_init)
        return false;

    if (state_post != other.state_post)
        return false;

    if (queue_init != other.queue_init)
        return false;

    if (queue_post != other.queue_post)
        return false;

    return true;

}

template<typename TSeq>
inline void Entity<TSeq>::distribute(Model<TSeq> * model)
{

    if (dist_fun)
    {

        dist_fun(*this, model);

    }

}

template<typename TSeq>
inline std::vector< size_t > & Entity<TSeq>::get_agents()
{
    return agents;
}

template<typename TSeq>
inline void Entity<TSeq>::print() const
{

    printf_epiworld(
        "Entity '%s' (id %i) with %i agents.\n",
        this->entity_name.c_str(),
        static_cast<int>(id),
        static_cast<int>(n_agents)
    );
}

template<typename TSeq>
inline void Entity<TSeq>::set_distribution(EntityToAgentFun<TSeq> fun)
{
    dist_fun = fun;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/entity-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/entities-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_ENTITIES_BONES_HPP
#define EPIWORLD_ENTITIES_BONES_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Agent;


/**
 * @brief Set of Entities (useful for building iterators)
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class Entities {
    friend class Entity<TSeq>;
    friend class Agent<TSeq>;
private:
    std::vector< Entity<TSeq> * >  dat;
    const size_t n_entities;

public:

    Entities() = delete;
    Entities(Agent<TSeq> & p);

    typename std::vector< Entity<TSeq> * >::iterator begin();
    typename std::vector< Entity<TSeq> * >::iterator end();

    Entity<TSeq> & operator()(size_t i);
    Entity<TSeq> & operator[](size_t i);

    size_t size() const noexcept;

    bool operator==(const Entities<TSeq> & other) const;

};

template<typename TSeq>
inline Entities<TSeq>::Entities(Agent<TSeq> & p) :
    n_entities(p.get_n_entities())
{

    dat.reserve(n_entities);
    for (size_t i = 0u; i < n_entities; ++i)
        dat.push_back(&p.get_entity(i));

}

template<typename TSeq>
inline typename std::vector< Entity<TSeq>* >::iterator Entities<TSeq>::begin()
{

    if (n_entities == 0u)
        return dat.end();
    
    return dat.begin();
}

template<typename TSeq>
inline typename std::vector< Entity<TSeq>* >::iterator Entities<TSeq>::end()
{
     
    return begin() + n_entities;
}

template<typename TSeq>
inline Entity<TSeq> & Entities<TSeq>::operator()(size_t i)
{

    if (i >= n_entities)
        throw std::range_error("Entity index out of range.");

    return *dat[i];

}

template<typename TSeq>
inline Entity<TSeq> & Entities<TSeq>::operator[](size_t i)
{

    return *dat[i];

}

template<typename TSeq>
inline size_t Entities<TSeq>::size() const noexcept 
{
    return n_entities;
}

template<typename TSeq>
inline bool Entities<TSeq>::operator==(const Entities<TSeq> & other) const
{

    if (n_entities != other.n_entities)
        return false;

    for (size_t i = 0u; i < dat.size(); ++i)
    {
        if (dat[i] != other.dat[i])
            return false;
    }

    return true;
}

/**
 * @brief Set of Entities (const) (useful for iterators)
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class Entities_const {
    friend class Virus<TSeq>;
    friend class Agent<TSeq>;
private:
    const std::vector< Entity<TSeq>* > dat;
    const size_t n_entities;

public:

    Entities_const() = delete;
    Entities_const(const Agent<TSeq> & p);

    typename std::vector< Entity<TSeq>* >::const_iterator begin();
    typename std::vector< Entity<TSeq>* >::const_iterator end();

    const Entity<TSeq> & operator()(size_t i);
    const Entity<TSeq> & operator[](size_t i);

    size_t size() const noexcept;

    bool operator==(const Entities_const<TSeq> & other) const;

};

template<typename TSeq>
inline Entities_const<TSeq>::Entities_const(const Agent<TSeq> & p) :
    n_entities(p.get_n_entities())
{

    dat.reserve(n_entities);
    for (size_t i = 0u; i < n_entities; ++i)
        dat.push_back(&p.get_entity(i));

}

template<typename TSeq>
inline typename std::vector< Entity<TSeq>* >::const_iterator Entities_const<TSeq>::begin() {

    if (n_entities == 0u)
        return dat.end();
    
    return dat.begin();
}

template<typename TSeq>
inline typename std::vector< Entity<TSeq>* >::const_iterator Entities_const<TSeq>::end() {
     
    return begin() + n_entities;
}

template<typename TSeq>
inline const Entity<TSeq> & Entities_const<TSeq>::operator()(size_t i)
{

    if (i >= n_entities)
        throw std::range_error("Entity index out of range.");

    return *dat[i];

}

template<typename TSeq>
inline const Entity<TSeq> & Entities_const<TSeq>::operator[](size_t i)
{

    return *dat[i];

}

template<typename TSeq>
inline size_t Entities_const<TSeq>::size() const noexcept 
{
    return n_entities;
}

template<typename TSeq>
inline bool Entities_const<TSeq>::operator==(const Entities_const<TSeq> & other) const
{
    
    if (n_entities != other.n_entities)
        return false;

    for (size_t i = 0u; i < dat.size(); ++i)
    {
        if (dat[i] != other.dat[i])
            return false;
    }

    return true;
}


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/entities-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


    
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/agent-meat-state.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSON_MEAT_STATE_HPP
#define EPIWORLD_PERSON_MEAT_STATE_HPP

// template<typename TSeq>
// class Model;

// template<typename TSeq>
// class Agent;


/**
 * @file agent-meat-state.hpp
 * @author George G. Vega Yon (g.vegayon en gmail)
 * @brief Sampling functions are getting big, so we keep them in a separate file.
 * @version 0.1
 * @date 2022-06-15
 * 
 * @copyright Copyright (c) 2022
 * 
 */
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//agent-meat-virus-sampling.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_AGENT_MEAT_VIRUS_SAMPLING
#define EPIWORLD_AGENT_MEAT_VIRUS_SAMPLING

/**
 * @brief Functions for sampling viruses
 * 
 */
namespace sampler {

/**
 * @brief Make a function to sample from neighbors
 * 
 * This is akin to the function default_update_susceptible, with the difference
 * that it will create a function that supports excluding states from the sampling
 * frame. For example, individuals who have acquired a virus can be excluded if
 * in incubation state.
 * 
 * @tparam TSeq 
 * @param exclude unsigned vector of states that need to be excluded from the sampling
 * @return Virus<TSeq>* of the selected virus. If none selected (or none
 * available,) returns a nullptr;
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline std::function<void(Agent<TSeq>*,Model<TSeq>*)> make_update_susceptible(
    std::vector< epiworld_fast_uint > exclude = {}
    )
{
  

    if (exclude.size() == 0u)
    {

        std::function<void(Agent<TSeq>*,Model<TSeq>*)> sampler =
            [](Agent<TSeq> * p, Model<TSeq> * m) -> void
            {

                if (p->get_virus() != nullptr)
                    throw std::logic_error(
                        std::string("Using the -default_update_susceptible- on agents WITH viruses makes no sense! ") +
                        std::string("Agent id ") + std::to_string(p->get_id()) +
                        std::string(" has a virus.")
                        );

                // This computes the prob of getting any neighbor variant
                size_t nviruses_tmp = 0u;
                for (auto & neighbor: p->get_neighbors()) 
                {
                    
                    auto & v = neighbor->get_virus();
                    if (v == nullptr)
                        continue;
                    
                    /* And it is a function of susceptibility_reduction as well */ 
                    m->array_double_tmp[nviruses_tmp] =
                        (1.0 - p->get_susceptibility_reduction(v, m)) * 
                        v->get_prob_infecting(m) * 
                        (1.0 - neighbor->get_transmission_reduction(v, m)) 
                        ; 
                
                    m->array_virus_tmp[nviruses_tmp++] = &(*v);
                        
                }

                // No virus to compute
                if (nviruses_tmp == 0u)
                    return;

                // Running the roulette
                int which = roulette(nviruses_tmp, m);

                if (which < 0)
                    return;

                p->set_virus(*m->array_virus_tmp[which], m);

                return; 
            };

        return sampler;

    } else {

        // Making room for the query
        std::shared_ptr<std::vector<bool>> exclude_agent_bool =
            std::make_shared<std::vector<bool>>(0);

        std::shared_ptr<std::vector<epiworld_fast_uint>> exclude_agent_bool_idx =
            std::make_shared<std::vector<epiworld_fast_uint>>(exclude);

        std::function<void(Agent<TSeq>*,Model<TSeq>*)> sampler =
            [exclude_agent_bool,exclude_agent_bool_idx](Agent<TSeq> * p, Model<TSeq> * m) -> void
            {

                // The first time we call it, we need to initialize the vector
                if (exclude_agent_bool->size() == 0u)
                {

                    exclude_agent_bool->resize(m->get_states().size(), false);
                    for (auto s : *exclude_agent_bool_idx)
                    {
                        if (s >= exclude_agent_bool->size())
                            throw std::logic_error(
                                std::string("You are trying to exclude a state that is out of range: ") +
                                std::to_string(s) + std::string(". There are only ") +
                                std::to_string(exclude_agent_bool->size()) + 
                                std::string(" states in the model.")
                                );

                        exclude_agent_bool->operator[](s) = true;

                    }

                }                    

                if (p->get_virus() != nullptr)
                    throw std::logic_error(
                        std::string("Using the -default_update_susceptible- on agents WITH viruses makes no sense! ") +
                        std::string("Agent id ") + std::to_string(p->get_id()) +
                        std::string(" has a virus.")
                        );

                // This computes the prob of getting any neighbor variant
                size_t nviruses_tmp = 0u;
                for (auto & neighbor: p->get_neighbors()) 
                {

                    // If the state is in the list, exclude it
                    if (exclude_agent_bool->operator[](neighbor->get_state()))
                        continue;

                    auto & v = neighbor->get_virus();
                    if (v == nullptr)
                        continue;
                            
                
                    /* And it is a function of susceptibility_reduction as well */ 
                    m->array_double_tmp[nviruses_tmp] =
                        (1.0 - p->get_susceptibility_reduction(v, m)) * 
                        v->get_prob_infecting(m) * 
                        (1.0 - neighbor->get_transmission_reduction(v, m)) 
                        ; 
                
                    m->array_virus_tmp[nviruses_tmp++] = &(*v);
                    
                }

                // No virus to compute
                if (nviruses_tmp == 0u)
                    return;

                // Running the roulette
                int which = roulette(nviruses_tmp, m);

                if (which < 0)
                    return;

                p->set_virus(*m->array_virus_tmp[which], m); 

                return;

            };

        return sampler;

    }
    
}

/**
 * @brief Make a function to sample from neighbors
 * 
 * This is akin to the function default_update_susceptible, with the difference
 * that it will create a function that supports excluding states from the sampling
 * frame. For example, individuals who have acquired a virus can be excluded if
 * in incubation state.
 * 
 * @tparam TSeq 
 * @param exclude unsigned vector of states that need to be excluded from the sampling
 * @return Virus<TSeq>* of the selected virus. If none selected (or none
 * available,) returns a nullptr;
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline std::function<Virus<TSeq>*(Agent<TSeq>*,Model<TSeq>*)> make_sample_virus_neighbors(
    std::vector< epiworld_fast_uint > exclude = {}
)
{
    if (exclude.size() == 0u)
    {

        std::function<Virus<TSeq>*(Agent<TSeq>*,Model<TSeq>*)> res = 
            [](Agent<TSeq> * p, Model<TSeq> * m) -> Virus<TSeq>* {

                if (p->get_virus() != nullptr)
                    throw std::logic_error(
                        std::string("Using the -default_update_susceptible- on agents WITH viruses makes no sense! ") +
                        std::string("Agent id ") + std::to_string(p->get_id()) +
                        std::string(" has a virus.")
                        );

                // This computes the prob of getting any neighbor variant
                size_t nviruses_tmp = 0u;
                for (auto & neighbor: p->get_neighbors()) 
                {
                    
                    if (neighbor->get_virus() == nullptr)
                        continue;

                    auto & v = neighbor->get_virus();

                    #ifdef EPI_DEBUG
                    if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
                        throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                    #endif
                        
                    /* And it is a function of susceptibility_reduction as well */ 
                    m->array_double_tmp[nviruses_tmp] =
                        (1.0 - p->get_susceptibility_reduction(v, m)) * 
                        v->get_prob_infecting(m) * 
                        (1.0 - neighbor->get_transmission_reduction(v, m)) 
                        ; 
                
                    m->array_virus_tmp[nviruses_tmp++] = &(*v);
                    
                }

                // No virus to compute
                if (nviruses_tmp == 0u)
                    return nullptr;

                // Running the roulette
                int which = roulette(nviruses_tmp, m);

                if (which < 0)
                    return nullptr;

                return m->array_virus_tmp[which]; 

            };

        return res;


    } else {

        // Making room for the query
        std::shared_ptr<std::vector<bool>> exclude_agent_bool =
            std::make_shared<std::vector<bool>>(0);

        std::shared_ptr<std::vector<epiworld_fast_uint>> exclude_agent_bool_idx =
            std::make_shared<std::vector<epiworld_fast_uint>>(exclude);


        std::function<Virus<TSeq>*(Agent<TSeq>*,Model<TSeq>*)> res = 
            [exclude_agent_bool,exclude_agent_bool_idx](Agent<TSeq> * p, Model<TSeq> * m) -> Virus<TSeq>* {

                // The first time we call it, we need to initialize the vector
                if (exclude_agent_bool->size() == 0u)
                {

                    exclude_agent_bool->resize(m->get_states().size(), false);
                    for (auto s : *exclude_agent_bool_idx)
                    {
                        if (s >= exclude_agent_bool->size())
                            throw std::logic_error(
                                std::string("You are trying to exclude a state that is out of range: ") +
                                std::to_string(s) + std::string(". There are only ") +
                                std::to_string(exclude_agent_bool->size()) + 
                                std::string(" states in the model.")
                                );

                        exclude_agent_bool->operator[](s) = true;

                    }

                }    
                
                if (p->get_virus() != nullptr)
                    throw std::logic_error(
                        std::string("Using the -default_update_susceptible- on agents WITH viruses makes no sense! ") +
                        std::string("Agent id ") + std::to_string(p->get_id()) +
                        std::string(" has a virus.")
                        );

                // This computes the prob of getting any neighbor variant
                size_t nviruses_tmp = 0u;
                for (auto & neighbor: p->get_neighbors()) 
                {

                    // If the state is in the list, exclude it
                    if (exclude_agent_bool->operator[](neighbor->get_state()))
                        continue;

                    if (neighbor->get_virus() == nullptr)
                        continue;

                    auto & v = neighbor->get_virus();
                            
                    #ifdef EPI_DEBUG
                    if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
                        throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                    #endif
                        
                    /* And it is a function of susceptibility_reduction as well */ 
                    m->array_double_tmp[nviruses_tmp] =
                        (1.0 - p->get_susceptibility_reduction(v, m)) * 
                        v->get_prob_infecting(m) * 
                        (1.0 - neighbor->get_transmission_reduction(v, m)) 
                        ; 
                
                    m->array_virus_tmp[nviruses_tmp++] = &(*v);
                    
                }

                // No virus to compute
                if (nviruses_tmp == 0u)
                    return nullptr;

                // Running the roulette
                int which = roulette(nviruses_tmp, m);

                if (which < 0)
                    return nullptr;

                return m->array_virus_tmp[which]; 

            };

        return res;

    }

}

/**
 * @brief Sample from neighbors pool of viruses (at most one)
 * 
 * This function samples at most one virus from the pool of
 * viruses from its neighbors. If no virus is selected, the function
 * returns a `nullptr`, otherwise it returns a pointer to the
 * selected virus.
 * 
 * This can be used to build a new update function (EPI_NEW_UPDATEFUN.)
 * 
 * @tparam TSeq 
 * @param p Pointer to person 
 * @param m Pointer to the model
 * @return Virus<TSeq>* of the selected virus. If none selected (or none
 * available,) returns a nullptr;
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline Virus<TSeq> * sample_virus_single(Agent<TSeq> * p, Model<TSeq> * m)
{

    if (p->get_virus() != nullptr)
        throw std::logic_error(
            std::string("Using the -default_update_susceptible- on agents WITH viruses makes no sense!") +
            std::string("Agent id ") + std::to_string(p->get_id()) +
            std::string(" has a virus.")
            );

    // This computes the prob of getting any neighbor variant
    size_t nviruses_tmp = 0u;
    for (auto & neighbor: p->get_neighbors()) 
    {   
        #ifdef EPI_DEBUG
        int _vcount_neigh = 0;
        #endif                

        if (neighbor->get_virus() == nullptr)
            continue;

        auto & v = neighbor->get_virus();

        #ifdef EPI_DEBUG
        if (nviruses_tmp >= m->array_virus_tmp.size())
            throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
        #endif
            
        /* And it is a function of susceptibility_reduction as well */ 
        m->array_double_tmp[nviruses_tmp] =
            (1.0 - p->get_susceptibility_reduction(v, m)) * 
            v->get_prob_infecting(m) * 
            (1.0 - neighbor->get_transmission_reduction(v, m)) 
            ; 
    
        m->array_virus_tmp[nviruses_tmp++] = &(*v);

        #ifdef EPI_DEBUG
        if (
            (m->array_double_tmp[nviruses_tmp - 1] < 0.0) |
            (m->array_double_tmp[nviruses_tmp - 1] > 1.0)
            )
        {
            printf_epiworld(
                "[epi-debug] Agent %i's virus %i has transmission prob outside of [0, 1]: %.4f!\n",
                static_cast<int>(neighbor->get_id()),
                static_cast<int>(_vcount_neigh++),
                m->array_double_tmp[nviruses_tmp - 1]
                );
        }
        #endif
            
    }


    // No virus to compute
    if (nviruses_tmp == 0u)
        return nullptr;

    #ifdef EPI_DEBUG
    m->get_db().n_transmissions_potential++;
    #endif

    // Running the roulette
    int which = roulette(nviruses_tmp, m);

    if (which < 0)
        return nullptr;

    #ifdef EPI_DEBUG
    m->get_db().n_transmissions_today++;
    #endif

    return m->array_virus_tmp[which]; 
    
}

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//agent-meat-virus-sampling.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/




template<typename TSeq = EPI_DEFAULT_TSEQ>
inline void default_update_susceptible(
    Agent<TSeq> * p,
    Model<TSeq> * m
    )
{

    Virus<TSeq> * virus = sampler::sample_virus_single<TSeq>(p, m);
    
    if (virus == nullptr)
        return;

    p->set_virus(*virus, m); 

    return;

}

template<typename TSeq = EPI_DEFAULT_TSEQ>
inline void default_update_exposed(Agent<TSeq> * p, Model<TSeq> * m) {

    if (p->get_virus() == nullptr)
        throw std::logic_error(
            std::string("Using the -default_update_exposed- on agents WITHOUT viruses makes no sense! ") +
            std::string("Agent id ") + std::to_string(p->get_id()) + std::string(" has no virus registered.")
            );

    // Die
    auto & virus = p->get_virus();
    m->array_double_tmp[0u] = 
        virus->get_prob_death(m) * (1.0 - p->get_death_reduction(virus, m)); 

    // Recover
    m->array_double_tmp[1u] = 
        1.0 - (1.0 - virus->get_prob_recovery(m)) * (1.0 - p->get_recovery_enhancer(virus, m)); 
    

    // Running the roulette
    int which = roulette(2u, m);

    if (which < 0)
        return;

    // Which roulette happen?
    if (which == 0u) // If odd
    {

        p->rm_agent_by_virus(m);
        
    } else {

        p->rm_virus(m);

    }

    return ;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/agent-meat-state.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/agent-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSON_BONES_HPP
#define EPIWORLD_PERSON_BONES_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Virus;

template<typename TSeq>
class Viruses;

template<typename TSeq>
class Viruses_const;

template<typename TSeq>
class Tool;

template<typename TSeq>
class Tools;

template<typename TSeq>
class Tools_const;

template<typename TSeq>
class Queue;

template<typename TSeq>
struct Event;

template<typename TSeq>
class Entity;

template<typename TSeq>
class Entities;

template<typename TSeq>
inline void default_add_virus(Event<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_add_tool(Event<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_add_entity(Event<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_virus(Event<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_tool(Event<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_entity(Event<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_change_state(Event<TSeq> & a, Model<TSeq> * m);



/**
 * @brief Agent (agents)
 * 
 * @tparam TSeq Sequence type (should match `TSeq` across the model)
 */
template<typename TSeq>
class Agent {
    friend class Model<TSeq>;
    friend class Virus<TSeq>;
    friend class Tool<TSeq>;
    friend class Tools<TSeq>;
    friend class Tools_const<TSeq>;
    friend class Queue<TSeq>;
    friend class Entities<TSeq>;
    friend class AgentsSample<TSeq>;
    friend void default_add_virus<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_add_tool<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_add_entity<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_virus<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_tool<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_entity<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
    friend void default_change_state<TSeq>(Event<TSeq> & a, Model<TSeq> * m);
private:
    
    Model<TSeq> * model;

    std::vector< size_t > neighbors;
    std::vector< size_t > neighbors_locations;
    size_t n_neighbors = 0u;

    std::vector< size_t > entities;
    std::vector< size_t > entities_locations;
    size_t n_entities = 0u;

    epiworld_fast_uint state = 0u;
    epiworld_fast_uint state_prev = 0u; ///< For accounting, if need to undo a change.
    
    int state_last_changed = -1; ///< Last time the agent was updated.
    int id = -1;
    
    VirusPtr<TSeq> virus = nullptr;

    std::vector< ToolPtr<TSeq> > tools;
    epiworld_fast_uint n_tools = 0u;

    std::vector< Agent<TSeq> * > sampled_agents = {};
    size_t sampled_agents_n      = 0u;
    std::vector< size_t > sampled_agents_left = {};
    size_t sampled_agents_left_n = 0u;
    int date_last_build_sample   = -99;

public:

    Agent();
    Agent(Agent<TSeq> && p);
    Agent(const Agent<TSeq> & p);
    Agent<TSeq> & operator=(const Agent<TSeq> & other_agent);

    /**
     * @name Add/Remove Virus/Tool
     * 
     * Any of these is ultimately reflected at the end of the iteration.
     * 
     * @param tool Tool to add
     * @param virus Virus to add
     * @param state_new state after the change
     * @param queue 
     */
    ///@{
    void add_tool(
        ToolPtr<TSeq> tool,
        Model<TSeq> * model,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
        );

    void add_tool(
        Tool<TSeq> tool,
        Model<TSeq> * model,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
        );

    void set_virus(
        VirusPtr<TSeq> virus,
        Model<TSeq> * model,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
        );

    void set_virus(
        Virus<TSeq> virus,
        Model<TSeq> * model,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
        );

    void add_entity(
        Entity<TSeq> & entity,
        Model<TSeq> * model,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
        );

    void rm_tool(
        epiworld_fast_uint tool_idx,
        Model<TSeq> * model,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_tool(
        ToolPtr<TSeq> & tool,
        Model<TSeq> * model,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_virus(
        Model<TSeq> * model,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_entity(
        epiworld_fast_uint entity_idx,
        Model<TSeq> * model,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_entity(
        Entity<TSeq> & entity,
        Model<TSeq> * model,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_agent_by_virus(
        Model<TSeq> * model,
        epiworld_fast_int state_new = -99,
        epiworld_fast_int queue = -99
    ); ///< Agent removed by virus
    ///@}
    
    /**
     * @name Get the rates (multipliers) for the agent
     * 
     * @param v A pointer to a virus.
     * @return epiworld_double 
     */
    ///@{
    epiworld_double get_susceptibility_reduction(VirusPtr<TSeq> v, Model<TSeq> * model);
    epiworld_double get_transmission_reduction(VirusPtr<TSeq> v, Model<TSeq> * model);
    epiworld_double get_recovery_enhancer(VirusPtr<TSeq> v, Model<TSeq> * model);
    epiworld_double get_death_reduction(VirusPtr<TSeq> v, Model<TSeq> * model);
    ///@}

    int get_id() const; ///< Id of the individual

    VirusPtr<TSeq> & get_virus();
    const VirusPtr<TSeq> & get_virus() const;

    ToolPtr<TSeq> & get_tool(int i);
    Tools<TSeq> get_tools();
    const Tools_const<TSeq> get_tools() const;
    size_t get_n_tools() const noexcept;

    void mutate_virus();
    void add_neighbor(
        Agent<TSeq> & p,
        bool check_source = true,
        bool check_target = true
        );

    /**
     * @brief Swaps neighbors between the current agent and agent `other`
     * 
     * @param other 
     * @param n_this 
     * @param n_other 
     */
    void swap_neighbors(
        Agent<TSeq> & other,
        size_t n_this,
        size_t n_other
    );

    std::vector< Agent<TSeq> * > get_neighbors();
    size_t get_n_neighbors() const;

    void change_state(
        Model<TSeq> * model,
        epiworld_fast_uint new_state,
        epiworld_fast_int queue = 0
        );

    const epiworld_fast_uint & get_state() const;

    void reset();

    bool has_tool(epiworld_fast_uint t) const;
    bool has_tool(std::string name) const;
    bool has_tool(const Tool<TSeq> & t) const;
    bool has_virus(epiworld_fast_uint t) const;
    bool has_virus(std::string name) const;
    bool has_virus(const Virus<TSeq> & v) const;
    bool has_entity(epiworld_fast_uint t) const;
    bool has_entity(std::string name) const;

    void print(Model<TSeq> * model, bool compressed = false) const;

    /**
     * @brief Access the j-th column of the agent
     * 
     * If an external array has been specified, then these two
     * functions can be used to access additional agent's features 
     * not included in the model.
     * 
     * The `operator[]` method is with no boundary check, whereas
     * the `operator()` method checks boundaries. The former can result
     * in a segfault.
     * 
     * 
     * @param j 
     * @return double& 
     */
    ///@{
    double & operator()(size_t j);
    double & operator[](size_t j);
    double operator()(size_t j) const;
    double operator[](size_t j) const;
    ///@}

    Entities<TSeq> get_entities();
    const Entities_const<TSeq> get_entities() const;
    const Entity<TSeq> & get_entity(size_t i) const;
    Entity<TSeq> & get_entity(size_t i);
    size_t get_n_entities() const;

    bool operator==(const Agent<TSeq> & other) const;
    bool operator!=(const Agent<TSeq> & other) const {return !operator==(other);};

};



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/agent-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/agent-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSON_MEAT_HPP
#define EPIWORLD_PERSON_MEAT_HPP

#define CHECK_COALESCE_(proposed_, virus_tool_, alt_) \
    if (static_cast<int>(proposed_) == -99) {\
        if (static_cast<int>(virus_tool_) == -99) \
            (proposed_) = (alt_);\
        else (proposed_) = (virus_tool_);}

// To large to add directly here
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//agent-events-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_AGENT_EVENTS_MEAT_HPP
#define EPIWORLD_AGENT_EVENTS_MEAT_HPP

template<typename TSeq>
inline void default_add_virus(Event<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> *  p = a.agent;
    VirusPtr<TSeq> v = a.virus;

    // Has a agent? If so, we need to register the transmission
    if (v->get_agent())
    {

        // ... only if not the same agent
        if (v->get_agent()->get_id() != p->get_id())
            m->get_db().record_transmission(
                v->get_agent()->get_id(),
                p->get_id(),
                v->get_id(),
                v->get_date() 
            );

    }
    
    p->virus = std::make_shared< Virus<TSeq> >(*v);
    p->virus->set_date(m->today());
    p->virus->set_agent(p);

    // Change of state needs to be recorded and updated on the
    // tools.
    if (p->state_prev != p->state)
    {
        auto & db = m->get_db();
        db.update_state(p->state_prev, p->state);

        for (size_t i = 0u; i < p->n_tools; ++i)
            db.update_tool(p->tools[i]->get_id(), p->state_prev, p->state);
    }

    // Lastly, we increase the daily count of the virus
    #ifdef EPI_DEBUG
    m->get_db().today_virus.at(v->get_id()).at(p->state)++;
    #else
    m->get_db().today_virus[v->get_id()][p->state]++;
    #endif

}

template<typename TSeq>
inline void default_add_tool(Event<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p = a.agent;
    ToolPtr<TSeq> t = a.tool;
    
    // Update tool accounting
    p->n_tools++;
    size_t n_tools = p->n_tools;

    if (n_tools <= p->tools.size())
        p->tools[n_tools - 1] = std::make_shared< Tool<TSeq> >(*t);
    else
        p->tools.push_back(std::make_shared< Tool<TSeq> >(*t));

    n_tools--;

    p->tools[n_tools]->set_date(m->today());
    p->tools[n_tools]->set_agent(p, n_tools);

    // Change of state needs to be recorded and updated on the
    // tools.
    if (p->state_prev != p->state)
    {
        auto & db = m->get_db();
        db.update_state(p->state_prev, p->state);

        if (p->virus)
            db.update_virus(p->virus->get_id(), p->state_prev, p->state);
    }

    m->get_db().today_tool[t->get_id()][p->state]++;


}

template<typename TSeq>
inline void default_rm_virus(Event<TSeq> & a, Model<TSeq> * model)
{

    Agent<TSeq> * p    = a.agent;
    VirusPtr<TSeq> & v = a.virus;

    // Calling the virus action over the removed virus
    v->post_recovery(model);

    p->virus = nullptr;

    // Change of state needs to be recorded and updated on the
    // tools.
    if (p->state_prev != p->state)
    {
        auto & db = model->get_db();
        db.update_state(p->state_prev, p->state);

        for (size_t i = 0u; i < p->n_tools; ++i)
            db.update_tool(p->tools[i]->get_id(), p->state_prev, p->state);
    }

    // The counters of the virus only needs to decrease
    #ifdef EPI_DEBUG
    model->get_db().today_virus.at(v->get_id()).at(p->state_prev)--;
    #else
    model->get_db().today_virus[v->get_id()][p->state_prev]--;
    #endif

    
    return;

}

template<typename TSeq>
inline void default_rm_tool(Event<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p   = a.agent;    
    ToolPtr<TSeq> & t = a.agent->tools[a.tool->pos_in_agent];

    if (--p->n_tools > 0)
    {
        p->tools[p->n_tools]->pos_in_agent = t->pos_in_agent;
        std::swap(
            p->tools[t->pos_in_agent],
            p->tools[p->n_tools]
            );
    }

    // Change of state needs to be recorded and updated on the
    // tools.
    if (p->state_prev != p->state)
    {
        auto & db = m->get_db();
        db.update_state(p->state_prev, p->state);

        if (p->virus)
            db.update_virus(p->virus->get_id(), p->state_prev, p->state);
    }

    // Lastly, we increase the daily count of the tool
    #ifdef EPI_DEBUG
    m->get_db().today_tool.at(t->get_id()).at(p->state_prev)--;
    #else
    m->get_db().today_tool[t->get_id()][p->state_prev]--;
    #endif

    return;

}

template<typename TSeq>
inline void default_change_state(Event<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p = a.agent;

    if (p->state_prev != p->state)
    {
        auto & db = m->get_db();
        db.update_state(p->state_prev, p->state);

        if (p->virus)
            db.update_virus(p->virus->get_id(), p->state_prev, p->state);

        for (size_t i = 0u; i < p->n_tools; ++i)
            db.update_tool(p->tools[i]->get_id(), p->state_prev, p->state);

    }

}

template<typename TSeq>
inline void default_add_entity(Event<TSeq> & a, Model<TSeq> *)
{

    Agent<TSeq> *  p = a.agent;
    Entity<TSeq> * e = a.entity;

    // Checking the agent and the entity are not linked
    if ((p->get_n_entities() > 0) && (e->size() > 0))
    {

        if (p->get_n_entities() > e->size()) // Slower search through the agent
        {
            for (size_t i = 0u; i < e->size(); ++i)
                if(static_cast<int>(e->operator[](i)) == p->get_id())
                    throw std::logic_error("An entity cannot be reassigned to an agent.");
        }
        else                                 // Slower search through the entity
        {
            for (size_t i = 0u; i < p->get_n_entities(); ++i)
                if(p->get_entity(i).get_id() == e->get_id())
                    throw std::logic_error("An entity cannot be reassigned to an agent.");
        }

        // It means that agent and entity were not associated.
    }

    // Adding the entity to the agent
    if (++p->n_entities <= p->entities.size())
    {

        p->entities[p->n_entities - 1]           = e->get_id();
        p->entities_locations[p->n_entities - 1] = e->n_agents;

    } else
    {
        p->entities.push_back(e->get_id());
        p->entities_locations.push_back(e->n_agents);
    }

    // Adding the agent to the entity
    // Adding the entity to the agent
    if (++e->n_agents <= e->agents.size())
    {

        e->agents[e->n_agents - 1]          = p->get_id();
        // Adjusted by '-1' since the list of entities in the agent just grew.
        e->agents_location[e->n_agents - 1] = p->n_entities - 1;

    } else
    {
        e->agents.push_back(p->get_id());
        e->agents_location.push_back(p->n_entities - 1);
    }

    // Today was the last modification
    // e->date_last_add_or_remove = m->today();
    
}

template<typename TSeq>
inline void default_rm_entity(Event<TSeq> & a, Model<TSeq> * m)
{
    
    Agent<TSeq> *  p = a.agent;    
    Entity<TSeq> * e = a.entity;
    size_t idx_agent_in_entity = a.idx_agent;
    size_t idx_entity_in_agent = a.idx_object;

    if (--p->n_entities > 0)
    {

        // When we move the end entity to the new location, the 
        // moved entity needs to reflect the change, i.e., where the
        // entity will now be located in the agent
        size_t agent_location_in_last_entity  =
            p->entities_locations[p->n_entities];

        Entity<TSeq> * last_entity =
            &m->get_entity(p->entities[p->n_entities]); ///< Last entity of the agent

        // The end entity will be located where the removed was
        last_entity->agents_location[agent_location_in_last_entity] =
            idx_entity_in_agent;

        // We now make the swap
        std::swap(
            p->entities[p->n_entities],
            p->entities[idx_entity_in_agent]
        );

    }

    if (--e->n_agents > 0)
    {

        // When we move the end agent to the new location, the 
        // moved agent needs to reflect the change, i.e., where the
        // agent will now be located in the entity
        size_t entity_location_in_last_agent = e->agents_location[e->n_agents];
        
        Agent<TSeq> * last_agent  =
            &m->get_agents()[e->agents[e->n_agents]]; ///< Last agent of the entity

        // The end entity will be located where the removed was
        last_agent->entities_locations[entity_location_in_last_agent] =
            idx_agent_in_entity;

        // We now make the swap
        std::swap(
            e->agents[e->n_agents],
            e->agents[idx_agent_in_entity]
        );

    }

    // Setting the date of the last removal
    // e->date_last_add_or_remove = m->today();

    return;

}
#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//agent-events-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



template<typename TSeq>
inline Agent<TSeq>::Agent() {}

template<typename TSeq>
inline Agent<TSeq>::Agent(Agent<TSeq> && p) :
    model(p.model),
    neighbors(std::move(p.neighbors)),
    neighbors_locations(std::move(p.neighbors_locations)),
    n_neighbors(p.n_neighbors),
    entities(std::move(p.entities)),
    entities_locations(std::move(p.entities_locations)),
    n_entities(p.n_entities),
    state(p.state),
    state_prev(p.state_prev), 
    state_last_changed(p.state_last_changed),
    id(p.id),
    tools(std::move(p.tools)), /// Needs to be adjusted
    n_tools(p.n_tools)
{

    state = p.state;
    id     = p.id;
    
    // Dealing with the virus
    if (p.virus != nullptr)
    {
        virus = std::move(p.virus);
        virus->set_agent(this);
    }

    int loc = 0;
    for (auto & t : tools)
    {
        
        // Will create a copy of the virus, with the exeption of
        // the virus code
        t->agent     = this;
        t->pos_in_agent = loc++;

    }
    
}

template<typename TSeq>
inline Agent<TSeq>::Agent(const Agent<TSeq> & p) :
    model(p.model),
    neighbors(p.neighbors),
    neighbors_locations(p.neighbors_locations),
    n_neighbors(p.n_neighbors),
    entities(p.entities),
    entities_locations(p.entities_locations),
    n_entities(p.n_entities),
    sampled_agents(0u),
    sampled_agents_n(0u),
    sampled_agents_left_n(0u),
    date_last_build_sample(-99)
{

    state = p.state;
    id     = p.id;
    
    // Dealing with the virus
    if (p.virus != nullptr)
    {
        virus = std::make_shared<Virus<TSeq>>(*p.virus);
        virus->set_agent(this);
    }
    

    tools.reserve(p.get_n_tools());
    n_tools = tools.size();
    for (size_t i = 0u; i < n_tools; ++i)
    {
        
        // Will create a copy of the virus, with the exeption of
        // the virus code
        tools.emplace_back(std::make_shared<Tool<TSeq>>(*p.tools[i]));
        tools.back()->set_agent(this, i);

    }
    
}

template<typename TSeq>
inline Agent<TSeq> & Agent<TSeq>::operator=(
    const Agent<TSeq> & other_agent
) 
{

    model = other_agent.model;

    neighbors = other_agent.neighbors;
    neighbors_locations = other_agent.neighbors_locations;
    n_neighbors = other_agent.n_neighbors;

    entities = other_agent.entities;
    entities_locations = other_agent.entities_locations;
    n_entities = other_agent.n_entities;

    sampled_agents.clear();
    sampled_agents_n = 0;
    sampled_agents_left_n = 0;
    date_last_build_sample = -99;

    // neighbors           = other_agent.neighbors;
    // entities            = other_agent.entities;
    // entities_locations  = other_agent.entities_locations;
    // n_entities          = other_agent.n_entities;
    state              = other_agent.state;
    state_prev         = other_agent.state_prev;
    state_last_changed = other_agent.state_last_changed;
    id                  = other_agent.id;
    
    if (other_agent.virus != nullptr)
    {
        virus = std::make_shared<Virus<TSeq>>(*other_agent.virus);
        virus->set_agent(this);
    } else
        virus = nullptr;
    
    // tools               = other_agent.tools;
    n_tools             = other_agent.n_tools;
    for (size_t i = 0u; i < n_tools; ++i)
    {
        tools[i] = std::make_shared<Tool<TSeq>>(*other_agent.tools[i]);
        tools[i]->set_agent(this, i);
    }
    
    return *this;
    
}

template<typename TSeq>
inline void Agent<TSeq>::add_tool(
    ToolPtr<TSeq> tool,
    Model<TSeq> * model,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
) {

    // Checking the virus exists
    if (tool->get_id() >= static_cast<int>(model->get_db().get_n_tools()))
        throw std::range_error("The tool with id: " + std::to_string(tool->get_id()) + 
            " has not been registered. There are only " + std::to_string(model->get_n_tools()) + 
            " included in the model.");

    CHECK_COALESCE_(state_new, tool->state_init, state);
    CHECK_COALESCE_(queue, tool->queue_init, Queue<TSeq>::NoOne);

    model->events_add(
        this, nullptr, tool, nullptr, state_new, queue, default_add_tool<TSeq>, -1, -1
        );

}

template<typename TSeq>
inline void Agent<TSeq>::add_tool(
    Tool<TSeq> tool,
    Model<TSeq> * model,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{
    ToolPtr<TSeq> tool_ptr = std::make_shared< Tool<TSeq> >(tool);
    add_tool(tool_ptr, model, state_new, queue);
}

template<typename TSeq>
inline void Agent<TSeq>::set_virus(
    VirusPtr<TSeq> virus,
    Model<TSeq> * model,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{

    // Checking the virus exists
    if (virus->get_id() >= static_cast<int>(model->get_db().get_n_viruses()))
        throw std::range_error("The virus with id: " + std::to_string(virus->get_id()) + 
            " has not been registered. There are only " + std::to_string(model->get_n_viruses()) + 
            " included in the model.");

    CHECK_COALESCE_(state_new, virus->state_init, state);
    CHECK_COALESCE_(queue, virus->queue_init, Queue<TSeq>::NoOne);

    model->events_add(
        this, virus, nullptr, nullptr, state_new, queue, default_add_virus<TSeq>, -1, -1
        );

}

template<typename TSeq>
inline void Agent<TSeq>::set_virus(
    Virus<TSeq> virus,
    Model<TSeq> * model,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{
    VirusPtr<TSeq> virus_ptr = std::make_shared< Virus<TSeq> >(virus);
    set_virus(virus_ptr, model, state_new, queue);
}

template<typename TSeq>
inline void Agent<TSeq>::add_entity(
    Entity<TSeq> & entity,
    Model<TSeq> * model,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{

    CHECK_COALESCE_(state_new, entity.state_init, state);
    CHECK_COALESCE_(queue, entity.queue_init, Queue<TSeq>::NoOne);

    if (model != nullptr)
    {

        model->events_add(
            this, nullptr, nullptr, &entity, state_new, queue, default_add_entity<TSeq>, -1, -1
        );

    }
    else // If no model is passed, then we assume that we only need to add the
         // model entity
    {

        Event<TSeq> a(
                this, nullptr, nullptr, &entity, state_new, queue, default_add_entity<TSeq>,
                -1, -1
            );

        default_add_entity(a, model); /* passing model makes nothing */

    }

}

template<typename TSeq>
inline void Agent<TSeq>::rm_tool(
    epiworld_fast_uint tool_idx,
    Model<TSeq> * model,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{

    CHECK_COALESCE_(state_new, tools[tool_idx]->state_post, state);
    CHECK_COALESCE_(queue, tools[tool_idx]->queue_post, Queue<TSeq>::NoOne);

    if (tool_idx >= n_tools)
        throw std::range_error(
            "The Tool you want to remove is out of range. This Agent only has " +
            std::to_string(n_tools) + " tools."
        );

    model->events_add(
        this, nullptr, tools[tool_idx], nullptr, state_new, queue, default_rm_tool<TSeq>, -1, -1
        );

}

template<typename TSeq>
inline void Agent<TSeq>::rm_tool(
    ToolPtr<TSeq> & tool,
    Model<TSeq> * model,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{

    if (tool->agent != this)
        throw std::logic_error("Cannot remove a virus from another agent!");

    model->events_add(
        this, nullptr, tool, nullptr, state_new, queue, default_rm_tool<TSeq>, -1, -1
        );

}

template<typename TSeq>
inline void Agent<TSeq>::rm_virus(
    Model<TSeq> * model,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{

    if (virus == nullptr)
        throw std::logic_error(
            "There is no virus to remove here!"
        );

    CHECK_COALESCE_(state_new, virus->state_post, state);
    CHECK_COALESCE_(queue, virus->queue_post, Queue<TSeq>::Everyone);

    model->events_add(
        this, virus, nullptr, nullptr, state_new, queue,
        default_rm_virus<TSeq>, -1, -1
        );
    
}

template<typename TSeq>
inline void Agent<TSeq>::rm_entity(
    epiworld_fast_uint entity_idx,
    Model<TSeq> * model,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{

    if (entity_idx >= n_entities)
        throw std::range_error(
            "The Entity you want to remove is out of range. This Agent only has " +
            std::to_string(n_entities) + " entitites."
        );
    else if (n_entities == 0u)
        throw std::logic_error(
            "There is entity to remove here!"
        );

    CHECK_COALESCE_(state_new, model->get_entity(entity_idx).state_post, state);
    CHECK_COALESCE_(queue, model->get_entity(entity_idx).queue_post, Queue<TSeq>::NoOne);

    model->events_add(
        this,
        nullptr,
        nullptr,
        &model->get_entity(entity_idx),
        state_new,
        queue, 
        default_rm_entity<TSeq>,
        entities_locations[entity_idx],
        entity_idx
    );
}

template<typename TSeq>
inline void Agent<TSeq>::rm_entity(
    Entity<TSeq> & entity,
    Model<TSeq> * model,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{

    // Looking for entity location in the agent
    int entity_idx = -1;
    for (size_t i = 0u; i < n_entities; ++i)
    {
        if (static_cast<int>(entities[i]) == entity.get_id())
        {
            entity_idx = i;
            break;
        }
    }

    if (entity_idx == -1)
        throw std::logic_error(
            std::string("The agent ") +
            std::to_string(id) +
            std::string(" is not associated with entity \"") +
            entity.get_name() +
            std::string("\".")
            );

    CHECK_COALESCE_(state_new, entity.state_post, state);
    CHECK_COALESCE_(queue, entity.queue_post, Queue<TSeq>::NoOne);

    model->events_add(
        this,
        nullptr,
        nullptr,
        &model->entities[entity.get_id()],
        state_new,
        queue, 
        default_rm_entity<TSeq>,
        entities_locations[entity_idx],
        entity_idx
    );
}

template<typename TSeq>
inline void Agent<TSeq>::rm_agent_by_virus(
    Model<TSeq> * model,
    epiworld_fast_int state_new,
    epiworld_fast_int queue
)
{

    CHECK_COALESCE_(state_new, virus->state_removed, state);
    CHECK_COALESCE_(queue, virus->queue_removed, Queue<TSeq>::Everyone);

    model->events_add(
        this, virus, nullptr, nullptr, state_new, queue,
        default_rm_virus<TSeq>, -1, -1
        );

}

template<typename TSeq>
inline epiworld_double Agent<TSeq>::get_susceptibility_reduction(
    VirusPtr<TSeq> v,
    Model<TSeq> * model
) {

    return model->susceptibility_reduction_mixer(this, v, model);
}

template<typename TSeq>
inline epiworld_double Agent<TSeq>::get_transmission_reduction(
    VirusPtr<TSeq> v,
    Model<TSeq> * model
) {
    return model->transmission_reduction_mixer(this, v, model);
}

template<typename TSeq>
inline epiworld_double Agent<TSeq>::get_recovery_enhancer(
    VirusPtr<TSeq> v,
    Model<TSeq> * model
) {
    return model->recovery_enhancer_mixer(this, v, model);
}

template<typename TSeq>
inline epiworld_double Agent<TSeq>::get_death_reduction(
    VirusPtr<TSeq> v,
    Model<TSeq> * model
) {
    return model->death_reduction_mixer(this, v, model);
}

template<typename TSeq>
inline int Agent<TSeq>::get_id() const
{
    return id;
}

template<typename TSeq>
inline VirusPtr<TSeq> & Agent<TSeq>::get_virus() {
    return virus;
}

template<typename TSeq>
inline const VirusPtr<TSeq> & Agent<TSeq>::get_virus() const {
    return virus;
}


template<typename TSeq>
inline Tools<TSeq> Agent<TSeq>::get_tools() {
    return Tools<TSeq>(*this);
}

template<typename TSeq>
inline const Tools_const<TSeq> Agent<TSeq>::get_tools() const {
    return Tools_const<TSeq>(*this);
}

template<typename TSeq>
inline ToolPtr<TSeq> & Agent<TSeq>::get_tool(int i)
{
    return tools.at(i);
}

template<typename TSeq>
inline size_t Agent<TSeq>::get_n_tools() const noexcept
{
    return n_tools;
}

template<typename TSeq>
inline void Agent<TSeq>::mutate_virus()
{

    virus->mutate();

}

template<typename TSeq>
inline void Agent<TSeq>::add_neighbor(
    Agent<TSeq> & p,
    bool check_source,
    bool check_target
) {
    // Can we find the neighbor?
    bool found = false;
    if (check_source)
    {

        for (auto & n: neighbors)    
            if (static_cast<int>(n) == p.get_id())
            {
                found = true;
                break;
            }

    }

    // Three things going on here:
    // - Where in the neighbor will this be
    // - What is the neighbor's id
    // - Increasing the number of neighbors
    if (!found)
    {

        neighbors_locations.push_back(p.get_n_neighbors());
        neighbors.push_back(p.get_id());
        n_neighbors++;

    }


    found = false;
    if (check_target)
    {

        for (auto & n: p.neighbors)
            if (static_cast<int>(n) == id)
            {
                found = true;
                break;
            }
    
    }

    if (!found)
    {

        p.neighbors_locations.push_back(n_neighbors - 1);
        p.neighbors.push_back(id);
        p.n_neighbors++;
        
    }
    

}

template<typename TSeq>
inline void Agent<TSeq>::swap_neighbors(
    Agent<TSeq> & other,
    size_t n_this,
    size_t n_other
)
{

    // Getting the agents
    auto & pop = model->population;
    auto & neigh_this  = pop[neighbors[n_this]];
    auto & neigh_other = pop[other.neighbors[n_other]];

    // Getting the locations in the neighbors
    size_t loc_this_in_neigh = neighbors_locations[n_this];
    size_t loc_other_in_neigh = other.neighbors_locations[n_other];

    // Changing ids
    std::swap(neighbors[n_this], other.neighbors[n_other]);

    if (!model->directed)
    {
        std::swap(
            neigh_this.neighbors[loc_this_in_neigh],
            neigh_other.neighbors[loc_other_in_neigh]
            );

        // Changing the locations
        std::swap(neighbors_locations[n_this], other.neighbors_locations[n_other]);
        
        std::swap(
            neigh_this.neighbors_locations[loc_this_in_neigh],
            neigh_other.neighbors_locations[loc_other_in_neigh]
            );
    }

}

template<typename TSeq>
inline std::vector< Agent<TSeq> *> Agent<TSeq>::get_neighbors()
{
    std::vector< Agent<TSeq> * > res(n_neighbors, nullptr);
    for (size_t i = 0u; i < n_neighbors; ++i)
        res[i] = &model->population[neighbors[i]];

    return res;
}

template<typename TSeq>
inline size_t Agent<TSeq>::get_n_neighbors() const
{
    return n_neighbors;
}

template<typename TSeq>
inline void Agent<TSeq>::change_state(
    Model<TSeq> * model,
    epiworld_fast_uint new_state,
    epiworld_fast_int queue
    )
{

    model->events_add(
        this, nullptr, nullptr, nullptr, new_state, queue,
        default_change_state<TSeq>, -1, -1
    );
    
    return;

}

template<typename TSeq>
inline const epiworld_fast_uint & Agent<TSeq>::get_state() const {
    return state;
}

template<typename TSeq>
inline void Agent<TSeq>::reset()
{

    this->virus = nullptr;

    this->tools.clear();
    n_tools = 0u;

    this->entities.clear();
    this->entities_locations.clear();
    this->n_entities = 0u;

    this->state = 0u;
    this->state_prev = 0u;

    this->state_last_changed = -1;
    
}

template<typename TSeq>
inline bool Agent<TSeq>::has_tool(epiworld_fast_uint t) const
{

    for (auto & tool : tools)
        if (tool->get_id() == static_cast<int>(t))
            return true;

    return false;

}

template<typename TSeq>
inline bool Agent<TSeq>::has_tool(std::string name) const
{

    for (auto & tool : tools)
        if (tool->get_name() == name)
            return true;

    return false;

}

template<typename TSeq>
inline bool Agent<TSeq>::has_tool(const Tool<TSeq> & tool) const
{

    return has_tool(tool.get_id());

}

template<typename TSeq>
inline bool Agent<TSeq>::has_virus(epiworld_fast_uint t) const
{
    if (virus->get_id() == static_cast<int>(t))
        return true;

    return false;
}

template<typename TSeq>
inline bool Agent<TSeq>::has_virus(std::string name) const
{
    
    if (virus->get_name() == name)
        return true;

    return false;

}

template<typename TSeq>
inline bool Agent<TSeq>::has_virus(const Virus<TSeq> & virus) const
{

    return has_virus(virus.get_id());

}

template<typename TSeq>
inline bool Agent<TSeq>::has_entity(epiworld_fast_uint t) const
{

    for (auto & entity : entities)
        if (entity == t)
            return true;

    return false;

}

template<typename TSeq>
inline bool Agent<TSeq>::has_entity(std::string name) const
{

    for (auto & entity : entities)
        if (model->get_entity(entity).get_name() == name)
            return true;

    return false;

}

template<typename TSeq>
inline void Agent<TSeq>::print(
    Model<TSeq> * model,
    bool compressed
    ) const
{

    if (compressed)
    {
        printf_epiworld(
            "Agent: %i, state: %s (%i), Has virus: %s, NTools: %ii NNeigh: %i\n",
            static_cast<int>(id),
            model->states_labels[state].c_str(),
            static_cast<int>(state),
            virus == nullptr ? std::string("no").c_str() : std::string("yes").c_str(),
            static_cast<int>(n_tools),
            static_cast<int>(neighbors.size())
        );
    }
    else {

        printf_epiworld("Information about agent id %i\n",
            static_cast<int>(this->id));
        printf_epiworld("  State        : %s (%i)\n",
            model->states_labels[state].c_str(), static_cast<int>(state));
        printf_epiworld("  Has virus    : %s\n", virus == nullptr ?
            std::string("no").c_str() : std::string("yes").c_str());
        printf_epiworld("  Tool count   : %i\n", static_cast<int>(n_tools));
        printf_epiworld("  Neigh. count : %i\n", static_cast<int>(neighbors.size()));

        size_t nfeats = model->get_agents_data_ncols();
        if (nfeats > 0)
        {

            printf_epiworld(
                "This model includes features (%i): [ ",
                static_cast<int>(nfeats)
                );

            int max_to_show = static_cast<int>((nfeats > 10)? 10 : nfeats);

            for (int k = 0; k < max_to_show; ++k)
            {
                printf_epiworld("%.2f", this->operator[](k));

                if (k != (max_to_show - 1))
                {
                    printf_epiworld(", ");
                } else {
                    printf_epiworld(" ]\n");
                }

            }
            
        }

    }

    return;

}

template<typename TSeq>
inline double & Agent<TSeq>::operator()(size_t j)
{

    if (model->agents_data_ncols <= j)
        throw std::logic_error("The requested feature of the agent is out of range.");

    return *(model->agents_data + j * model->size() + id);

}

template<typename TSeq>
inline double & Agent<TSeq>::operator[](size_t j)
{
    return *(model->agents_data + j * model->size() + id);
}

template<typename TSeq>
inline double Agent<TSeq>::operator()(size_t j) const
{

    if (model->agents_data_ncols <= j)
        throw std::logic_error("The requested feature of the agent is out of range.");

    return *(model->agents_data + j * model->size() + id);

}

template<typename TSeq>
inline double Agent<TSeq>::operator[](size_t j) const
{
    return *(model->agents_data + j * model->size() + id);
}

template<typename TSeq>
inline Entities<TSeq> Agent<TSeq>::get_entities()
{
    return Entities<TSeq>(*this);
}

template<typename TSeq>
inline const Entities_const<TSeq> Agent<TSeq>::get_entities() const
{
    return Entities_const<TSeq>(*this);
}

template<typename TSeq>
inline const Entity<TSeq> & Agent<TSeq>::get_entity(size_t i) const
{
    if (n_entities == 0)
        throw std::range_error("Agent id " + std::to_string(id) + " has no entities.");

    if (i >= n_entities)
        throw std::range_error("Trying to get to an agent's entity outside of the range.");

    return model->get_entity(entities[i]);
}

template<typename TSeq>
inline Entity<TSeq> & Agent<TSeq>::get_entity(size_t i)
{
    if (n_entities == 0)
        throw std::range_error("Agent id " + std::to_string(id) + " has no entities.");

    if (i >= n_entities)
        throw std::range_error("Trying to get to an agent's entity outside of the range.");

    return model->get_entity(entities[i]);
}

template<typename TSeq>
inline size_t Agent<TSeq>::get_n_entities() const
{
    return n_entities;
}

template<typename TSeq>
inline bool Agent<TSeq>::operator==(const Agent<TSeq> & other) const
{

    EPI_DEBUG_FAIL_AT_TRUE(
        n_neighbors != other.n_neighbors,
        "Agent:: n_eighbors don't match"
        )

    
    for (size_t i = 0u; i < n_neighbors; ++i)
    {
        EPI_DEBUG_FAIL_AT_TRUE(
            neighbors[i] != other.neighbors[i],
            "Agent:: neighbor[i] don't match"
        )
    }
    
    EPI_DEBUG_FAIL_AT_TRUE(
        n_entities != other.n_entities,
        "Agent:: n_entities don't match"
        )
    
    
    for (size_t i = 0u; i < n_entities; ++i)
    {
        EPI_DEBUG_FAIL_AT_TRUE(
            entities[i] != other.entities[i],
            "Agent:: entities[i] don't match"
        )
    }

    EPI_DEBUG_FAIL_AT_TRUE(
        state != other.state,
        "Agent:: state don't match"
        )
        

    EPI_DEBUG_FAIL_AT_TRUE(
        state_prev != other.state_prev,
        "Agent:: state_prev don't match"
        )
        

    // EPI_DEBUG_FAIL_AT_TRUE(
    //     state_last_changed != other.state_last_changed,
    //     "Agent:: state_last_changed don't match"
    //     ) ///< Last time the agent was updated.

    EPI_DEBUG_FAIL_AT_TRUE(
        ((virus == nullptr) && (other.virus != nullptr)) ||
            ((virus != nullptr) && (other.virus == nullptr)),
        "Agent:: virus don't match"
    )

    if ((virus != nullptr) && (other.virus != nullptr))
    {
        EPI_DEBUG_FAIL_AT_TRUE(
            *virus != *other.virus,
            "Agent:: virus doesn't match"
        )
    }
    
    EPI_DEBUG_FAIL_AT_TRUE(n_tools != other.n_tools, "Agent:: n_tools don't match")

    for (size_t i = 0u; i < n_tools; ++i)
    {
        
        EPI_DEBUG_FAIL_AT_TRUE(
            tools[i] != other.tools[i],
            "Agent:: tools[i] don't match"
        )
         
    }   
    
    return true;
    
}

#undef CHECK_COALESCE_

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/agent-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/agentssample-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_AGENTS_BONES_HPP
#define EPIWORLD_AGENTS_BONES_HPP

class SAMPLETYPE {
public:
    static const int MODEL  = 0;
    static const int ENTITY = 1;
    static const int AGENT  = 2;
};

// template<typename TSeq>
// class Agent;

// template<typename TSeq>
// class Model;

// template<typename TSeq>
// class Entity;

/**
 * @brief Sample of agents
 * 
 * This class allows sampling agents from Entity<TSeq> and Model<TSeq>.
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class AgentsSample {
private:

    size_t sample_size = 0u;

    std::vector< Agent<TSeq>* >* agents = nullptr; ///< Pointer to sample of agents
    size_t * agents_n = nullptr;                    ///< Size of sample of agents
    
    std::vector< size_t >* agents_left = nullptr;  ///< Pointer to agents left (iota)
    size_t * agents_left_n = nullptr;               ///< Size of agents left

    Model<TSeq> * model   = nullptr;   ///< Extracts runif() and (if the case) population.
    Entity<TSeq> * entity = nullptr; ///
    Agent<TSeq> * agent   = nullptr;
    
    int sample_type = SAMPLETYPE::AGENT;
    std::vector< size_t > states = {};

    void sample_n(size_t n); ///< Backbone function for sampling


public:

    // Not available (for now)
    AgentsSample() = delete;                             ///< Default constructor
    AgentsSample(const AgentsSample<TSeq> & a) = delete; ///< Copy constructor
    AgentsSample(AgentsSample<TSeq> && a) = delete;      ///< Move constructor

    AgentsSample(
        Model<TSeq> & model_, size_t n,
        std::vector< size_t > states_ = {},
        bool truncate = false
        );

    AgentsSample(
        Model<TSeq> * model,
        Entity<TSeq> & entity_,
        size_t n,
        std::vector< size_t > states_ = {},
        bool truncate = false
        );

    AgentsSample(
        Model<TSeq> * model,
        Agent<TSeq> & agent_,
        size_t n,
        std::vector< size_t > states_ = {},
        bool truncate = false
        );

    ~AgentsSample();

    typename std::vector< Agent<TSeq> * >::iterator begin();
    typename std::vector< Agent<TSeq> * >::iterator end();

    Agent<TSeq> * operator[](size_t n);
    Agent<TSeq> * operator()(size_t n);
    size_t size() const noexcept;

};

template<typename TSeq>
inline AgentsSample<TSeq>::AgentsSample(
    Model<TSeq> & model_,
    size_t n,
    std::vector< size_t > states_,
    bool truncate
    ) {

    states = states_;

    if (truncate)
    {
        
        if (n > model_.size())
            n = model_.size();

    } else if (n > model_.size())
        throw std::logic_error(
            "There are only " + std::to_string(model_.size()) + " agents. You cannot " +
            "sample " + std::to_string(n));

    sample_size   = n;
    model         = &model_;
    sample_type   = SAMPLETYPE::MODEL;

    agents        = &model_.sampled_population;
    agents_n      = &model_.sampled_population_n;

    agents_left   = &model_.population_left;
    agents_left_n = &model_.population_left_n;

    sample_n(n);
    
    return; 

}

template<typename TSeq>
inline AgentsSample<TSeq>::AgentsSample(
    Model<TSeq> * model,
    Entity<TSeq> & entity_,
    size_t n,
    std::vector< size_t > states_,
    bool truncate
    ) {

    states = states_;

    if (truncate)
    {

        if (n > entity_.size())
            n = entity_.size();

    } else if (n > entity_.size())
        throw std::logic_error(
            "There are only " + std::to_string(entity_.size()) + " agents. You cannot " +
            "sample " + std::to_string(n));

    sample_size   = n;
    model         = &entity_.model;
    sample_type   = SAMPLETYPE::ENTITY;

    agents        = &entity_.sampled_agents;
    agents_n      = &entity_.sampled_agents_n;

    agents_left   = &entity_.sampled_agents_left;
    agents_left_n = &entity_.sampled_agents_left_n;

    sample_n(n);

    return; 

}

/**
 * @brief Sample from the agent's entities
 * 
 * For example, how many individuals the agent contacts in a given point in time.
 * 
 * @tparam TSeq 
 * @param agent_ 
 * @param n Sample size
 * @param truncate If the agent has fewer than `n` connections, then truncate = true
 * will automatically reduce the number of possible samples. Otherwise, if false, then
 * it returns an error.
 */
template<typename TSeq>
inline AgentsSample<TSeq>::AgentsSample(
    Model<TSeq> * model,
    Agent<TSeq> & agent_,
    size_t n,
    std::vector< size_t > states_,
    bool truncate
    )
{

    states = states_;
    sample_type = SAMPLETYPE::AGENT;
    
    agent         = &agent_;

    agents        = &agent_.sampled_agents;
    agents_n      = &agent_.sampled_agents_n;

    // Computing the cumulative sum of counts across entities
    size_t agents_in_entities = 0;
    Entities<TSeq> entities_a = agent->get_entities();

    std::vector< size_t > cum_agents_count(entities_a.size(), 0);
    int idx = -1;
    for (auto & e : entities_a)
    {
        if (++idx == 0)
            cum_agents_count[idx] = (e->size() - 1u);
        else
            cum_agents_count[idx] = (
                (e->size() - 1u) + 
                cum_agents_count[idx - 1]
            );

        agents_in_entities += (e->size() - 1u);
    }

    if (truncate)
    {
        
        if (n > agents_in_entities)
            n = agents_in_entities;

    } else if (n > agents_in_entities)
        throw std::logic_error(
            "There are only " + std::to_string(agents_in_entities) +
            " agents. You cannot " +
            "sample " + std::to_string(n)
            );

    sample_size = n;

    if (agents->size() < n)
        agents->resize(n);

    size_t i_obs = 0u;
    for (size_t i = 0u; i < sample_size; ++i)
    {

        // Sampling a single agent from the set of entities
        int jth = std::floor(model->runif() * agents_in_entities);
        for (size_t e = 0u; e < cum_agents_count.size(); ++e)
        {
            
            // Are we in the limit?
            if (jth <= cum_agents_count[e])
            {
                size_t agent_idx = 0u;
                if (e == 0) // From the first group
                    agent_idx = entities_a[e][jth]->get_id();
                else
                    agent_idx = entities_a[e][jth - cum_agents_count[e - 1]]->get_id();


                // Checking if states was specified
                if (states.size())
                {

                    // Getting the state
                    size_t state = model->population[agent_idx].get_state();

                    if (std::find(states.begin(), states.end(), state) != states.end())
                        continue;

                }
                
                agents->operator[](i_obs++) = &(model->population[agent_idx]);

                break;
            }

        }
    }

    return; 

}

template<typename TSeq>
inline AgentsSample<TSeq>::~AgentsSample() {}

template<typename TSeq>
inline size_t AgentsSample<TSeq>::size() const noexcept
{ 
    return this->sample_size;
}

template<typename TSeq>
inline Agent<TSeq> * AgentsSample<TSeq>::operator[](size_t i)
{

    return agents->operator[](i);

}

template<typename TSeq>
inline Agent<TSeq> * AgentsSample<TSeq>::operator()(size_t i)
{

    if (i >= this->sample_size)
        throw std::range_error("The requested agent is out of range.");

    return agents->operator[](i);

}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator AgentsSample<TSeq>::begin()
{

    if (sample_size > 0u)
        return agents->begin();
    else
        return agents->end();

}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator AgentsSample<TSeq>::end()
{

    return agents->begin() + sample_size;

}

template<typename TSeq>
inline void AgentsSample<TSeq>::sample_n(size_t n)
{

    // Reducing size
    if (states.size())
    {
            
        // Getting the number of agents left
        agents_left->clear();

        if (sample_type == SAMPLETYPE::MODEL)
        {

            // Making some room
            agents_left->reserve(model->size());

            // Iterating through the agents in the population
            for (size_t a_i = 0u; a_i < model->population.size(); ++a_i)
            {

                // If the agent is within the selected set of states,
                // then we add it to the list of agents left
                size_t s = model->population[a_i].get_state();
                if (std::find(states.begin(), states.end(), s) != states.end())
                    agents_left->push_back(a_i);

            }

        } 
        else if (sample_type == SAMPLETYPE::ENTITY)
        {

            // Making room
            agents_left->reserve(entity->size());

            // Iterating through the agents in the entity
            for (size_t a_i = 0u; a_i < entity->size(); ++a_i)
            {
                size_t s = model->population[entity->agents[a_i]].get_state();
                if (std::find(states.begin(), states.end(), s) != states.end())
                    agents_left->push_back(a_i);

            }

        }

    } else {

        // Checking if the size of the entity has changed (or hasn't been initialized)
        if (sample_type == SAMPLETYPE::MODEL)
        {

            if (model->size() != agents_left->size())
            {
                agents_left->resize(model->size(), 0u);
                std::iota(agents_left->begin(), agents_left->end(), 0u);
            }

        } else if (sample_type == SAMPLETYPE::ENTITY) {

            if (entity->size() != agents_left->size())
            {

                agents_left->resize(entity->size(), 0u);
                std::iota(agents_left->begin(), agents_left->end(), 0u);

            }

        } 

    }

    // Restart the counter of agents left
    *agents_left_n = agents_left->size();

    // Making sure we have enough room for the sample of agents
    if (agents->size() < sample_size)
        agents->resize(sample_size, nullptr);

    if (sample_type == SAMPLETYPE::MODEL)
    {

        #ifdef EPI_DEBUG
        std::vector< bool > __sampled(model->size(), true);
        for (auto & a_i: *agents_left)
            __sampled[a_i] = false;
        #endif

        for (size_t i = 0u; i < n; ++i)
        {

            // Sampling from 0 to (agents_left_n - 1)
            size_t ith_ = static_cast<size_t>(model->runif() * ((*agents_left_n)--));
            
            // Getting the id of the agent and adding it to the list of agents
            size_t ith  = agents_left->operator[](ith_);
            agents->operator[](i) = &model->population[ith];

            #ifdef EPI_DEBUG
            if (__sampled[ith])
                throw std::logic_error("The same agent was sampled twice.");
            else
                __sampled[ith] = true;
            #endif

            // Updating list
            std::swap(
                agents_left->operator[](ith_),
                agents_left->operator[](*agents_left_n)
                );

        }


    }
    else if (sample_type == SAMPLETYPE::ENTITY)
    {

        #ifdef EPI_DEBUG
        std::vector< bool > __sampled(entity->size(), true);
        for (auto & a_i: *agents_left)
            __sampled[a_i] = false;
        #endif

        for (size_t i = 0u; i < n; ++i)
        {

            size_t ith_ = static_cast<size_t>(model->runif() * ((*agents_left_n)--));
            size_t ith  = agents_left->operator[](ith_);
            agents->operator[](i) = &model->population[entity->agents[ith]];

            #ifdef EPI_DEBUG
            if (__sampled[ith])
                throw std::logic_error("The same agent was sampled twice.");
            else
                __sampled[ith] = true;
            #endif

            // Updating list
            std::swap(agents_left->operator[](ith_), agents_left->operator[](*agents_left_n));

        }

    }

    return;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/agentssample-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/groupsampler-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef GROUPSAMPLER_BONES_HPP
#define GROUPSAMPLER_BONES_HPP

/**
 * @brief Weighted sampling of groups
 */
template<typename TSeq> 
class GroupSampler {

private:
    
    std::vector< double > contact_matrix; ///< Contact matrix between groups
    std::vector< size_t > group_sizes;    ///< Sizes of the groups
    std::vector< double > cumulate;       ///< Cumulative sum of the contact matrix (row-major for faster access)

    /**
     * @brief Get the index of the contact matrix
     * 
     * The matrix is a vector stored in column-major order.
     * 
     * @param i Index of the row
     * @param j Index of the column
     * @return Index of the contact matrix
     */
    inline int idx(const int i, const int j, bool rowmajor = false) const
    {
        
        if (rowmajor)
            return i * group_sizes.size() + j;
        
        return j * group_sizes.size() + i; 

    }

public:

    GroupSampler() {};

    GroupSampler(
        const std::vector< double > & contact_matrix_,
        const std::vector< size_t > & group_sizes_,
        bool normalize = true
    );

    int sample_1(
        Model<TSeq> * model,
        const int origin_group
        );

    void sample_n(
        Model<TSeq> * model,
        std::vector< int > & sample,
        const int origin_group,
        const int nsamples
    );

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/groupsampler-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/groupsampler-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef GROUPSAMPLER_MEAT_HPP
#define GROUPSAMPLER_MEAT_HPP

template<typename TSeq>
inline GroupSampler<TSeq>::GroupSampler(
        const std::vector< double > & contact_matrix_,
        const std::vector< size_t > & group_sizes_,
        bool normalize
    ): contact_matrix(contact_matrix_), group_sizes(group_sizes_) {


        this->cumulate.resize(contact_matrix.size());
        std::fill(cumulate.begin(), cumulate.end(), 0.0);

        // Cumulative sum
        for (size_t j = 0; j < group_sizes.size(); ++j)
        {
            for (size_t i = 0; i < group_sizes.size(); ++i)
                cumulate[idx(i, j, true)] += 
                    cumulate[idx(i, j - 1, true)] +
                    contact_matrix[idx(i, j)];
        }

        if (normalize)
        {
            for (size_t i = 0; i < group_sizes.size(); ++i)
            {
                double sum = 0.0;
                for (size_t j = 0; j < group_sizes.size(); ++j)
                    sum += contact_matrix[idx(i, j, true)];
                for (size_t j = 0; j < group_sizes.size(); ++j)
                    contact_matrix[idx(i, j, true)] /= sum;
            }
        }

    };

template<typename TSeq>
int GroupSampler<TSeq>::sample_1(
    Model<TSeq> * model,
    const int origin_group
    )
{

    // Random number
    double r = model->runif();

    // Finding the group
    size_t j = 0;
    while (r > cumulate[idx(origin_group, j, true)])
        ++j;

    // Adjusting the prob
    r = r - (j == 0 ? 0.0 : cumulate[idx(origin_group, j - 1, true)]);

    int res = static_cast<int>(
        std::floor(r * group_sizes[j])
    );

    // Making sure we are not picking outside of the group
    if (res >= static_cast<int>(group_sizes[j]))
        res = static_cast<int>(group_sizes[j]) - 1;

    return model->get_entities()[j][res]->get_id();

}

template<typename TSeq>
void GroupSampler<TSeq>::sample_n(
    Model<TSeq> * model,
    std::vector< int > & sample,
    const int origin_group,
    const int nsamples
)
{

    for (int i = 0; i < nsamples; ++i)
        sample[i] = sample_1(model, origin_group);

    return;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/groupsampler-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/models/models.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_HPP
#define EPIWORLD_MODELS_HPP

namespace epimodels {

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/init-functions.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_INIT_FUNCTIONS_HPP
#define EPIWORLD_MODELS_INIT_FUNCTIONS_HPP

/**
 * @brief Creates an initial function for the SIR-like models
 * The function is used for the initial states of the model.
*/
template<typename TSeq>
inline std::function<void(epiworld::Model<TSeq>*)> create_init_function_sir(
    std::vector< double > proportions_
) {

    // Checking widths
    if (proportions_.size() != 1u)
        throw std::invalid_argument(
            "The vector of proportions must have a single element."
            );

    // Proportion should be within [0, 1]
    if ((proportions_[0] < 0.0) || (proportions_[0] > 1.0))
        throw std::invalid_argument(
            "The proportion must be within (0, 1)."
            );

    double prop = proportions_[0u];

    std::function<void(epiworld::Model<TSeq>*)> fun =
    [prop] (epiworld::Model<TSeq> * model) -> void {

        // Figuring out information about the viruses
        double tot = 0.0;
        double n   = static_cast<double>(model->size());
        for (const auto & agent: model->get_agents())
        {
            if (agent.get_virus() != nullptr)
                tot += 1.0;
        }
        tot /= n;

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        size_t nrecovered = prop * tot_left * n;
        
        epiworld::AgentsSample<TSeq> sample(
            *model,
            nrecovered,
            {0u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample)
            agent->change_state(model, 2, Queue<TSeq>::NoOne);
        
        // Running the events
        model->events_run();

        return;

    };

    return fun;

}

/**
 * @brief Creates an initial function for the SIR-like models
 * The function is used for the initial states of the model.
*/
template<typename TSeq>
inline std::function<void(epiworld::Model<TSeq>*)> create_init_function_sird(
    std::vector< double > prop
) {

    // Check length of prop equals two
    if (prop.size() != 2u)
        throw std::invalid_argument(
            "The vector of proportions must have two elements."
            );

    // Check elements in prop are within [0, 1] and sum up to 1
    double tot = 0.0;
    for (auto & v : prop)
    {
        if ((v < 0.0) || (v > 1.0))
            throw std::invalid_argument(
                "The proportion must be within (0, 1)."
                );
        tot += v;
    }

    if (tot >= 1.0)
        throw std::invalid_argument(
            "The proportions must sum up to 1."
            );

    std::function<void(epiworld::Model<TSeq>*)> fun =
    [prop] (epiworld::Model<TSeq> * model) -> void {

        // Figuring out information about the viruses
        double tot = 0.0;
        double n   = static_cast<double>(model->size());
        for (const auto & agent: model->get_agents())
        {
            if (agent.get_virus() != nullptr)
                tot += 1.0;
        }
        tot /= n;

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        size_t nrecovered = prop[0u] * tot_left * n;
        size_t ndeceased  = prop[01] * tot_left * n;
        
        epiworld::AgentsSample<TSeq> sample_recover(
            *model,
            nrecovered,
            {0u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_recover)
            agent->change_state(model, 2, Queue<TSeq>::NoOne);

        epiworld::AgentsSample<TSeq> sample_deceased(
            *model,
            ndeceased,
            {0u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_deceased)
            agent->change_state(model, 3, Queue<TSeq>::NoOne);
        
        // Running the events
        model->events_run();

        return;

    };

    return fun;

}


/**
 * @brief Creates an initial function for the SEIR-like models
 * The function is used for the initial states of the model.
*/
template<typename TSeq>
inline std::function<void(epiworld::Model<TSeq>*)> create_init_function_seir(
    std::vector< double > proportions_
) {

    // Checking widths
    if (proportions_.size() != 2u) {
        throw std::invalid_argument("-proportions_- must have two entries.");
    }

    // proportions_ are values between 0 and 1, otherwise error
    for (auto & v : proportions_)
        if ((v < 0.0) || (v > 1.0))
            throw std::invalid_argument(
                "-proportions_- must have values between 0 and 1."
                );


    std::function<void(epiworld::Model<TSeq>*)> fun = 
        [proportions_] (epiworld::Model<TSeq> * model) -> void {

        // Figuring out information about the viruses
        double tot = 0.0;
        double n   = static_cast<double>(model->size());
        for (const auto & agent: model->get_agents())
        {
            if (agent.get_virus() != nullptr)
                tot += 1.0;
        }
        tot /= n;

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        size_t nexposed   = proportions_[0u] * tot * n;
        size_t nrecovered = proportions_[1u] * tot_left * n;
        
        epiworld::AgentsSample<TSeq> sample_suscept(
            *model,
            nrecovered,
            {0u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_suscept)
            agent->change_state(model, 3, Queue<TSeq>::NoOne);

        epiworld::AgentsSample<TSeq> sample_exposed(
            *model,
            nexposed,
            {1u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_exposed)
            agent->change_state(model, 2, Queue<TSeq>::NoOne);
        
        // Running the events
        model->events_run();

        return;

    };

    return fun;

}

/**
 * @brief Creates an initial function for the SEIR-like models
 * The function is used for the initial states of the model.
*/
template<typename TSeq>
inline std::function<void(epiworld::Model<TSeq>*)> create_init_function_seird(
    std::vector< double > proportions_
) {

    // Checking widths
    if (proportions_.size() != 3u) {
        throw std::invalid_argument("-proportions_- must have three entries.");
    }

    // proportions_ are values between 0 and 1, otherwise error
    for (auto & v : proportions_)
        if ((v < 0.0) || (v > 1.0))
            throw std::invalid_argument(
                "-proportions_- must have values between 0 and 1."
                );

    // Last first two terms shouldn't add up to more than 1
    if ((proportions_[1u] + proportions_[2u]) > 1.0)
        throw std::invalid_argument(
            "The last two terms of -proportions_- must add up to less than 1."
            );

    std::function<void(epiworld::Model<TSeq>*)> fun = 
        [proportions_] (epiworld::Model<TSeq> * model) -> void {

        // Figuring out information about the viruses
        double tot = 0.0;
        double n   = static_cast<double>(model->size());

        for (const auto & agent: model->get_agents())
        {
            if (agent.get_virus() != nullptr)
                tot += 1.0;
        }
        tot /= n;

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        size_t nexposed   = proportions_[0u] * tot * n;
        size_t nrecovered = proportions_[1u] * tot_left * n;
        size_t ndeceased  = proportions_[2u] * tot_left * n;
        
        epiworld::AgentsSample<TSeq> sample_suscept(
            *model,
            nrecovered,
            {0u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_suscept)
            agent->change_state(model, 3, Queue<TSeq>::NoOne);

        epiworld::AgentsSample<TSeq> sample_exposed(
            *model,
            nexposed,
            {1u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_exposed)
            agent->change_state(model, 2, Queue<TSeq>::NoOne);

        // Running the events
        model->events_run();

        // Setting the initial states for the deceased
        epiworld::AgentsSample<TSeq> sample_deceased(
            *model,
            ndeceased,
            {0u},
            true
            );
        
        // Setting up the initial states
        for (auto & agent : sample_deceased)
            agent->change_state(model, 4, Queue<TSeq>::NoOne);
        
        // Running the events
        model->events_run();

        return;

    };

    return fun;

}


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/init-functions.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/globalevents.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_GLOBALEVENTS_HPP
#define EPIWORLD_GLOBALEVENTS_HPP

// This function creates a global action that distributes a tool
// to agents with probability p.
/**
 * @brief Global event that distributes a tool to agents with probability p.
 * 
 * @tparam TSeq Sequence type (should match `TSeq` across the model)
 * @param p Probability of distributing the tool.
 * @param tool Tool function.
 * @return std::function<void(Model<TSeq>*)> 
 */
template<typename TSeq>
inline std::function<void(Model<TSeq>*)> globalevent_tool(
    Tool<TSeq> & tool,
    double p
) {

    std::function<void(Model<TSeq>*)> fun = [p,&tool](
        Model<TSeq> * model
        ) -> void {

        for (auto & agent : model->get_agents())
        {

            // Check if the agent has the tool
            if (agent.has_tool(tool))
                continue;

            // Adding the tool
            if (model->runif() < p)
                agent.add_tool(tool, model);
            
        
        }

        #ifdef EPIWORLD_DEBUG
        tool.print();
        #endif

        return;
            

    };

    return fun;

}

// Same function as above, but p is now a function of a vector of coefficients
// and a vector of variables.
/**
 * @brief Global event that distributes a tool to agents with probability
 * p = 1 / (1 + exp(-\sum_i coef_i * agent(vars_i))).
 * 
 * @tparam TSeq Sequence type (should match `TSeq` across the model)
 * @param coefs Vector of coefficients.
 * @param vars Vector of variables.
 * @param tool_fun Tool function.
 * @return std::function<void(Model<TSeq>*)> 
 */
template<typename TSeq>
inline std::function<void(Model<TSeq>*)> globalevent_tool_logit(
    Tool<TSeq> & tool,
    std::vector< size_t > vars,
    std::vector< double > coefs
) {

    std::function<void(Model<TSeq>*)> fun = [coefs,vars,&tool](
        Model<TSeq> * model
        ) -> void {

        for (auto & agent : model->get_agents())
        {

            // Check if the agent has the tool
            if (agent.has_tool(tool))
                continue;

            // Computing the probability using a logit. Uses OpenMP reduction
            // to sum the coefficients.
            double p = 0.0;
            #if defined(__OPENMP) || defined(_OPENMP)
            #pragma omp parallel for reduction(+:p)
            #endif
            for (size_t i = 0u; i < coefs.size(); ++i)
                p += coefs.at(i) * agent(vars[i]);

            p = 1.0 / (1.0 + std::exp(-p));

            // Adding the tool
            if (model->runif() < p)
                agent.add_tool(tool, model);
            
        
        }

        #ifdef EPIWORLD_DEBUG
        tool.print();
        #endif

        return;
            

    };

    return fun;

}

// A global action that updates a parameter in the model.
/**
 * @brief Global event that updates a parameter in the model.
 * 
 * @tparam TSeq Sequence type (should match `TSeq` across the model)
 * @param param Parameter to update.
 * @param value Value to update the parameter to.
 * @return std::function<void(Model<TSeq>*)> 
 */
template<typename TSeq>
inline std::function<void(Model<TSeq>*)> globalevent_set_param(
    std::string param,
    double value
) {

    std::function<void(Model<TSeq>*)> fun = [value,param](
        Model<TSeq> * model
        ) -> void {

        model->set_param(param, value);

        return;
            

    };

    return fun;

}
#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/globalevents.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/sis.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SIS_HPP 
#define EPIWORLD_MODELS_SIS_HPP

/**
 * @brief Template for a Susceptible-Infected-Susceptible (SIS) model
 * 
 * @param vname std::string Name of the virus
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery_rate rate of the immune system
 */
template<typename TSeq = int>
class ModelSIS : public epiworld::Model<TSeq>
{

public:

    static const int SUSCEPTIBLE = 0;
    static const int INFECTED    = 1;

    ModelSIS() {};

    ModelSIS(
        ModelSIS<TSeq> & model,
        const std::string & vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate
    );

    ModelSIS(
        const std::string & vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate
    );

};

template<typename TSeq>
inline ModelSIS<TSeq>::ModelSIS(
    ModelSIS<TSeq> & model,
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate
    )
{

    model.set_name("Susceptible-Infected-Susceptible (SIS)");

    // Adding statuses
    model.add_state("Susceptible", epiworld::default_update_susceptible<TSeq>);
    model.add_state("Infected", epiworld::default_update_exposed<TSeq>);

    // Setting up parameters
    model.add_param(transmission_rate, "Transmission rate");
    model.add_param(recovery_rate, "Recovery rate");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(ModelSIS<TSeq>::INFECTED, ModelSIS<TSeq>::SUSCEPTIBLE, ModelSIS<TSeq>::SUSCEPTIBLE);
    
    virus.set_prob_infecting(&model("Transmission rate"));
    virus.set_prob_recovery(&model("Recovery rate"));
    virus.set_prob_death(0.0);
    
    model.add_virus(virus);

    return;

}

template<typename TSeq>
inline ModelSIS<TSeq>::ModelSIS(
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate
    )
{

    ModelSIS<TSeq>(
        *this,
        vname,
        prevalence,
        transmission_rate,
        recovery_rate
    );    

    return;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/sis.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/sir.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery_rate rate of the immune system
 */
template<typename TSeq = int>
class ModelSIR : public epiworld::Model<TSeq>
{
public:

    ModelSIR() {};

    ModelSIR(
        ModelSIR<TSeq> & model,
        const std::string & vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate
    );

    ModelSIR(
        const std::string & vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate
    );

    /**
     * @brief Set the initial states of the model
     * @param proportions_ Double vector with a single element:
     * - The proportion of non-infected individuals who have recovered.
    */
    ModelSIR<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    );
    
};

template<typename TSeq>
inline ModelSIR<TSeq>::ModelSIR(
    ModelSIR<TSeq> & model,
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate
    )
{

    // Adding statuses
    model.add_state("Susceptible", epiworld::default_update_susceptible<TSeq>);
    model.add_state("Infected", epiworld::default_update_exposed<TSeq>);
    model.add_state("Recovered");

    // Setting up parameters
    model.add_param(recovery_rate, "Recovery rate");
    model.add_param(transmission_rate, "Transmission rate");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(1,2,2);
    
    virus.set_prob_recovery(&model("Recovery rate"));
    virus.set_prob_infecting(&model("Transmission rate"));
    
    model.add_virus(virus);

    model.set_name("Susceptible-Infected-Recovered (SIR)");

    return;
   
}

template<typename TSeq>
inline ModelSIR<TSeq>::ModelSIR(
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate
    )
{

    ModelSIR<TSeq>(
        *this,
        vname,
        prevalence,
        transmission_rate,
        recovery_rate
        );

    return;

}

template<typename TSeq>
inline ModelSIR<TSeq> & ModelSIR<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
) {

    Model<TSeq>::initial_states_fun =
        create_init_function_sir<TSeq>(proportions_)
        ;

    return *this;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/sir.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/seir.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SEIR_HPP
#define EPIWORLD_MODELS_SEIR_HPP

/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence epiworld_double Initial prevalence the immune system
 * @param transmission_rate epiworld_double Transmission rate of the virus
 * @param avg_incubation_days epiworld_double Average incubation days of the virus.
 * @param recovery_rate epiworld_double Recovery rate of the virus.
 */
template<typename TSeq = int>
class ModelSEIR : public epiworld::Model<TSeq>
{

public:
    static const int SUSCEPTIBLE = 0;
    static const int EXPOSED     = 1;
    static const int INFECTED    = 2;
    static const int REMOVED     = 3;

    ModelSEIR() {};

    ModelSEIR(
        ModelSEIR<TSeq> & model,
        const std::string & vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate
    );

    ModelSEIR(
        const std::string & vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate
    );
    
    epiworld::UpdateFun<TSeq> update_exposed_seir = [](
        epiworld::Agent<TSeq> * p,
        epiworld::Model<TSeq> * m
    ) -> void {

        // Getting the virus
        auto v = p->get_virus();

        // Does the agent become infected?
        if (m->runif() < 1.0/(v->get_incubation(m)))
            p->change_state(m, ModelSEIR<TSeq>::INFECTED);

        return;    
    };
      

    epiworld::UpdateFun<TSeq> update_infected_seir = [](
        epiworld::Agent<TSeq> * p,
        epiworld::Model<TSeq> * m
    ) -> void {
        // Does the agent recover?
        if (m->runif() < (m->par("Recovery rate")))
            p->rm_virus(m);

        return;    
    };

    /**
     * @brief Set up the initial states of the model.
     * @param proportions_ Double vector with the following values:
     * - 0: Proportion of non-infected agents who are removed.
     * - 1: Proportion of exposed agents to be set as infected.
    */
    ModelSEIR<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    );

};


template<typename TSeq>
inline ModelSEIR<TSeq>::ModelSEIR(
    ModelSEIR<TSeq> & model,
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate
    )
{

    // Adding statuses
    model.add_state("Susceptible", epiworld::default_update_susceptible<TSeq>);
    model.add_state("Exposed", model.update_exposed_seir);
    model.add_state("Infected", model.update_infected_seir);
    model.add_state("Removed");

    // Setting up parameters
    model.add_param(transmission_rate, "Transmission rate");
    model.add_param(avg_incubation_days, "Incubation days");
    model.add_param(recovery_rate, "Recovery rate");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(ModelSEIR<TSeq>::EXPOSED, ModelSEIR<TSeq>::REMOVED, ModelSEIR<TSeq>::REMOVED);

    virus.set_prob_infecting(&model("Transmission rate"));
    virus.set_incubation(&model("Incubation days"));
    virus.set_prob_recovery(&model("Recovery rate"));
    
    // Adding the tool and the virus
    model.add_virus(virus);
    
    model.set_name("Susceptible-Exposed-Infected-Removed (SEIR)");

    return;
   
}

template<typename TSeq>
inline ModelSEIR<TSeq>::ModelSEIR(
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate
    )
{

    ModelSEIR<TSeq>(
        *this,
        vname,
        prevalence,
        transmission_rate,
        avg_incubation_days,
        recovery_rate
        );

    return;

}

template<typename TSeq>
inline ModelSEIR<TSeq> & ModelSEIR<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /**/
) {

    Model<TSeq>::initial_states_fun =
        create_init_function_seir<TSeq>(proportions_)
        ;

    return *this;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/seir.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/surveillance.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SURVEILLANCE_HPP
#define EPIWORLD_MODELS_SURVEILLANCE_HPP

template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSURV : public epiworld::Model<TSeq> {

private:
    // state
    static const int SUSCEPTIBLE           = 0;
    static const int LATENT                = 1;
    static const int SYMPTOMATIC           = 2;
    static const int SYMPTOMATIC_ISOLATED  = 3; // sampled and discovered
    static const int ASYMPTOMATIC          = 4;
    static const int ASYMPTOMATIC_ISOLATED = 5;
    static const int RECOVERED             = 6;
    static const int REMOVED               = 7;

public:

    /**
     * @name Construct a new ModelSURV object
     * 
     * The ModelSURV class simulates a survaillence model where agents can be
     * isolated, even if asyptomatic.
     * 
     * @param vname String. Name of the virus
     * @param prevalence Integer. Number of initial cases of the virus.
     * @param efficacy_vax Double. Efficacy of the vaccine (1 - P(acquire the disease)).
     * @param latent_period Double. Shape parameter of a `Gamma(latent_period, 1)`
     *   distribution. This coincides with the expected number of latent days.
     * @param infect_period Double. Shape parameter of a `Gamma(infected_period, 1)`
     *   distribution. This coincides with the expected number of infectious days.
     * @param prob_symptoms Double. Probability of generating symptoms.
     * @param prop_vaccinated Double. Probability of vaccination. Coincides with
     *   the initial prevalence of vaccinated individuals.
     * @param prop_vax_redux_transm Double. Factor by which the vaccine reduces
     *   transmissibility.
     * @param prop_vax_redux_infect Double. Factor by which the vaccine reduces
     *   the chances of becoming infected.
     * @param surveillance_prob Double. Probability of testing an agent.
     * @param prob_transmission Double. Raw transmission probability.
     * @param prob_death Double. Raw probability of death for symptomatic individuals.
     * @param prob_noreinfect Double. Probability of no re-infection.
     * 
     * @details
     * This model features the following states:
     * 
     * - Susceptible
     * - Latent
     * - Symptomatic
     * - Symptomatic isolated
     * - Asymptomatic
     * - Asymptomatic isolated
     * - Recovered
     * - Removed    
     * 
     * @returns An object of class `epiworld_surv`
     * 
     */
    ///@{
    ModelSURV() {};

    ModelSURV(
        ModelSURV<TSeq> & model,
        const std::string & vname,
        epiworld_fast_uint prevalence               = 50,
        epiworld_double efficacy_vax          = 0.9,
        epiworld_double latent_period         = 3u,
        epiworld_double infect_period         = 6u,
        epiworld_double prob_symptoms         = 0.6,
        epiworld_double prop_vaccinated       = 0.25,
        epiworld_double prop_vax_redux_transm = 0.5,
        epiworld_double prop_vax_redux_infect = 0.5,
        epiworld_double surveillance_prob     = 0.001,
        epiworld_double prob_transmission     = 1.0,
        epiworld_double prob_death            = 0.001,
        epiworld_double prob_noreinfect       = 0.9
    );

    ModelSURV(
        const std::string & vname,
        epiworld_fast_uint prevalence         = 50,
        epiworld_double efficacy_vax          = 0.9,
        epiworld_double latent_period         = 3u,
        epiworld_double infect_period         = 6u,
        epiworld_double prob_symptoms         = 0.6,
        epiworld_double prop_vaccinated       = 0.25,
        epiworld_double prop_vax_redux_transm = 0.5,
        epiworld_double prop_vax_redux_infect = 0.5,
        epiworld_double surveillance_prob     = 0.001,
        epiworld_double prob_transmission     = 1.0,
        epiworld_double prob_death            = 0.001,
        epiworld_double prob_noreinfect       = 0.9
    );
    ///@}

};

template<typename TSeq>
inline ModelSURV<TSeq>::ModelSURV(
    ModelSURV<TSeq> & model,
    const std::string & vname,
    epiworld_fast_uint prevalence,
    epiworld_double efficacy_vax,
    epiworld_double latent_period,
    epiworld_double infect_period,
    epiworld_double prob_symptoms,
    epiworld_double prop_vaccinated,
    epiworld_double prop_vax_redux_transm,
    epiworld_double prop_vax_redux_infect,
    epiworld_double surveillance_prob,
    epiworld_double prob_transmission,
    epiworld_double prob_death,
    epiworld_double prob_noreinfect
    )
{

    EPI_NEW_UPDATEFUN_LAMBDA(surveillance_update_susceptible, TSeq) {

        // This computes the prob of getting any neighbor variant
        epiworld_fast_uint nviruses_tmp = 0u;
        for (auto & neighbor: p->get_neighbors()) 
        {
                    
            auto & v = neighbor->get_virus();

            if (v == nullptr)
                continue;
                
            /* And it is a function of susceptibility_reduction as well */ 
            epiworld_double tmp_transmission = 
                (1.0 - p->get_susceptibility_reduction(v, m)) * 
                v->get_prob_infecting(m) * 
                (1.0 - neighbor->get_transmission_reduction(v, m)) 
                ; 
        
            m->array_double_tmp[nviruses_tmp]  = tmp_transmission;
            m->array_virus_tmp[nviruses_tmp++] = &(*v);
        }

        // No virus to compute on
        if (nviruses_tmp == 0)
            return;

        // Running the roulette
        int which = roulette(nviruses_tmp, m);

        if (which < 0)
            return;

        p->set_virus(*m->array_virus_tmp[which], m); 
        return;

    };


    epiworld::UpdateFun<TSeq> surveillance_update_exposed = 
    [](epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m) -> void
    {

        epiworld::VirusPtr<TSeq> & v = p->get_virus(); 
        epiworld_double p_die = v->get_prob_death(m) * (1.0 - p->get_death_reduction(v, m)); 
        
        epiworld_fast_uint days_since_exposed = m->today() - v->get_date();
        epiworld_fast_uint state = p->get_state();

        // Figuring out latent period
        if (v->get_data().size() == 0u)
        {
            epiworld_double latent_days = m->rgamma(m->par("Latent period"), 1.0);
            v->get_data().push_back(latent_days);

            v->get_data().push_back(
                m->rgamma(m->par("Infect period"), 1.0) + latent_days
            );
        }
        
        // If still latent, nothing happens
        if (days_since_exposed <= v->get_data()[0u])
            return;

        // If past days infected + latent, then bye.
        if (days_since_exposed >= v->get_data()[1u])
        {
            p->rm_virus(m);
            return;
        }

        // If it is infected, then it can be asymptomatic or symptomatic
        if (state == ModelSURV<TSeq>::LATENT)
        {

            // Will be symptomatic?
            if (EPI_RUNIF() < m->par("Prob of symptoms"))
                p->change_state(m, ModelSURV<TSeq>::SYMPTOMATIC);
            else
                p->change_state(m, ModelSURV<TSeq>::ASYMPTOMATIC);
            
            return;

        }
        
        // Otherwise, it can be removed
        if (EPI_RUNIF() < p_die)
        {
            p->change_state(m, ModelSURV<TSeq>::REMOVED, -1);
            return;
        }
        
        return;

    };

    std::vector< epiworld_fast_uint > exposed_state = {
        SYMPTOMATIC,
        SYMPTOMATIC_ISOLATED,
        ASYMPTOMATIC,
        ASYMPTOMATIC_ISOLATED,
        LATENT
    };

    epiworld::GlobalFun<TSeq> surveillance_program = 
    [exposed_state](
        epiworld::Model<TSeq>* m
        ) -> void
    {

        // How many will we find
        std::binomial_distribution<> bdist(m->size(), m->par("Surveilance prob."));
        int nsampled = bdist(*m->get_rand_endgine());

        int to_go = nsampled + 1;

        epiworld_double ndetected        = 0.0;
        epiworld_double ndetected_asympt = 0.0;
        
        auto & pop = m->get_agents();
        std::vector< bool > sampled(m->size(), false);
        
        while (to_go-- > 0)
        {

            // Who is the lucky one
            epiworld_fast_uint i = static_cast<epiworld_fast_uint>(std::floor(EPI_RUNIF() * m->size()));

            if (sampled[i])
                continue;

            sampled[i] = true;
            epiworld::Agent<TSeq> * p = &pop[i];
            
            // If still exposed for the next term
            if (epiworld::IN(p->get_state(), exposed_state ))
            {

                ndetected += 1.0;
                if (p->get_state() == ModelSURV<TSeq>::ASYMPTOMATIC)
                {
                    ndetected_asympt += 1.0;
                    p->change_state(m, ModelSURV<TSeq>::ASYMPTOMATIC_ISOLATED);
                }
                else 
                {
                    p->change_state(m, ModelSURV<TSeq>::SYMPTOMATIC_ISOLATED);
                }

            }

        }

        // Writing the user data
        std::vector< int > totals;
        m->get_db().get_today_total(nullptr,&totals);
        m->add_user_data(
            {
                static_cast<epiworld_double>(nsampled),
                ndetected,
                ndetected_asympt,
                static_cast<epiworld_double>(totals[ModelSURV<TSeq>::ASYMPTOMATIC])
            }
            );


    };

    model.add_state("Susceptible", surveillance_update_susceptible);
    model.add_state("Latent", surveillance_update_exposed);
    model.add_state("Symptomatic", surveillance_update_exposed);
    model.add_state("Symptomatic isolated", surveillance_update_exposed);
    model.add_state("Asymptomatic", surveillance_update_exposed);
    model.add_state("Asymptomatic isolated", surveillance_update_exposed);
    model.add_state("Recovered");
    model.add_state("Removed");

    // General model parameters
    model.add_param(latent_period, "Latent period");
    model.add_param(infect_period, "Infect period");
    model.add_param(prob_symptoms, "Prob of symptoms");
    model.add_param(surveillance_prob, "Surveilance prob.");
    model.add_param(efficacy_vax, "Vax efficacy");
    model.add_param(prop_vax_redux_transm, "Vax redux transmission");
    model.add_param(prob_transmission, "Prob of transmission");
    model.add_param(prob_death, "Prob. death");
    model.add_param(prob_noreinfect, "Prob. no reinfect");

    // Virus ------------------------------------------------------------------
    epiworld::Virus<TSeq> covid("Covid19", prevalence, false);
    covid.set_state(LATENT, RECOVERED, REMOVED);
    covid.set_post_immunity(&model("Prob. no reinfect"));
    covid.set_prob_death(&model("Prob. death"));

    epiworld::VirusFun<TSeq> ptransmitfun = [](
        epiworld::Agent<TSeq> * p,
        epiworld::Virus<TSeq> &,
        epiworld::Model<TSeq> * m
        ) -> epiworld_double
    {
        // No chance of infecting
        epiworld_fast_uint  s = p->get_state();
        if (s == ModelSURV<TSeq>::LATENT)
            return static_cast<epiworld_double>(0.0);
        else if (s == ModelSURV<TSeq>::SYMPTOMATIC_ISOLATED)
            return static_cast<epiworld_double>(0.0);
        else if (s == ModelSURV<TSeq>::ASYMPTOMATIC_ISOLATED)
            return static_cast<epiworld_double>(0.0);

        // Otherwise
        return m->par("Prob of transmission");
    };

    covid.set_prob_infecting_fun(ptransmitfun);
    
    model.add_virus(covid);

    model.set_user_data({"nsampled", "ndetected", "ndetected_asympt", "nasymptomatic"});
    model.add_globalevent(surveillance_program, "Surveilance program", -1);
   
    // Vaccine tool -----------------------------------------------------------
    epiworld::Tool<TSeq> vax("Vaccine", prop_vaccinated, true);
    vax.set_susceptibility_reduction(&model("Vax efficacy"));
    vax.set_transmission_reduction(&model("Vax redux transmission"));
    
    model.add_tool(vax);

    model.set_name("Surveillance");

    return;

}

template<typename TSeq>
inline ModelSURV<TSeq>::ModelSURV(
    const std::string & vname,
    epiworld_fast_uint prevalence,
    epiworld_double efficacy_vax,
    epiworld_double latent_period,
    epiworld_double infect_period,
    epiworld_double prob_symptoms,
    epiworld_double prop_vaccinated,
    epiworld_double prop_vax_redux_transm,
    epiworld_double prop_vax_redux_infect,
    epiworld_double surveillance_prob,
    epiworld_double prob_transmission,
    epiworld_double prob_death,
    epiworld_double prob_noreinfect
    )
{

    ModelSURV(
        *this,
        vname,
        prevalence,
        efficacy_vax,
        latent_period,
        infect_period,
        prob_symptoms,
        prop_vaccinated,
        prop_vax_redux_transm,
        prop_vax_redux_infect,
        surveillance_prob,
        prob_transmission,
        prob_death,
        prob_noreinfect
    );

    return;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/surveillance.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/sirconnected.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SIRCONNECTED_HPP 
#define EPIWORLD_MODELS_SIRCONNECTED_HPP

template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSIRCONN : public epiworld::Model<TSeq>
{

private:

    std::vector< epiworld::Agent<TSeq> * > infected;
    void update_infected();

public:

    static const int SUSCEPTIBLE = 0;
    static const int INFECTED    = 1;
    static const int RECOVERED   = 2;

    
    ModelSIRCONN() {};

    ModelSIRCONN(
        ModelSIRCONN<TSeq> & model,
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate
    );

    ModelSIRCONN(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate
    );

    ModelSIRCONN<TSeq> & run(
        epiworld_fast_uint ndays,
        int seed = -1
    );
    
    void reset();

    Model<TSeq> * clone_ptr();

    /**
     * @brief Set the initial states of the model
     * @param proportions_ Double vector with a single element:
     * - The proportion of non-infected individuals who have recovered.
    */
    ModelSIRCONN<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    );

    /**
     * @brief Get the infected individuals
     * @return std::vector< epiworld::Agent<TSeq> * > 
     */
    size_t get_n_infected() const
    {
        return infected.size();
    }

    /***
     * @brief Compute expected generation time
     * @param max_days Maximum number of days.
     * @param max_contacts Maximum number of contacts.
     */
    std::vector< double > generation_time_expected(
        int max_days = 200,
        int max_contacts = 200
    ) const;

};

template<typename TSeq>
inline void ModelSIRCONN<TSeq>::update_infected()
{

    infected.clear();
    infected.reserve(this->size());

    for (auto & p : this->get_agents())
    {
        if (p.get_state() == ModelSIRCONN<TSeq>::INFECTED)
        {
            infected.push_back(&p);
        }
    }

    Model<TSeq>::set_rand_binom(
        this->get_n_infected(),
        static_cast<double>(Model<TSeq>::par("Contact rate"))/
            static_cast<double>(Model<TSeq>::size())
    );

    return;

}

template<typename TSeq>
inline ModelSIRCONN<TSeq> & ModelSIRCONN<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{

    Model<TSeq>::run(ndays, seed);
    return *this;

}

template<typename TSeq>
inline void ModelSIRCONN<TSeq>::reset()
{

    Model<TSeq>::reset();

    this->update_infected();

    return;

}

template<typename TSeq>
inline Model<TSeq> * ModelSIRCONN<TSeq>::clone_ptr()
{
    
    ModelSIRCONN<TSeq> * ptr = new ModelSIRCONN<TSeq>(
        *dynamic_cast<const ModelSIRCONN<TSeq>*>(this)
        );

    return dynamic_cast< Model<TSeq> *>(ptr);

}

/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence Initial prevalence (proportion)
 * @param contact_rate Average number of contacts (interactions) per step.
 * @param transmission_rate Probability of transmission
 * @param recovery_rate Probability of recovery
 */
template<typename TSeq>
inline ModelSIRCONN<TSeq>::ModelSIRCONN(
    ModelSIRCONN<TSeq> & model,
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate
    // epiworld_double prob_reinfection
    )
{

    epiworld::UpdateFun<TSeq> update_susceptible = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            int ndraw = m->rbinom();

            if (ndraw == 0)
                return;

            ModelSIRCONN<TSeq> * model = dynamic_cast<ModelSIRCONN<TSeq> *>(m);
            size_t ninfected = model->get_n_infected();

            // Drawing from the set
            int nviruses_tmp = 0;
            for (int i = 0; i < ndraw; ++i)
            {
                // Now selecting who is transmitting the disease
                int which = static_cast<int>(
                    std::floor(ninfected * m->runif())
                );

                /* There is a bug in which runif() returns 1.0. It is rare, but
                 * we saw it here. See the Notes section in the C++ manual
                 * https://en.cppreference.com/mwiki/index.php?title=cpp/numeric/random/uniform_real_distribution&oldid=133329
                 * And the reported bug in GCC:
                 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63176
                 * 
                 */
                if (which == static_cast<int>(ninfected))
                    --which;

                epiworld::Agent<TSeq> & neighbor = *model->infected[which];

                // Can't sample itself
                if (neighbor.get_id() == p->get_id())
                    continue;

                // The neighbor is infected because it is on the list!
                if (neighbor.get_virus() == nullptr)
                    continue;

                auto & v = neighbor.get_virus();

                #ifdef EPI_DEBUG
                if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
                    throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                #endif
                    
                /* And it is a function of susceptibility_reduction as well */ 
                m->array_double_tmp[nviruses_tmp] =
                    (1.0 - p->get_susceptibility_reduction(v, m)) * 
                    v->get_prob_infecting(m) * 
                    (1.0 - neighbor.get_transmission_reduction(v, m)) 
                    ; 
            
                m->array_virus_tmp[nviruses_tmp++] = &(*v);
                 
            }

            // No virus to compute
            if (nviruses_tmp == 0u)
                return;

            // Running the roulette
            int which = roulette(nviruses_tmp, m);

            if (which < 0)
                return;

            p->set_virus(*m->array_virus_tmp[which], m);

            return; 

        };


    epiworld::UpdateFun<TSeq> update_infected = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void {

            auto state = p->get_state();

            if (state == ModelSIRCONN<TSeq>::INFECTED)
            {


                // Odd: Die, Even: Recover
                epiworld_fast_uint n_events = 0u;
                // Recover
                m->array_double_tmp[n_events++] = 
                    1.0 - (1.0 - p->get_virus()->get_prob_recovery(m)) *
                        (1.0 - p->get_recovery_enhancer(p->get_virus(), m)); 

                #ifdef EPI_DEBUG
                if (n_events == 0u)
                {
                    printf_epiworld(
                        "[epi-debug] agent %i has 0 possible events!!\n",
                        static_cast<int>(p->get_id())
                        );
                    throw std::logic_error("Zero events in exposed.");
                }
                #else
                if (n_events == 0u)
                    return;
                #endif
                

                // Running the roulette
                int which = roulette(n_events, m);

                if (which < 0)
                    return;

                // Which roulette happen?
                p->rm_virus(m);

                return ;

            } else
                throw std::logic_error(
                    "This function can only be applied to infected individuals. (SIR)"
                    ) ;

            return;

        };

    // state
    model.add_state("Susceptible", update_susceptible);
    model.add_state("Infected", update_infected);
    model.add_state("Recovered");

    // Setting up parameters
    model.add_param(contact_rate, "Contact rate");
    model.add_param(transmission_rate, "Transmission rate");
    model.add_param(recovery_rate, "Recovery rate");
    // model.add_param(prob_reinfection, "Prob. Reinfection");

    // Adding update function
    epiworld::GlobalFun<TSeq> update = [](epiworld::Model<TSeq> * m) -> void
    {
        ModelSIRCONN<TSeq> * model = dynamic_cast<ModelSIRCONN<TSeq> *>(m);
        model->update_infected();
        
        return;
    };

    model.add_globalevent(update, "Update infected individuals");
    
    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(1, 2, 2);
    virus.set_prob_infecting(&model("Transmission rate"));
    virus.set_prob_recovery(&model("Recovery rate"));

    model.add_virus(virus);

    model.queuing_off(); // No queuing need

    model.agents_empty_graph(n);

    model.set_name("Susceptible-Infected-Removed (SIR) (connected)");

    return;

}

template<typename TSeq>
inline ModelSIRCONN<TSeq>::ModelSIRCONN(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate
    )
{

    ModelSIRCONN(
        *this,
        vname,
        n,
        prevalence,
        contact_rate,
        transmission_rate,
        recovery_rate
    );

    return;

}

template<typename TSeq>
inline ModelSIRCONN<TSeq> & ModelSIRCONN<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /**/ 
) {

    Model<TSeq>::initial_states_fun = 
        create_init_function_sir<TSeq>(proportions_)
        ;

    return *this;

}

template<typename TSeq>
inline std::vector< double > ModelSIRCONN<TSeq>::generation_time_expected(
    int max_days,
    int max_contacts
) const
{

    // Retrieving total counts
    std::vector< int > h_date;
    std::vector< std::string > h_state;
    std::vector< int > h_counts;
    const auto this_const = dynamic_cast<const ModelSIRCONN<TSeq> *>(this);
    this_const->get_db().get_hist_total(
        &h_date,
        &h_state,
        &h_counts
    );

    // Retrieving information on susceptibles
    std::vector< double > S(this_const->get_ndays(), 0.0);
    for (size_t i = 0; i < h_date.size(); ++i)
    {
        if (h_state[i] == "Susceptible")
            S[h_date[i]] += h_counts[i];
    }

    // The generation time in the SIR model starts from 1, as agents 
    // spend at least one day in the infected state before starting
    // transmitting.
    std::vector< double > gen_times(this_const->get_ndays(), 1.0);
    double p_c = this_const->par("Contact rate")/this_const->size();
    double p_i = this_const->par("Transmission rate");
    double p_r = this_const->par("Recovery rate");
    for (size_t i = 0u; i < this_const->get_ndays(); ++i)
    {
        gen_times[i] = gen_int_mean(
            S[i],
            p_c,
            p_i,
            p_r,
            max_days,
            max_contacts
        );

    }

    return gen_times;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/sirconnected.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/seirconnected.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SEIRCONNECTED_HPP
#define EPIWORLD_MODELS_SEIRCONNECTED_HPP

template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSEIRCONN : public epiworld::Model<TSeq> 
{
private:
    std::vector< epiworld::Agent<TSeq> * > infected;
    void update_infected();

public:

    static const int SUSCEPTIBLE = 0;
    static const int EXPOSED     = 1;
    static const int INFECTED    = 2;
    static const int RECOVERED   = 3;


    ModelSEIRCONN() {};

    ModelSEIRCONN(
        ModelSEIRCONN<TSeq> & model,
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate
    );
    
    ModelSEIRCONN(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate
    );

    ModelSEIRCONN<TSeq> & run(
        epiworld_fast_uint ndays,
        int seed = -1
    );

    void reset();

    Model<TSeq> * clone_ptr();

    /**
     * @brief Set the initial states of the model
     * @param proportions_ Double vector with a single element:
     * - The proportion of non-infected individuals who have recovered.
    */
    ModelSEIRCONN<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    );

    size_t get_n_infected() const { return infected.size(); }

    /***
     * @brief Compute expected generation time
     * @param max_days Maximum number of days.
     * @param max_contacts Maximum number of contacts.
     */
    std::vector< double > generation_time_expected(
        int max_days = 200,
        int max_contacts = 200
    ) const;

};

template<typename TSeq>
inline void ModelSEIRCONN<TSeq>::update_infected()
{

    infected.clear();
    infected.reserve(this->size());

    for (auto & p : this->get_agents())
    {
        if (p.get_state() == ModelSEIRCONN<TSeq>::INFECTED)
        {
            infected.push_back(&p);
        }
    }

    Model<TSeq>::set_rand_binom(
        this->get_n_infected(),
        static_cast<double>(Model<TSeq>::par("Contact rate"))/
            static_cast<double>(Model<TSeq>::size())
    );

    return;

}

template<typename TSeq>
inline ModelSEIRCONN<TSeq> & ModelSEIRCONN<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{
    
    Model<TSeq>::run(ndays, seed);

    return *this;

}

template<typename TSeq>
inline void ModelSEIRCONN<TSeq>::reset()
{

    Model<TSeq>::reset();
    this->update_infected();

    return;

}

template<typename TSeq>
inline Model<TSeq> * ModelSEIRCONN<TSeq>::clone_ptr()
{
    
    ModelSEIRCONN<TSeq> * ptr = new ModelSEIRCONN<TSeq>(
        *dynamic_cast<const ModelSEIRCONN<TSeq>*>(this)
        );

    return dynamic_cast< Model<TSeq> *>(ptr);

}

/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence Initial prevalence (proportion)
 * @param contact_rate Average number of contacts (interactions) per step.
 * @param transmission_rate Probability of transmission
 * @param recovery_rate Probability of recovery
 */
template<typename TSeq>
inline ModelSEIRCONN<TSeq>::ModelSEIRCONN(
    ModelSEIRCONN<TSeq> & model,
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate
    // epiworld_double prob_reinfection
    )
{

    epiworld::UpdateFun<TSeq> update_susceptible = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            // Sampling how many individuals
            int ndraw = m->rbinom();

            if (ndraw == 0)
                return;

            ModelSEIRCONN<TSeq> * model = dynamic_cast<ModelSEIRCONN<TSeq> *>(m);
            size_t ninfected = model->get_n_infected();

            // Drawing from the set
            int nviruses_tmp = 0;
            for (int i = 0; i < ndraw; ++i)
            {
                // Now selecting who is transmitting the disease
                int which = static_cast<int>(
                    std::floor(ninfected * m->runif())
                );

                /* There is a bug in which runif() returns 1.0. It is rare, but
                 * we saw it here. See the Notes section in the C++ manual
                 * https://en.cppreference.com/mwiki/index.php?title=cpp/numeric/random/uniform_real_distribution&oldid=133329
                 * And the reported bug in GCC:
                 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63176
                 * 
                 */
                if (which == static_cast<int>(ninfected))
                    --which;

                epiworld::Agent<TSeq> & neighbor = *model->infected[which];

                // Can't sample itself
                if (neighbor.get_id() == p->get_id())
                    continue;

                // The neighbor is infected by construction
                auto & v = neighbor.get_virus();

                #ifdef EPI_DEBUG
                if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
                    throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                #endif
                    
                /* And it is a function of susceptibility_reduction as well */ 
                m->array_double_tmp[nviruses_tmp] =
                    (1.0 - p->get_susceptibility_reduction(v, m)) * 
                    v->get_prob_infecting(m) * 
                    (1.0 - neighbor.get_transmission_reduction(v, m)) 
                    ; 
            
                m->array_virus_tmp[nviruses_tmp++] = &(*v);

            }

            // No virus to compute
            if (nviruses_tmp == 0u)
                return;

            // Running the roulette
            int which = roulette(nviruses_tmp, m);

            if (which < 0)
                return;

            p->set_virus(
                *m->array_virus_tmp[which],
                m,
                ModelSEIRCONN<TSeq>::EXPOSED
                );

            return; 

        };

    epiworld::UpdateFun<TSeq> update_infected = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void {

            auto state = p->get_state();

            if (state == ModelSEIRCONN<TSeq>::EXPOSED)
            {

                // Getting the virus
                auto & v = p->get_virus();

                // Does the agent become infected?
                if (m->runif() < 1.0/(v->get_incubation(m)))
                {

                    p->change_state(m, ModelSEIRCONN<TSeq>::INFECTED);
                    return;

                }


            } else if (state == ModelSEIRCONN<TSeq>::INFECTED)
            {


                // Odd: Die, Even: Recover
                epiworld_fast_uint n_events = 0u;
                const auto & v = p->get_virus();

                // Recover
                m->array_double_tmp[n_events++] = 
                    1.0 - (1.0 - v->get_prob_recovery(m)) * (1.0 - p->get_recovery_enhancer(v, m)); 

                #ifdef EPI_DEBUG
                if (n_events == 0u)
                {
                    printf_epiworld(
                        "[epi-debug] agent %i has 0 possible events!!\n",
                        static_cast<int>(p->get_id())
                        );
                    throw std::logic_error("Zero events in exposed.");
                }
                #else
                if (n_events == 0u)
                    return;
                #endif
                

                // Running the roulette
                int which = roulette(n_events, m);

                if (which < 0)
                    return;

                // Which roulette happen?
                p->rm_virus(m);

                return ;

            } else
                throw std::logic_error("This function can only be applied to exposed or infected individuals. (SEIR)") ;

            return;

        };

    // Setting up parameters
    model.add_param(contact_rate, "Contact rate");
    model.add_param(transmission_rate, "Prob. Transmission");
    model.add_param(recovery_rate, "Prob. Recovery");
    model.add_param(avg_incubation_days, "Avg. Incubation days");
    
    // state
    model.add_state("Susceptible", update_susceptible);
    model.add_state("Exposed", update_infected);
    model.add_state("Infected", update_infected);
    model.add_state("Recovered");

    // Adding update function
    epiworld::GlobalFun<TSeq> update = [](
        epiworld::Model<TSeq> * m
        ) -> void
        {

            ModelSEIRCONN<TSeq> * model = dynamic_cast<ModelSEIRCONN<TSeq> *>(m);

            model->update_infected();

            return;

        };

    model.add_globalevent(update, "Update infected individuals");


    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(
        ModelSEIRCONN<TSeq>::EXPOSED,
        ModelSEIRCONN<TSeq>::RECOVERED,
        ModelSEIRCONN<TSeq>::RECOVERED
        );

    virus.set_prob_infecting(&model("Prob. Transmission"));
    virus.set_prob_recovery(&model("Prob. Recovery"));
    virus.set_incubation(&model("Avg. Incubation days"));

    model.add_virus(virus);

    model.queuing_off(); // No queuing need

    // Adding the empty population
    model.agents_empty_graph(n);

    model.set_name("Susceptible-Exposed-Infected-Removed (SEIR) (connected)");

    return;

}

template<typename TSeq>
inline ModelSEIRCONN<TSeq>::ModelSEIRCONN(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate
    )
{

    ModelSEIRCONN(
        *this,
        vname,
        n,
        prevalence,
        contact_rate,
        transmission_rate,
        avg_incubation_days,
        recovery_rate
    );

    return;

}

template<typename TSeq>
inline ModelSEIRCONN<TSeq> & ModelSEIRCONN<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
)
{

    Model<TSeq>::initial_states_fun =
        create_init_function_seir<TSeq>(proportions_)
        ;

    return *this;

}

template<typename TSeq>
inline std::vector< double > ModelSEIRCONN<TSeq>::generation_time_expected(
    int max_days,
    int max_contacts
) const
{

    // Retrieving total counts
    std::vector< int > h_date;
    std::vector< std::string > h_state;
    std::vector< int > h_counts;
    const auto this_const = dynamic_cast<const ModelSEIRCONN<TSeq> *>(this);
    this_const->get_db().get_hist_total(
        &h_date,
        &h_state,
        &h_counts
    );

    // Retrieving information on susceptibles
    std::vector< double > S(this_const->get_ndays(), 0.0);
    for (size_t i = 0; i < h_date.size(); ++i)
    {
        if (h_state[i] == "Susceptible")
            S[h_date[i]] += h_counts[i];
    }

    // Computing the expected number of days in exposed
    double days_exposed = this_const->par("Avg. Incubation days");

    // The generation time in the SEIR model starts from 2, as agents 
    // spend at least one day in the exposed state, and 1 day in the 
    // infectious state before starting transmitting.
    std::vector< double > gen_times(
        this_const->get_ndays(), 1.0 + days_exposed
        );
        
    double p_c = this_const->par("Contact rate")/this_const->size();
    double p_i = this_const->par("Prob. Transmission");
    double p_r = this_const->par("Prob. Recovery");

    for (size_t i = 0u; i < this_const->get_ndays(); ++i)
    {
        gen_times[i] += gen_int_mean(
            S[i],
            p_c,
            p_i,
            p_r,
            max_days,
            max_contacts
        );

    }

    return gen_times;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/seirconnected.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/sird.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_SIRD_H 
#define EPIWORLD_SIRD_H

/**
 * @brief Template for a Susceptible-Infected-Removed-Deceased (SIRD) model
 */
template<typename TSeq = int>
class ModelSIRD : public epiworld::Model<TSeq>
{
public:

    ModelSIRD() {};

    
    /**
     * @brief Constructs a new SIRD model with the given parameters.
     * 
     * @param model The SIRD model to copy from.
     * @param vname The name of the vertex associated with this model.
     * @param prevalence The initial prevalence of the disease in the population.
     * @param transmission_rate The rate at which the disease spreads from infected to susceptible individuals.
     * @param recovery_rate The rate at which infected individuals recover and become immune.
     * @param death_rate The rate at which infected individuals die.
     */
    ///@{
    ModelSIRD(
        ModelSIRD<TSeq> & model,
        const std::string & vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate, 
        epiworld_double death_rate
    );

    ModelSIRD(
        const std::string & vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate, 
        epiworld_double death_rate
    );
    ///@}

    /**
     * @brief Set the initial states of the model
     * @param proportions_ Double vector with two elements:
     * - The proportion of non-infected individuals who have recovered.
     * - The proportion of non-infected individuals who have died.
    */
    ModelSIRD<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    );
    
};

template<typename TSeq>
inline ModelSIRD<TSeq>::ModelSIRD(
    ModelSIRD<TSeq> & model,
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate, 
    epiworld_double death_rate
    )
{

    // Adding statuses
    model.add_state("Susceptible", epiworld::default_update_susceptible<TSeq>);
    model.add_state("Infected", epiworld::default_update_exposed<TSeq>);
    model.add_state("Recovered"),
    model.add_state("Deceased")
    ;

    // Setting up parameters
    model.add_param(recovery_rate, "Recovery rate");
    model.add_param(transmission_rate, "Transmission rate"),
    model.add_param(death_rate, "Death rate");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(1,2,3);
    virus.set_prob_recovery(&model("Recovery rate"));
    virus.set_prob_infecting(&model("Transmission rate"));
    virus.set_prob_death(&model("Death rate"));
    
    model.add_virus(virus);

    model.set_name("Susceptible-Infected-Recovered-Deceased (SIRD)");

    return;
   
}

template<typename TSeq>
inline ModelSIRD<TSeq>::ModelSIRD(
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate,
    epiworld_double death_rate
    )
{

    ModelSIRD<TSeq>(
        *this,
        vname,
        prevalence,
        transmission_rate,
        recovery_rate, 
        death_rate
        );

    return;

}

template<typename TSeq>
inline ModelSIRD<TSeq> & ModelSIRD<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /**/ 
) {

    Model<TSeq>::initial_states_fun = 
        create_init_function_sird<TSeq>(proportions_)
        ;

    return *this;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/sird.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/sisd.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SISD_HPP 
#define EPIWORLD_MODELS_SISD_HPP

/**
 * @brief Template for a Susceptible-Infected-Susceptible-Deceased (SISD) model
 * 
 * @param vname std::string Name of the virus
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery_rate rate of the immune system
 * @param inital_death epiworld_double Initial death_rate of the immune system
 */
template<typename TSeq = int>
class ModelSISD : public epiworld::Model<TSeq>
{

public:

    ModelSISD() {};

    ModelSISD(
        ModelSISD<TSeq> & model,
        const std::string & vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate,
        epiworld_double death_rate
    );

    ModelSISD(
        const std::string & vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate,
        epiworld_double death_rate
    );

};

template<typename TSeq>
inline ModelSISD<TSeq>::ModelSISD(
    ModelSISD<TSeq> & model,
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate,
    epiworld_double death_rate
    )
{

    model.set_name("Susceptible-Infected-Susceptible-Deceased (SISD)");

    // Adding statuses
    model.add_state("Susceptible", epiworld::default_update_susceptible<TSeq>);
    model.add_state("Infected", epiworld::default_update_exposed<TSeq>);
    model.add_state("Deceased");

    // Setting up parameters
    model.add_param(transmission_rate, "Transmission rate");
    model.add_param(recovery_rate, "Recovery rate");
    model.add_param(death_rate, "Death rate");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(1,0,2);
    
    virus.set_prob_infecting(&model("Transmission rate"));
    virus.set_prob_recovery(&model("Recovery rate"));
    virus.set_prob_death(0.01);
    
    model.add_virus(virus);

    return;

}

template<typename TSeq>
inline ModelSISD<TSeq>::ModelSISD(
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate,
    epiworld_double death_rate
    )
{

    ModelSISD<TSeq>(
        *this,
        vname,
        prevalence,
        transmission_rate,
        recovery_rate,
        death_rate
    );    

    return;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/sisd.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/seird.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SEIRD_HPP
#define EPIWORLD_MODELS_SEIRD_HPP

/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed-Deceased (SEIRD) model
*/
template<typename TSeq = int>
class ModelSEIRD : public epiworld::Model<TSeq>
{
  
public:
  static const int SUSCEPTIBLE = 0;
  static const int EXPOSED     = 1;
  static const int INFECTED    = 2;
  static const int REMOVED     = 3;
  static const int DECEASED    = 4;
  
  ModelSEIRD() {};
  
  /**
   * @brief Constructor for the SEIRD model.
   * 
   * @tparam TSeq Type of the sequence used in the model.
   * @param model Reference to the SEIRD model.
   * @param vname Name of the model.
   * @param prevalence Prevalence of the disease.
   * @param transmission_rate Transmission rate of the disease.
   * @param avg_incubation_days Average incubation period of the disease.
   * @param recovery_rate Recovery rate of the disease.
   * @param death_rate Death rate of the disease.
   */
  ModelSEIRD(
    ModelSEIRD<TSeq> & model,
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    epiworld_double death_rate
  );
  
  /**
   * @brief Constructor for the SEIRD model.
   * 
   * @param vname Name of the model.
   * @param prevalence Initial prevalence of the disease.
   * @param transmission_rate Transmission rate of the disease.
   * @param avg_incubation_days Average incubation period of the disease.
   * @param recovery_rate Recovery rate of the disease.
   * @param death_rate Death rate of the disease.
   */
  ModelSEIRD(
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    epiworld_double death_rate
  );
  
  epiworld::UpdateFun<TSeq> update_exposed_seir = [](
    epiworld::Agent<TSeq> * p,
    epiworld::Model<TSeq> * m
  ) -> void {

    // Getting the virus
    auto v = p->get_virus();

    // Does the agent become infected?
    if (m->runif() < 1.0/(v->get_incubation(m)))
      p->change_state(m, ModelSEIRD<TSeq>::INFECTED);

    return;
  };

  
  epiworld::UpdateFun<TSeq> update_infected = [](
    epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
  ) -> void {
          
    // Odd: Die, Even: Recover
    epiworld_fast_uint n_events = 0u;

    const auto & v = p->get_virus();
      
    // Die
    m->array_double_tmp[n_events++] = 
      v->get_prob_death(m) * (1.0 - p->get_death_reduction(v, m)); 
    
    // Recover
    m->array_double_tmp[n_events++] = 
      1.0 - (1.0 - v->get_prob_recovery(m)) * (1.0 - p->get_recovery_enhancer(v, m)); 
    
    
#ifdef EPI_DEBUG
    if (n_events == 0u)
    {
      printf_epiworld(
        "[epi-debug] agent %i has 0 possible events!!\n",
        static_cast<int>(p->get_id())
      );
      throw std::logic_error("Zero events in exposed.");
    }
#else
    if (n_events == 0u)
      return;
#endif
    
    
    // Running the roulette
    int which = roulette(n_events, m);
    
    if (which < 0)
      return;
    
    // Which roulette happen?
    if ((which % 2) == 0) // If odd
    {
      
      p->rm_agent_by_virus(m);
      
    } else {
      
      p->rm_virus(m);
      
    }
    
    return ;
      
    
    
    return;
    
  };

  ModelSEIRD<TSeq> & initial_states(
    std::vector< double > proportions_,
    std::vector< int > queue_ = {}
  );

};



template<typename TSeq>
inline ModelSEIRD<TSeq>::ModelSEIRD(
    ModelSEIRD<TSeq> & model,
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    epiworld_double death_rate
)
{
  
  // Adding statuses
  model.add_state("Susceptible", epiworld::default_update_susceptible<TSeq>);
  model.add_state("Exposed",  model.update_exposed_seir);
  model.add_state("Infected", model.update_infected);
  model.add_state("Removed");
  model.add_state("Deceased");
  
  // Setting up parameters
  model.add_param(transmission_rate, "Transmission rate");
  model.add_param(avg_incubation_days, "Incubation days");
  model.add_param(recovery_rate, "Recovery rate");
  model.add_param(death_rate, "Death rate");
  
  // Preparing the virus -------------------------------------------
  epiworld::Virus<TSeq> virus(vname, prevalence, true);
  virus.set_state(ModelSEIRD<TSeq>::EXPOSED, ModelSEIRD<TSeq>::REMOVED, ModelSEIRD<TSeq>::DECEASED);
  
  virus.set_prob_infecting(&model("Transmission rate"));
  virus.set_incubation(&model("Incubation days"));
  virus.set_prob_death(&model("Death rate"));
  virus.set_prob_recovery(&model("Recovery rate"));
  
  // Adding the tool and the virus
  model.add_virus(virus);
  
  model.set_name("Susceptible-Exposed-Infected-Removed-Deceased (SEIRD)");
  
  return;
  
}

template<typename TSeq>
inline ModelSEIRD<TSeq>::ModelSEIRD(
    const std::string & vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    epiworld_double death_rate
)
{
  
  ModelSEIRD<TSeq>(
    *this,
    vname,
    prevalence,
    transmission_rate,
    avg_incubation_days,
    recovery_rate,
    death_rate
  );
  
  return;
  
}

template<typename TSeq>
inline ModelSEIRD<TSeq> & ModelSEIRD<TSeq>::initial_states(
  std::vector< double > proportions_,
  std::vector< int > /**/
) {

  Model<TSeq>::initial_states_fun =
    create_init_function_seird<TSeq>(proportions_)
    ;

  return *this;

}


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/seird.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/sirdconnected.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SIRDCONNECTED_HPP 
#define EPIWORLD_MODELS_SIRDCONNECTED_HPP

template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSIRDCONN : public epiworld::Model<TSeq>
{
public:
    static const int SUSCEPTIBLE = 0;
    static const int INFECTED    = 1;
    static const int RECOVERED   = 2;
    static const int DECEASED    = 3;

    ModelSIRDCONN() {
        
        // tracked_agents_infected.reserve(1e4);
        // tracked_agents_infected_next.reserve(1e4);

    };

    ModelSIRDCONN(
        ModelSIRDCONN<TSeq> & model,
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate, 
        epiworld_double death_rate
    );

    ModelSIRDCONN(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate,
        epiworld_double death_rate
    );

    // Tracking who is infected and who is not
    // std::vector< epiworld::Agent<TSeq>* > tracked_agents_infected = {};
    // std::vector< epiworld::Agent<TSeq>* > tracked_agents_infected_next = {};
    // std::vector< epiworld_double >        tracked_agents_weight        = {};
    // std::vector< epiworld_double >        tracked_agents_weight_next   = {};

    // int tracked_ninfected = 0;
    // int tracked_ninfected_next = 0;
    // epiworld_double tracked_current_infect_prob = 0.0;

    ModelSIRDCONN<TSeq> & run(
        epiworld_fast_uint ndays,
        int seed = -1
    );
    
    void reset();

    Model<TSeq> * clone_ptr();


};

template<typename TSeq>
inline ModelSIRDCONN<TSeq> & ModelSIRDCONN<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{

    Model<TSeq>::run(ndays, seed);

    return *this;

}

template<typename TSeq>
inline void ModelSIRDCONN<TSeq>::reset()
{

    Model<TSeq>::reset();

    // Model<TSeq>::set_rand_binom(
    //     Model<TSeq>::size(),
    //     static_cast<double>(
    //         Model<TSeq>::par("Contact rate"))/
    //         static_cast<double>(Model<TSeq>::size())
    //     );

    return;

}

template<typename TSeq>
inline Model<TSeq> * ModelSIRDCONN<TSeq>::clone_ptr()
{
    
    ModelSIRDCONN<TSeq> * ptr = new ModelSIRDCONN<TSeq>(
        *dynamic_cast<const ModelSIRDCONN<TSeq>*>(this)
        );

    return dynamic_cast< Model<TSeq> *>(ptr);

}

/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence Initial prevalence (proportion)
 * @param contact_rate Average number of contacts (interactions) per step.
 * @param transmission_rate Probability of transmission
 * @param recovery_rate Probability of recovery
 * @param death_rate Probability of death
 */
template<typename TSeq>
inline ModelSIRDCONN<TSeq>::ModelSIRDCONN(
    ModelSIRDCONN<TSeq> & model,
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate,
    epiworld_double death_rate
    // epiworld_double prob_reinfection
    )
{



    epiworld::UpdateFun<TSeq> update_susceptible = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            // Sampling how many individuals
            m->set_rand_binom(
                m->size(),
                static_cast<double>(
                    m->par("Contact rate"))/
                    static_cast<double>(m->size())
            );

            int ndraw = m->rbinom();

            if (ndraw == 0)
                return;

            // Drawing from the set
            int nviruses_tmp = 0;
            for (int i = 0; i < ndraw; ++i)
            {
                // Now selecting who is transmitting the disease
                int which = static_cast<int>(
                    std::floor(m->size() * m->runif())
                );

                /* There is a bug in which runif() returns 1.0. It is rare, but
                 * we saw it here. See the Notes section in the C++ manual
                 * https://en.cppreference.com/mwiki/index.php?title=cpp/numeric/random/uniform_real_distribution&oldid=133329
                 * And the reported bug in GCC:
                 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63176
                 * 
                 */
                if (which == static_cast<int>(m->size()))
                    --which;

                // Can't sample itself
                if (which == static_cast<int>(p->get_id()))
                    continue;

                // If the neighbor is infected, then proceed
                auto & neighbor = m->get_agents()[which];
                if (neighbor.get_state() == ModelSIRDCONN<TSeq>::INFECTED)
                {

                    const auto & v = neighbor.get_virus();
                    
                    #ifdef EPI_DEBUG
                    if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
                        throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                    #endif
                        
                    /* And it is a function of susceptibility_reduction as well */ 
                    m->array_double_tmp[nviruses_tmp] =
                        (1.0 - p->get_susceptibility_reduction(v, m)) * 
                        v->get_prob_infecting(m) * 
                        (1.0 - neighbor.get_transmission_reduction(v, m)) 
                        ; 
                
                    m->array_virus_tmp[nviruses_tmp++] = &(*v);

                }
            }

            // No virus to compute
            if (nviruses_tmp == 0u)
                return;

            // Running the roulette
            int which = roulette(nviruses_tmp, m);

            if (which < 0)
                return;

            p->set_virus(*m->array_virus_tmp[which], m);

            return; 

        };


    epiworld::UpdateFun<TSeq> update_infected = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void {

            auto state = p->get_state();

            if (state == ModelSIRDCONN<TSeq>::INFECTED)
            {


                // Odd: Die, Even: Recover
                epiworld_fast_uint n_events = 0u;
                const auto & v = p->get_virus();
                    
                // Die
                m->array_double_tmp[n_events++] = 
                v->get_prob_death(m) * (1.0 - p->get_death_reduction(v, m)); 
                
                // Recover
                m->array_double_tmp[n_events++] = 
                1.0 - (1.0 - v->get_prob_recovery(m)) * (1.0 - p->get_recovery_enhancer(v, m)); 
                
    #ifdef EPI_DEBUG
                if (n_events == 0u)
                {
                    printf_epiworld(
                    "[epi-debug] agent %i has 0 possible events!!\n",
                    static_cast<int>(p->get_id())
                    );
                    throw std::logic_error("Zero events in exposed.");
                }
    #else
                if (n_events == 0u)
                    return;
    #endif
                
                
                // Running the roulette
                int which = roulette(n_events, m);
                
                if (which < 0)
                    return;
                
                // Which roulette happen?
                if ((which % 2) == 0) // If odd
                {
                    
                    p->rm_agent_by_virus(m);
                    
                } else {
                    
                    p->rm_virus(m);
                    
                }

                return ;

            } else
                throw std::logic_error("This function can only be applied to infected individuals. (SIR)") ;

            return;

        };

    // state
    model.add_state("Susceptible", update_susceptible);
    model.add_state("Infected", update_infected);
    model.add_state("Recovered");
    model.add_state("Deceased");
      

    // Setting up parameters
    model.add_param(contact_rate, "Contact rate");
    model.add_param(transmission_rate, "Transmission rate");
    model.add_param(recovery_rate, "Recovery rate");
    model.add_param(death_rate, "Death rate");
    // model.add_param(prob_reinfection, "Prob. Reinfection");
    
    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(1, 2, 3);
    virus.set_prob_infecting(&model("Transmission rate"));
    virus.set_prob_recovery(&model("Recovery rate"));
    virus.set_prob_death(&model("Death rate"));
    
    model.add_virus(virus);

    model.queuing_off(); // No queuing need

    model.agents_empty_graph(n);

    model.set_name("Susceptible-Infected-Removed-Deceased (SIRD) (connected)");

    return;

}

template<typename TSeq>
inline ModelSIRDCONN<TSeq>::ModelSIRDCONN(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate,
    epiworld_double death_rate
    )
{

    ModelSIRDCONN(
        *this,
        vname,
        n,
        prevalence,
        contact_rate,
        transmission_rate,
        recovery_rate,
        death_rate
    );

    return;

}


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/sirdconnected.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/seirdconnected.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SEIRDCONNECTED_HPP
#define EPIWORLD_MODELS_SEIRDCONNECTED_HPP

template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSEIRDCONN : public epiworld::Model<TSeq> 
{
private:
    std::vector< epiworld::Agent<TSeq> * > infected;
    void update_infected();

public:

    static const int SUSCEPTIBLE = 0;
    static const int EXPOSED     = 1;
    static const int INFECTED    = 2;
    static const int REMOVED     = 3;
    static const int DECEASED    = 4;

    ModelSEIRDCONN() {};

    ModelSEIRDCONN(
        ModelSEIRDCONN<TSeq> & model,
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate,
        epiworld_double death_rate
    );
    
    ModelSEIRDCONN(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate,
        epiworld_double death_rate
    );

    ModelSEIRDCONN<TSeq> & run(
        epiworld_fast_uint ndays,
        int seed = -1
    );

    void reset();

    Model<TSeq> * clone_ptr();

    /**
     * @brief Set up the initial states of the model.
     * @param proportions_ Double vector with the following values:
     * - 0: Proportion of non-infected agents who are removed.
     * - 1: Proportion of exposed agents to be set as infected.
    */
    ModelSEIRDCONN<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    );

    size_t get_n_infected() const
    {
        return infected.size();
    }

};

template<typename TSeq>
inline void ModelSEIRDCONN<TSeq>::update_infected()
{
    infected.clear();
    infected.reserve(this->size());

    for (auto & p : this->get_agents())
    {
        if (p.get_state() == ModelSEIRDCONN<TSeq>::INFECTED)
        {
            infected.push_back(&p);
        }
    }

    Model<TSeq>::set_rand_binom(
        this->get_n_infected(),
        static_cast<double>(Model<TSeq>::par("Contact rate"))/
            static_cast<double>(Model<TSeq>::size())
    );

    return; 
}

template<typename TSeq>
inline ModelSEIRDCONN<TSeq> & ModelSEIRDCONN<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{
    
    Model<TSeq>::run(ndays, seed);

    return *this;

}

template<typename TSeq>
inline void ModelSEIRDCONN<TSeq>::reset()
{

    Model<TSeq>::reset();

    this->update_infected();

    return;

}

template<typename TSeq>
inline Model<TSeq> * ModelSEIRDCONN<TSeq>::clone_ptr()
{
    
    ModelSEIRDCONN<TSeq> * ptr = new ModelSEIRDCONN<TSeq>(
        *dynamic_cast<const ModelSEIRDCONN<TSeq>*>(this)
        );

    return dynamic_cast< Model<TSeq> *>(ptr);

}

/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence Initial prevalence (proportion)
 * @param contact_rate Average number of contacts (interactions) per step.
 * @param transmission_rate Probability of transmission
 * @param recovery_rate Probability of recovery
 * @param death_rate Probability of death
 */
template<typename TSeq>
inline ModelSEIRDCONN<TSeq>::ModelSEIRDCONN(
    ModelSEIRDCONN<TSeq> & model,
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    epiworld_double death_rate
    // epiworld_double prob_reinfection
    )
{

    epiworld::UpdateFun<TSeq> update_susceptible = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            // Sampling how many individuals
            int ndraw = m->rbinom();

            if (ndraw == 0)
                return;

            ModelSEIRDCONN<TSeq> * model = dynamic_cast<ModelSEIRDCONN<TSeq> *>(
                m
                );

            size_t ninfected = model->get_n_infected();

            // Drawing from the set
            int nviruses_tmp = 0;
            for (int i = 0; i < ndraw; ++i)
            {
                // Now selecting who is transmitting the disease
                int which = static_cast<int>(
                    std::floor(ninfected * m->runif())
                );

                /* There is a bug in which runif() returns 1.0. It is rare, but
                 * we saw it here. See the Notes section in the C++ manual
                 * https://en.cppreference.com/mwiki/index.php?title=cpp/numeric/random/uniform_real_distribution&oldid=133329
                 * And the reported bug in GCC:
                 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63176
                 * 
                 */
                if (which == static_cast<int>(ninfected))
                    --which;

                epiworld::Agent<TSeq> & neighbor = *model->infected[which];

                // Can't sample itself
                if (neighbor.get_id() == p->get_id())
                    continue;

                // All neighbors in this set are infected by construction
                const auto & v = neighbor.get_virus();
            
                #ifdef EPI_DEBUG
                if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
                    throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                #endif
                    
                /* And it is a function of susceptibility_reduction as well */ 
                m->array_double_tmp[nviruses_tmp] =
                    (1.0 - p->get_susceptibility_reduction(v, m)) * 
                    v->get_prob_infecting(m) * 
                    (1.0 - neighbor.get_transmission_reduction(v, m)) 
                    ; 
            
                m->array_virus_tmp[nviruses_tmp++] = &(*v);
            }

            // No virus to compute
            if (nviruses_tmp == 0u)
                return;

            // Running the roulette
            int which = roulette(nviruses_tmp, m);

            if (which < 0)
                return;

            p->set_virus(
                *m->array_virus_tmp[which],
                m,
                ModelSEIRDCONN<TSeq>::EXPOSED
                );

            return; 

        };

    epiworld::UpdateFun<TSeq> update_infected = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void {

            auto state = p->get_state();

            if (state == ModelSEIRDCONN<TSeq>::EXPOSED)
            {

                // Getting the virus
                auto & v = p->get_virus();

                // Does the agent become infected?
                if (m->runif() < 1.0/(v->get_incubation(m)))
                {

                    p->change_state(m, ModelSEIRDCONN<TSeq>::INFECTED);
                    return;

                }


            } else if (state == ModelSEIRDCONN<TSeq>::INFECTED)
            {

                // Odd: Die, Even: Recover
                epiworld_fast_uint n_events = 0u;
                const auto & v = p->get_virus();
                
                // Die
                m->array_double_tmp[n_events++] = 
                    v->get_prob_death(m) * (1.0 - p->get_death_reduction(v, m)); 
                
                // Recover
                m->array_double_tmp[n_events++] = 
                    1.0 - (1.0 - v->get_prob_recovery(m)) * (1.0 - p->get_recovery_enhancer(v, m)); 
                                
                #ifdef EPI_DEBUG
                if (n_events == 0u)
                {
                printf_epiworld(
                    "[epi-debug] agent %i has 0 possible events!!\n",
                    static_cast<int>(p->get_id())
                );
                throw std::logic_error("Zero events in exposed.");
                }
                #else
                if (n_events == 0u)
                return;
                #endif
                
                
                // Running the roulette
                int which = roulette(n_events, m);
                
                if (which < 0)
                return;
                
                // Which roulette happen?
                if ((which % 2) == 0) // If odd
                {
                
                    p->rm_agent_by_virus(m);
                
                } else {
                
                    p->rm_virus(m);
                
                }

                return ;

            } else
                throw std::logic_error("This function can only be applied to exposed or infected individuals. (SEIRD)") ;

            return;

        };

    // Setting up parameters
    model.add_param(contact_rate, "Contact rate");
    model.add_param(transmission_rate, "Prob. Transmission");
    model.add_param(recovery_rate, "Prob. Recovery");
    model.add_param(avg_incubation_days, "Avg. Incubation days");
    model.add_param(death_rate, "Death rate");
    
    // state
    model.add_state("Susceptible", update_susceptible);
    model.add_state("Exposed", update_infected);
    model.add_state("Infected", update_infected);
    model.add_state("Removed");
    model.add_state("Deceased");


    // Adding update function
    epiworld::GlobalFun<TSeq> update = [](epiworld::Model<TSeq> * m) -> void
    {
        ModelSEIRDCONN<TSeq> * model = dynamic_cast<ModelSEIRDCONN<TSeq> *>(m);
        
        if (model == nullptr)
            throw std::logic_error(
                std::string("Internal error in the ModelSEIRDCONN model: ") +
                std::string("The model returns a null pointer.")
            );
        else
            model->update_infected();
        
        return;
    };

    model.add_globalevent(update, "Update infected individuals");


    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(
        ModelSEIRDCONN<TSeq>::EXPOSED,
        ModelSEIRDCONN<TSeq>::REMOVED,
        ModelSEIRDCONN<TSeq>::DECEASED
        );

    virus.set_prob_infecting(&model("Prob. Transmission"));
    virus.set_prob_recovery(&model("Prob. Recovery"));
    virus.set_incubation(&model("Avg. Incubation days"));
    virus.set_prob_death(&model("Death rate"));
    model.add_virus(virus);

    model.queuing_off(); // No queuing need

    // Adding the empty population
    model.agents_empty_graph(n);

    model.set_name("Susceptible-Exposed-Infected-Removed-Deceased (SEIRD) (connected)");

    return;

}

template<typename TSeq>
inline ModelSEIRDCONN<TSeq>::ModelSEIRDCONN(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    epiworld_double death_rate
    )
{

    ModelSEIRDCONN(
        *this,
        vname,
        n,
        prevalence,
        contact_rate,
        transmission_rate,
        avg_incubation_days,
        recovery_rate,
        death_rate
    );

    return;

}

template<typename TSeq>
inline ModelSEIRDCONN<TSeq> & ModelSEIRDCONN<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /**/
) {

    Model<TSeq>::initial_states_fun =
        create_init_function_seird<TSeq>(proportions_)
        ;

    return *this;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/seirdconnected.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/sirlogit.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


// #include "../epiworld.hpp"

#ifndef EPIWORLD_MODELS_SIRLOGIT_HPP 
#define EPIWORLD_MODELS_SIRLOGIT_HPP


/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @details
 * In this model, infection and recoveru probabilities are computed
 * using a logit model. Particularly, the probability of infection
 * is computed as:
 * 
 * \f[
 * \frac{1}{1 + \exp\left(-\left(\beta_0 E_i + \sum_{i=1}^{n} \beta_i x_i\right)\right)}
 * \f]
 * 
 * where \f$\beta_0\f$ is the exposure coefficient and \f$E_i\f$ is the exposure
 * number, \f$\beta_i\f$ are the
 * coefficients for the features \f$x_i\f$ of the agents, and \f$n\f$ is the
 * number of features. The probability of recovery is computed as:
 * 
 * \f[
 * \frac{1}{1 + \exp\left(-\left(\sum_{i=1}^{n} \beta_i x_i\right)\right)}
 * \f]
 * 
 * where \f$\beta_i\f$ are the coefficients for the features \f$x_i\f$ of the agents,
 * and \f$n\f$ is the number of features.
 * 
 * @param TSeq Type of the sequence (e.g. std::vector, std::deque)
 
*/
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSIRLogit : public epiworld::Model<TSeq>
{
private:
    static const int SUSCEPTIBLE = 0;
    static const int INFECTED    = 1;
    static const int RECOVERED   = 2;

public:

    ModelSIRLogit() {};

    /**
      * @param vname Name of the virus.
      * @param coefs_infect Double ptr. Infection coefficients.
      * @param coefs_recover Double ptr. Recovery coefficients.
      * @param ncoef_infect Unsigned int. Number of infection coefficients.
      * @param ncoef_recover Unsigned int. Number of recovery coefficients.
      * @param coef_infect_cols Vector<unsigned int>. Ids of infection vars.
      * @param coef_recover_cols Vector<unsigned int>. Ids of recover vars.
    */
    ModelSIRLogit(
        ModelSIRLogit<TSeq> & model,
        const std::string & vname,
        double * data,
        size_t ncols,
        std::vector< double > coefs_infect,
        std::vector< double > coefs_recover,
        std::vector< size_t > coef_infect_cols,
        std::vector< size_t > coef_recover_cols,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate,
        epiworld_double prevalence
    );

    ModelSIRLogit(
        const std::string & vname,
        double * data,
        size_t ncols,
        std::vector< double > coefs_infect,
        std::vector< double > coefs_recover,
        std::vector< size_t > coef_infect_cols,
        std::vector< size_t > coef_recover_cols,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate,
        epiworld_double prevalence
    );

    ModelSIRLogit<TSeq> & run(
        epiworld_fast_uint ndays,
        int seed = -1
    );

    Model<TSeq> * clone_ptr();

    void reset();
    
    std::vector< double > coefs_infect;
    std::vector< double > coefs_recover;
    std::vector< size_t > coef_infect_cols;
    std::vector< size_t > coef_recover_cols;

};



template<typename TSeq>
inline ModelSIRLogit<TSeq> & ModelSIRLogit<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{

    Model<TSeq>::run(ndays, seed);
    return *this;

}

template<typename TSeq>
inline Model<TSeq> * ModelSIRLogit<TSeq>::clone_ptr()
{
    
    ModelSIRLogit<TSeq> * ptr = new ModelSIRLogit<TSeq>(
        *dynamic_cast<const ModelSIRLogit<TSeq>*>(this)
        );

    return dynamic_cast< Model<TSeq> *>(ptr);

}

template<typename TSeq>
inline void ModelSIRLogit<TSeq>::reset()
{

    /* Checking specified columns in the model */
    for (const auto & c : coef_infect_cols)
    {
        if (c >= Model<TSeq>::agents_data_ncols)
            throw std::range_error("Columns specified in coef_infect_cols out of range.");
    }

    for (const auto & c : coef_recover_cols)
    {
        if (c >= Model<TSeq>::agents_data_ncols)
            throw std::range_error("Columns specified in coef_recover_cols out of range.");
    }

    /* Checking attributes */ 
    if (coefs_infect.size() != (coef_infect_cols.size() + 1u))
        throw std::logic_error(
            "The number of coefficients (infection) doesn't match the number of features. It must be as many features of the agents plus 1 (exposure.)"
            );

    if (coefs_recover.size() != coef_recover_cols.size())
        throw std::logic_error(
            "The number of coefficients (recovery) doesn't match the number of features. It must be as many features of the agents."
            );
    
    Model<TSeq>::reset();

    return;

}

/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence Initial prevalence (proportion)
 * @param contact_rate Average number of contacts (interactions) per step.
 * @param prob_transmission Probability of transmission
 * @param prob_recovery Probability of recovery
 */
template<typename TSeq>
inline ModelSIRLogit<TSeq>::ModelSIRLogit(
    ModelSIRLogit<TSeq> & model,
    const std::string & vname,
    double * data,
    size_t ncols,
    std::vector< double > coefs_infect,
    std::vector< double > coefs_recover,
    std::vector< size_t > coef_infect_cols,
    std::vector< size_t > coef_recover_cols,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate,
    epiworld_double prevalence
    )
{

    if (coef_infect_cols.size() == 0u)
        throw std::logic_error("No columns specified for coef_infect_cols.");

    if (coef_recover_cols.size() == 0u)
        throw std::logic_error("No columns specified for coef_recover_cols.");

    // Saving the variables
    model.set_agents_data(
        data, ncols
    );

    model.coefs_infect = coefs_infect;
    model.coefs_recover = coefs_recover;
    model.coef_infect_cols = coef_infect_cols;
    model.coef_recover_cols = coef_recover_cols;

    epiworld::UpdateFun<TSeq> update_susceptible = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            // Getting the right type
            ModelSIRLogit<TSeq> * _m = dynamic_cast<ModelSIRLogit<TSeq>*>(m);

            // Exposure coefficient
            const double coef_exposure = _m->coefs_infect[0u];

            // This computes the prob of getting any neighbor variant
            size_t nviruses_tmp = 0u;

            double baseline = 0.0;
            for (size_t k = 0u; k < _m->coef_infect_cols.size(); ++k)
                baseline += p->operator[](k) * _m->coefs_infect[k + 1u];

            for (auto & neighbor: p->get_neighbors()) 
            {
                
                if (neighbor->get_virus() == nullptr)
                    continue;

                auto & v = neighbor->get_virus();

                #ifdef EPI_DEBUG
                if (nviruses_tmp >= m->array_virus_tmp.size())
                    throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                #endif
                    
                /* And it is a function of susceptibility_reduction as well */ 
                m->array_double_tmp[nviruses_tmp] =
                    baseline +
                    (1.0 - p->get_susceptibility_reduction(v, m)) * 
                    v->get_prob_infecting(m) * 
                    (1.0 - neighbor->get_transmission_reduction(v, m))  *
                    coef_exposure
                    ; 

                // Applying the plogis function
                m->array_double_tmp[nviruses_tmp] = 1.0/
                    (1.0 + std::exp(-m->array_double_tmp[nviruses_tmp]));
            
                m->array_virus_tmp[nviruses_tmp++] = &(*v);

            }

            // No virus to compute
            if (nviruses_tmp == 0u)
                return;

            // Running the roulette
            int which = roulette(nviruses_tmp, m);

            if (which < 0)
                return;

            p->set_virus(*m->array_virus_tmp[which], m);

            return;

        };

    epiworld::UpdateFun<TSeq> update_infected = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            // Getting the right type
            ModelSIRLogit<TSeq> * _m = dynamic_cast<ModelSIRLogit<TSeq>*>(m);

            // Computing recovery probability once
            double prob    = 0.0;
            #if defined(__OPENMP) || defined(_OPENMP)
            #pragma omp simd reduction(+:prob)
            #endif
            for (size_t i = 0u; i < _m->coefs_recover.size(); ++i)
                prob += p->operator[](i) * _m->coefs_recover[i];

            // Computing logis
            prob = 1.0/(1.0 + std::exp(-prob));

            if (prob > m->runif())
                p->rm_virus(m);
            
            return;

        };

    // state
    model.add_state("Susceptible", update_susceptible);
    model.add_state("Infected", update_infected);
    model.add_state("Recovered");

    // Setting up parameters
    // model.add_param(contact_rate, "Contact rate");
    model.add_param(transmission_rate, "Transmission rate");
    model.add_param(recovery_rate, "Recovery rate");
    // model.add_param(prob_reinfection, "Prob. Reinfection");
    
    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(
        ModelSIRLogit<TSeq>::INFECTED,
        ModelSIRLogit<TSeq>::RECOVERED,
        ModelSIRLogit<TSeq>::RECOVERED
        );

    virus.set_prob_infecting(&model("Transmission rate"));
    virus.set_prob_recovery(&model("Recovery rate"));

    // virus.set_prob

    model.add_virus(virus);

    model.set_name("Susceptible-Infected-Removed (SIR) (logit)");

    return;

}

template<typename TSeq>
inline ModelSIRLogit<TSeq>::ModelSIRLogit(
    const std::string & vname,
    double * data,
    size_t ncols,
    std::vector< double > coefs_infect,
    std::vector< double > coefs_recover,
    std::vector< size_t > coef_infect_cols,
    std::vector< size_t > coef_recover_cols,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate,
    epiworld_double prevalence
    )
{

    ModelSIRLogit(
        *this,
        vname,
        data,
        ncols,
        coefs_infect,
        coefs_recover,
        coef_infect_cols,
        coef_recover_cols,
        transmission_rate,
        recovery_rate,
        prevalence
    );

    return;

}


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/sirlogit.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/diffnet.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_DIFFNET_H 
#define EPIWORLD_DIFFNET_H

/**
 * @brief Template for a Network Diffusion Model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery rate of the immune system
 */
template<typename TSeq = int>
class ModelDiffNet : public epiworld::Model<TSeq>
{
private:
public:

    ModelDiffNet() {};

    ModelDiffNet(
        ModelDiffNet<TSeq> & model,
        const std::string & innovation_name,
        epiworld_double prevalence,
        epiworld_double prob_adopt,
        bool normalize_exposure = true,
        double * agents_data = nullptr,
        size_t data_ncols = 0u,
        std::vector< size_t > data_cols = {},
        std::vector< double > params = {}
    );

    ModelDiffNet(
        const std::string & innovation_name,
        epiworld_double prevalence,
        epiworld_double prob_adopt,
        bool normalize_exposure = true,
        double * agents_data = nullptr,
        size_t data_ncols = 0u,
        std::vector< size_t > data_cols = {},
        std::vector< double > params = {}
    );
    
    static const int NONADOPTER = 0;
    static const int ADOPTER    = 1;

    bool normalize_exposure = true;
    std::vector< size_t > data_cols;
    std::vector< double > params;
};

template<typename TSeq>
inline ModelDiffNet<TSeq>::ModelDiffNet(
    ModelDiffNet<TSeq> & model,
    const std::string & innovation_name,
    epiworld_double prevalence,
    epiworld_double prob_adopt,
    bool normalize_exposure,
    double * agents_data,
    size_t data_ncols,
    std::vector< size_t > data_cols,
    std::vector< double > params
    )
{

    // Adding additional parameters
    this->normalize_exposure = normalize_exposure;
    this->data_cols = data_cols;
    this->params = params;

    epiworld::UpdateFun<TSeq> update_non_adopters = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
    ) -> void {

        // Measuring exposure
        // If the neighbor is infected, then proceed
        size_t nviruses = m->get_n_viruses();
        std::vector< Virus<TSeq>* > innovations(nviruses, {});
        std::vector< bool > stored(nviruses, false);
        std::vector< double > exposure(nviruses, 0.0);

        ModelDiffNet<TSeq> * diffmodel = dynamic_cast<ModelDiffNet<TSeq>*>(m);

        Agent<TSeq> & agent = *p;

        // For each one of the possible innovations, we have to compute
        // the adoption probability, which is a function of exposure
        for (auto & neighbor: agent.get_neighbors())
        {

            if (neighbor->get_state() == ModelDiffNet<TSeq>::ADOPTER)
            {

                auto & v = neighbor->get_virus();
                
                if (v == nullptr)
                    continue;
    
                /* And it is a function of susceptibility_reduction as well */ 
                double p_i =
                    (1.0 - agent.get_susceptibility_reduction(v, m)) * 
                    (1.0 - agent.get_transmission_reduction(v, m)) 
                    ; 
            
                size_t vid = v->get_id();
                if (!stored[vid])
                {
                    stored[vid] = true;
                    innovations[vid] = &(*v);
                }
                exposure[vid] += p_i;


            }

        }

        // Computing probability of adoption
        for (size_t i = 0u; i < nviruses; ++i)
        {

            if (diffmodel->normalize_exposure)
                exposure.at(i) /= agent.get_n_neighbors();

            for (auto & j: diffmodel->data_cols)
                exposure.at(i) += agent(j) * diffmodel->params.at(j);

            // Baseline probability of adoption
            double p = m->get_viruses()[i]->get_prob_infecting(m);
            exposure.at(i) += std::log(p) - std::log(1.0 - p);

            // Computing as log
            exposure.at(i) = 1.0/(1.0 + std::exp(-exposure.at(i)));

        }

        // Running the roulette to see is an innovation is adopted
        int which = roulette<int>(exposure, m);

        // No innovation was adopted
        if (which < 0)
            return;

        // Otherwise, it is adopted from any of the neighbors
        agent.set_virus(
            *innovations.at(which),
            m,
            ModelDiffNet::ADOPTER
        );

        return;

        };

    // Adding agents data
    model.set_agents_data(agents_data, data_ncols);
    
    // Adding statuses
    model.add_state("Non adopters", update_non_adopters);
    model.add_state("Adopters");

    // Adding parameters
    std::string parname = std::string("Prob. Adopting ") + innovation_name;
    model.add_param(prob_adopt, parname);

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> innovation(innovation_name, prevalence, true);
    innovation.set_state(1,1,1);
    
    innovation.set_prob_infecting(&model(parname));
    
    model.add_virus(innovation);

    model.set_name(
        std::string("Diffusion of Innovations - ") + innovation_name);

    return;
   
}

template<typename TSeq>
inline ModelDiffNet<TSeq>::ModelDiffNet(
    const std::string & innovation_name,
    epiworld_double prevalence,
    epiworld_double prob_adopt,
    bool normalize_exposure,
    double * agents_data,
    size_t data_ncols,
    std::vector< size_t > data_cols,
    std::vector< double > params
    )
{

    ModelDiffNet<TSeq>(
        *this,
        innovation_name,
        prevalence,
        prob_adopt,
        normalize_exposure,
        agents_data,
        data_ncols,
        data_cols,
        params
        );

    return;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/diffnet.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/seirmixing.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SEIRMIXING_HPP
#define EPIWORLD_MODELS_SEIRMIXING_HPP

/**
 * @file seirentitiesconnected.hpp
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model with mixing
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSEIRMixing : public epiworld::Model<TSeq> 
{
private:
    std::vector< std::vector< epiworld::Agent<TSeq> * > > infected;
    void update_infected();
    std::vector< epiworld::Agent<TSeq> * > sampled_agents;
    size_t sample_agents(
        epiworld::Agent<TSeq> * agent,
        std::vector< epiworld::Agent<TSeq> * > & sampled_agents
        );
    double adjusted_contact_rate;
    std::vector< double > contact_matrix;

    size_t index(size_t i, size_t j, size_t n) {
        return j * n + i;
    }

public:

    static const int SUSCEPTIBLE = 0;
    static const int EXPOSED     = 1;
    static const int INFECTED    = 2;
    static const int RECOVERED   = 3;

    ModelSEIRMixing() {};
    
    /**
     * @brief Constructs a ModelSEIRMixing object.
     *
     * @param model A reference to an existing ModelSEIRMixing object.
     * @param vname The name of the ModelSEIRMixing object.
     * @param n The number of entities in the model.
     * @param prevalence The initial prevalence of the disease in the model.
     * @param contact_rate The contact rate between entities in the model.
     * @param transmission_rate The transmission rate of the disease in the model.
     * @param avg_incubation_days The average incubation period of the disease in the model.
     * @param recovery_rate The recovery rate of the disease in the model.
     * @param contact_matrix The contact matrix between entities in the model. Specified in
     * column-major order.
     */
    ModelSEIRMixing(
        ModelSEIRMixing<TSeq> & model,
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate,
        std::vector< double > contact_matrix
    );
    
    /**
     * @brief Constructs a ModelSEIRMixing object.
     *
     * @param vname The name of the ModelSEIRMixing object.
     * @param n The number of entities in the model.
     * @param prevalence The initial prevalence of the disease in the model.
     * @param contact_rate The contact rate between entities in the model.
     * @param transmission_rate The transmission rate of the disease in the model.
     * @param avg_incubation_days The average incubation period of the disease in the model.
     * @param recovery_rate The recovery rate of the disease in the model.
     * @param contact_matrix The contact matrix between entities in the model.
     */
    ModelSEIRMixing(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate,
        std::vector< double > contact_matrix
    );

    ModelSEIRMixing<TSeq> & run(
        epiworld_fast_uint ndays,
        int seed = -1
    );

    void reset();

    Model<TSeq> * clone_ptr();

    /**
     * @brief Set the initial states of the model
     * @param proportions_ Double vector with a single element:
     * - The proportion of non-infected individuals who have recovered.
    */
    ModelSEIRMixing<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    );

    size_t get_n_infected(size_t group) const
    {
        return infected[group].size();
    }

    void set_contact_matrix(std::vector< double > cmat)
    {
        contact_matrix = cmat;
        return;
    };

};

template<typename TSeq>
inline void ModelSEIRMixing<TSeq>::update_infected()
{

    auto & agents = Model<TSeq>::get_agents();
    auto & entities = Model<TSeq>::get_entities();

    infected.resize(entities.size());
    sampled_agents.resize(agents.size());

    // Checking contact matrix's rows add to one
    size_t nentities = entities.size();
    if (this->contact_matrix.size() !=  nentities*nentities)
        throw std::length_error(
            std::string("The contact matrix must be a square matrix of size ") +
            std::string("nentities x nentities. ") +
            std::to_string(this->contact_matrix.size()) +
            std::string(" != ") + std::to_string(nentities*nentities) +
            std::string(".")
            );

    for (size_t i = 0u; i < entities.size(); ++i)
    {
        double sum = 0.0;
        for (size_t j = 0u; j < entities.size(); ++j)
        {
            if (this->contact_matrix[index(i, j, nentities)] < 0.0)
                throw std::range_error(
                    std::string("The contact matrix must be non-negative. ") +
                    std::to_string(this->contact_matrix[index(i, j, nentities)]) +
                    std::string(" < 0.")
                    );
            sum += this->contact_matrix[index(i, j, nentities)];
        }
        if (sum < 0.999 || sum > 1.001)
            throw std::range_error(
                std::string("The contact matrix must have rows that add to one. ") +
                std::to_string(sum) +
                std::string(" != 1.")
                );
    }

    for (size_t i = 0; i < entities.size(); ++i)
    {
        infected[i].clear();
        infected[i].reserve(agents.size());
    }
    
    for (auto & a : agents)
    {

        if (a.get_state() == ModelSEIRMixing<TSeq>::INFECTED)
        {
            if (a.get_n_entities() > 0u)
                infected[a.get_entity(0u).get_id()].push_back(&a);
        }

    }

    // Adjusting contact rate
    adjusted_contact_rate = Model<TSeq>::get_param("Contact rate") /
        agents.size();

    return;

}

template<typename TSeq>
inline size_t ModelSEIRMixing<TSeq>::sample_agents(
    epiworld::Agent<TSeq> * agent,
    std::vector< epiworld::Agent<TSeq> * > & sampled_agents
    )
{

    size_t agent_group_id = agent->get_entity(0u).get_id();
    size_t ngroups = infected.size();

    int samp_id = 0;
    for (size_t g = 0; g < infected.size(); ++g)
    {

        // How many from this entity?
        int nsamples = epiworld::Model<TSeq>::rbinom(
            infected[g].size(),
            adjusted_contact_rate * contact_matrix[
                index(agent_group_id, g, ngroups)
            ]
        );

        if (nsamples == 0)
            continue;

        // Sampling from the entity
        for (int s = 0; s < nsamples; ++s)
        {

            // Randomly selecting an agent
            int which = epiworld::Model<TSeq>::runif() * infected[g].size();

            // Correcting overflow error
            if (which >= static_cast<int>(infected[g].size()))
                which = static_cast<int>(infected[g].size()) - 1;

            auto & a = infected[g][which];

            // Can't sample itself
            if (a->get_id() == agent->get_id())
                continue;

            sampled_agents[samp_id++] = a;
            
        }

    }
    
    return samp_id;

}

template<typename TSeq>
inline ModelSEIRMixing<TSeq> & ModelSEIRMixing<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{

    Model<TSeq>::run(ndays, seed);
    return *this;

}

template<typename TSeq>
inline void ModelSEIRMixing<TSeq>::reset()
{

    Model<TSeq>::reset();
    this->update_infected();

    return;

}

template<typename TSeq>
inline Model<TSeq> * ModelSEIRMixing<TSeq>::clone_ptr()
{
    
    ModelSEIRMixing<TSeq> * ptr = new ModelSEIRMixing<TSeq>(
        *dynamic_cast<const ModelSEIRMixing<TSeq>*>(this)
        );

    return dynamic_cast< Model<TSeq> *>(ptr);

}


/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence Initial prevalence (proportion)
 * @param contact_rate Average number of contacts (interactions) per step.
 * @param transmission_rate Probability of transmission
 * @param recovery_rate Probability of recovery
 */
template<typename TSeq>
inline ModelSEIRMixing<TSeq>::ModelSEIRMixing(
    ModelSEIRMixing<TSeq> & model,
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    std::vector< double > contact_matrix
    )
{

    // Setting up the contact matrix
    this->contact_matrix = contact_matrix;

    epiworld::UpdateFun<TSeq> update_susceptible = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            if (p->get_n_entities() == 0)
                return;

            // Downcasting to retrieve the sampler attached to the
            // class
            ModelSEIRMixing<TSeq> * m_down =
                dynamic_cast<ModelSEIRMixing<TSeq> *>(m);

            size_t ndraws = m_down->sample_agents(p, m_down->sampled_agents);

            if (ndraws == 0u)
                return;

            
            // Drawing from the set
            int nviruses_tmp = 0;
            for (size_t n = 0u; n < ndraws; ++n)
            {

                auto & neighbor = m_down->sampled_agents[n];

                auto & v = neighbor->get_virus();

                #ifdef EPI_DEBUG
                if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
                    throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                #endif
                    
                /* And it is a function of susceptibility_reduction as well */ 
                m->array_double_tmp[nviruses_tmp] =
                    (1.0 - p->get_susceptibility_reduction(v, m)) * 
                    v->get_prob_infecting(m) * 
                    (1.0 - neighbor->get_transmission_reduction(v, m)) 
                    ; 
            
                m->array_virus_tmp[nviruses_tmp++] = &(*v);

            }

            // Running the roulette
            int which = roulette(nviruses_tmp, m);

            if (which < 0)
                return;

            p->set_virus(
                *m->array_virus_tmp[which],
                m,
                ModelSEIRMixing<TSeq>::EXPOSED
                );

            return; 

        };

    epiworld::UpdateFun<TSeq> update_infected = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void {

            auto state = p->get_state();

            if (state == ModelSEIRMixing<TSeq>::EXPOSED)
            {

                // Getting the virus
                auto & v = p->get_virus();

                // Does the agent become infected?
                if (m->runif() < 1.0/(v->get_incubation(m)))
                {

                    p->change_state(m, ModelSEIRMixing<TSeq>::INFECTED);
                    return;

                }


            } else if (state == ModelSEIRMixing<TSeq>::INFECTED)
            {


                // Odd: Die, Even: Recover
                epiworld_fast_uint n_events = 0u;
                const auto & v = p->get_virus();

                // Recover
                m->array_double_tmp[n_events++] = 
                    1.0 - (1.0 - v->get_prob_recovery(m)) * (1.0 - p->get_recovery_enhancer(v, m)); 

                #ifdef EPI_DEBUG
                if (n_events == 0u)
                {
                    printf_epiworld(
                        "[epi-debug] agent %i has 0 possible events!!\n",
                        static_cast<int>(p->get_id())
                        );
                    throw std::logic_error("Zero events in exposed.");
                }
                #else
                if (n_events == 0u)
                    return;
                #endif
                

                // Running the roulette
                int which = roulette(n_events, m);

                if (which < 0)
                    return;

                // Which roulette happen?
                p->rm_virus(m);

                return ;

            } else
                throw std::logic_error("This function can only be applied to exposed or infected individuals. (SEIR)") ;

            return;

        };

    // Setting up parameters
    model.add_param(contact_rate, "Contact rate");
    model.add_param(transmission_rate, "Prob. Transmission");
    model.add_param(recovery_rate, "Prob. Recovery");
    model.add_param(avg_incubation_days, "Avg. Incubation days");
    
    // state
    model.add_state("Susceptible", update_susceptible);
    model.add_state("Exposed", update_infected);
    model.add_state("Infected", update_infected);
    model.add_state("Recovered");

    // Global function
    epiworld::GlobalFun<TSeq> update = [](epiworld::Model<TSeq> * m) -> void
    {

        ModelSEIRMixing<TSeq> * m_down =
            dynamic_cast<ModelSEIRMixing<TSeq> *>(m);

        m_down->update_infected();

        return;

    };

    model.add_globalevent(update, "Update infected individuals");


    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(
        ModelSEIRMixing<TSeq>::EXPOSED,
        ModelSEIRMixing<TSeq>::RECOVERED,
        ModelSEIRMixing<TSeq>::RECOVERED
        );

    virus.set_prob_infecting(&model("Prob. Transmission"));
    virus.set_prob_recovery(&model("Prob. Recovery"));
    virus.set_incubation(&model("Avg. Incubation days"));

    model.add_virus(virus);

    model.queuing_off(); // No queuing need

    // Adding the empty population
    model.agents_empty_graph(n);

    model.set_name("Susceptible-Exposed-Infected-Removed (SEIR) with Mixing");

    return;

}

template<typename TSeq>
inline ModelSEIRMixing<TSeq>::ModelSEIRMixing(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    std::vector< double > contact_matrix
    )
{   

    this->contact_matrix = contact_matrix;

    ModelSEIRMixing(
        *this,
        vname,
        n,
        prevalence,
        contact_rate,
        transmission_rate,
        avg_incubation_days,
        recovery_rate,
        contact_matrix
    );

    return;

}

template<typename TSeq>
inline ModelSEIRMixing<TSeq> & ModelSEIRMixing<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
)
{

    Model<TSeq>::initial_states_fun =
        create_init_function_seir<TSeq>(proportions_)
        ;

    return *this;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/seirmixing.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//models/sirmixing.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODELS_SIRMIXING_HPP
#define EPIWORLD_MODELS_SIRMIXING_HPP

/**
 * @file seirentitiesconnected.hpp
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model with mixing
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSIRMixing : public epiworld::Model<TSeq> 
{
private:
    std::vector< std::vector< epiworld::Agent<TSeq> * > > infected;
    void update_infected_list();
    std::vector< epiworld::Agent<TSeq> * > sampled_agents;
    size_t sample_agents(
        epiworld::Agent<TSeq> * agent,
        std::vector< epiworld::Agent<TSeq> * > & sampled_agents
        );
    double adjusted_contact_rate;
    std::vector< double > contact_matrix;

    size_t index(size_t i, size_t j, size_t n) {
        return j * n + i;
    }

public:

    static const int SUSCEPTIBLE = 0;
    static const int INFECTED    = 1;
    static const int RECOVERED   = 2;

    ModelSIRMixing() {};
    
    /**
     * @brief Constructs a ModelSIRMixing object.
     *
     * @param model A reference to an existing ModelSIRMixing object.
     * @param vname The name of the ModelSIRMixing object.
     * @param n The number of entities in the model.
     * @param prevalence The initial prevalence of the disease in the model.
     * @param contact_rate The contact rate between entities in the model.
     * @param transmission_rate The transmission rate of the disease in the model.
     * @param recovery_rate The recovery rate of the disease in the model.
     * @param contact_matrix The contact matrix between entities in the model.
     */
    ModelSIRMixing(
        ModelSIRMixing<TSeq> & model,
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate,
        std::vector< double > contact_matrix
    );
    
    /**
     * @brief Constructs a ModelSIRMixing object.
     *
     * @param vname The name of the ModelSIRMixing object.
     * @param n The number of entities in the model.
     * @param prevalence The initial prevalence of the disease in the model.
     * @param contact_rate The contact rate between entities in the model.
     * @param transmission_rate The transmission rate of the disease in the model.
     * @param recovery_rate The recovery rate of the disease in the model.
     * @param contact_matrix The contact matrix between entities in the model.
     */
    ModelSIRMixing(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate,
        std::vector< double > contact_matrix
    );

    ModelSIRMixing<TSeq> & run(
        epiworld_fast_uint ndays,
        int seed = -1
    );

    void reset();

    Model<TSeq> * clone_ptr();

    /**
     * @brief Set the initial states of the model
     * @param proportions_ Double vector with a single element:
     * - The proportion of non-infected individuals who have recovered.
    */
    ModelSIRMixing<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    );

    size_t get_n_infected(size_t group) const
    {
        return infected[group].size();
    }

    void set_contact_matrix(std::vector< double > cmat)
    {
        contact_matrix = cmat;
        return;
    };

};

template<typename TSeq>
inline void ModelSIRMixing<TSeq>::update_infected_list()
{

    auto & agents = Model<TSeq>::get_agents();
    auto & entities = Model<TSeq>::get_entities();

    infected.resize(entities.size());
    sampled_agents.resize(agents.size());

    // Checking contact matrix's rows add to one
    size_t nentities = entities.size();
    if (this->contact_matrix.size() !=  nentities*nentities)
        throw std::length_error(
            std::string("The contact matrix must be a square matrix of size ") +
            std::string("nentities x nentities. ") +
            std::to_string(this->contact_matrix.size()) +
            std::string(" != ") + std::to_string(nentities*nentities) +
            std::string(".")
            );

    for (size_t i = 0u; i < entities.size(); ++i)
    {
        double sum = 0.0;
        for (size_t j = 0u; j < entities.size(); ++j)
        {
            if (this->contact_matrix[index(i, j, nentities)] < 0.0)
                throw std::range_error(
                    std::string("The contact matrix must be non-negative. ") +
                    std::to_string(this->contact_matrix[index(i, j, nentities)]) +
                    std::string(" < 0.")
                    );
            sum += this->contact_matrix[index(i, j, nentities)];
        }
        if (sum < 0.999 || sum > 1.001)
            throw std::range_error(
                std::string("The contact matrix must have rows that add to one. ") +
                std::to_string(sum) +
                std::string(" != 1.")
                );
    }

    for (size_t i = 0; i < entities.size(); ++i)
    {
        infected[i].clear();
        infected[i].reserve(agents.size());
    }
    
    for (auto & a : agents)
    {

        if (a.get_state() == ModelSIRMixing<TSeq>::INFECTED)
        {
            if (a.get_n_entities() > 0u)
                infected[a.get_entity(0u).get_id()].push_back(&a);
        }

    }

    // Adjusting contact rate
    adjusted_contact_rate = Model<TSeq>::get_param("Contact rate") /
        agents.size();

    return;

}

template<typename TSeq>
inline size_t ModelSIRMixing<TSeq>::sample_agents(
    epiworld::Agent<TSeq> * agent,
    std::vector< epiworld::Agent<TSeq> * > & sampled_agents
    )
{

    size_t agent_group_id = agent->get_entity(0u).get_id();
    size_t ngroups = infected.size();

    int samp_id = 0;
    for (size_t g = 0; g < infected.size(); ++g)
    {

        // How many from this entity?
        int nsamples = epiworld::Model<TSeq>::rbinom(
            infected[g].size(),
            adjusted_contact_rate * contact_matrix[
                index(agent_group_id, g, ngroups)
            ]
        );

        if (nsamples == 0)
            continue;

        // Sampling from the entity
        for (int s = 0; s < nsamples; ++s)
        {

            // Randomly selecting an agent
            int which = epiworld::Model<TSeq>::runif() * infected[g].size();

            // Correcting overflow error
            if (which >= static_cast<int>(infected[g].size()))
                which = static_cast<int>(infected[g].size()) - 1;

            auto & a = infected[g][which];

            // Can't sample itself
            if (a->get_id() == agent->get_id())
                continue;

            sampled_agents[samp_id++] = a;
            
        }

    }
    
    return samp_id;

}

template<typename TSeq>
inline ModelSIRMixing<TSeq> & ModelSIRMixing<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{
    
    Model<TSeq>::run(ndays, seed);
    return *this;

}

template<typename TSeq>
inline void ModelSIRMixing<TSeq>::reset()
{

    Model<TSeq>::reset();
    this->update_infected_list();

    return;

}

template<typename TSeq>
inline Model<TSeq> * ModelSIRMixing<TSeq>::clone_ptr()
{
    
    ModelSIRMixing<TSeq> * ptr = new ModelSIRMixing<TSeq>(
        *dynamic_cast<const ModelSIRMixing<TSeq>*>(this)
        );

    return dynamic_cast< Model<TSeq> *>(ptr);

}


/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence Initial prevalence (proportion)
 * @param contact_rate Average number of contacts (interactions) per step.
 * @param transmission_rate Probability of transmission
 * @param recovery_rate Probability of recovery
 */
template<typename TSeq>
inline ModelSIRMixing<TSeq>::ModelSIRMixing(
    ModelSIRMixing<TSeq> & model,
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate,
    std::vector< double > contact_matrix
    )
{

    // Setting up the contact matrix
    this->contact_matrix = contact_matrix;

    epiworld::UpdateFun<TSeq> update_susceptible = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            if (p->get_n_entities() == 0)
                return;

            // Downcasting to retrieve the sampler attached to the
            // class
            ModelSIRMixing<TSeq> * m_down =
                dynamic_cast<ModelSIRMixing<TSeq> *>(m);

            size_t ndraws = m_down->sample_agents(p, m_down->sampled_agents);

            if (ndraws == 0u)
                return;

            
            // Drawing from the set
            int nviruses_tmp = 0;
            for (size_t n = 0u; n < ndraws; ++n)
            {

                auto & neighbor = m_down->sampled_agents[n];

                auto & v = neighbor->get_virus();

                #ifdef EPI_DEBUG
                if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
                    throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                #endif
                    
                /* And it is a function of susceptibility_reduction as well */ 
                m->array_double_tmp[nviruses_tmp] =
                    (1.0 - p->get_susceptibility_reduction(v, m)) * 
                    v->get_prob_infecting(m) * 
                    (1.0 - neighbor->get_transmission_reduction(v, m)) 
                    ; 
            
                m->array_virus_tmp[nviruses_tmp++] = &(*v);

            }

            // Running the roulette
            int which = roulette(nviruses_tmp, m);

            if (which < 0)
                return;

            p->set_virus(
                *m->array_virus_tmp[which],
                m,
                ModelSIRMixing<TSeq>::INFECTED
                );

            return; 

        };

    epiworld::UpdateFun<TSeq> update_infected = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void {

            auto state = p->get_state();

            if (state == ModelSIRMixing<TSeq>::INFECTED)
            {


                // Odd: Die, Even: Recover
                epiworld_fast_uint n_events = 0u;
                const auto & v = p->get_virus();

                // Recover
                m->array_double_tmp[n_events++] = 
                    1.0 - (1.0 - v->get_prob_recovery(m)) * (1.0 - p->get_recovery_enhancer(v, m)); 

                #ifdef EPI_DEBUG
                if (n_events == 0u)
                {
                    printf_epiworld(
                        "[epi-debug] agent %i has 0 possible events!!\n",
                        static_cast<int>(p->get_id())
                        );
                    throw std::logic_error("Zero events in infected.");
                }
                #else
                if (n_events == 0u)
                    return;
                #endif
                

                // Running the roulette
                int which = roulette(n_events, m);

                if (which < 0)
                    return;

                // Which roulette happen?
                p->rm_virus(m);

                return ;

            } else
                throw std::logic_error("This function can only be applied to infected individuals. (SIR)") ;

            return;

        };

    // Setting up parameters
    model.add_param(contact_rate, "Contact rate");
    model.add_param(transmission_rate, "Prob. Transmission");
    model.add_param(recovery_rate, "Prob. Recovery");
    
    // state
    model.add_state("Susceptible", update_susceptible);
    model.add_state("Infected", update_infected);
    model.add_state("Recovered");

    // Global function
    epiworld::GlobalFun<TSeq> update = [](epiworld::Model<TSeq> * m) -> void
    {

        ModelSIRMixing<TSeq> * m_down =
            dynamic_cast<ModelSIRMixing<TSeq> *>(m);

        m_down->update_infected_list();

        return;

    };

    model.add_globalevent(update, "Update infected individuals");


    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname, prevalence, true);
    virus.set_state(
        ModelSIRMixing<TSeq>::INFECTED,
        ModelSIRMixing<TSeq>::RECOVERED,
        ModelSIRMixing<TSeq>::RECOVERED
        );

    virus.set_prob_infecting(&model("Prob. Transmission"));
    virus.set_prob_recovery(&model("Prob. Recovery"));

    model.add_virus(virus);

    model.queuing_off(); // No queuing need

    // Adding the empty population
    model.agents_empty_graph(n);

    model.set_name("Susceptible-Infected-Removed (SIR) with Mixing");

    return;

}

template<typename TSeq>
inline ModelSIRMixing<TSeq>::ModelSIRMixing(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate,
    std::vector< double > contact_matrix
    )
{   

    this->contact_matrix = contact_matrix;

    ModelSIRMixing(
        *this,
        vname,
        n,
        prevalence,
        contact_rate,
        transmission_rate,
        recovery_rate,
        contact_matrix
    );

    return;

}

template<typename TSeq>
inline ModelSIRMixing<TSeq> & ModelSIRMixing<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
)
{

    Model<TSeq>::initial_states_fun =
        create_init_function_sir<TSeq>(proportions_)
        ;

    return *this;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//models/sirmixing.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/




}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/models/models.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



}

#endif 
