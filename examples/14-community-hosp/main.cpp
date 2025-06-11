#include "../../include/epiworld/epiworld.hpp"

/***
 * A concrete example with a model that includes two populations.
 * - One, a ‘community’ population and the other,
 * - a ‘hospital’ population.
 * In this model, individuals can move from the community to the hospital (admission) and move from the hospital back into the community (discharge). In both settings, there can be an infectious disease process (e.g. SIS), but we would assume that transmission does not occur between the community and the hospital (of course, this could be relaxed in the future). But through admission and discharge, infections in the community impact dynamics in the hospital and the reverse is true as well.
 */

using namespace epiworld;

enum States : size_t {
    Susceptible = 0u,
    Infected,
    Infected_Hospitalized
};

// A sampler that excludes infected from the hospital
auto sampler_suscept = sampler::make_sample_virus_neighbors<>(
    {States::Infected_Hospitalized}
    );

/**
 * - Susceptibles only live in the community.
 * - Infection from individuals in the community only.
 * - Once they become infected, they may be hospitalized or not.
 */

inline void update_susceptible(Agent<int> * p, Model<int> * m)
{

    auto virus = sampler_suscept(p, m);
    if (virus != nullptr)
    {
        if (m->par("Prob hospitalization") > m->runif())
            p->set_virus(*virus, m, States::Infected_Hospitalized);
        else
            p->set_virus(*virus, m, States::Infected);
    }


    return;

}

/**
 * Infected individuals may:
 * 
 * - Stay the same
 * - Recover
 * - Be hospitalized
 * 
 * Notice that the roulette makes the probabilities to sum to 1.
 */
inline void update_infected(Agent<int> * p, Model<int> * m)
{

    // Vector of probabilities
    std::vector< epiworld_double > probs = {
        m->par("Prob hospitalization"),
        m->par("Prob recovery")
    };

    // Sampling:
    // - (-1) Nothing happens
    // - (0) Hospitalization
    // - (1) Recovery
    int res = roulette<>(probs, m);

    if (res == 0)
        p->change_state(m, States::Infected_Hospitalized);
    else if (res == 1)
        p->rm_virus(m, States::Susceptible);

    return;

}

/**
 * Infected individuals who are hospitalized may:
 * - Stay infected.
 * - Recover (and then be discharged)
 * - Stay the same and be discharged.
 */
inline void update_infected_hospitalized(Agent<int> * p, Model<int> * m)
{

    if (m->par("Prob recovery") > m->runif()) {
        p->rm_virus(m, States::Susceptible);
    } else if (m->par("Discharge infected") > m->runif()) {
        p->change_state(m, States::Infected);
    }
    
    return;

}

int main() {

    // Using the mixing model as a baseline
    Model<> model;

    // model.add_state("Susceptible", default_update_susceptible<>); // State 0
    model.add_state("Susceptible", update_susceptible); // State 0
    model.add_state("Infected", update_infected);       // State 1
    model.add_state(
        "Infected (hospitalized)",
        update_infected_hospitalized
        ); // State 2         

    // Adding a new virus
    Virus<> mrsa("MRSA");
    mrsa.set_state(1, 0, 0);
    mrsa.set_prob_infecting(.1);
    mrsa.set_prob_recovery(.33);
    mrsa.set_distribution(distribute_virus_randomly<>(0.01));

    model.add_virus(mrsa);

    // Add a population
    model.agents_smallworld(1000, 4, 0.1, false);

    model.add_param(0.1, "Prob hospitalization");
    model.add_param(0.33, "Prob recovery");
    model.add_param(0.1, "Discharge infected");

    // Adding a new population
    model.run(100, 1231);
    model.print();

    return 0;

}
