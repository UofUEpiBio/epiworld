#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

// Original data will be an integer vector
#define DAT std::vector<int>
static DAT base_seq = {true, false, false, true, true, false, true, false, true, false, false};

// Defining mutation and transmission functions
EPI_NEW_MUTFUN(covid19_mut, DAT) {
    
    if (EPI_RUNIF() < MPAR(0))
    {
        // Picking a location at random
        int idx = std::floor(EPI_RUNIF() * v.get_sequence()->size());
        DAT tmp_seq = *v.get_sequence();
        tmp_seq[idx] = !v.get_sequence()->at(idx); 

        // Updating its sequence
        v.set_sequence(tmp_seq);

        return true;
    }

    return false;
    
}

// Post covid recovery
EPI_NEW_POSTRECOVERYFUN(post_covid, DAT) {

    auto Tptr = m->get_tools()[3u];

    Tptr->set_sequence(*v.get_sequence());
    p->add_tool(Tptr);

}

int main() {

    // Initializing the model and reading population --------------------------
    epiworld::Model<DAT> model;

    model.add_status("Susceptible", epiworld::default_update_susceptible<DAT>);
    model.add_status("Exposed", epiworld::default_update_exposed<DAT>);
    model.add_status("Recovered");
    model.add_status("Removed");

    model.agents_from_adjlist(
        "edgelist.txt", // Filepath
        1000,           // Vertex count
        0,              // Lines to skip
        false           // Directed?
        );

    // Setting up the model parameters 
    model.add_param(0.001, "Mutation rate");
    model.add_param(0.90, "vax efficacy");
    model.add_param(0.0001, "vax death");
    model.add_param(0.10, "imm efficacy");
    model.add_param(0.10, "imm recovery");
    model.add_param(0.001, "imm death");
    model.add_param(0.90, "imm trans");
    model.add_param(0.01, "virus death");

    // Initializing disease ---------------------------------------------------
    epiworld::Virus<DAT> covid19("COVID19");
    covid19.set_sequence(base_seq);
    covid19.set_mutation(covid19_mut);
    covid19.set_post_recovery(post_covid); 
    covid19.set_prob_death(&model("virus death"));

    covid19.set_status(1,2,3);

    // Creating tools ---------------------------------------------------------
    epiworld::Tool<DAT> vaccine("Vaccine");
    vaccine.set_susceptibility_reduction(&model("vax efficacy"));
    vaccine.set_recovery_enhancer(0.4);
    vaccine.set_death_reduction(&model("vax death"));
    vaccine.set_transmission_reduction(0.5);
    
    epiworld::Tool<DAT> mask("Face masks");
    mask.set_susceptibility_reduction(0.8);
    mask.set_transmission_reduction(0.05);seq0

    epiworld::Tool<DAT> immune("Immune system");
    immune.set_susceptibility_reduction(&model("imm efficacy"));
    immune.set_recovery_enhancer(&model("imm recovery"));
    immune.set_death_reduction(&model("imm death"));
    immune.set_transmission_reduction(&model("imm trans"));
    DAT seq0(base_seq.size(), false);
    immune.set_sequence(seq0);

    epiworld::Tool<DAT> post_immunity("Post Immune");
    post_immunity.set_susceptibility_reduction(1.0);

    // Adding the virus and the tools to the model ----------------------------
    model.add_virus(covid19, 0.01); 

    model.add_tool(immune, 1.0);
    model.add_tool(vaccine, 0.5);
    model.add_tool(mask, 0.5);
    model.add_tool_n(post_immunity, 0);
    
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
        "tool_info.txt",
        "tool_hist.txt",
        "total.txt",
        "transmisions.txt",
        "transition.txt",
        "reproductive.txt"
    );

    model.write_edgelist("simple-world-edgelist.txt");

    model.print();

    return 0;

}