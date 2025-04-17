#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Rt", "[Rt]") {

    int n_seeds = 2000;
    double n = 100000;
    double R0 = 1.25;
    double trate = 0.9;
    double rrate = 1.0/7.0;
    double crate = R0 * rrate / trate;

    // Creating a simple ModelSIR model
    epimodels::ModelSIRCONN<> model_0(
        "a virus", n, static_cast<double>(n_seeds)/n, crate, trate, rrate
    );

    // Function to distribute the virus to the first 10 agents
    auto dist_virus = [n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(v, m);
        return;
    };

    model_0.get_virus(0).set_distribution(dist_virus);

    model_0.run(60, 222);

    model_0.print(false);

    auto repnum = model_0.get_db().reproductive_number();

    int n_seed = -1, n_records = 0;
    double rts = 0.0;
    for (auto & i: repnum)
    {
        if (i.first[1] == -1)
            n_seed = i.second;
        else if (i.first[1] < n_seeds)
        {
            rts += static_cast<double>(i.second);
            ++n_records;
        }
    }

    std::cout << "Number of seeds: " << n_seed << std::endl;
    std::cout << "Rt: " << rts / static_cast<double>(n_seed) << 
        " (expected: " << R0 << ")" << std::endl;

    
    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(n_seed == n_seeds);
    REQUIRE_FALSE(moreless(rts/static_cast<double>(n_seed), R0, 0.05));
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}