// #define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

template<typename TSeq>
EntityToAgentFun<TSeq> dist_factory(int n) {
    return [](
    Entity<> * e, Model<> * m
    ) -> Agent<> * {

        return new Agent<>(e, m);

    };
}

int main() {

    std::vector< double > contact_matrix = {
        0.9, 0.1, 0.1,
        0.05, 0.8, .2,
        0.05, 0.1, 0.7
    };

    epimodels::ModelSEIRMixing<> model(
        "Flu", // std::string vname,
        100000, // epiworld_fast_uint n,
        0.01,// epiworld_double prevalence,
        4.0,// epiworld_double contact_rate,
        0.1,// epiworld_double transmission_rate,
        4.0,// epiworld_double avg_incubation_days,
        1.0/7.0,// epiworld_double recovery_rate,
        contact_matrix
    );

    // Creating three groups
    Entity<> e1("Entity 1");
    Entity<> e2("Entity 2");
    Entity<> e3("Entity 3");

    model.add_entity_n(e1, 10000/3);
    model.add_entity_n(e2, 10000/3);
    model.add_entity_n(e3, 10000/3);

    // Running and checking the results
    model.run(50, 123);
    model.print();

    return 0;

}
