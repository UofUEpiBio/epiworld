#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include "../include/epiworld/epiworld.hpp"

// Original data will be an integer vector
#define DAT std::vector<int>
DAT base_seq = {0, 1, 2, 1, 3};

// Defining mutation and transmission functions
inline void covid19_mut(
    epiworld::Virus<DAT> * v
) {

    // Adding a mutation to the first base
    v->get_sequence()->at(0u) += 1;
    
}

inline double covid19_trans(
    epiworld::Virus<DAT> * v,
    epiworld::Person<DAT> * p
) {
    return 0.5;
}

int main() {

    using namespace epiworld;

    // For some reason it is ambiguous
    epiworld::Model<DAT> model;

    // Initializing disease
    epiworld::Virus<DAT> covid19(base_seq);

    covid19.set_mutation(covid19_mut);
    covid19.set_transmisibility(covid19_trans);

    // Initializing vector of individuals
    std::vector< epiworld::Person<DAT>> persons;
    persons.push_back(epiworld::Person<DAT>());
    persons.push_back(epiworld::Person<DAT>());

    // Creating disease


    return 0;

}