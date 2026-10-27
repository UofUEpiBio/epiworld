#include "../../include/epiworld/epiworld.hpp"

/***
 * A concrete example with a model that includes two populations.
 * - One, a ‘community’ population and the other,
 * - a ‘hospital’ population.
 * In this model, individuals can move from the community to the hospital (admission) and move from the hospital back into the community (discharge). In both settings, there can be an infectious disease process (e.g. SIS), but we would assume that transmission does not occur between the community and the hospital (of course, this could be relaxed in the future). But through admission and discharge, infections in the community impact dynamics in the hospital and the reverse is true as well.
 */

using namespace epiworld;

class CommunityHospModel : public Model<int> {
public:
    unsigned long long susceptible_state;
    unsigned long long infected_state;
    unsigned long long infected_hospitalized_state;

    explicit CommunityHospModel(size_t n);
};

/**
 * - Susceptibles only live in the community.
 * - Infection from individuals in the community only.
 * - Once they become infected, they may be hospitalized or not.
 */

inline void update_susceptible(Agent<int> * p, Model<int> * m)
{
    auto hm = static_cast<CommunityHospModel*>(m);

    // Drawing at most one virus from the neighbors who transmit (infected
    // agents in the hospital do not; see the constructor)
    Virus<int> * virus = m->sample_virus(p);

    if (virus == nullptr)
        return;

    if (m->par("Prob hospitalization") > m->runif())
        p->set_virus(*m, *virus, hm->infected_hospitalized_state);
    else
        p->set_virus(*m, *virus, hm->infected_state);

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
    auto hm = static_cast<CommunityHospModel*>(m);

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
        p->change_state(*m, hm->infected_hospitalized_state);
    else if (res == 1)
        p->rm_virus(*m, hm->susceptible_state);

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
    auto hm = static_cast<CommunityHospModel*>(m);

    if (m->par("Prob recovery") > m->runif()) {
        p->rm_virus(*m, hm->susceptible_state);
    } else if (m->par("Discharge infected") > m->runif()) {
        p->change_state(*m, hm->infected_state);
    }

    return;

}

CommunityHospModel::CommunityHospModel(size_t n) : Model<int>(n)
{
    this->susceptible_state = this->add_state("Susceptible", update_susceptible);
    this->infected_state = this->add_state("Infected", update_infected);
    this->infected_hospitalized_state = this->add_state(
        "Infected (hospitalized)", update_infected_hospitalized);

    // Only infected agents in the community transmit
    this->set_infectious_states({static_cast< int >(this->infected_state)});
}

int main() {
    auto model = CommunityHospModel(1000);

    // Adding a new virus
    Virus<> mrsa("MRSA");
    mrsa.set_state(model.infected_state, model.susceptible_state, model.susceptible_state);
    mrsa.set_prob_infecting(.1);
    mrsa.set_prob_recovery(.33);
    mrsa.set_distribution(distribute_virus_randomly<>(0.01));

    model.add_virus(mrsa);

    // Add a population
    model.add_layer(layer::network(
        "network", rgraph_smallworld(1000, 4, 0.1, false, model)
    ));

    model.add_param(0.1, "Prob hospitalization");
    model.add_param(0.33, "Prob recovery");
    model.add_param(0.1, "Discharge infected");

    // Adding a new population
    model.run(100, 1231);
    model.print();

    return 0;

}
