#ifndef EPIWORLD_TESTS_HPP
#define EPIWORLD_TESTS_HPP

#include <vector>
#include <iostream>
#include <type_traits>
#include <exception>
#if defined(_OPENMP)
    #include <omp.h>
#endif
#include "../include/epiworld/epiworld.hpp"

/**
 * Returns true if the absolute difference between a and b is greater than eps.
 */
template<typename T = epiworld_double>
inline bool moreless(T a, T b, T eps = static_cast<T>(1)) {
    return (std::abs(a - b) > eps);
}

std::string file_reader(std::string fname) {
    // Create a text string, which is used to output the text file
    std::string myText, res;

    // Read from the text file
    std::ifstream MyReadFile(fname);

    // Check if fname exists and is readable. If not, throw an exception
    if (!MyReadFile.good()) {
        throw std::runtime_error(
            "File " + fname + " does not exist or is not readable"
        );
    }

    // Use a while loop together with the getline() function to read the file line by line
    while (getline(MyReadFile, myText)) {
        // Output the text from the file
        res += myText;
    }

    // Close the file
    MyReadFile.close();

    return res;
}

template<typename TSeq = EPI_DEFAULT_TSEQ>
inline epiworld::EntityToAgentFun<TSeq> dist_factory(int from, int to) {
    return [from,
            to](epiworld::Entity<TSeq>& e, epiworld::Model<TSeq>* m) -> void {
        auto& agents = m->get_agents();
        for (int i = from; i < to; ++i) {
            e.add_agent(&agents[i], m);
        }

        return;
    };
}

template<typename TSeq = EPI_DEFAULT_TSEQ>
inline epiworld::VirusToAgentFun<TSeq> dist_virus(int i) {
    return [i](epiworld::Virus<TSeq>& v, epiworld::Model<TSeq>* m) -> void {
        m->get_agents()[i].set_virus(v, m);
        return;
    };
}

#ifndef CATCH_CONFIG_MAIN

    #define EPIWORLD_TEST_CASE(desc, tag) int main()

#else

    #define EPIWORLD_TEST_CASE(desc, tag) TEST_CASE(desc, tag)

#endif

#endif