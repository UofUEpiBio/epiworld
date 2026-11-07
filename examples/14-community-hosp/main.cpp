#include "../../include/epiworld/epiworld.hpp"

/***
 * A concrete example with a model that includes two populations.
 * - One, a ‘community’ population and the other,
 * - a ‘hospital’ population.
 * In this model, individuals can move from the community to the hospital (admission) and move from the hospital back into the community (discharge). In both settings, there can be an infectious disease process (e.g. SIS), but we would assume that transmission does not occur between the community and the hospital (of course, this could be relaxed in the future). But through admission and discharge, infections in the community impact dynamics in the hospital and the reverse is true as well.
 */

using namespace epiworld;

/**
 * - Susceptibles only live in the community.
 * - Infection from individuals in the community only.
 * - Once they become infected, they may be hospitalized or not.
 * - Infected individuals may be hospitalized or recover (competing daily
 *   rates); hospitalized ones may recover or be discharged infected.
 */
inline Virus mrsa_virus()
{
    Virus mrsa("MRSA");
    mrsa.add_stage("Susceptible");
    const int infected = mrsa.add_stage("Infected", stage::infectious);
    const int hospitalized = mrsa.add_stage("Infected (hospitalized)", stage::carrier);

    // Infected agents in the hospital do not transmit
    mrsa.infection(0, infected, 0.1).to({
        {infected, Value::complement("Prob hospitalization")},
        {hospitalized, "Prob hospitalization"}
    });
    mrsa.rate(infected, hospitalized, "Prob hospitalization");
    mrsa.rate(infected, 0, "Prob recovery");
    mrsa.rate(hospitalized, 0, "Prob recovery");
    mrsa.rate(hospitalized, infected, "Discharge infected");
    return mrsa;
}

int main() {
    Model model(1000);

    model.add_param(0.1, "Prob hospitalization");
    model.add_param(0.33, "Prob recovery");
    model.add_param(0.1, "Discharge infected");

    // Adding the virus
    model.add_virus(mrsa_virus(), assign::share(0.01));

    // Add a population
    model.add_layer(layer::network(
        "network", rgraph_smallworld(1000, 4, 0.1, false, model)
    ));

    // Running the model
    model.run(100, 1231);
    model.print();

    return 0;

}
