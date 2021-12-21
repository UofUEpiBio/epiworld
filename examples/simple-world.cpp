#include <iostream>
#include <memory>
#include <vector>
#include <random>

#include "../include/epiworld/epiworld.hpp"

// Original data will be an integer vector
#define DAT std::vector<bool>
static DAT base_seq = {true, false, false, true, true, false, true, false, true, false, false};

enum epipar {
    MUTATION_PROB,
    VACCINE_EFFICACY,
    IMMUNE_EFFICACY,
    VARIANT_MORTALITY,
    BASELINE_INFECCTIOUSNESS,
    IMMUNE_LEARN_RATE
};

// Defining mutation and transmission functions
EPI_MUTFUN(covid19_mut, DAT) {

    if (EPI_RUNIF() < EPI_PARAMS(MUTATION_PROB))
    {
        // Picking a location at random
        int idx = std::floor(m->runif() * v->get_sequence()->size());
        DAT tmp_seq = *v->get_sequence();
        tmp_seq[idx] = !v->get_sequence()->at(idx); 

        // Updating its sequence
        v->set_sequence(tmp_seq);

        return true;
    }

    return false;
    
}



// Getting the vaccine
EPI_NEW_TOOL(vaccine_eff, DAT) {

    return EPI_PARAMS(VACCINE_EFFICACY);

}

EPI_NEW_TOOL(vaccine_rec, DAT) {
    return 0.4;
}

EPI_NEW_TOOL(vaccine_death, DAT) {
    return EPI_PARAMS(VARIANT_MORTALITY);
}

EPI_NEW_TOOL(vaccine_trans, DAT) {
    return 0.5;
}

// Wearing a Mask
EPI_NEW_TOOL(mask_eff, DAT) {
    return 0.8;
}

EPI_NEW_TOOL(mask_trans, DAT) {
    return 0.05;
}

// Immune system
EPI_NEW_TOOL(immune_eff, DAT) {

    return 0.3;

}

EPI_NEW_TOOL(immune_rec, DAT) {

    return EPI_PARAMS(IMMUNE_EFFICACY);

}

EPI_NEW_TOOL(immune_death, DAT) {

    return EPI_PARAMS(VARIANT_MORTALITY);

}

EPI_NEW_TOOL(immune_trans, DAT) {

    return EPI_PARAMS(BASELINE_INFECCTIOUSNESS);

}

int main(int argc, char* argv[]) {


    if ((argc != 3) & (argc != 1))
        std::logic_error("You need to specify seed and number of steps (in that order).");

    int seed;
    unsigned int nsteps;
    double preval;
    if (argc == 4)
    {
        seed   = strtol(argv[1], nullptr, 0);
        nsteps = strtol(argv[2], nullptr, 0);
        preval = strtol(argv[3], nullptr, 0);

    } else {
        seed   = 159;
        nsteps = 60;
        preval = 0.005;
    }


    using namespace epiworld;

    // Initializing the model
    epiworld::Model<DAT> model;
    
    // Setting up the model parameters
    model.params().resize(6u, 0.0);
    model.params()[MUTATION_PROB]            = 0.000025;
    model.params()[VACCINE_EFFICACY]         = 0.90;
    model.params()[IMMUNE_EFFICACY]          = 0.50;
    model.params()[VARIANT_MORTALITY]        = 0.001;
    model.params()[BASELINE_INFECCTIOUSNESS] = 0.5;
    model.params()[IMMUNE_LEARN_RATE]        = 0.05;

    // Initializing disease
    epiworld::Virus<DAT> covid19(base_seq);
    covid19.set_mutation(covid19_mut);

    model.add_virus(covid19, preval); // 0.5% will have the virus at first

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
    epiworld::Progress pbar(nsteps, 80);

    // Initializing the simulation
    for (unsigned int t = 0; t < nsteps; ++t)
    {

        model.update_status();
        model.mutate_variant();
        model.next();

        // 10% of rewire
        model.rewire_degseq(floor(model.size() * .1));

        pbar.next();

    }

    pbar.end();
    
    EPIWORLD_CLOCK_END("Run model")

    // Writing off the results
    model.get_db().write_data(
        "variants_info.txt",
        "variants.txt",
        "total.txt",
        "transmisions.txt"
    );

    model.write_edgelist("simple-world-edgelist.txt");

    return 0;

}