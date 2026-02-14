#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("sample", "[model-methods]") {

    // Creating a model
    Model<> model;

    // Declaring the three statuses in the model
    model.add_state("Susceptible", epiworld::default_update_susceptible<>);
    auto exposed_state = model.add_state("Exposed", epiworld::default_update_exposed<>);
    auto recovered_state = model.add_state("Recovered");
    auto removed_state = model.add_state("Removed");

    // Adding the tool and virus
    Virus<> virus("covid 19", 50, false);
    virus.set_post_immunity(1.0);
    virus.set_state(exposed_state, recovered_state, removed_state);
    virus.set_prob_death(.01);
    model.add_virus(virus);

    epiworld::Tool<> tool("vaccine", .5, true);
    model.add_tool(tool);

    // Generating a random pop
    model.agents_smallworld(10000);

    // Running the model
    model.run(100, 123);
    model.print();

    (void) model.get_db().get_transition_probability();

    AgentsSample<> agents(model, .05 * 10000);


    printf_epiworld("Total sampled: %lu\n", agents.size());
    for (auto & a: agents)
        a->print();



}
