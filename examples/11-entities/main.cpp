// #define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main() {
    constexpr epiworld_fast_uint n_agents = 5000u;
    constexpr size_t group1_end = n_agents * 3u / 10u;
    constexpr size_t group2_end = n_agents * 6u / 10u;

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

    // Three groups: the first 30% of the agents, the next 30%, and the rest
    std::vector< size_t > groups(n_agents);
    for (size_t i = 0u; i < n_agents; ++i)
        groups[i] = (i < group1_end) ? 0u : ((i < group2_end) ? 1u : 2u);

    // Contacts by group, from a contact matrix
    auto mixing = layer::mixing<>("mixing");
    mixing->set_groups(groups, 3u);
    mixing->set_contact_matrix(contact_matrix);
    model.add_layer(mixing);

    // Running and checking the results
    model.run(50, 123);
    model.print();

    return 0;

}
