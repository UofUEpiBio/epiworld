// #define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

int main()
{

    // Setting up model --------------------------------------------------------
    epiworld::Model model(200);


    model.set_user_data({"agent_id", "virus_id"});

    model.add_layer(epiworld::layer::network(
        "network", epiworld::rgraph_smallworld(200, 5, .1, false, model)
    ));

    model.add_param(.9, "infectiousness");
    model.add_param(.3, "recovery");

    // Setting up virus --------------------------------------------------------
    // Infected agents recover at the rate their tools give them
    epiworld::Virus v("covid");
    v.add_stage("Susceptible");
    v.add_stage("Exposed", epiworld::stage::infectious);
    v.add_stage("Removed");
    v.infection(0, 1, "infectiousness");
    const int recovery = v.rate(1, 2, 0.0).effect(epiworld::Effect::recovery);

    // Setting up tool ---------------------------------------------------------
    epiworld::Tool is("immune system");
    is.set_susceptibility_reduction(.3);
    is.set_death_reduction(.9);
    is.set_recovery_enhancer("recovery");

    model.add_tool(is, epiworld::assign::share(1.0));
    model.add_virus(v, epiworld::assign::count(5));

    // Each recovery is recorded in the user data (the recovered are immune:
    // Removed has no infection)
    model.react(
        epiworld::Trigger::fired(recovery),
        [](epiworld::Agent & p, epiworld::Model & m) {
            m.add_user_data({
                static_cast< epiworld_double >(p.get_id()),
                static_cast< epiworld_double >(0) // the only virus
            });
        }
    );
    model.run(112, 30);
    model.print();

    model.get_user_data().print();

    model.get_user_data().write("user-data.txt");

}
