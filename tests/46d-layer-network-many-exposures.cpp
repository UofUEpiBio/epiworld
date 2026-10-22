#include "tests.hpp"

using namespace epiworld;

// An agent's exposures add up across its network layers, with no ceiling: here
// agent 0 is the hub of three star layers whose 1,000 leaves are all infected,
// so it is exposed 3,000 times a day -- more than the 2,048 slots the sampler's
// scratch arrays used to have. Under roulette its chance of infection in a day
// is R / (1 + R), with R the sum of the odds p / (1 - p) of its 3,000 contacts.
EPIWORLD_TEST_CASE("Network layers - many exposures", "[layer-network]") {

    const size_t n = 1001u;
    const double p = 0.0005;

    std::vector< int > source(n - 1u, 0), target;
    std::vector< size_t > leaves;
    for (size_t i = 1u; i < n; ++i)
    {
        target.push_back(static_cast< int >(i));
        leaves.push_back(i);
    }

    epimodels::ModelSIR<> model("flu", n, 0.0, p, 0.0);
    model.verbose_off();
    model.get_virus(0).set_distribution(distribute_virus_to_set<>(leaves));
    for (std::string name : {"a", "b", "c"})
        model.add_layer(layer::network(
            name, AdjList(source, target, static_cast< int >(n), false)
        ));

    const double odds     = 3.0 * (n - 1u) * p / (1.0 - p);
    const double expected = odds / (1.0 + odds);

    for (std::string mode : {"pull", "push"})
    {

        size_t nsims = 400u, infected = 0u;
        model.set_transmission_mode(mode);
        model.run_multiple(
            1, nsims, 46,
            [&infected](size_t, Model<> * m) -> void {
                if (m->get_agent(0u).get_virus() != nullptr)
                    ++infected;
            },
            true, false, 1
        );

        double share = static_cast< double >(infected) / nsims;
        double se    = std::sqrt(expected * (1.0 - expected) / nsims);
        REQUIRE(std::abs(share - expected) < 4.0 * se);

    }

}
