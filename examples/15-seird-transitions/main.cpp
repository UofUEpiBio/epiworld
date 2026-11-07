/**
 * @file main.cpp
 * @brief SEIRD model declared as a natural history
 *
 * Builds a Susceptible-Exposed-Infected-Recovered-Deceased (SEIRD) model by
 * declaring the virus's stages and the transitions between them: daily rates
 * read from model parameters, with the two exits from Infected competing.
 *
 * Flow:
 *   S --virus--> E --rate--> I --rate--> R
 *                                \--rate--> D
 */
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main()
{
    constexpr epiworld_fast_uint n_agents = 5000u;

    Model model(n_agents);

    // Model parameters
    model.add_param(0.3, "Transmission rate");
    model.add_param(1.0 / 7.0, "E->I transition rate");
    model.add_param(0.5, "I->R transition rate");
    model.add_param(0.01, "I->D transition rate");

    // The virus and its stages
    Virus virus("A flu");
    const int S = virus.add_stage("Susceptible");
    const int E = virus.add_stage("Exposed", stage::carrier);
    const int I = virus.add_stage("Infected", stage::infectious);
    const int R = virus.add_stage("Recovered");
    const int D = virus.add_stage("Deceased");
    virus.set_contact(D, 0.0);

    virus.infection(S, E, "Transmission rate");
    virus.rate(E, I, "E->I transition rate");
    virus.rate(I, R, "I->R transition rate").effect(Effect::recovery);
    virus.rate(I, D, "I->D transition rate").effect(Effect::death);
    model.add_virus(virus, assign::share(0.01));

    // Population
    model.add_layer(layer::network(
        "network", rgraph_smallworld(n_agents, 20, 0.01, false, model)
    ));

    // Run and display
    model.run(100, 123);
    model.print();

    return 0;

}
