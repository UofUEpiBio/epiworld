#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#define N_THREADS 8
#else
#define N_THREADS 4
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles model with risk-based quarantine",
    "[ModelMeaslesMixingRiskQuarantine]"
) {

    size_t nsims = 400u;
    size_t n = 1000u;
    
    double R0       = 1.8;
    double c_rate   = 10.0;
    double p_infect = R0 / (c_rate) * (1.0/4.0);

    // Simple contact matrix (single group, all mixing)
    std::vector<double> contact_matrix = {1.0};
    
    epimodels::ModelMeaslesMixingRiskQuarantine<> model(
        n,           // Number of agents
        0.005,       // Initial prevalence
        c_rate,      // Contact rate
        p_infect,    // Transmission rate
        0.9,         // Vaccination efficacy
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        contact_matrix, // Contact matrix
        0.2,         // Hospitalization rate
        7.0,         // Hospitalization duration
        3.0,         // Days undetected
        21,          // Quarantine period high risk
        14,          // Quarantine period medium risk
        7,           // Quarantine period low risk
        .8,          // Quarantine willingness
        .8,          // Isolation willingness
        // A negative isolation suppresses the whole
        // quarantine process
        -1,           // Isolation period
        0.0,         // Proportion vaccinated
        // A negative detection rate suppresses the whole
        // quarantine process
        -1,         // Detection rate during quarantine
        1.0,         // Contact tracing success rate
        4u           // Contact tracing days prior
    );

    // Adding a single entity (population group)
    model.add_entity(Entity<>("Population", dist_factory<>(0, n)));

    // Virus will be distributed to 5 random agents, in each
    // replicate of the model
    model.get_virus(0).set_distribution(
        distribute_virus_randomly<>(5, false)
    );
    
    std::vector< std::vector< epiworld_double > > transitions(nsims);
    std::vector< epiworld_double > R0s(nsims * 5, -1.0);

    auto saver = tests_create_saver(transitions, R0s, 5);

    // Run simulation
    model.run_multiple(100, nsims, 123, saver, true, true, N_THREADS);
    model.print();

    auto tmat = tests_calculate_avg_transitions(transitions, model);
    tests_print_avg_transitions(tmat, model);

    // Averaging R0
    auto avg_R0 = std::accumulate(R0s.begin(), R0s.end(), 0.0) /
        static_cast<epiworld_double>(R0s.size());

    #define mat(i, j) tmat[(j)*model.get_n_states() + (i)]
    std::cout << "Average R0 from index cases: " <<
        avg_R0 <<  " vs expected " << R0 << std::endl;

    // Looking into the transition matrix -----------------------

    // From Exposed
    std::cout << "Transition from Exposed to Prodromal: " <<
        mat(1, 2) << " (expected: " <<
        1.0/model("Incubation period") << ")" << std::endl;

    // From Prodromal
    std::cout << "Transition from Prodromal to Rash: " <<
        mat(2, 3) << " (expected: " <<
        1.0/model("Prodromal period") << ")" << std::endl;

    // From Rash
    std::cout << "Transition from Rash to Recovery: " <<
        mat(3, 12) << " (expected: " <<
        (1.0 - model("Hospitalization rate") - 1.0/model("Rash period")) << ")" <<
        std::endl;

    std::cout << "Transition from Rash to Hospitalized: " <<
        mat(3, 11) << " (expected: " <<
        model("Hospitalization rate") << ")" << std::endl;

    // From hospitalized
    std::cout << "Transition from Hospitalized to Recovery: " <<
        mat(11, 12) << " (expected: " <<
        1.0/model("Hospitalization period") << ")" << std::endl;

    #ifdef CATCH_CONFIG_MAIN
    // Validating some transitions -------------------------------
    REQUIRE_FALSE(
        moreless(mat(1, 2), 1.0/model("Incubation period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(2, 3), 1.0/model("Prodromal period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(
            mat(3, 12),
            (1.0 - model("Hospitalization rate") - 1.0/model("Rash period")),
            0.1
        )
    );

    REQUIRE_FALSE(
        moreless(mat(3, 11), model("Hospitalization rate"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(11, 12), 1.0/model("Hospitalization period"), 0.1)
    );
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
}

#undef mat
#undef N_THREADS
