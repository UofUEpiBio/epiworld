#include "tests.hpp"
#include "measles-stages.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles model (no quarantine)",
    "[ModelMeaslesSchoolOff]"
) {
    
    // Queuing doesn't matter and get results that are meaningful
    int n_seeds = 1;
    measles::ModelMeaslesSchool<> model_0(
        500,    // Number of agents
        n_seeds, // Number of initial cases
        2.0,     // Contact rate
        0.2,     // Transmission rate
        0.9,     // Vaccination efficacy
        0.3,     // Vaccination reduction recovery rate
        7.0,     // Incubation period
        4.0,     // Prodromal period
        5.0,     // Rash period
        3.0,     // Days undetected
        0.04411765,     // Hospitalization rate
        7.0,     // Hospitalization duration
        0.0,     // Proportion vaccinated
        21u,     // Quarantine period
        .8,      // Quarantine willingness
        4u       // Isolation period
    );

    // Shutting off the quarantine feature
    // model_0.set_param("Quarantine period", -1.0);
    // model_0.set_param("Isolation period", -1.0);

    // Setting the distribution function of the initial cases
    model_0.assign_virus(0, assign::agents(first_agents(n_seeds)));

    size_t nsims = 500;
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * n_seeds, -1.0);
    std::vector< double > outbreak_sizes(nsims, 0.0);
    std::vector< double > hospitalizations(nsims, 0.0);
        
    auto saver = tests_create_saver(transitions, R0s, n_seeds, nullptr, &outbreak_sizes, &hospitalizations);

    model_0.run_multiple(200, nsims, 1231, saver, true, true, 4);
    
    model_0.print(false);

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

    // Average hospitalizations
    double obs_hosp_probability = 0.0;
    for (auto i = 0u; i < hospitalizations.size(); ++i)
    {
        if (hospitalizations[i] >= 0.0)
            obs_hosp_probability += hospitalizations[i]/outbreak_sizes[i];
        else
            throw std::range_error(
                "The number of hospitalizations is negative. This should not happen."
            );
    }
    obs_hosp_probability /= static_cast<epiworld_double>(nsims);

    // Checking especific values in the transitions
    // The engine's states are the (stage, status) pairs: state = stage * 4 +
    // status (stages S, L, P, Rash, Severe, Recovered; statuses Free,
    // Quarantined, Isolated, Hospitalized)
    #define mat(i, j) avg_transitions[j*n_states + i]
    // Exits from Rash with the drawn rash period (see measles-stages.hpp)
    auto rash = measles_rash_exits(
        model_0("Rash period"),
        model_0("Rash period sd"),
        model_0("Hospitalization rate"),
        1.0/model_0("Days undetected")
    );
    double R0_theo = model_0("Contact rate") * model_0("Transmission rate") *
        model_0("Prodromal period");

    // R0
    REQUIRE_FALSE(moreless(R0_observed, R0_theo, 0.25));

    // Transition to prodromal
    REQUIRE_FALSE(
        moreless(mat(4, 8) + mat(1, 8) + mat(21, 8), 1.0/model_0("Incubation period"), 0.05));

    // Transition to rash
    REQUIRE_FALSE(
        moreless(mat(8, 12) + mat(8, 14), 1.0/model_0("Prodromal period"), 0.05)
    );

    // Transition to isolated
    REQUIRE_FALSE(
        moreless(
            mat(12, 14),
            (1.0/model_0("Days undetected")) * (1.0 - mat(12, 20) - mat(12, 19)), 0.05
        )
    );

    // Transition to hospitalized
    REQUIRE_FALSE(
        moreless(mat(12, 19), model_0("Hospitalization rate"),
        0.05)
    );
    REQUIRE_FALSE(
        moreless(0.0 + mat(14, 19), model_0("Hospitalization rate"), 0.05)
    );

    // Transition to recovered
    // Agents move between Rash and Isolated (and back when isolation
    // ends), so the recovery rate of either state depends on the isolation
    // policy. 14k and 14l test the rash period; the share hospitalized is
    // checked below.
    // Agents enter Isolated part-way through their rash period, so its
    // recovery rate has no simple form; 14k-measles-stages.cpp tests that
    // isolation keeps the rash period.

    // Transition from hospitalized to recovered
    REQUIRE_FALSE(moreless(mat(19, 20), 1.0/model_0("Hospitalization period"), 0.05));

    // Hospitalization probability
    REQUIRE_FALSE(
        moreless(
            rash.p_hospitalized,
            obs_hosp_probability,
            0.05
        )
    );

    // Reproductive number
    std::cout << "Reproductive number (lower): "
              << R0_observed << " (expected ~" << R0_theo << ")" << std::endl;

    // Transition to prodromal
    std::cout << "Transition to prodromal: "
              << mat(4, 8)  + mat(1, 8) + mat(21, 8) <<
              " (expected ~" << 1.0/model_0("Incubation period") << ")" << std::endl;

    // Transition to rash
    std::cout << "Transition to rash: "
              << mat(8, 12) + mat(8, 14) << " (expected ~" << 1.0/model_0("Prodromal period") << ")" << std::endl;

    // Transition to isolated
    std::cout << "Transition to isolated: "
              << mat(12, 14) << " (expected ~" << (1.0/model_0("Days undetected")) * (1.0 - mat(12, 20) - mat(12, 19)) << ")" << std::endl;

    // Transition to hospitalized
    std::cout << "Transition to hospitalized (prodromal): "
              << mat(12, 19) << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;
    std::cout << "Transition to hospitalized (rash): "
              << 0.0 + mat(14, 19) << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;

    // Transition to recovered    
    std::cout << "Transition to recovered (prodromal): "
              << mat(12, 20) << std::endl;
    std::cout << "Transition to recovered (rash): "
              << mat(14, 22) + mat(14, 20) << std::endl;

    // Transition from hospitalized to recovered
    std::cout << "Transition from hospitalized to recovered: "
              << mat(19, 20) << " (expected ~" << 1.0/model_0("Hospitalization period") << ")" << std::endl;


    // Hospitalization probability
    std::cout << "Hospitalization probability: "
              << rash.p_hospitalized << " (observed ~" << obs_hosp_probability << ")" << std::endl;

    std::cout << "Outbreak size: " << stats::mean(outbreak_sizes) << std::endl;
    #undef mat

    
}