#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

// The layers of 23-layers (households, workplaces, public spaces) plus
// transit: a morning and an evening trip on weekdays, with familiar strangers
int main() {

    const size_t n = 20000u;

    epimodels::ModelSEIR model(
        "a virus", // Name of the virus
        n,         // Number of agents
        0.001,     // Initial prevalence
        0.02,      // Transmission probability (per contact)
        4.0,       // Latent period
        1.0 / 7.0  // Recovery rate
    );

    // Weekdays: five days on, two off (the run starts on a Monday)
    const std::vector< bool > weekdays = {true, true, true, true, true, false, false};

    // Households of four: each member meets each housemate with probability
    // 0.5 a day
    std::vector< size_t > household(n);
    for (size_t i = 0u; i < n; ++i)
        household[i] = i / 4u;

    auto home = layer::mixing("household");
    home->set_groups(assign::labels(household), n / 4u);
    home->set_contact_prob({0.5});
    model.add_layer(home);

    // Workplaces: the same ten or so coworkers, on weekdays
    auto work = layer::network("work", graph::smallworld(10, 0.05));
    work->set_schedule(weekdays);
    model.add_layer(work);

    // Transit: half the agents commute on 25 routes, each with 8 morning trips
    // of 50 riders (the trips are the groups, the routes the pools). A rider
    // takes its usual trip with probability 0.8 a day, and otherwise another
    // trip of its route (familiar strangers), and meets each other rider of
    // the trip with probability 0.02. The evening trips are another layer:
    // the same commuters, in other trips.
    const size_t routes = 25u, trips_per_route = 8u, riders = 50u;
    const size_t trips = routes * trips_per_route;

    std::vector< size_t > pools(trips);
    for (size_t g = 0u; g < trips; ++g)
        pools[g] = g / trips_per_route;

    auto commute = [&](std::string name, size_t shift) {
        std::vector< int > trip(n, -1);
        for (size_t k = 0u; k < trips * riders; ++k)
            trip[(2u * k + shift) % n] = static_cast< int >(
                (k * 7u + shift) % trips
            );

        auto t = layer::mixing(std::move(name));
        t->set_groups(assign::labels(trip), trips);
        t->set_contact_prob(0.02);
        t->set_pools(pools);
        t->set_persistence(0.8);
        t->set_schedule(weekdays);
        return t;
    };
    model.add_layer(commute("transit am", 0u));
    model.add_layer(commute("transit pm", 2u));

    // Public spaces: three age groups, met afresh every day, more on weekends
    // (a weekly cycle of the contact rate: 4 on weekdays, 6 on weekends)
    auto public_spaces = layer::mixing("public");
    public_spaces->set_groups(assign::shares({{0, 0.25}, {1, 0.5}, {2, 0.25}}), 3u);
    public_spaces->set_contact_matrix({
        0.6, 0.3, 0.1,
        0.2, 0.6, 0.2,
        0.1, 0.4, 0.5
    });
    model.add_param(4.0, "Public contact rate");
    public_spaces->set_contact_rate("Public contact rate");
    model.set_param_cycle("Public contact rate", {4, 4, 4, 4, 4, 6, 6});
    model.add_layer(public_spaces);

    model.run(120, 2131);
    model.print();

    // Where and when the transmissions happened
    std::vector< int > date, source, target, virus, exposure, layer_id;
    model.get_db().get_transmissions(
        date, source, target, virus, exposure, layer_id
    );

    const size_t nl = model.get_n_layers();
    std::vector< size_t > weekday(nl, 0u), weekend(nl, 0u);
    size_t total = 0u;
    for (size_t k = 0u; k < layer_id.size(); ++k)
    {
        if (layer_id[k] < 0)
            continue;

        const bool on = weekdays[static_cast< size_t >(date[k] - 1) % 7u];
        ++(on ? weekday : weekend)[static_cast< size_t >(layer_id[k])];
        ++total;
    }

    printf_epiworld("Transmissions by layer (%zu in all):\n", total);
    printf_epiworld("  %-12s %9s %9s\n", "", "weekdays", "weekends");
    for (size_t l = 0u; l < nl; ++l)
    {
        printf_epiworld(
            "  %-12s %8.1f%% %8.1f%%\n", model.get_layer(l).get_name().c_str(),
            total > 0u ? 100.0 * weekday[l] / total : 0.0,
            total > 0u ? 100.0 * weekend[l] / total : 0.0
        );
    }

    return 0;

}
