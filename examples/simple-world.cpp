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
        int idx = std::floor(EPI_RUNIF() * v->get_sequence()->size());
        DAT tmp_seq = *v->get_sequence();
        tmp_seq[idx] = !v->get_sequence()->at(idx); 

        // Updating its sequence
        v->set_sequence(tmp_seq);

        return true;
    }

    return false;
    
}

// If before the third day of infection, then
    // no infectious
#define CHECK_LATENT() \
    if ((m->today() - v->get_date()) <= 3) \
        return 0.0;

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

    CHECK_LATENT()
    return 0.5;
}

// Wearing a Mask
EPI_NEW_TOOL(mask_eff, DAT) {
    return 0.8;
}

EPI_NEW_TOOL(mask_trans, DAT) {

    CHECK_LATENT()
    return 0.05;
}

// Immune system
EPI_NEW_TOOL(immune_eff, DAT) {

    return EPI_PARAMS(IMMUNE_EFFICACY);

}

EPI_NEW_TOOL(immune_rec, DAT) {
    return EPI_PARAMS(IMMUNE_EFFICACY);
}

EPI_NEW_TOOL(immune_death, DAT) {
    return EPI_PARAMS(VARIANT_MORTALITY);
}

EPI_NEW_TOOL(immune_trans, DAT) {
    CHECK_LATENT()
    return EPI_PARAMS(BASELINE_INFECCTIOUSNESS);
}

// We assume individuals cannot become reinfected with the
// same variant
EPI_NEW_TOOL(post_rec_efficacy, DAT) 
{

    const auto vseq = v->get_sequence();
    const auto tseq = t->get_sequence();

    // If different, then no help
    for (unsigned int i = 0; i < vseq->size(); ++i)
        if (vseq->at(i) != tseq->at(i))
            return 0.0;

    // If completely matches, then it is almost 100% efficacy
    return 0.95;
    
        
}

EPI_RECFUN(post_covid, DAT) {

    epiworld::Tool<DAT> immunity;
    immunity.set_sequence(*v->get_sequence());
    immunity.set_efficacy(post_rec_efficacy);
    p->add_tool(m->today(), immunity);

}

int main(int argc, char* argv[]) {


    if ((argc != 5) & (argc != 1))
        std::logic_error("You need to specify seed and number of steps (in that order).");

    int seed;
    unsigned int nsteps;
    double preval,mutrate;
    if (argc == 5)
    {
        seed   = strtol(argv[1], nullptr, 0);
        nsteps = strtol(argv[2], nullptr, 0);
        preval = strtod(argv[3], nullptr);
        mutrate = strtod(argv[4], nullptr);

    } else {
        seed    = 159;
        nsteps  = 60;
        preval  = 0.005;
        mutrate = 0.000025;
    }


    using namespace epiworld;

    // Initializing the model
    epiworld::Model<DAT> model;
    
    // Setting up the model parameters, these are six
    model.params().resize(6u, 0.0);
    model.params()[MUTATION_PROB]            = mutrate;
    model.params()[VACCINE_EFFICACY]         = 0.90;
    model.params()[IMMUNE_EFFICACY]          = 0.10;
    model.params()[VARIANT_MORTALITY]        = 0.001;
    model.params()[BASELINE_INFECCTIOUSNESS] = 0.90;
    model.params()[IMMUNE_LEARN_RATE]        = 0.05;

    // Initializing disease
    epiworld::Virus<DAT> covid19(base_seq, "COVID19");
    covid19.set_mutation(covid19_mut);
    covid19.set_post_recovery(post_covid);

    model.add_virus(covid19, preval); // 0.5% will have the virus at first

    // Reading network structure
    model.pop_from_adjlist("edgelist.txt", 0, true);

    // Creating tools
    epiworld::Tool<DAT> vaccine("Vaccine");
    vaccine.set_efficacy(vaccine_eff);
    vaccine.set_recovery(vaccine_rec);
    vaccine.set_death(vaccine_death);
    vaccine.set_transmisibility(vaccine_trans);

    epiworld::Tool<DAT> mask("Face masks");
    mask.set_efficacy(mask_eff);
    mask.set_transmisibility(mask_trans);

    epiworld::Tool<DAT> immune("Immune system");
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
    
    model.init(nsteps, seed);  

    // Screen information
    model.print();

    // Creating a progress bar
    EPIWORLD_CLOCK_START("(01) Run model")

    // Initializing the simulation
    EPIWORLD_RUN(model) 
    {

        // We can execute these components in whatever order the
        // user needs.
        model.update_status();
        model.mutate_variant();
        model.next();

        // In this case we are applying degree sequence rewiring
        // to change the network just a bit.
        model.rewire_degseq(floor(model.size() * .1));

    }

    
    EPIWORLD_CLOCK_END("(01) Run model")

    // Writing off the results
    model.get_db().write_data(
        "variants_info.txt",
        "variants.txt",
        "total.txt",
        "transmisions.txt"
    );

    model.write_edgelist("simple-world-edgelist.txt");

    EPIWORLD_CLOCK_REPORT("--- ELAPSED TIMES ---")

    model.print();

    return 0;

}