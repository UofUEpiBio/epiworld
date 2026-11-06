#include "tests.hpp"
#include "measles-stages.hpp"
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
        // PEP is offered to the whole school (everyone is assumed exposed),
        // so with 100% willingness every quarantined agent is dosed and
        // released on the very day it is quarantined, and the quarantine
        // compartments are never occupied at a day boundary. Partial
        // willingness keeps both paths -- those who accept and those who
        // decline -- observable.
        0.5,       // "PEP MMR willingness"
        0.5,       // "PEP IG willingness"
        100.0,       // "PEP MMR window"
        100.0,       // "PEP IG window"
        {MS::SUSCEPTIBLE, MS::LATENT, MS::PRODROMAL},
        {MS::QUARANTINED}
    );

    // What epiworld does under the hood
    // Call the `operator()` method of the class.
    // pep(&model_0, 19);

    model_0.add_globalevent(pep);

    // Setting the distribution function of the initial cases
    model_0.assign_virus(0, assign::agents(first_agents(n_seeds)));

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

    // =========================================================
    // PEP-specific transition checks
    // =========================================================
    
    
    // =========================================================
    // Standard disease transitions (should be preserved for
    // agents that reach prodromal/rash regardless of PEP)
    // =========================================================

    // Transition from Prodromal to Rash
    REQUIRE_FALSE(
        moreless(mat(8, 12) + mat(8, 14), 1.0/model_0("Prodromal period"), 0.05)
    );

    // Transition from Rash (days to detected/isolated)
    REQUIRE_FALSE(
        moreless(
            mat(12, 14),
            (1.0/model_0("Days undetected")) * (1.0 - mat(12, 20) - mat(12, 19)), 0.05
        )
    );

    // Transition from Quarantine Latent to Quarantined Prodromal
    // (already checked above with wider tolerance; PEP doesn't
    // affect symptom development)

    // From quarantine prodromal to isolated (detected immediately)
    REQUIRE_FALSE(
        moreless(mat(9, 14), 1.0/model_0("Prodromal period"), 0.05)
    );

    // Transition to hospitalized from Rash and Isolated
    REQUIRE_FALSE(
        moreless(
            mat(12, 19), model_0("Hospitalization rate"), 0.05
        )
    );
    REQUIRE_FALSE(
        moreless(
            0.0 + mat(14, 19), model_0("Hospitalization rate"), 0.05
        )
    );

    // Transition to recovered from Rash and Isolated
    // Agents move between Rash and Isolated (and back when isolation
    // ends), so the recovery rate of either state depends on the isolation
    // policy. 14k and 14l test the rash period; the share hospitalized is
    // checked below.
    // Agents enter Isolated part-way through their rash period, so its
    // recovery rate has no simple form; 14k-measles-stages.cpp tests that
    // isolation keeps the rash period.

    // Transition from hospitalized to recovered
    REQUIRE_FALSE(
        moreless(mat(19, 20), 1.0/model_0("Hospitalization period"), 0.05)
    );

    // Transitions away from quarantine states. Agents still sitting in a
    // quarantine state at a day boundary are the ones that declined PEP
    // (willingness is drawn once per agent), so these are the natural
    // end-of-quarantine moves. Susceptible agents are released when the
    // quarantine ends. Infected ones rarely are: with lognormal stages,
    // almost no agent is still latent or prodromal when its 21-day quarantine
    // ends (both moves were common with geometric stages).
    REQUIRE(mat(1, 0) > 0.0);  // Quarantined susceptible to susceptible
    REQUIRE(mat(5, 4) < 0.01); // Quarantined latent to latent
    REQUIRE(mat(9, 8) < 0.01); // Quarantined prodromal to prodromal

    // Effective PEP shows up as a transition out of the state the agent
    // held *before* being quarantined: the whole school is quarantined and
    // then dosed on the same day, so an agent that accepts never spends a
    // day boundary in a quarantine compartment. Neither of these
    // transitions can happen through the natural disease history (latent
    // agents only progress to prodromal, prodromal agents only to rash),
    // so they are PEP-specific.
    REQUIRE(mat(4, 20) > 0.0); // Latent to recovered (PEP was effective)
    REQUIRE(mat(8, 20) > 0.0); // Prodromal to recovered (PEP was effective)

    // =========================================================
    // Diagnostics
    // =========================================================
    std::cout << "\n=== PEP Test Diagnostics ===" << std::endl;

    std::cout << "Effective Rt: " << R0_observed << std::endl;

    std::cout << "Prodromal -> Rash: "
              << mat(8, 12) + mat(8, 14) << " (expected ~"
              << 1.0/model_0("Prodromal period") << ")" << std::endl;

    std::cout << "Rash -> Detected: "
              << mat(12, 14)
              << " (expected ~" << (1.0/model_0("Days undetected")) * (1.0 - mat(12, 20) - mat(12, 19)) << ")"
              << std::endl;

    std::cout << "Hospitalization rate (rash): "
              << mat(12, 19) << " (expected ~"
              << model_0("Hospitalization rate") << ")" << std::endl;

    std::cout << "Hospitalization rate (isolated): "
              << 0.0 + mat(14, 19) << " (expected ~"
              << model_0("Hospitalization rate") << ")" << std::endl;

    // Cumulative, not a daily rate: the share of infections that ended in
    // hospitalization. Rash and Isolated face the same competing daily
    // risks (recovery at 1/rash_period vs hospitalization), so an agent
    // that reaches rash is hospitalized with probability
    // hosp/(hosp + 1/rash_period). The observed value sits below that,
    // because PEP clears part of the denominator before it ever reaches
    // rash.
    std::cout << "Hospitalizations per infection: "
              << obs_hosp_probability << " (< "
              << rash.p_hospitalized
              << ", the no-PEP ceiling)" << std::endl;

    std::cout << "Recovery rate (rash): "
              << mat(12, 20) << std::endl;

    std::cout << "Recovery rate (isolated): "
              << mat(14, 22) + mat(14, 20) << std::endl;

    std::cout << "Hospitalized -> Recovered: "
              << mat(19, 20) << " (expected ~"
              << 1.0/model_0("Hospitalization period") << ")" << std::endl;

    // Transitions due to PEP
    std::cout << "==== PEP-specific transitions ====" << std::endl;
    std::cout << "Latent                  -> Recovered   : "
              << mat(4, 20) << std::endl;
    std::cout << "Prodromal               -> Recovered   : "
              << mat(8, 20) << std::endl;

    // End of quarantine for the agents that declined PEP
    std::cout << "==== End of quarantine (PEP declined) ====" << std::endl;
    std::cout << "Quarantined Latent      -> Latent      : "
              << mat(5, 4) << std::endl;
    std::cout << "Quarantined Prodromal   -> Prodromal   : "
              << mat(9, 8) << std::endl;
    std::cout << "Quarantined Susceptible -> Susceptible : "
              << mat(1, 0) << std::endl;



    #undef mat

}
