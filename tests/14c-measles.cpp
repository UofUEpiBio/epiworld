#include "tests.hpp"

#define calc_r0(ans_observed, ans_expected, r0, model) \
    double (ans_observed) = std::accumulate(r0.begin(), r0.end(), 0.0); \
    (ans_observed) /= static_cast<epiworld_double>(r0.size()); \
    std::fill(r0.begin(), r0.end(), -1.0); \
    double (ans_expected) = (model)("Contact rate") * \
        (model)("Transmission rate") * \
        (model)("Prodromal period");

using namespace epiworld;

EPIWORLD_TEST_CASE("Measles model (R0)", "[ModelMeaslesSchoolR0]") {
    
    // Queuing doesn't matter and get results that are meaningful
    int n_seeds = 10;
    int n_threads = 2;
    int n_days = 90;
    size_t nsims = 400;
    epimodels::ModelMeaslesSchool<> model_0(
        5000,    // Number of agents
        n_seeds, // Number of initial cases
        2.5,     // Contact rate
        0.05,     // Transmission rate
        0.9,     // Vaccination efficacy
        0.3,     // Vaccination reduction recovery rate
        7.0,     // Incubation period
        8.0,     // Prodromal period
        5.0,     // Rash period
        3.0,     // Days undetected
        0.2,     // Hospitalization rate
        7.0,     // Hospitalization duration
        0.0,     // Proportion vaccinated
        21u,     // Quarantine period
        .8,      // Quarantine willingness
        4u       // Isolation period
    );

    // Shutting off the quarantine feature
    model_0("Quarantine period") = -1.0;

    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(v, m);
        return;
    });

    std::vector< std::vector<epiworld_double> > transitions(nsims);
    std::vector< epiworld_double > R0s(nsims * n_seeds, -1.0);
    auto saver = [&transitions, &R0s, n_seeds](size_t n, Model<>* m) -> void{

        // Recording the R0 from the index case
        auto rts = m->get_db().get_reproductive_number();      
        for (int i = 0; i < n_seeds; ++i)
            R0s[n_seeds * n + i] = static_cast<epiworld_double>(rts[{0, i, 0}]);

    };

    // Different runs
    model_0.run_multiple(n_days, nsims, 1231, saver, true, true, n_threads);
    calc_r0(R0_obs1, R0_exp1, R0s, model_0);

    model_0("Contact rate") = model_0("Contact rate") * 1.5;
    model_0.run_multiple(n_days, nsims, 1231, saver, true, true, n_threads);
    calc_r0(R0_obs2, R0_exp2, R0s, model_0);

    model_0("Contact rate") = model_0("Contact rate")  * 1.5;
    model_0.run_multiple(n_days, nsims, 1231, saver, true, true, n_threads);
    calc_r0(R0_obs3, R0_exp3, R0s, model_0);

    model_0("Contact rate") = model_0("Contact rate")  * 1.5;
    model_0.run_multiple(n_days, nsims, 1231, saver, true, true, n_threads);
    calc_r0(R0_obs4, R0_exp4, R0s, model_0);

    std::cout << "Testing R0 in Measles" << std::endl;
    std::cout << "R0 obs " << R0_obs1 << " vs exp " << R0_exp1 << std::endl;
    std::cout << "R0 obs " << R0_obs2 << " vs exp " << R0_exp2 << std::endl;
    std::cout << "R0 obs " << R0_obs3 << " vs exp " << R0_exp3 << std::endl;
    std::cout << "R0 obs " << R0_obs4 << " vs exp " << R0_exp4 << std::endl;

    REQUIRE_FALSE(moreless(R0_obs1, R0_exp1, 0.2));
    REQUIRE_FALSE(moreless(R0_obs2, R0_exp2, 0.2));
    REQUIRE_FALSE(moreless(R0_obs3, R0_exp3, 0.2));
    REQUIRE_FALSE(moreless(R0_obs4, R0_exp4, 0.2));


    
}

#undef calc_r0