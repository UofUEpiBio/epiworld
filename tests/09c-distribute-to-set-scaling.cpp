#include "tests.hpp"

using namespace epiworld;

// distribute_tool_to_set()/distribute_virus_to_set() used to capture the
// agent IDs by value. Since the distribution function is stored in the
// Tool/Virus and every agent receives a clone of it, distributing to k agents
// cost O(k^2). Compare against distribute_*_randomly(), which is O(k).
EPIWORLD_TEST_CASE(
    "Distributing to a large set is linear", "[DistFuns][DistToSet]"
) {

    size_t n = 100000u;
    size_t k = n / 2;

    std::vector< size_t > set;
    for (size_t i = 0u; i < k; ++i)
        set.push_back(2u * i);

    auto time_it = [&](
        ToolToAgentFun<> tool_fun, VirusToAgentFun<> virus_fun
    ) -> double {

        epimodels::ModelSIRCONN<> model(
            "a virus", n, 0.0, 0.0, 0.0, 0.0
        );
        model.get_virus(0).set_distribution(virus_fun);

        Tool<> tool("tool");
        tool.set_distribution(tool_fun);
        model.add_tool(tool);

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

    double t_random = time_it(
        distribute_tool_randomly<>(static_cast< double >(k), false),
        distribute_virus_randomly<>(static_cast< double >(k), false)
    );

    double t_set = time_it(
        distribute_tool_to_set<>(set),
        distribute_virus_to_set<>(set)
    );

    #ifdef EPI_DEBUG
    std::cout << "Distributing to " << k << " agents: randomly "
        << t_random << "s, to set " << t_set << "s" << std::endl;
    #endif

    // With the O(k^2) copies, t_set was >100x t_random at this size.
    REQUIRE(t_set < 10.0 * t_random + 0.1);

}
