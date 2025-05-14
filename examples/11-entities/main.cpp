// #define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

template<typename TSeq = EPI_DEFAULT_TSEQ>
EntityToAgentFun<TSeq> dist_factory(int from, int to) {
    return [from, to](Entity<>& e, Model<>* m) -> void {
        auto& agents = m->get_agents();
        for (size_t i = from; i < to; ++i) {
            e.add_agent(&agents[i], m);
        }

        return;
    };
}

int main() {
    std::vector<double> contact_matrix =
        {0.9, 0.1, 0.1, 0.05, 0.8, .2, 0.05, 0.1, 0.7};

    epimodels::ModelSEIRMixing<> model(
        "Flu", // std::string vname,
        10000, // epiworld_fast_uint n,
        0.01, // epiworld_double prevalence,
        10.0, // epiworld_double contact_rate,
        0.1, // epiworld_double transmission_rate,
        4.0, // epiworld_double avg_incubation_days,
        1.0 / 7.0, // epiworld_double recovery_rate,
        contact_matrix
    );

    // Creating three groups
    Entity<> e1("Entity 1", dist_factory<>(0, 3000));
    Entity<> e2("Entity 2", dist_factory<>(3000, 6000));
    Entity<> e3("Entity 3", dist_factory<>(6000, 10000));

    model.add_entity(e1);
    model.add_entity(e2);
    model.add_entity(e3);

    // Running and checking the results
    model.run(50, 123);
    model.print();

    return 0;
}
