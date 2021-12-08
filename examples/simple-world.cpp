#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include "../include/epiworld/epiworld.hpp"

// Original data will be an integer vector
#define DAT std::vector<int>
DAT base_seq = {0, 1, 2, 1, 3};
#define POP_SIZE 20

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

#define MAKE_TOOL(a,b) inline double \
    (a)(epiworld::Person< b > * p, epiworld::Virus< b > * v, epiworld::Model< b > * m)

MAKE_TOOL(vaccine_eff, DAT) {
    return 0.8;
}

MAKE_TOOL(vaccine_rec, DAT) {
    return 0.3;
}

MAKE_TOOL(vaccine_dath, DAT) {
    return 0.1;
}

int main() {

    using namespace epiworld;

    // Initializing the world. This will include POP_SIZE
    // individuals
    epiworld::Model<DAT> model(POP_SIZE);

    // Initializing disease
    epiworld::Virus<DAT> covid19(base_seq);

    covid19.set_mutation(covid19_mut);
    covid19.set_transmisibility(covid19_trans);

    // Creating tool
    epiworld::Tool<DAT> vaccine;
    vaccine.set_efficacy(vaccine_eff);
    vaccine.set_recovery(vaccine_rec);
    vaccine.set_death(vaccine_dath);

    // First half of the individuals is vaccinated
    for (int i = 0; i < POP_SIZE; ++i)
        model(i).add_tool(0, vaccine);
    
    // The other half has the virus
    for (int i = 0; i < POP_SIZE; ++i)
        model(i).add_virus(0, covid19);

    // Now, we show the rates for each individual ------------------------------
    for (int i = 0; i < POP_SIZE; ++i)
        printf(
            "(%2i) E: %.2f, R: %.2f, D: %.2f\n",
            i,
            model(i).get_efficacy(),
            model(i).get_recovery(),
            model(i).get_death()
            );

    return 0;

}