#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles model with mixing (no quarantine)",
    "[ModelMeaslesMixingOff]"
) {
    
    // Queuing doesn't matter and get results that are meaningful
    int n_seeds = 5;
    
    // Simple contact matrix (single group, all mixing)
    std::vector<double> contact_matrix = {2.0};
    
    measles::ModelMeaslesMixing<> model_0(
        1000,        // Number of agents
        n_seeds / 1000.0, // Initial prevalence
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
    model_0.set_param("Quarantine period", -1.0);
    // model_0.set_param("Isolation period", -1.0);

    // Adding a single entity (population group)
    model_0.add_entity(Entity<>("Population", dist_factory<>(0, 1000)));

    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    size_t nsims = 400; // Reduced for faster testing
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * n_seeds, -1.0);
    std::vector< double > outbreak_sizes(nsims, 0.0);
    std::vector< double > hospitalizations(nsims, 0.0);
        
    auto saver = tests_create_saver(transitions, R0s, n_seeds, nullptr, &outbreak_sizes, &hospitalizations);

    model_0.
        run_multiple(200, nsims, 1231, saver, true, true, 4).
        print(false);

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

    // Checking specific values in the transitions
    #define mat(i, j) avg_transitions[j*n_states + i]
    double p_recovered = 1.0/model_0("Rash period");
    double R0_theo = contact_matrix[0] * model_0("Transmission rate") *
        model_0("Prodromal period");

    // R0 - should be approximately equal to theoretical value
    REQUIRE_FALSE(moreless(R0_observed, R0_theo, 0.3));

    // Transition from exposed to prodromal
    REQUIRE_FALSE(moreless(mat(1, 2), 1.0/model_0("Incubation period"), 0.1));

    // Transition from prodromal to rash
    REQUIRE_FALSE(
        moreless(mat(2, 3) + mat(2, 4), 1.0/model_0("Prodromal period"), 0.1)
    );

    // Transition from rash to detected (detection)
    // Includes Isolated + IsolatedRecovered + Hospitalized (detected hosp folded in)
    REQUIRE_FALSE(
        moreless(
            mat(3, 4) + mat(3, 5) + mat(3, 10),
            1.0/model_0("Days undetected") +
            (1.0 - 1.0/model_0("Days undetected")) * model_0("Hospitalization rate"),
            0.1
        )
    );

    // Transition to hospitalized from rash
    REQUIRE_FALSE(
        moreless(mat(3, 10), model_0("Hospitalization rate"),
        0.1)
    );
    REQUIRE_FALSE(
        moreless(mat(4, 10), model_0("Hospitalization rate"), 0.1)
    );

    // Transition to recovered from rash
    REQUIRE_FALSE(moreless(mat(3, 5) + mat(3, 11), p_recovered, 0.1));
    REQUIRE_FALSE(moreless(mat(4, 5) + mat(4, 11), p_recovered, 0.1));

    // Transition from hospitalized to recovered
    REQUIRE_FALSE(moreless(mat(10, 11), 1.0/model_0("Hospitalization period"), 0.1));

    // Hospitalization probability
    REQUIRE_FALSE(
        moreless(
            model_0("Hospitalization rate")/(
                model_0("Hospitalization rate") + p_recovered
            ),
            obs_hosp_probability,
            0.1
        )
    );
    
    // Reproductive number
    std::cout << "Reproductive number: "
              << R0_observed << " (expected ~" << R0_theo << ")" << std::endl;

    // Transition from exposed to prodromal
    std::cout << "Transition to prodromal: "
              << mat(1, 2) << " (expected ~" << 1.0/model_0("Incubation period") << ")" << std::endl;

    // Transition from prodromal to rash
    std::cout << "Transition to rash: "
              << mat(2, 3) + mat(2, 4) << " (expected ~" << 1.0/model_0("Prodromal period") << ")" << std::endl;

    // Transition from rash to detected (detection)
    std::cout << "Transition to detected: "
              << mat(3, 4) + mat(3, 5) + mat(3, 10) << " (expected ~" <<
              1.0/model_0("Days undetected") +
              (1.0 - 1.0/model_0("Days undetected")) * model_0("Hospitalization rate")
              << ")" << std::endl;

    // Transition to hospitalized from rash
    std::cout << "Transition to hospitalized (rash): "
              << mat(3, 10) << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;
    std::cout << "Transition to hospitalized (isolated): "
              << mat(4, 10) << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;

    // Transition to recovered from rash    
    std::cout << "Transition to recovered (rash): "
              << mat(3, 5) + mat(3, 11) << " (expected ~" << p_recovered << ")" << std::endl;
    std::cout << "Transition to recovered (isolated): "
              << mat(4, 5) + mat(4, 11) << " (expected ~" << p_recovered << ")" << std::endl;

    // Transition from hospitalized to recovered
    std::cout << "Transition from hospitalized to recovered: "
              << mat(10, 11) << " (expected ~" << 1.0/model_0("Hospitalization period") << ")" << std::endl;

    // Hospitalization probability
    std::cout << "Hospitalization probability: "
              << model_0("Hospitalization rate")/(
                  model_0("Hospitalization rate") + p_recovered
              ) << " (observed ~" << obs_hosp_probability << ")" << std::endl;
    #undef mat

    
}
