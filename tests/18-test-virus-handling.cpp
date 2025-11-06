// #define EPI_DEBUG_VIRUS
#include "tests.hpp"

using namespace epiworld;
EPIWORLD_TEST_CASE(
    "Virus set_state and get_state",
    "[virus-set-get-state]"
)
{
    
    size_t n = 10000;

    epimodels::ModelSIRCONN<> model(
        "TestModel",                // std::string vname
        n,                        // epiworld_fast_uint n
        1.0/static_cast<double>(n),                       // epiworld_double prevalence
        4,                        // epiworld_double contact_rate
        0.5,                        // epiworld_double transmission_rate
        1.0/7.0                      // epiworld_double recovery_rate
    );
    
    model.run(10); // Run the model for 10 days

    model.print();
}