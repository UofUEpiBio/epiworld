// #define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

template<typename TSeq = EPI_DEFAULT_TSEQ>
EntityToAgentFun<TSeq> dist_factory(size_t from, size_t to) {
    return [from, to](Entity<> & e, Model<> * m) -> void {

            auto & agents = m->get_agents();
            for (size_t i = from; i < to; ++i)
            {
                e.add_agent(&agents[i], *m);
            }
            
            return;

        };
}

int main() {
    constexpr epiworld_fast_uint n_agents = 5000u;
    constexpr size_t entity1_end = n_agents * 3u / 10u;
    constexpr size_t entity2_end = n_agents * 6u / 10u;

    std::vector< double > contact_matrix = {
        9.0, 1.0, 1.0,
        0.5, 8.0, 2.0,
        0.5, 1.0, 7.0
    };

    epimodels::ModelSEIR<> model(
        "Flu", // std::string vname,
        n_agents, // epiworld_fast_uint n,
        0.01,// epiworld_double prevalence,
        0.1,// epiworld_double transmission_prob,
        4.0,// epiworld_double latent_period,
        1.0/7.0 // epiworld_double recovery_rate
    );

    // Contacts by group, from a contact matrix; each agent's group is its
    // entity (the three below)
    auto mixing = layer::mixing<>("mixing");
    mixing->set_contact_matrix(contact_matrix);
    model.add_layer(mixing);

    // Creating three groups
    Entity<> e1("Entity 1", dist_factory<>(0, entity1_end));
    Entity<> e2("Entity 2", dist_factory<>(entity1_end, entity2_end));
    Entity<> e3("Entity 3", dist_factory<>(entity2_end, n_agents));

    model.add_entity(e1);
    model.add_entity(e2);
    model.add_entity(e3);

    // Running and checking the results
    model.run(50, 123);
    model.print();

    return 0;

}
