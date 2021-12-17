#include <iostream>
#include <memory>
#include <vector>
#include <random>

#include "../include/epiworld/epiworld.hpp"

// Original data will be an integer vector
#define DAT std::vector<bool>
static DAT base_seq = {true, false, false, true, true, false, true, false, true, false, false};
#define MUTATION_PROB      0.00025
#define INITIAL_PREVALENCE 0.005
#define N_DAYS             60
#define VACCINE_EFFICACY   0.90
#define IMMUNE_EFFICACY    0.50
#define VARIANT_MORTALITY  0.001
#define BASELINE_INFECCTIOUSNESS 0.9
#define IMMUNE_LEARN_RATE 0.05

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

#define MAKE_TOOL(a,b) inline double \
    (a)(\
        epiworld::Tool<b> * t, \
        epiworld::Person< b > * p, epiworld::Virus< b > * v, epiworld::Model< b > * m)


// Getting the vaccine
MAKE_TOOL(vaccine_eff, DAT) {

    double dist = 0.0;
    const auto & virusseq = v->get_sequence();
    for (unsigned int i = 0; i < virusseq->size(); ++i)
        dist += std::fabs(virusseq->at(i) - base_seq[i]);
    
    return (1.0 - dist/virusseq->size()) * VACCINE_EFFICACY;
    // return VACCINE_EFFICACY;

}

MAKE_TOOL(vaccine_rec, DAT) {
    return 0.4;
}

MAKE_TOOL(vaccine_death, DAT) {
    return VARIANT_MORTALITY;
}

MAKE_TOOL(vaccine_trans, DAT) {
    return 0.5;
}

// Wearing a Mask
MAKE_TOOL(mask_eff, DAT) {
    return 0.8;
}

MAKE_TOOL(mask_trans, DAT) {
    return 0.05;
}

// Immune system
MAKE_TOOL(immune_eff, DAT) {
    return 0.3;
}

MAKE_TOOL(immune_rec, DAT) {

    double dist = 0.0;
    const auto & virusseq = v->get_sequence();
    auto & immune_sequence = t->get_sequence_unique();

    // Deciding whether to mutate or not
    if (m->runif() < IMMUNE_LEARN_RATE)
    {
        int k = floor(m->runif() * immune_sequence.size());
        immune_sequence[k] = virusseq->at(k); 
    }
    
    for (unsigned int i = 0; i < virusseq->size(); ++i)
    {
        // With 30% chance we will match that part of the code
        dist += std::fabs(virusseq->at(i) - immune_sequence[i]);

    }
    
    return (1.0 - dist/virusseq->size()) * IMMUNE_EFFICACY;
    // return 0.5;


}

MAKE_TOOL(immune_death, DAT) {

    return VARIANT_MORTALITY;

}

MAKE_TOOL(immune_trans, DAT) {

    return BASELINE_INFECCTIOUSNESS;

}

int main(int argc, char* argv[]) {


    if ((argc != 3) & (argc != 1))
        std::logic_error("You need to specify seed and number of steps (in that order).");

    int seed;
    unsigned int nsteps;
    if (argc == 3)
    {
        seed   = strtol(argv[1], nullptr, 0);
        nsteps = strtol(argv[2], nullptr, 0);

    } else {
        seed   = 159;
        nsteps = N_DAYS;
    }


    using namespace epiworld;

    // Initializing disease
    epiworld::Virus<DAT> covid19(base_seq);
    covid19.set_mutation(covid19_mut);

    // Initializing the world. This will include POP_SIZE
    // individuals
    epiworld::Model<DAT> model;
    model.add_virus(covid19, INITIAL_PREVALENCE); // 0.5% will have the virus at first

    // Reading network structure
    model.pop_from_adjlist("edgelist.txt", 0, true);

    // Creating tools
    epiworld::Tool<DAT> vaccine;
    vaccine.set_efficacy(vaccine_eff);
    vaccine.set_recovery(vaccine_rec);
    vaccine.set_death(vaccine_death);
    vaccine.set_transmisibility(vaccine_trans);

    epiworld::Tool<DAT> mask;
    mask.set_efficacy(mask_eff);
    mask.set_transmisibility(mask_trans);

    epiworld::Tool<DAT> immune;
    immune.set_efficacy(immune_eff);
    immune.set_recovery(immune_rec);
    immune.set_death(immune_death);
    immune.set_transmisibility(immune_trans);
    DAT seq0(base_seq.size(), false);
    immune.set_sequence_unique(seq0);


    // Randomly adding tools to the population
    model.add_tool(vaccine, 0.5);
    model.add_tool(mask, 0.5);
    model.add_tool(immune, 1.0);
    
    model.init(seed);

    // Creating a progress bar
    EPIWORLD_CLOCK_START()
    epiworld::Progress p(nsteps, 80);

    // Initializing the simulation
    for (unsigned int t = 0; t < nsteps; ++t)
    {

        model.update_status();
        model.mutate_variant();
        model.next();

        p.next();

    }

    p.end();
    
    EPIWORLD_CLOCK_END("Run model")

    // Writing off the results
    model.get_db().write_data(
        "variants.txt",
        "total.txt"
    );

    model.write_edgelist("simple-world-edgelist.txt");

    return 0;

}