#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

// Original data will be an integer vector
#define DAT std::vector<int>
static DAT base_seq = {true, false, false, true, true, false, true, false, true, false, false};

// Defining mutation and transmission functions
EPI_NEW_MUTFUN(covid19_mut, DAT) {

    if (EPI_RUNIF() < m->par("Mutation rate"))
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
    p->add_tool(Tptr, m);

}

int main() {

    // Initializing the model and reading population --------------------------
    epiworld::Model<DAT> model;

    model.add_state("Susceptible", epiworld::default_update_susceptible<DAT>);
    auto exposed_state = model.add_state("Exposed", epiworld::default_update_exposed<DAT>);
    auto recovered_state = model.add_state("Recovered");
    auto removed_state = model.add_state("Removed");

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
    epiworld::Virus<DAT> covid19("COVID19", 0.01, true);
    covid19.set_sequence(base_seq);
    covid19.set_mutation(covid19_mut);
    covid19.set_post_recovery(post_covid);
    covid19.set_prob_death(&model("virus death"));

    covid19.set_state(exposed_state, recovered_state, removed_state);

    // Creating tools ---------------------------------------------------------
    epiworld::Tool<DAT> vaccine("Vaccine", 0.5, true);
    vaccine.set_susceptibility_reduction(&model("vax efficacy"));
    vaccine.set_recovery_enhancer(0.4);
    vaccine.set_death_reduction(&model("vax death"));
    vaccine.set_transmission_reduction(0.5);

    epiworld::Tool<DAT> mask("Face masks", 0.5, true);
    mask.set_susceptibility_reduction(0.8);
    mask.set_transmission_reduction(0.05);

    epiworld::Tool<DAT> immune("Immune system", 1.0, true);
    immune.set_susceptibility_reduction(&model("imm efficacy"));
    immune.set_recovery_enhancer(&model("imm recovery"));
    immune.set_death_reduction(&model("imm death"));
    immune.set_transmission_reduction(&model("imm trans"));
    DAT seq0(base_seq.size(), false);
    immune.set_sequence(seq0);

    epiworld::Tool<DAT> post_immunity("Post Immune", 0, true);
    post_immunity.set_susceptibility_reduction(1.0);

    // Adding the virus and the tools to the model ----------------------------
    model.add_virus(covid19);

    model.add_tool(immune);
    model.add_tool(vaccine);
    model.add_tool(mask);
    model.add_tool(post_immunity);

    // Initializing and printing information about the model ------------------
    model.queuing_off(); // Not working with rewiring just yet.
    model.set_rewire_fun(
        [](std::vector<epiworld::Agent<DAT>>* agents,
           epiworld::Model<std::vector<int>>* m,
           float p)
        {
            epiworld::rewire_degseq<DAT>(
                reinterpret_cast<epiworld::AdjList*>(agents),
                m,
                p
            );
        }
    );


    model.set_rewire_prop(0.10);

    // Screen information
    model.print();

    // Running the model
    model.run(60, 123);

    // Writing off the results
    model.write_data(
        "examples/04-advanced-usage/variants_info.txt",
        "examples/04-advanced-usage/variants.txt",
        "examples/04-advanced-usage/tool_info.txt",
        "examples/04-advanced-usage/tool_hist.txt",
        "examples/04-advanced-usage/total.txt",
        "examples/04-advanced-usage/transmisions.txt",
        "examples/04-advanced-usage/transition.txt",
        "examples/04-advanced-usage/reproductive.txt",
	"",
	"",
	"",
        ""
    );

    model.write_edgelist("examples/04-advanced-usage/simple-world-edgelist.txt");

    model.print();

    return 0;

}
