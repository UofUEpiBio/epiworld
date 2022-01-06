#include <iostream>
#include <memory>
#include <vector>
#include <random>

#include "../include/epiworld/epiworld.hpp"

// Original data will be an integer vector
#define DAT std::vector<bool>
static DAT base_seq = {true, false, false, true, true, false, true, false, true, false, false};

// Defining mutation and transmission functions
EPI_NEW_MUTFUN(covid19_mut, DAT) {
    
    if (EPI_RUNIF() < *(v->p00))
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

// Getting the vaccine
EPI_NEW_TOOL(vaccine_eff, DAT) {return TPAR(00);} 
EPI_NEW_TOOL(vaccine_rec, DAT) {return 0.4;}
EPI_NEW_TOOL(vaccine_death, DAT) {return TPAR(01);}
EPI_NEW_TOOL(vaccine_trans, DAT) {return 0.5;}

// Wearing a Mask
EPI_NEW_TOOL(mask_eff, DAT) {return 0.8;}
EPI_NEW_TOOL(mask_trans, DAT) {return 0.05;}

// Immune system
EPI_NEW_TOOL(immune_eff, DAT) {return TPAR(00);}
EPI_NEW_TOOL(immune_rec, DAT) {return TPAR(01);}
EPI_NEW_TOOL(immune_death, DAT) {return TPAR(02);}
EPI_NEW_TOOL(immune_trans, DAT) {return TPAR(03);}

// Post covid recovery
EPI_NEW_RECFUN(post_covid, DAT) {

    epiworld::Tool<DAT> immunity;
    immunity.set_sequence(*v->get_sequence());

    // Post efficacy the individual has full immunity
    EPI_NEW_TOOL_LAMBDA(post_efficacy, DAT) {return 1.0;};

    immunity.set_efficacy(post_efficacy);
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

    // Initializing the model and reading population --------------------------
    epiworld::Model<DAT> model;

    model.pop_from_adjlist("edgelist.txt", 0, false);

    // Initializing disease ---------------------------------------------------
    epiworld::Virus<DAT> covid19(base_seq, "COVID19");
    covid19.set_mutation(covid19_mut);
    covid19.set_post_recovery(post_covid);  

    // Creating tools ---------------------------------------------------------
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

    // Setting up the model parameters ----------------------------------------   
    covid19.add_param(mutrate, "Mutation rate", model);

    immune.add_param(0.10, "imm efficacy", model);
    immune.add_param(0.10, "imm recovery", model);
    immune.add_param(0.001, "imm death", model);
    immune.add_param(0.90, "imm trans", model);

    vaccine.add_param(0.90, "vax efficacy", model);
    vaccine.add_param(0.0001, "vax death", model);

    // Adding the virus and the tools to the model ----------------------------
    model.add_virus(covid19, preval); 

    model.add_tool(vaccine, 0.5);
    model.add_tool(mask, 0.5);
    model.add_tool(immune, 1.0);
    
    // Initializing and printing information about the model ------------------
    model.init(nsteps, seed);
    model.set_rewire_fun(epiworld::rewire_degseq<DAT>);  
    model.set_rewire_prop(0.10);

    // Screen information
    model.print();

    // Running the model
    model.run();

    // Writing off the results
    model.write_data(
        "variants_info.txt",
        "variants.txt",
        "total.txt",
        "transmisions.txt"
    );

    model.write_edgelist("simple-world-edgelist.txt");

    model.print();

    return 0;

}