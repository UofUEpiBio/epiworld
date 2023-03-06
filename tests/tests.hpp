#ifndef EPIWORLD_TESTS_HPP
#define EPIWORLD_TESTS_HPP


#include <vector>
#include <iostream>
#include <type_traits>
#include <exception>
#include <omp.h>
#include "../include/epiworld/epiworld.hpp"

std::string file_reader(std::string fname)
{
    // Create a text string, which is used to output the text file
    std::string myText, res;

    // Read from the text file
    std::ifstream MyReadFile(fname);

    // Use a while loop together with the getline() function to read the file line by line
    while (getline (MyReadFile, myText)) {
        // Output the text from the file
        res += myText;
    }

    // Close the file
    MyReadFile.close();

    return res;
}

#ifndef CATCH_CONFIG_MAIN

    
    #define EPIWORLD_TEST_CASE(desc, tag) \
        int main() 

    
#else

    #define EPIWORLD_TEST_CASE(desc, tag) \
        TEST_CASE(desc, tag) 

#endif


#endif