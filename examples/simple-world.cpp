#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include "../include/epiworld/epiworld.hpp"

// Original data will be an integer vector
#define DAT std::vector<bool>
static DAT base_seq = {true, false, false, true, true};
#define POP_SIZE 20
#define MUTATION_PROB 0.5

// Defining mutation and transmission functions
inline void covid19_mut(
    epiworld::Virus<DAT> * v
) {

    epiworld::Model<DAT> * mptr = v->get_model();

    if (mptr->runif() < MUTATION_PROB)
    {
        // Picking a location at random
        int idx = std::floor(mptr->runif() * v->get_sequence()->size());
        v->get_sequence()->at(idx) = !v->get_sequence()->at(idx); 
    }
    
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

    double dist = 0.0;
    const auto & virusseq = v->get_sequence();
    for (int i = 0; i < virusseq->size(); ++i)
        dist += std::fabs(virusseq->at(i) - base_seq[i]);
    
    return (1.0 - dist/virusseq->size()) * 0.8;
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
    model.init();

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
    printf_epiworld("The Efficacy (E), Recovery (R), and Death (D) rates:\n");
    for (int i = 0; i < POP_SIZE; ++i)
        printf_epiworld(
            "  (%2i) E: %.2f, R: %.2f, D: %.2f\n",
            i,
            model(i).get_efficacy(),
            model(i).get_recovery(),
            model(i).get_death()
            );

    // What happens if we make things mutate now and then ----------------------
    printf_epiworld(
        "The Efficacy (E), Recovery (R), and Death (D) rates (with mutation):\n"
        );
    for (int i = 0; i < POP_SIZE; ++i)
    {

        // New sequence
        printf_epiworld("(%i2) oldseq: ", i);
        for (const auto & s : *model(i).get_virus(0u).get_sequence())
            printf_epiworld("%i ", s ? 1 : 0);

        // Applying random mutations
        model(i).mutate_virus();

        // New sequence
        printf_epiworld("\n(%i2) oldseq: ", i);
        for (const auto & s : *model(i).get_virus(0u).get_sequence())
            printf_epiworld("%i ", s ? 1 : 0);

        // Printing new results
        printf_epiworld(
            "\n  (%2i) E: %.2f, R: %.2f, D: %.2f\n",
            i,
            model(i).get_efficacy(),
            model(i).get_recovery(),
            model(i).get_death()
            );
    }


    return 0;

}