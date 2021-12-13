#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include "../include/epiworld/epiworld.hpp"

// Original data will be an integer vector
#define DAT std::vector<bool>
static DAT base_seq = {true, false, false, true, true};
#define POP_SIZE 5000
#define MUTATION_PROB 0.025

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
    return 0.8;
}

#define MAKE_TOOL(a,b) inline double \
    (a)(\
        epiworld::Tool<b> * t, \
        epiworld::Person< b > * p, epiworld::Virus< b > * v, epiworld::Model< b > * m)

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
    return 0.01;
}

MAKE_TOOL(mask_eff, DAT) {
    return 0.8;
}

MAKE_TOOL(raw_eff, DAT) {
    return 0.2;
}

MAKE_TOOL(raw_rec, DAT) {

    double dist = 0.0;
    const auto & virusseq = v->get_sequence();
    for (int i = 0; i < virusseq->size(); ++i)
    {
        // With 30% chance we will match that part of the code
        if (m->runif() < .3)
            t->get_sequence_unique()[i] = virusseq->at(i);
        dist += std::fabs(virusseq->at(i) - t->get_sequence_unique()[i]);
    }
    
    return (1.0 - dist/virusseq->size()) * 0.8;

}

MAKE_TOOL(raw_death, DAT) {
    return 0.005;
}


int main(int argc, char* argv[]) {


    if ((argc != 3) & (argc != 1))
        std::logic_error("You need to specify seed and number of steps (in that order).");

    int seed, nsteps;
    if (argc == 3)
    {
        seed   = strtol(argv[1], nullptr, 0);
        nsteps = strtol(argv[2], nullptr, 0);

    } else {
        seed   = 1253;
        nsteps = 365;
    }


    using namespace epiworld;

    // Initializing disease
    epiworld::Virus<DAT> covid19(base_seq);

    covid19.set_mutation(covid19_mut);
    covid19.set_transmisibility(covid19_trans);

    // Initializing the world. This will include POP_SIZE
    // individuals
    epiworld::Model<DAT> model(POP_SIZE);
    model.add_virus(covid19, 0.025); // 5% will have the virus at first

    // Reading network structure
    model.pop_from_adjlist("edgelist.txt");

    model.init(seed);

    // Creating tool
    epiworld::Tool<DAT> vaccine;
    vaccine.set_efficacy(vaccine_eff);
    vaccine.set_recovery(vaccine_rec);
    vaccine.set_death(vaccine_dath);

    epiworld::Tool<DAT> mask;
    mask.set_efficacy(mask_eff);

    epiworld::Tool<DAT> immune;
    immune.set_efficacy(raw_eff);
    immune.set_recovery(raw_rec);
    immune.set_death(raw_death);
    DAT seq0(base_seq.size(), false);
    immune.set_sequence_unique(seq0);

    
    // // Response of immune system post recovery
    // epiworld::Tool<DAT> immune_build;
    // immune_build.set_efficacy(add_immunity);

    // First half of the individuals is vaccinated
    for (int i = 0; i < POP_SIZE/4; ++i)
        model(i).add_tool(0, vaccine);
    
    // One half wears the mask
    for (int i = POP_SIZE/4*3; i < POP_SIZE; ++i)
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

        printf_epiworld("\n  infected : %i", model.get_variant_nifected()[i]);
        printf_epiworld("\n  deceased : %i", model.get_db().get_today_variant("ndeceased")[i]);
        printf_epiworld("\n  recovered: %i\n", model.get_db().get_today_variant("nrecovered")[i]);
    }

    model.get_db().write_data(
        "variants.txt",
        "total.txt"
    );

    return 0;

}