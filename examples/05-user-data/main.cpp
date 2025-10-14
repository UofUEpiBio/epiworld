#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

int main()
{

    // Setting up model --------------------------------------------------------
    epiworld::Model<> model;
    model.add_state("Susceptible", epiworld::default_update_susceptible<>);
    auto exposed_state = model.add_state("Exposed", epiworld::default_update_exposed<>);
    auto removed_state = model.add_state("Removed");


    model.set_user_data({"agent_id", "virus_id"});

    model.agents_from_adjlist(
        epiworld::rgraph_smallworld(200, 5, .1, false, model)
    );

    model.add_param(.9, "infectiousness");
    model.add_param(.3, "recovery");

    // Setting up virus --------------------------------------------------------
    epiworld::Virus<> v("covid", 5, false);
    v.set_state(exposed_state, removed_state, removed_state);

    EPI_NEW_POSTRECOVERYFUN_LAMBDA(immunity, int)
    {
        // Post immunity
        auto Tpr = m->get_tools()[1u];
        p->add_tool(Tpr, m);

        m->add_user_data({
            static_cast< epiworld_double >(p->get_id()),
            static_cast< epiworld_double >(v.get_id())
            });

    };

    v.set_post_recovery(immunity);
    v.set_prob_infecting(&model("infectiousness"));

    // Setting up tool ---------------------------------------------------------
    epiworld::Tool<> is("immune system", 1.0, true);
    is.set_susceptibility_reduction(.3);
    is.set_death_reduction(.9);
    is.set_recovery_enhancer(&model("recovery"));

    epiworld::Tool<> postImm("post immunity", 0, false);
    postImm.set_susceptibility_reduction(1.0);

    model.add_tool(is);
    model.add_tool(postImm);
    model.add_virus(v);
    model.run(112, 30);
    model.print();

    model.get_user_data().print();

    model.get_user_data().write("user-data.txt");

}
