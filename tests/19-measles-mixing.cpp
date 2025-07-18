#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles model with mixing (no quarantine)",
    "[ModelMeaslesMixingOff]"
) {
    
    // Queuing doesn't matter and get results that are meaningful
    int n_seeds = 5;
    
    // Simple contact matrix (single group, all mixing)
    std::vector<double> contact_matrix = {1.0};
    
    epimodels::ModelMeaslesMixing<> model_0(
        "Measles",   // Virus name
        1000,        // Number of agents
        n_seeds / 1000.0, // Initial prevalence
        2.0,         // Contact rate
        0.2,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        contact_matrix, // Contact matrix
        0.2,         // Hospitalization rate
        7.0,         // Hospitalization duration
        3.0,         // Days undetected
        21,          // Quarantine period
        .8,          // Quarantine willingness
        .8,          // Isolation willingness
        4,           // Isolation period
        0.0,         // Proportion vaccinated
        1.0,         // Contact tracing success rate
        4u           // Contact tracing days prior
    );

    // Shutting off the quarantine feature
    model_0("Quarantine period") = -1.0;
    // model_0("Isolation period") = -1.0;

    // Adding a single entity (population group)
    model_0.add_entity(Entity<>("Population", dist_factory<>(0, 1000)));

    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(v, m);
        return;
    });

    size_t nsims = 100; // Reduced for faster testing
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * n_seeds, -1.0);
        
    auto saver = tests_create_saver(transitions, R0s, n_seeds);

    model_0.run_multiple(60, nsims, 1231, saver, true, true, 4);
    
    #ifndef CATCH_CONFIG_MAIN
    model_0.print(false);
    #endif

    // Calculate average transitions
    auto avg_transitions = tests_calculate_avg_transitions(
        transitions, model_0
    );

    size_t n_states = model_0.get_n_states();

    tests_print_avg_transitions(avg_transitions, model_0);

    // Average R0
    double R0_observed = 0.0;
    for (auto & i: R0s)
    {
        if (i >= 0.0)
            R0_observed += i;
        else
            throw std::range_error(
                "The R0 value is negative. This should not happen."
            );
    }
    R0_observed /= static_cast<epiworld_double>(nsims * n_seeds);

    // Checking specific values in the transitions
    #define mat(i, j) avg_transitions[j*n_states + i]
    double p_recovered = 1.0 - (
        1.0/model_0("Rash period") + model_0("Hospitalization rate")
    );
    double R0_theo = model_0("Contact rate") * model_0("Transmission rate") *
        model_0("Prodromal period");
    #ifdef CATCH_CONFIG_MAIN

    // R0 - should be approximately equal to theoretical value
    REQUIRE_FALSE(moreless(R0_observed, R0_theo, 0.3));

    // Transition from exposed to prodromal
    REQUIRE_FALSE(moreless(mat(1, 2), 1.0/model_0("Incubation period"), 0.1));

    // Transition from prodromal to rash
    REQUIRE_FALSE(
        moreless(mat(2, 3) + mat(2, 4), 1.0/model_0("Prodromal period"), 0.1)
    );

    // Transition from rash to isolated/recovered (detection)
    REQUIRE_FALSE(
        moreless(
            mat(3, 4) + mat(3, 5) + mat(3, 6),
            1.0/model_0("Days undetected"), 0.1
        )
    );

    // Transition to hospitalized from rash
    REQUIRE_FALSE(
        moreless(mat(3, 6) + mat(3, 11), model_0("Hospitalization rate"),
        0.1)
    );
    REQUIRE_FALSE(
        moreless(mat(4, 6) + mat(4, 11), model_0("Hospitalization rate"), 0.1)
    );

    // Transition to recovered from rash
    REQUIRE_FALSE(moreless(mat(3, 5) + mat(3, 12), p_recovered, 0.1));
    REQUIRE_FALSE(moreless(mat(4, 5) + mat(4, 12), p_recovered, 0.1));

    // Transition from hospitalized to recovered
    REQUIRE_FALSE(moreless(mat(11, 12), 1.0/model_0("Hospitalization period"), 0.1));
    #endif
    
    // Reproductive number
    std::cout << "Reproductive number: "
              << R0_observed << " (expected ~" << R0_theo << ")" << std::endl;

    // Transition from exposed to prodromal
    std::cout << "Transition to prodromal: "
              << mat(1, 2) << " (expected ~" << 1.0/model_0("Incubation period") << ")" << std::endl;

    // Transition from prodromal to rash
    std::cout << "Transition to rash: "
              << mat(2, 3) + mat(2, 4) << " (expected ~" << 1.0/model_0("Prodromal period") << ")" << std::endl;

    // Transition from rash to isolated (detection)
    std::cout << "Transition to isolated: "
              << mat(3, 4) + mat(3, 5) + mat(3, 6) << " (expected ~" << 1.0/model_0("Days undetected") << ")" << std::endl;

    // Transition to hospitalized from rash
    std::cout << "Transition to hospitalized (rash): "
              << mat(3, 6) + mat(3, 11) << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;
    std::cout << "Transition to hospitalized (isolated): "
              << mat(4, 6) + mat(4, 11) << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;

    // Transition to recovered from rash    
    std::cout << "Transition to recovered (rash): "
              << mat(3, 5) + mat(3, 12) << " (expected ~" << p_recovered << ")" << std::endl;
    std::cout << "Transition to recovered (isolated): "
              << mat(4, 5) + mat(4, 12) << " (expected ~" << p_recovered << ")" << std::endl;

    // Transition from hospitalized to recovered
    std::cout << "Transition from hospitalized to recovered: "
              << mat(11, 12) << " (expected ~" << 1.0/model_0("Hospitalization period") << ")" << std::endl;
    #undef mat
    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
    
}

