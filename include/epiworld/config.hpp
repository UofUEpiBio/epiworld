#ifndef EPIWORLD_CONFIG_HPP
#define EPIWORLD_CONFIG_HPP

#ifndef printf_epiworld
    #define printf_epiworld fflush(stdout);printf
#endif

#define EPIWORLD_TIMING 1
#ifdef EPIWORLD_TIMING
    #include <chrono>
    #define EPIWORLD_CLOCK_SETUP() \
        std::map<std::string,std::pair<double,unsigned int>> epiworld_clock_timing;
    #define EPIWORLD_CLOCK_START(a) \
        auto epiworld_clock_start = std::chrono::steady_clock::now(); \
        if (epiworld::epiworld_clock_timing.find(a) == epiworld::epiworld_clock_timing.end()) \
            epiworld::epiworld_clock_timing[a] = std::pair<double,int>(0,1); \
        else \
            epiworld::epiworld_clock_timing[a].second++;
    #define EPIWORLD_CLOCK_END(a) \
        auto epiworld_clock_end   = std::chrono::steady_clock::now(); \
        std::chrono::duration<double,std::milli> epiworld_elapsed = \
            epiworld_clock_end-epiworld_clock_start; \
        epiworld::epiworld_clock_timing[a].first += epiworld_elapsed.count();
    #define EPIWORLD_CLOCK_REPORT(a) \
        /* Start by computing the sizes and relative scale of things */ \
        double mint = 1000.0; \
        unsigned int maxsize = 0;\
        for (auto & t : epiworld::epiworld_clock_timing) { \
            if (mint > t.second.first) \
                mint = t.second.first; \
            if (maxsize < t.first.size()) \
                maxsize = t.first.size() + 1;\
        } \
        std::string t_unit = mint > 1000 ? "s" : "ms"; \
        std::string epiworld_msg = "%-" + std::to_string(maxsize) + "s: % 8.2f%s (avg %.2f for %i counts)\n";\
        printf_epiworld("%s\n", a); \
        for (auto & times : epiworld::epiworld_clock_timing) \
        {\
            printf_epiworld(\
                epiworld_msg.c_str(),\
                times.first.c_str(),\
                times.second.first, \
                t_unit.c_str(), \
                times.second.first / times.second.second * (t_unit == "s" ? 1.0/1000.0 : 1.0),\
                times.second.second);\
        }
#else
    #define EPIWORLD_CLOCK_SETUP()
    #define EPIWORLD_CLOCK_START(a)
    #define EPIWORLD_CLOCK_END(a)
    #define EPIWORLD_CLOCK_REPORT()
#endif

EPIWORLD_CLOCK_SETUP()

#define DECEASED     -99
#define INFECTED     -1
#define HEALTHY      0
#define RECOVERED    1
#define INCUBATION   2
#define ASYMPTOMATIC 3

#endif
