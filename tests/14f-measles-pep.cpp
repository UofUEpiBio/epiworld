#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Measles PEP intervention", "[ModelMeaslesPEP]") {

    int n_seeds = 1;
    epimodels::ModelMeaslesSchool<> model_0(
        1000,    // Number of agents
        n_seeds, // Number of initial cases
        2.0,     // Contact rate
        0.2,     // Transmission rate
        0.9,     // Vaccination efficacy
        0.3,     // Vaccination reduction recovery rate
        7.0,     // Incubation period
        4.0,     // Prodromal period
        5.0,     // Rash period
        3,       // Days undetected
        0.1,     // Hospitalization rate
        7.0,     // Hospitalization duration
        0.1,     // Proportion vaccinated
        21u,     // Quarantine period
        1.0,      // Quarantine willingness
        4u,      // Isolation period
        1.0,     // PEP efficacy
        1.0      // PEP willingness
    );

    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    size_t nsims = 500;
    std::vector< std::vector<epiworld_double> > transitions(nsims);
    std::vector< epiworld_double > R0s(nsims * n_seeds, -1.0);
    std::vector< double > hospitalizations(nsims, 0), outbreak_sizes(nsims, 0);
    auto saver = tests_create_saver(
        transitions, R0s, n_seeds, nullptr, &outbreak_sizes, &hospitalizations
    );

    model_0.run_multiple(60, nsims, 1231, saver, true, true, 2);

    model_0.print();

    // Creating an average across the transitions vectors
    auto avg_transitions = tests_calculate_avg_transitions(
        transitions, model_0
    );

    tests_print_avg_transitions(avg_transitions, model_0);

    auto n_states = model_0.get_n_states();

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

    #define mat(i, j) avg_transitions[j*n_states + i]
    double p_recovered = 1.0/model_0("Rash period");

    // =========================================================
    // PEP-specific transition checks
    // =========================================================

    // Exposed -> Recovered should exist (PEP clears the virus
    // via agent_recovers in the _update_exposed function)
    REQUIRE(mat(1, 12) > 0.01);


    // =========================================================
    // Standard disease transitions (should be preserved for
    // agents that reach prodromal/rash regardless of PEP)
    // =========================================================

    // Transition from Prodromal to Rash
    REQUIRE_FALSE(
        moreless(mat(2, 3) + mat(2, 4), 1.0/model_0("Prodromal period"), 0.05)
    );

    // Transition from Rash (days to detected/isolated)
    REQUIRE_FALSE(
        moreless(
            mat(3, 4) + mat(3, 5) + mat(3, 6),
            1.0/model_0("Days undetected"), 0.05
        )
    );

    // Transition from Quarantine Exposed to Quarantined Prodromal
    // (already checked above with wider tolerance; PEP doesn't
    // affect symptom development)

    // From quarantine prodromal to isolated (detected immediately)
    REQUIRE_FALSE(
        moreless(mat(9, 4), 1.0/model_0("Prodromal period"), 0.05)
    );

    // Transition to hospitalized from Rash and Isolated
    REQUIRE_FALSE(
        moreless(
            mat(3, 6) + mat(3, 11), model_0("Hospitalization rate"), 0.05
        )
    );
    REQUIRE_FALSE(
        moreless(
            mat(4, 6) + mat(4, 11), model_0("Hospitalization rate"), 0.05
        )
    );

    // Transition to recovered from Rash and Isolated
    REQUIRE_FALSE(moreless(mat(3, 5) + mat(3, 12), p_recovered, 0.05));
    REQUIRE_FALSE(moreless(mat(4, 5) + mat(4, 12), p_recovered, 0.05));

    // Transition from hospitalized to recovered
    REQUIRE_FALSE(
        moreless(mat(11, 12), 1.0/model_0("Hospitalization period"), 0.05)
    );

    // =========================================================
    // Hospitalization probability with PEP
    // =========================================================
    // With PEP, some exposed agents recover before reaching the
    // rash stage, so the overall hospitalization probability
    // (hospitalizations / outbreak_size) should be LOWER than
    // the theoretical no-PEP value.
    double theoretical_hosp_no_pep = model_0("Hospitalization rate") / (
        model_0("Hospitalization rate") + p_recovered
    );

    REQUIRE(obs_hosp_probability < theoretical_hosp_no_pep);

    // Rows and columns 7 and 8 (Quarantined Exposed and Quarantined
    // Susceptible) should be empty since willingness is 100%
    double col_sum_q_exposed = 0.0, col_sum_q_susceptible = 0.0;
    double row_sum_q_exposed = 0.0, row_sum_q_susceptible = 0.0;
    for (size_t i = 0u; i < n_states; ++i)
    {
        col_sum_q_exposed += mat(i, 7);
        col_sum_q_susceptible += mat(i, 8);
        row_sum_q_exposed += mat(7, i);
        row_sum_q_susceptible += mat(8, i);
    }

    REQUIRE(col_sum_q_exposed < 1e-10);
    REQUIRE(col_sum_q_susceptible < 1e-10);
    REQUIRE(row_sum_q_exposed < 1e-10);
    REQUIRE(row_sum_q_susceptible < 1e-10);

    // =========================================================
    // Diagnostics
    // =========================================================
    std::cout << "\n=== PEP Test Diagnostics ===" << std::endl;

    std::cout << "Effective Rt: " << R0_observed << std::endl;

    std::cout << "Exposed -> Recovered (PEP): "
              << mat(1, 12) << std::endl;

    std::cout << "Prodromal -> Rash: "
              << mat(2, 3) + mat(2, 4) << " (expected ~"
              << 1.0/model_0("Prodromal period") << ")" << std::endl;

    std::cout << "Rash -> Detected: "
              << mat(3, 4) + mat(3, 5) + mat(3, 6)
              << " (expected ~" << 1.0/model_0("Days undetected") << ")"
              << std::endl;

    std::cout << "Hospitalization rate (rash): "
              << mat(3, 6) + mat(3, 11) << " (expected ~"
              << model_0("Hospitalization rate") << ")" << std::endl;

    std::cout << "Hospitalization rate (isolated): "
              << mat(4, 6) + mat(4, 11) << " (expected ~"
              << model_0("Hospitalization rate") << ")" << std::endl;

    std::cout << "Recovery rate (rash): "
              << mat(3, 5) + mat(3, 12) << " (expected ~"
              << p_recovered << ")" << std::endl;

    std::cout << "Recovery rate (isolated): "
              << mat(4, 5) + mat(4, 12) << " (expected ~"
              << p_recovered << ")" << std::endl;

    std::cout << "Hospitalized -> Recovered: "
              << mat(11, 12) << " (expected ~"
              << 1.0/model_0("Hospitalization period") << ")" << std::endl;

    std::cout << "Hosp probability (observed): "
              << obs_hosp_probability
              << " (no-PEP theoretical: " << theoretical_hosp_no_pep << ")"
              << std::endl;

    #undef mat

}
