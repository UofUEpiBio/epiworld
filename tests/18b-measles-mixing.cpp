#include "tests.hpp"
#include "measles-stages.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;


EPIWORLD_TEST_CASE(
    "Measles model with mixing (quarantine transitions)",
    "[ModelMeaslesMixingOn]"
) {
    
    // Queuing doesn't matter and get results that are meaningful
    int n_seeds = 5;
    double n_agents = 9'000.0;
    size_t n_agents_sz = static_cast<size_t>(n_agents);
    
    // Simple contact matrix (single group, all mixing)
    std::vector<double> contact_matrix(9u, 4.0/3.0);
    
    measles::ModelMeaslesMixing<> model_0(
        n_agents_sz,        // Number of agents
        n_seeds / n_agents, // Initial prevalence
        0.2,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        0.2,         // Hospitalization rate
        7.0,         // Hospitalization duration
        2.0,         // Days undetected
        21,          // Quarantine period
        1.0,          // Quarantine willingness
        1.0,          // Isolation willingness
        4,           // Isolation period
        0.0,         // Proportion vaccinated
        1.0,         // Contact tracing success rate
        4u           // Contact tracing days prior
    );

    // Adding a single group
    add_group_mixing(
        model_0, contact_matrix,
        blocks({0u, n_agents_sz/3, 2*n_agents_sz/3, n_agents_sz})
    );


    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    size_t nsims = 200; // Reduced for faster testing
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * n_seeds, -1.0);
    std::vector<std::vector<int>> final_stages(nsims);
    std::vector< double > outbreak_sizes(nsims, 0.0);
    std::vector< double > hospitalizations(nsims, 0.0);
    StatusStageRates rates(nsims);

    auto saver_base = tests_create_saver(
        transitions, R0s, n_seeds, nullptr, &outbreak_sizes, &hospitalizations
    );
    auto saver_rates = rates.saver();
    auto saver_final = final_stage_saver(final_stages);
    auto saver = [&](size_t n, Model<> * m) {
        saver_base(n, m);
        saver_rates(n, m);
        saver_final(n, m);
    };

    model_0.
        run_multiple(200, nsims, 1231, saver, true, true, 4).
        print();

    rates.pool(model_0);

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

    using M = measles::ModelMeaslesMixing<>;

    // Exits from Rash with the drawn rash period (see measles-stages.hpp)
    auto rash = measles_rash_exits(
        model_0("Rash period"),
        model_0("Rash period sd"),
        model_0("Hospitalization rate"),
        1.0/model_0("Days undetected")
    );
    // Effective contact rate is the row sum of the contact matrix
    // (3 groups, each with 4/3 contacts → total 4.0 per agent)
    double effective_contact_rate = 3.0 * contact_matrix[0]; // = 4.0
    double R0_theo = effective_contact_rate * model_0("Transmission rate") *
        model_0("Prodromal period");

    // The stages last their drawn durations, quarantined or not
    double latent_to_prodromal = rates.stage_rate(M::LATENT, M::PRODROMAL);
    double prodromal_to_rash   = rates.stage_rate(M::PRODROMAL, M::RASH);

    // Free agents with rash are detected (and isolated) or hospitalized;
    // contact tracing isolates some more
    double detected = rates.status_rate_in(M::FREE, M::ISOLATED, M::RASH) +
        rates.status_rate_in(M::FREE, M::HOSPITALIZED, M::RASH);
    double detected_theo = 1.0/model_0("Days undetected") +
        (1.0 - 1.0/model_0("Days undetected")) * model_0("Hospitalization rate");

    // Quarantined agents are isolated at rash onset
    double quarantined_isolated =
        rates.status_rate_in(M::QUARANTINED, M::ISOLATED, M::PRODROMAL);

    double hosp_free     = rates.status_rate_in(M::FREE, M::HOSPITALIZED, M::RASH);
    double hosp_isolated = rates.status_rate_in(M::ISOLATED, M::HOSPITALIZED, M::RASH);
    double discharged    = rates.status_rate(M::HOSPITALIZED, M::FREE);

    // Reproductive number
    std::cout <<
        "====================\n" <<
        "In the case of R0, we don't expect to have a big difference\n" <<
        "between quarantine and no quarantine. The biggest difference\n" <<
        "is in the final size of the outbreak.\n" <<
        "====================" <<
        std::endl;

    std::cout << "Reproductive number: "
              << R0_observed << " (expected ~" << R0_theo << ")" << std::endl;
    std::cout << "Latent -> Prodromal: " << latent_to_prodromal
              << " (expected ~" << 1.0/model_0("Incubation period") << ")" << std::endl;
    std::cout << "Prodromal -> Rash: " << prodromal_to_rash
              << " (expected ~" << 1.0/model_0("Prodromal period") << ")" << std::endl;
    std::cout << "Detected or hospitalized (free, rash): " << detected
              << " (expected ~" << detected_theo << ")" << std::endl;
    std::cout << "Quarantined -> Isolated (prodromal): " << quarantined_isolated
              << " (expected ~" << 1.0/model_0("Prodromal period") << ")" << std::endl;
    std::cout << "Hospitalized (free, rash): " << hosp_free
              << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;
    std::cout << "Hospitalized (isolated, rash): " << hosp_isolated
              << " (expected ~" << model_0("Hospitalization rate") << ")" << std::endl;
    std::cout << "Discharged: " << discharged
              << " (expected ~" << 1.0/model_0("Hospitalization period") << ")" << std::endl;
    std::cout << "Hospitalization probability: "
              << rash.p_hospitalized << " (observed ~" << obs_hosp_probability << ")" << std::endl;

    REQUIRE_FALSE(moreless(R0_observed, R0_theo, 0.3));
    REQUIRE_FALSE(moreless(latent_to_prodromal, 1.0/model_0("Incubation period"), 0.1));
    REQUIRE_FALSE(moreless(prodromal_to_rash, 1.0/model_0("Prodromal period"), 0.1));
    REQUIRE_FALSE(moreless(detected, detected_theo, 0.1));
    REQUIRE_FALSE(moreless(quarantined_isolated, 1.0/model_0("Prodromal period"), 0.1));
    REQUIRE_FALSE(moreless(hosp_free, model_0("Hospitalization rate"), 0.1));
    REQUIRE_FALSE(moreless(hosp_isolated, model_0("Hospitalization rate"), 0.1));
    REQUIRE_FALSE(moreless(discharged, 1.0/model_0("Hospitalization period"), 0.1));
    REQUIRE_FALSE(moreless(rash.p_hospitalized, obs_hosp_probability, 0.1));

    // Looking at the final outbreak size
    (void) test_compute_final_sizes(
        final_stages,
        {static_cast< size_t >(M::SUSCEPTIBLE)},
        nsims, true
    );

}
