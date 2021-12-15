#include <iostream>
#include <memory>
#include <vector>
#include <random>

#include "../include/epiworld/epiworld.hpp"

// Original data will be an integer vector
#define DAT std::vector<bool>
static DAT base_seq = {true, false, false, true, true, false, true, false, true, false, false};
#define MUTATION_PROB      0.005
#define INITIAL_PREVALENCE 0.005

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
    for (int i = 0; i < virusseq->size(); ++i)
        dist += std::fabs(virusseq->at(i) - base_seq[i]);
    
    return (1.0 - dist/virusseq->size()) * 0.8;
}

MAKE_TOOL(vaccine_rec, DAT) {
    return 0.4;
}

MAKE_TOOL(vaccine_death, DAT) {
    return 0.005;
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
    for (int i = 0; i < virusseq->size(); ++i)
    {
        // With 30% chance we will match that part of the code
        if (m->runif() < .5)
            immune_sequence[i] = virusseq->at(i);
        dist += std::fabs(virusseq->at(i) - immune_sequence[i]);
        
    }
    
    return (1.0 - dist/virusseq->size()) * 0.8;


}

MAKE_TOOL(immune_death, DAT) {

    return 0.01 * (1.0 - 1.0/(m->today() - v->get_date()));

}

MAKE_TOOL(immune_trans, DAT) {

    if ((m->today() - v->get_date()) > 7)
        return 1.0;
    else
        return 0.5;

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
        seed   = 15;
        nsteps = 365 * 5;
    }


    using namespace epiworld;

    // Initializing disease
    epiworld::Virus<DAT> covid19(base_seq);
    covid19.set_mutation(covid19_mut);

    // Initializing the world. This will include POP_SIZE
    // individuals
    epiworld::Model<DAT> model;
    model.add_virus(covid19, INITIAL_PREVALENCE); // 5% will have the virus at first

    // Reading network structure
    model.pop_from_adjlist("edgelist.txt", 0, true);

    model.init(seed);

    // Creating tool
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

    
    // // Response of immune system post recovery
    // epiworld::Tool<DAT> immune_build;
    // immune_build.set_efficacy(add_immunity);

    // First half of the individuals is vaccinated
    for (int i = 0; i < model.size()/4; ++i)
        model(i).add_tool(0, vaccine);
    
    // One half wears the mask
    for (int i = 0; i < model.size(); ++i)
        model(i).add_tool(0, mask);

    // Immune system
    for (int i = 0; i < model.size(); ++i)
        model(i).add_tool(0, immune);

    // Initializing the simulation
    for (int t = 0; t < nsteps; ++t)
    {
        model.update_status();
        model.mutate_variant();
        model.next();

    }
    
    // Writing off the results
    model.get_db().write_data(
        "variants.txt",
        "total.txt"
    );

    model.write_edgelist("simple-world-edgelist.txt");

    return 0;

}