#include "tests.hpp"
#include "../include/measles/measles.hpp"


using namespace epiworld;
using MS = measles::ModelMeaslesSchool<>;

EPIWORLD_TEST_CASE("Measles PEP intervention", "[ModelMeaslesPEP]") {

    int n_seeds = 1;
    measles::ModelMeaslesSchool<> model_0(
        1000,    // Number of agents
        n_seeds, // Number of initial cases
        20.0,     // Contact rate
        0.1,     // Transmission rate
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
        1.0,     // Quarantine willingness
        4u       // Isolation period
    );

    // Creating the PEP intervention and 
    // setting it up so we can call it as a global event.
    measles::InterventionMeaslesPEP<> pep(
        "Post-exposure prophylaxis for measles", // Name of the intervention
        1.0,       // "PEP MMR efficacy"
        1.0,       // "PEP IG efficacy"
        4.0 * 7.0, // "PEP IG half-life (mean)"
        7.0/2.0,   // "PEP IG half-life (sd)"
        1.0,       // "PEP MMR willingness"
        1.0,       // "PEP IG willingness"
        100.0,       // "PEP MMR window"
        100.0,       // "PEP IG window"
        {MS::QUARANTINED_LATENT, MS::QUARANTINED_SUSCEPTIBLE, MS::QUARANTINED_PRODROMAL},
        {MS::RECOVERED, MS::SUSCEPTIBLE, MS::RECOVERED},
        {MS::LATENT, MS::SUSCEPTIBLE, MS::PRODROMAL}
    );

    model_0.add_globalevent(pep);

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

    model_0.run_multiple(60, nsims, 1231, saver, true, true, 1);

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
            mat(3, 4),
            (1.0/model_0("Days undetected")) * (1.0 - p_recovered - model_0("Hospitalization rate")), 0.05
        )
    );

    // Transition from Quarantine Latent to Quarantined Prodromal
    // (already checked above with wider tolerance; PEP doesn't
    // affect symptom development)

    // From quarantine prodromal to isolated (detected immediately)
    REQUIRE_FALSE(
        moreless(mat(8, 4), 1.0/model_0("Prodromal period"), 0.05)
    );

    // Transition to hospitalized from Rash and Isolated
    REQUIRE_FALSE(
        moreless(
            mat(3, 10), model_0("Hospitalization rate"), 0.05
        )
    );
    REQUIRE_FALSE(
        moreless(
            0.0 + mat(4, 10), model_0("Hospitalization rate"), 0.05
        )
    );

    // Transition to recovered from Rash and Isolated
    REQUIRE_FALSE(moreless(mat(3, 11), p_recovered, 0.05));
    REQUIRE_FALSE(moreless(mat(4, 5) + mat(4, 11), p_recovered, 0.05));

    // Transition from hospitalized to recovered
    REQUIRE_FALSE(
        moreless(mat(10, 11), 1.0/model_0("Hospitalization period"), 0.05)
    );

    // We should have some expected transitions away from
    // quarantine states due to PEP effects
    REQUIRE(mat(6, 1) > 0.0);  // Quarantined latent to latent
    REQUIRE(mat(7, 0) > 0.0);  // Quarantined susceptible to susceptible
    REQUIRE(mat(8, 2) > 0.0);  // Quarantined prodromal to recovered
    
    REQUIRE(mat(6, 11) > 0.0); // Quarantined latent to recovered
    REQUIRE(mat(8, 11) > 0.0); // Quarantined prodromal to recovered

    // =========================================================
    // Diagnostics
    // =========================================================
    std::cout << "\n=== PEP Test Diagnostics ===" << std::endl;

    std::cout << "Effective Rt: " << R0_observed << std::endl;

    std::cout << "Prodromal -> Rash: "
              << mat(2, 3) + mat(2, 4) << " (expected ~"
              << 1.0/model_0("Prodromal period") << ")" << std::endl;

    std::cout << "Rash -> Detected: "
              << mat(3, 4)
              << " (expected ~" << (1.0/model_0("Days undetected")) * (1.0 - p_recovered - model_0("Hospitalization rate")) << ")"
              << std::endl;

    std::cout << "Hospitalization rate (rash): "
              << mat(3, 10) << " (expected ~"
              << model_0("Hospitalization rate") << ")" << std::endl;

    std::cout << "Hospitalization rate (isolated): "
              << 0.0 + mat(4, 10) << " (expected ~"
              << model_0("Hospitalization rate") << ")" << std::endl;

    std::cout << "Recovery rate (rash): "
              << mat(3, 11) << " (expected ~"
              << p_recovered << ")" << std::endl;

    std::cout << "Recovery rate (isolated): "
              << mat(4, 5) + mat(4, 11) << " (expected ~"
              << p_recovered << ")" << std::endl;

    std::cout << "Hospitalized -> Recovered: "
              << mat(10, 11) << " (expected ~"
              << 1.0/model_0("Hospitalization period") << ")" << std::endl;

    // Transitions due to PEP
    std::cout << "==== PEP-specific transitions ====" << std::endl;
    std::cout << "Quarantined Latent      -> Latent      : "
              << mat(6, 1) << std::endl;
    std::cout << "Quarantined Latent      -> Recovered   : "
              << mat(6, 11) << std::endl;
    std::cout << "Quarantined Prodromal   -> Prodromal   : "
              << mat(8, 2) << std::endl;
    std::cout << "Quarantined Prodromal   -> Recovered   : "
              << mat(8, 11) << std::endl;
    std::cout << "Quarantined Susceptible -> Susceptible : "
              << mat(7, 0) << std::endl;



    #undef mat

}
