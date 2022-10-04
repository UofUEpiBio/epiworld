#ifndef EPIWORLD_TESTS_HPP
#define EPIWORLD_TESTS_HPP

#include "../include/epiworld/epiworld.hpp"

#include <vector>
#include <iostream>
#include <type_traits>
#include <exception>

#ifndef CATCH_CONFIG_MAIN

    
    #define EPIWORLD_TEST_CASE(desc, tag) \
        int main() 

    
#else

    #define EPIWORLD_TEST_CASE(desc, tag) \
        TEST_CASE(desc, tag) 

#endif


#endif