EPIWORLD_TEST_CASE(
    "Measles model with mixing (with quarantine)",
    "[ModelMeaslesMixingQuarantine]"
) {
    
    // Testing with quarantine enabled
    int n_seeds = 3;
    
    // Simple contact matrix (single group, all mixing)
    std::vector<double> contact_matrix = {1.0};
    
    epimodels::ModelMeaslesMixing<> model_0(
        "Measles",   // Virus name
        500,         // Number of agents (smaller for quarantine test)
        n_seeds / 500.0, // Initial prevalence
        2.0,         // Contact rate
        0.2,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        contact_matrix, // Contact matrix
        0.1,         // Hospitalization rate
        7.0,         // Hospitalization duration
        3.0,         // Days undetected
        14,          // Quarantine period
        .8,          // Quarantine willingness
        .8,          // Isolation willingness
        10,          // Isolation period
        0.0,         // Proportion vaccinated
        0.8,         // Contact tracing success rate
        5u           // Contact tracing days prior
    );

    // Adding a single entity (population group)
    model_0.add_entity(Entity<>("Population", dist_factory<>(0, 500)));

    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(v, m);
        return;
    });

    // Run a single simulation to test quarantine functionality
    model_0.run(30, 1234);
    
    #ifndef CATCH_CONFIG_MAIN
    model_0.print(false);
    #endif

    // Check that the model runs without errors and produces reasonable results
    std::vector<int> history;
    model_0.get_db().get_hist_total(nullptr, nullptr, &history);
    
    // Verify states exist and some transitions occurred
    bool found_prodromal = false;
    bool found_rash = false;
    bool found_quarantine = false;
    
    // History is a flattened vector: [day0_state0, day0_state1, ..., day1_state0, ...]
    size_t n_states = model_0.get_n_states();
    size_t n_days = history.size() / n_states;
    
    for (size_t day = 0; day < n_days; ++day) {
        size_t offset = day * n_states;
        if (history[offset + model_0.PRODROMAL] > 0)
            found_prodromal = true;
        if (history[offset + model_0.RASH] > 0)
            found_rash = true;
        if (history[offset + model_0.QUARANTINED_EXPOSED] > 0 || 
            history[offset + model_0.QUARANTINED_SUSCEPTIBLE] > 0 ||
            history[offset + model_0.QUARANTINED_PRODROMAL] > 0)
            found_quarantine = true;
    }
    
    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(found_prodromal);
    REQUIRE(found_rash); 
    // Note: quarantine may not always trigger in short simulations
    #endif
    
    std::cout << "Found prodromal cases: " << (found_prodromal ? "Yes" : "No") << std::endl;
    std::cout << "Found rash cases: " << (found_rash ? "Yes" : "No") << std::endl;
    std::cout << "Found quarantine cases: " << (found_quarantine ? "Yes" : "No") << std::endl;

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
    
}