#ifndef EPIWORLD_CONFIG_HPP
#define EPIWORLD_CONFIG_HPP

#ifndef printf_epiworld
    #define printf_epiworld fflush(stdout);printf
#endif

#ifdef EPIWORLD_USE_OMP
    #include <omp.h>
#else

#endif


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
