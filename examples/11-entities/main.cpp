#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main() {

    epimodels::ModelSEIREntitiesConn model(
        "Flu", // std::string vname,
        10000, // epiworld_fast_uint n,
        0.01,// epiworld_double prevalence,
        4.0,// epiworld_double contact_rate,
        0.1,// epiworld_double transmission_rate,
        4.0,// epiworld_double avg_incubation_days,
        1.0/7.0,// epiworld_double recovery_rate,
        {.1, .1, .8},// std::vector< epiworld_double > entities,
        {"A", "B", "C"}// std::vector< std::string > entities_names
    );

    // Running and checking the results
    model.run(50, 123);
    model.print();

    return 0;

}
