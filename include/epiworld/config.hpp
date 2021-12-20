#ifndef EPIWORLD_CONFIG_HPP
#define EPIWORLD_CONFIG_HPP

#ifndef printf_epiworld
    #define printf_epiworld fflush(stdout);printf
#endif

#ifdef EPIWORLD_TIMING
    #include <chrono>
    #define EPIWORLD_CLOCK_START() \
        auto epiworld_clock_start = std::chrono::steady_clock::now();
    #define EPIWORLD_CLOCK_END(a) \
        auto epiworld_clock_end   = std::chrono::steady_clock::now(); \
        std::chrono::duration<double,std::milli> epiworld_elapsed = \
            epiworld_clock_end-epiworld_clock_start;\
        std::string epoworld_clock_str = a; \
        printf_epiworld(\
            "(elapsed) %-20s: % 8.2fms\n",\
            epoworld_clock_str.c_str(),\
            epiworld_elapsed.count());
#else
    #define EPIWORLD_CLOCK_START()
    #define EPIWORLD_CLOCK_END(a)
#endif

#define DECEASED     -99
#define INFECTED     -1
#define HEALTHY      0
#define RECOVERED    1
#define INCUBATION   2
#define ASYMPTOMATIC 3

#endif
