#include "tests.hpp"

using namespace epiworld;

// A tool that targets a virus also acts on its variants
EPIWORLD_TEST_CASE("Tool targeting a mutating virus", "[tool][target][mutation]") {

    // No recovery: infected agents stay infected
    epimodels::ModelSIRCONN<> model("A", 1000, 0.0, 4.0, 0.5, 0.0);
    model.verbose_off();

    // Seeding the odd agents 1, ..., 39, with a virus that mutates every day
    std::vector< size_t > seeds;
    for (size_t i = 1u; i < 40u; i += 2u)
        seeds.push_back(i);

    Virus<> & virus = model.get_virus(0);
    virus.set_distribution(distribute_virus_to_set<>(seeds));
    virus.set_mutation(
        [](Agent<> *, Virus<> & v, Model<> *) -> bool {
            v.set_sequence(v.get_sequence() + 1);
            return true;
        });

    // Protecting the even agents against A
    std::vector< size_t > protected_ids;
    for (size_t i = 0u; i < model.size(); i += 2u)
        protected_ids.push_back(i);

    Tool<> tool("Vaccine against A");
    tool.set_susceptibility_reduction(1.0);
    tool.set_distribution(distribute_tool_to_set<>(protected_ids));
    tool.add_target(virus);
    model.add_tool(tool);

    model.run(30, 2231);

    // Variants were recorded
    REQUIRE(model.get_n_viruses() > 10u);

    size_t n_infected_variant = 0u;
    for (auto & agent : model.get_agents())
    {

        const auto & v = agent.get_virus();

        if (agent.get_id() % 2 == 0)
        {
            // Protected agents are never infected by any variant
            REQUIRE(agent.get_n_tools() == 1u);
            REQUIRE(v == nullptr);
        }
        else if (v != nullptr)
        {
            // Every variant belongs to the lineage of A
            REQUIRE(v->get_lineage_id() == 0);
            if (v->get_id() > 0)
                ++n_infected_variant;
        }

    }

    // The variants spread among the unprotected agents
    REQUIRE(n_infected_variant > seeds.size());

}
