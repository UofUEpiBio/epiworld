#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include "../include/epiworld/epiworld.hpp"

// Original data will be an integer vector
#define DAT std::vector<bool>
static DAT base_seq = {true, false, false, true, true};
#define POP_SIZE 100
#define MUTATION_PROB 0.1

// Defining mutation and transmission functions
inline bool covid19_mut(
    epiworld::Virus<DAT> * v
) {

    epiworld::Model<DAT> * mptr = v->get_model();

    if (mptr->runif() < MUTATION_PROB)
    {
        // Picking a location at random
        int idx = std::floor(mptr->runif() * v->get_sequence()->size());
        DAT tmp_seq = *v->get_sequence();
        tmp_seq[idx] = !v->get_sequence()->at(idx); 

        // Updating its sequence
        v->set_sequence(tmp_seq);

        return true;
    }

    return false;
    
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

MAKE_TOOL(mask_eff, DAT) {
    return 0.5;
}

MAKE_TOOL(raw_eff, DAT) {
    return 0.1;
}


int main(int argc, char* argv[]) {

    int nsteps;
    if (argc == 1)
        nsteps = 1000;
    else
        nsteps = strtol(argv[1], nullptr, 0);


    using namespace epiworld;

    // Initializing disease
    epiworld::Virus<DAT> covid19(base_seq);

    covid19.set_mutation(covid19_mut);
    covid19.set_transmisibility(covid19_trans);

    // Initializing the world. This will include POP_SIZE
    // individuals
    epiworld::Model<DAT> model(POP_SIZE);
    model.add_virus(covid19, 0.95); // 50% will have the virus at first
    model.seed(1231);

    // Reading network structure
    model.pop_from_adjlist("edgelist.txt");

    model.init();

    // Creating tool
    epiworld::Tool<DAT> vaccine;
    vaccine.set_efficacy(vaccine_eff);
    vaccine.set_recovery(vaccine_rec);
    vaccine.set_death(vaccine_dath);

    epiworld::Tool<DAT> mask;
    mask.set_efficacy(mask_eff);

    epiworld::Tool<DAT> immune;
    immune.set_efficacy(raw_eff);

    // First half of the individuals is vaccinated
    for (int i = 0; i < POP_SIZE/2; ++i)
        model(i).add_tool(0, vaccine);
    
    // One half wears the mask
    for (int i = POP_SIZE/2; i < POP_SIZE; ++i)
        model(i).add_tool(0, mask);

    // Immune system
    for (int i = 0; i < POP_SIZE; ++i)
        model(i).add_tool(0, immune);

    
    // Initializing the simulation
    for (int t = 0; t < nsteps; ++t)
    {
        model.update_status();
        model.next();

        if ((t % 1000 == 0))
            printf_epiworld("Step %d\n", t);
    }
    

    int nvar = model.get_nvariants();
    printf_epiworld("Total variants: %i\n", nvar);
    for (int i = 0; i < nvar; ++i)
    {
        const DAT & variant = model.get_variant_sequence()[i];
        for (int j = 0; j < variant.size(); ++j)
            printf_epiworld("%i", variant[j] ? 1 : 0);

        printf_epiworld(": %i\n", model.get_variant_nifected()[i]);
    }


    return 0;

}