#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

int main()
{

    // Setting up model --------------------------------------------------------
    epiworld::Model<> model;
    model.set_user_data({"person_id", "virus_id"});
    
    model.pop_from_adjlist(
        epiworld::rgraph_smallworld(200, 5, .1, false, model)
    );

    model.add_param(.9, "infectiousness");
    model.add_param(.3, "recovery");

    // Setting up virus --------------------------------------------------------
    epiworld::Virus<> v("covid");
    EPI_NEW_POSTRECOVERYFUN_LAMBDA(immunity, bool)
    {
        
        epiworld::Tool<> immune("immune");
        immune.set_susceptibility_reduction(1.0);
        p->add_tool(m->today(), immune);

        m->add_user_data({
            static_cast< epiworld_double >(p->get_id()),
            static_cast< epiworld_double >(v->get_id())
            });

    };

    v.set_post_recovery(immunity);
    v.set_prob_infecting(&model("infectiousness"));
    
    // Setting up tool ---------------------------------------------------------
    epiworld::Tool<> is("immune system");
    is.set_susceptibility_reduction(.3);
    is.set_death_reduction(.9);
    is.set_recovery_enhancer(&model("recovery"));

    model.add_tool(is, 1.0);
    model.add_virus_n(v, 5);

    model.init(112, 30);

    model.run();
    model.print();

    model.get_user_data().print();

    model.get_user_data().write("user-data.txt");

}
