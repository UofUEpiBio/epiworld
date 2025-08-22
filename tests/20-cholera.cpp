#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Cholera", "[Cholera]") {

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelCholera<> model_0(
        1000, // Agents
        1,   // Initial prevalence
        0.1,  // Environment load
        0.1,  // pp_t_rate
        0.2,  // e_t_rate
        0.3,  // hs_rate
        0.5,  // loss_rate_imm
        0.6,  // rec_rate
        0.7,  // shedd_rate
        0.8,  // b_death_rate_env
        0.1,  // eb_ingested
        0.9,  // i_growth_rate
        1.0,  // b_carry_capacity
        1.2,  // b_death_rate
        1.3,  // pb_contact_rate
        1.4,  // p_i_rate
        1.5,  // cd_size
        1.6,  // b_size
        1.7,  // p_death_rate
        1.8,  // b_death_rate_lysis
        1.9   // nu
    );

    model_0.print();

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}