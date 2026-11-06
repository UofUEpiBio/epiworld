#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main() {

    const size_t n = 20000u;

    epimodels::ModelSEIR<> model(
        "a virus", // Name of the virus
        n,         // Number of agents
        0.001,     // Initial prevalence
        0.02,      // Transmission probability (per contact)
        4.0,       // Latent period
        1.0 / 7.0  // Recovery rate
    );

    // Households of four: each member meets each housemate with probability
    // 0.5 a day
    std::vector< size_t > household(n);
    for (size_t i = 0u; i < n; ++i)
        household[i] = i / 4u;

    auto home = layer::mixing<>("household");
    home->set_groups(assign::labels(household), n / 4u);
    home->set_contact_prob({0.5});
    model.add_layer(home);

    // Workplaces: the same ten or so coworkers every day (a small world,
    // redrawn at every run)
    model.add_layer(layer::network("work", graph::smallworld<>(10, 0.05)));

    // Public spaces: three age groups (a quarter children, half adults, a
    // quarter older adults), drawn at random; entry (g, h) is the share of the
    // daily public contacts of someone in g made with people in h
    auto public_spaces = layer::mixing<>("public");
    public_spaces->set_groups(assign::shares({{0, 0.25}, {1, 0.5}, {2, 0.25}}), 3u);
    public_spaces->set_contact_matrix({
        0.6, 0.3, 0.1,
        0.2, 0.6, 0.2,
        0.1, 0.4, 0.5
    });
    model.add_param(4.0, "Public contact rate");
    public_spaces->set_contact_rate("Public contact rate");
    model.add_layer(public_spaces);

    model.run(120, 2131);
    model.print();

    // Where the transmissions happened
    std::vector< int > date, source, target, virus, exposure, layer_id;
    model.get_db().get_transmissions(
        date, source, target, virus, exposure, layer_id
    );

    std::vector< size_t > by_layer(model.get_n_layers(), 0u);
    size_t total = 0u;
    for (int l : layer_id)
        if (l >= 0)
        {
            ++by_layer[static_cast< size_t >(l)];
            ++total;
        }

    printf_epiworld("Transmissions by layer (%zu in all):\n", total);
    for (size_t l = 0u; l < by_layer.size(); ++l)
    {
        printf_epiworld(
            "  %-10s %6.1f%%\n", model.get_layer(l).get_name().c_str(),
            total > 0u ? 100.0 * by_layer[l] / total : 0.0
        );
    }

    return 0;

}
