#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

// Original data will be an integer vector
#define DAT std::vector<bool>
static DAT base_seq = {true, false, false, true, true, false, true, false, true, false, false};

// Defining mutation and transmission functions
EPI_NEW_MUTFUN(covid19_mut, DAT) {
    
    if (EPI_RUNIF() < MPAR(0))
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

// Post covid recovery
EPI_NEW_VIRUSFUN(post_covid, DAT) {

    epiworld::Tool<DAT> immunity;
    immunity.set_sequence(*v->get_sequence());

    immunity.set_susceptibility_reduction(1.0);
    p->add_tool(m->today(), immunity);

}

int main() {

    // Initializing the model and reading population --------------------------
    epiworld::Model<DAT> model;

    model.pop_from_adjlist("edgelist.txt", 0, false);

    // Setting up the model parameters 
    model.add_param(0.001, "Mutation rate");
    model.add_param(0.90, "vax efficacy");
    model.add_param(0.0001, "vax death");
    model.add_param(0.10, "imm efficacy");
    model.add_param(0.10, "imm recovery");
    model.add_param(0.001, "imm death");
    model.add_param(0.90, "imm trans");

    // Initializing disease ---------------------------------------------------
    epiworld::Virus<DAT> covid19("COVID19");
    covid19.set_sequence(base_seq);
    covid19.set_mutation(covid19_mut);
    covid19.set_post_recovery(post_covid);  

    // Creating tools ---------------------------------------------------------
    epiworld::Tool<DAT> vaccine("Vaccine");
    vaccine.set_susceptibility_reduction(&model("vax efficacy"));
    vaccine.set_recovery_enhancer(0.4);
    vaccine.set_death_reduction(&model("vax death"));
    vaccine.set_transmission_reduction(0.5);
    
    epiworld::Tool<DAT> mask("Face masks");
    mask.set_susceptibility_reduction(0.8);
    mask.set_transmission_reduction(0.05);

    epiworld::Tool<DAT> immune("Immune system");
    immune.set_susceptibility_reduction(&model("imm efficacy"));
    immune.set_recovery_enhancer(&model("imm recovery"));
    immune.set_death_reduction(&model("imm death"));
    immune.set_transmission_reduction(&model("imm trans"));
    DAT seq0(base_seq.size(), false);
    immune.set_sequence_unique(seq0);

    // Adding the virus and the tools to the model ----------------------------
    model.add_virus(covid19, 0.01); 

    model.add_tool(vaccine, 0.5);
    model.add_tool(mask, 0.5);
    model.add_tool(immune, 1.0);
    
    // Initializing and printing information about the model ------------------
    model.init(60, 1231);
    model.queuing_off(); // Not working with rewiring just yet.
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
        "transmisions.txt",
        "transition.txt"
    );

    model.write_edgelist("simple-world-edgelist.txt");

    model.print();

    return 0;

}