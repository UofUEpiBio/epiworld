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
    #define epiworld_fast_uint std::uint_fast16_t
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
using VirusPtr = std::shared_ptr< Virus< TSeq > >;

template<typename TSeq>
using ToolPtr = std::shared_ptr< Tool< TSeq > >;

template<typename TSeq>
using ToolFun = std::function<epiworld_double(Tool<TSeq>&,Person<TSeq>*,VirusPtr<TSeq>,Model<TSeq>*)>;

template<typename TSeq>
using MixerFun = std::function<epiworld_double(Person<TSeq>*,VirusPtr<TSeq>,Model<TSeq>*)>;

template<typename TSeq>
using MutFun = std::function<bool(Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using PostRecoveryFun = std::function<void(Person<TSeq>*,VirusPtr<TSeq>,Model<TSeq>*)>;

template<typename TSeq>
using VirusFun = std::function<epiworld_double(Person<TSeq>*,Virus<TSeq>&,Model<TSeq>*)>;

template<typename TSeq>
using UpdateFun = std::function<void(Person<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using GlobalFun = std::function<void(Model<TSeq>*)>;

template<typename TSeq>
struct Action;

template<typename TSeq>
using ActionFun = std::function<void(Action<TSeq>&,Model<TSeq>*)>;

/**
 * @brief Action data for update an agent
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
struct Action {
    Person<TSeq> * person;
    VirusPtr<TSeq> virus;
    ToolPtr<TSeq> tool;
    epiworld_fast_uint virus_idx;
    epiworld_fast_uint tool_idx;
    epiworld_fast_uint new_status;
    epiworld_fast_int queue;
    ActionFun<TSeq> call;
public:
/**
     * @brief Construct a new Action object
     * 
     * All the parameters are rather optional.
     * 
     * @param person_ Person over who the action will happen
     * @param virus_ Virus to add
     * @param tool_ Tool to add
     * @param virus_idx Index of virus to be removed (if needed)
     * @param tool_idx Index of tool to be removed (if needed)
     * @param new_status_ Next status
     * @param queue_ Efect on the queue
     * @param call_ The action call (if needed)
     */
    Action(
        Person<TSeq> * person_,
        VirusPtr<TSeq> virus_,
        ToolPtr<TSeq> tool_,
        epiworld_fast_uint virus_idx_,
        epiworld_fast_uint tool_idx_,
        epiworld_fast_uint new_status_,
        epiworld_fast_int queue_,
        ActionFun<TSeq> call_
    ) : person(person_), virus(virus_), tool(tool_), virus_idx(virus_idx_),
        tool_idx(tool_idx_), new_status(new_status_), queue(queue_), call(call_) {};
};

/**
 * @brief List of possible states in the model
 *  
 */
enum STATUS {
    SUSCEPTIBLE,
    EXPOSED,
    REMOVED
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
    #define EPI_DEFAULT_VIRUS_PROB_RECOVERY     0.5
#endif

#ifndef EPI_DEFAULT_VIRUS_PROB_DEATH
    #define EPI_DEFAULT_VIRUS_PROB_DEATH        0.0
#endif
///@}

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