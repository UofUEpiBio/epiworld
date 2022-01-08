#ifndef EPIWORLD_CONFIG_HPP
#define EPIWORLD_CONFIG_HPP

#ifndef printf_epiworld
    #define printf_epiworld fflush(stdout);printf
#endif

#ifdef EPIWORLD_USE_OMP
    #include <omp.h>
#else

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
using ToolFun = std::function<double(Tool<TSeq>*,Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using MixerFun = std::function<double(PersonTools<TSeq>*,Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using MutFun = std::function<bool(Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using VirusFun = std::function<void(Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using UpdateFun = std::function<unsigned int(Person<TSeq>*,Model<TSeq>*)>;

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

#ifndef DEFAULT_TOOL_EFFICACY
    #define DEFAULT_TOOL_EFFICACY        0.0
#endif

#ifndef DEFAULT_TOOL_TRANSMISIBILITY
    #define DEFAULT_TOOL_TRANSMISIBILITY 1.0
#endif

#ifndef DEFAULT_TOOL_RECOVERY
    #define DEFAULT_TOOL_RECOVERY        1.0
#endif

#ifndef DEFAULT_TOOL_DEATH
    #define DEFAULT_TOOL_DEATH           1.0
#endif

#ifndef DEFAULT_VIRUS_INFECTIOUSNESS
    #define DEFAULT_VIRUS_INFECTIOUSNESS 1.0
#endif

#ifndef DEFAULT_VIRUS_PERSISTANCE
    #define DEFAULT_VIRUS_PERSISTANCE    0.5
#endif

#ifndef DEFAULT_VIRUS_DEATH
    #define DEFAULT_VIRUS_DEATH          0.0
#endif

#endif
