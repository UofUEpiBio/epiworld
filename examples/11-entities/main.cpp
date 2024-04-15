// #define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main() {

    std::vector< double > contact_matrix = {
        0.9, 0.1, 0.1,
        0.05, 0.8, .2,
        0.05, 0.1, 0.7
    };

    epimodels::ModelSEIREntitiesConn model(
        "Flu", // std::string vname,
        100000, // epiworld_fast_uint n,
        0.01,// epiworld_double prevalence,
        4.0,// epiworld_double contact_rate,
        0.1,// epiworld_double transmission_rate,
        4.0,// epiworld_double avg_incubation_days,
        1.0/7.0,// epiworld_double recovery_rate,
        {.1, .1, .8},// std::vector< epiworld_double > entities,
        {"A", "B", "C"},// std::vector< std::string > entities_names
        contact_matrix
    );

    // Running and checking the results
    model.run(50, 123);
    model.print();

    return 0;

}
