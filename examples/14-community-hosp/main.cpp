#include "../../epiworld.hpp"

/***
 * A concrete example would be a model that includes two populations.
 * - One, a ‘community’ population and the other,
 * - a ‘hospital’ population.
 * In this model, individuals can move from the community to the hospital (admission) and move from the hospital back into the community (discharge). In both settings, there can be an infectious disease process (e.g. SIS), but we would assume that transmission does not occur between the community and the hospital (of course, this could be relaxed in the future). But through admission and discharge, infections in the community impact dynamics in the hospital and the reverse is true as well.
 */

using namespace epiworld;

template<typename TSeq = EPI_DEFAULT_TSEQ>
inline void my_suscept(
    Agent<TSeq> * p,
    Model<TSeq> * m
    )
{

    // Counting how many entities the agent has
    auto n_entities = p->get_n_entities();

    if (n_entities == 0)
    {
        // You can move agents to an entity
        // p->add_entity(m->get_entity("Hospital"));
        return;

    }

    Virus<TSeq> * virus = sampler::sample_virus_single<TSeq>(p, m);

    // You can remove them from the entity
    // p->rm_entity(m->get_entity("Hospital"));
    
    if (virus == nullptr)
        return;

    p->set_virus(*virus, m); 

    return;

}

int main() {

    // Using the mixing model as a baseline
    Model<> model;

    // model.add_state("Susceptible", default_update_susceptible<>); // State 0
    model.add_state("Susceptible", my_suscept<>); // State 0
    model.add_state("Infected", default_update_exposed<>);        // State 1

    Entity<> hospital("Hospital");
    hospital.set_distribution(distribute_entity_randomly<>(.5, true, true));
    model.add_entity(hospital);

    // Adding a new virus
    Virus<> mrsa("MRSA");
    mrsa.set_state(1, 0, 0);
    mrsa.set_prob_infecting(.1);
    mrsa.set_prob_recovery(.33);
    mrsa.set_distribution(distribute_virus_randomly<>(0.01));

    model.add_virus(mrsa);

    // Add a population
    model.agents_smallworld(1000, 4, 0.1, false);

    // Adding a new population
    model.run(100, 1231);
    model.print();

    return 0;

}