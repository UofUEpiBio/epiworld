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
using VirusFun = std::function<void(Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using UpdateFun = std::function<unsigned int(Person<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using GlobalFun = std::function<void(Model<TSeq>*)>;

/**
 * @brief List of possible states in the model
 * 
 * @details Currently, only `DECEASED`, `INFECTED`, `RECOVERED`, and `HEALTHY` are used.
 * 
 */
enum STATUS {
    HEALTHY,
    INFECTED,
    RECOVERED,
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

#ifndef DEFAULT_VIRUS_INFECTIOUSNESS
    #define DEFAULT_VIRUS_INFECTIOUSNESS        1.0
#endif

#ifndef DEFAULT_VIRUS_PERSISTANCE
    #define DEFAULT_VIRUS_PERSISTANCE           0.5
#endif

#ifndef DEFAULT_VIRUS_DEATH
    #define DEFAULT_VIRUS_DEATH                 0.0
#endif

#endif
