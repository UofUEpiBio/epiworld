#include "tests.hpp"

using namespace epiworld;

// Placing a virus and a tool on a large given set of agents
// (assign::agents()) costs about what a random placement of the same number
// does: the ids are shared, not copied per agent.
EPIWORLD_TEST_CASE(
    "Assigning to a large set is linear", "[assign][scaling]"
) {

    size_t n = 100000u;
    size_t k = n / 2;

    std::vector< size_t > set;
    for (size_t i = 0u; i < k; ++i)
        set.push_back(2u * i);

    auto time_it = [&](Assign<> who) -> double {

        epimodels::ModelSIR<> model("a virus", n, 0.0, 0.0, 0.0);
        add_random_mixing(model, 0.0);
        model.assign_virus(0, who);
        model.add_tool(Tool<>("tool"), who);
        model.verbose_off();

        auto start = std::chrono::steady_clock::now();
        model.run(0, 1);
        std::chrono::duration< double > elapsed =
            std::chrono::steady_clock::now() - start;

        size_t n_tools = 0u, n_virus = 0u;
        for (auto & agent : model.get_agents())
        {
            n_tools += agent.get_n_tools();
            n_virus += (agent.get_virus() != nullptr) ? 1u : 0u;
        }

        REQUIRE(n_tools == k);
        REQUIRE(n_virus == k);

        return elapsed.count();

    };

    double t_random = time_it(assign::count<>(k));
    double t_set = time_it(assign::agents<>(set));

    #ifdef EPI_DEBUG
    std::cout << "Assigning " << k << " agents: randomly "
        << t_random << "s, to a set " << t_set << "s" << std::endl;
    #endif

    REQUIRE(t_set < 10.0 * t_random + 0.1);

}
