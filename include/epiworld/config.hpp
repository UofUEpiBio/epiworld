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
using PostRecFun = std::function<void(Person<TSeq>*,Virus<TSeq>*,Model<TSeq>*)>;

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


#endif
