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

#endif
