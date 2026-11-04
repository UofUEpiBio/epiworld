#include "tests.hpp"
#include "measles-stages.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles risk quarantine - transition matrix validation",
    "[ModelMeaslesMixingRiskQuarantine_transitions]"
) {
    
    // Contact matrix for 3 groups with equal mixing
    size_t nsims = 400;
    size_t n     = 300;
    std::vector<double> contact_matrix(9u, 2.0/3.0);
    
    measles::ModelMeaslesMixingRiskQuarantine<> model(
        n,           // Number of agents
        0.1 ,        // Initial prevalence
        0.2,         // Transmission rate
        0.9,         // Vaccination efficacy
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        0.2,         // Hospitalization rate
        7.0,         // Hospitalization duration
        3.0,         // Days undetected
        21,          // Quarantine period high risk
        14,          // Quarantine period medium risk
        7,           // Quarantine period low risk
        .9,          // Quarantine willingness
        .9,          // Isolation willingness
        4,           // Isolation period
        0.0,         // Proportion vaccinated
        0.1,         // Detection rate during quarantine
        1.0,         // Contact tracing success rate
        4u           // Contact tracing days prior
    );

    // Three groups (the last agent of each third is in none)
    add_group_mixing(
        model, contact_matrix,
        ranges(n, {{0u, n/3 - 1}, {n/3, 2*n/3 - 1}, {2*n/3, n - 1}})
    );

    // Moving the virus to the first agent
    model.get_virus(0).set_distribution(
        distribute_virus_randomly<>(10.0, false)
    );

    // Run multiple simulations to get transition matrix
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * 10, -1.0);
    std::vector< double > outbreak_sizes(nsims, 0.0);
    std::vector< double > hospitalizations(nsims, 0.0);

    auto saver = tests_create_saver(transitions, R0s, 10, nullptr, &outbreak_sizes, &hospitalizations);
    model.run_multiple(60, nsims, 123, saver, true, true, 4);

    // Briefly printing the model
    model.print(false);

    // Calculate average transitions
    auto avg_transitions = tests_calculate_avg_transitions(transitions, model);

    tests_print_avg_transitions(avg_transitions, model);
    using MRQ_ = measles::ModelMeaslesMixingRiskQuarantine<>;
    // The state of a status and a stage
    #define S(status, stage) MRQ_::state(MRQ_::status, MRQ_::stage)
    #define mat(i, j) avg_transitions[j * model.get_n_states() + i]

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

    std::cout << "\n=== TRANSITION MATRIX VALIDATION ===" << std::endl;
    
    // From exposed
    std::cout << "Transition from Exposed to Prodromal: " <<
        mat(S(FREE, LATENT), S(FREE, PRODROMAL)) + mat(S(FREE, LATENT), S(QUARANTINED, PRODROMAL)) << " (expected: " << 
        1.0/model("Incubation period") << ")" << std::endl;

    // From prodromal
    std::cout << "Transition from Prodromal to Rash: " <<
        mat(S(FREE, PRODROMAL), S(FREE, RASH)) + mat(S(FREE, PRODROMAL), S(ISOLATED, RASH)) << " (expected: " << 
        1.0/model("Prodromal period") << ")" << std::endl;

    // From Rash
    std::cout << "Transition from Rash to Recovered: " <<
        mat(S(FREE, RASH), S(FREE, RECOVERED)) + mat(S(FREE, RASH), S(ISOLATED, RECOVERED)) << " (expected: " << 
        (1.0 - model("Hospitalization rate") - 1.0/model("Rash period")) << ")" << std::endl;

    std::cout << "Transition from Rash to Hospitalized: " <<
        mat(S(FREE, RASH), S(HOSPITALIZED, RASH)) << " (expected: " <<
        model("Hospitalization rate") << ")" << std::endl;

    // Isolated
    std::cout << "Transition from Isolated to hospitalized: " <<
        mat(S(ISOLATED, RASH), S(HOSPITALIZED, RASH)) << " (expected: " <<
        model("Hospitalization rate") << ")" << std::endl;
    
    // Quarantined Exposed
    std::cout << "Transition from Quarantined Exposed to Prodromal: " <<
        mat(S(QUARANTINED, LATENT), S(FREE, PRODROMAL)) + mat(S(QUARANTINED, LATENT), S(QUARANTINED, PRODROMAL)) << " (expected: " <<
        1.0/model("Incubation period") << ")" << std::endl;

    // Quarantined Prodromal
    std::cout << "Transition from Quarantined Prodromal to Rash: " <<
        mat(S(QUARANTINED, PRODROMAL), S(FREE, RASH)) + mat(S(QUARANTINED, PRODROMAL), S(ISOLATED, RASH)) << " (expected: " <<
        1.0/model("Prodromal period") << ")" << std::endl;

    // From Hospitalized
    std::cout << "Transition from Hospitalized to Recovered: " <<
        mat(S(HOSPITALIZED, RASH), S(FREE, RECOVERED)) << " (expected: " <<
        1.0/model("Hospitalization period") << ")" << std::endl;
        
    // Some transitions should be zero. Building a vector of the transitions
    // we expect not to be zero, so the rest should be zero.
    std::set< std::pair<size_t, size_t> > non_zero_transitions = {
        // From Susceptible
        {S(FREE, SUSCEPTIBLE), S(FREE, SUSCEPTIBLE)}, {S(FREE, SUSCEPTIBLE), S(FREE, LATENT)}, {S(FREE, SUSCEPTIBLE), S(QUARANTINED, LATENT)}, {S(FREE, SUSCEPTIBLE), S(QUARANTINED, SUSCEPTIBLE)},
        // From Latent
        {S(FREE, LATENT), S(FREE, LATENT)}, {S(FREE, LATENT), S(FREE, PRODROMAL)}, {S(FREE, LATENT), S(QUARANTINED, LATENT)}, {S(FREE, LATENT), S(QUARANTINED, PRODROMAL)},
        // From Prodromal
        {S(FREE, PRODROMAL), S(FREE, PRODROMAL)}, {S(FREE, PRODROMAL), S(FREE, RASH)}, {S(FREE, PRODROMAL), S(ISOLATED, RASH)}, {S(FREE, PRODROMAL), S(QUARANTINED, PRODROMAL)},
        // From Rash
        {S(FREE, RASH), S(FREE, RASH)}, {S(FREE, RASH), S(ISOLATED, RASH)}, {S(FREE, RASH), S(ISOLATED, RECOVERED)}, {S(FREE, RASH), S(HOSPITALIZED, RASH)}, {S(FREE, RASH), S(FREE, RECOVERED)},
        // From Isolated
        {S(ISOLATED, RASH), S(ISOLATED, RASH)}, {S(ISOLATED, RASH), S(ISOLATED, RECOVERED)}, {S(ISOLATED, RASH), S(HOSPITALIZED, RASH)}, {S(ISOLATED, RASH), S(FREE, RECOVERED)}, {S(ISOLATED, RASH), S(FREE, RASH)},
        // From Isolated Recovered
        {S(ISOLATED, RECOVERED), S(ISOLATED, RECOVERED)}, {S(ISOLATED, RECOVERED), S(FREE, RECOVERED)},
        // From Quarantined Latent
        {S(QUARANTINED, LATENT), S(QUARANTINED, LATENT)}, {S(QUARANTINED, LATENT), S(FREE, LATENT)}, {S(QUARANTINED, LATENT), S(FREE, PRODROMAL)}, {S(QUARANTINED, LATENT), S(QUARANTINED, PRODROMAL)},
        // From Quarantined Susceptible
        {S(QUARANTINED, SUSCEPTIBLE), S(QUARANTINED, SUSCEPTIBLE)}, {S(QUARANTINED, SUSCEPTIBLE), S(FREE, SUSCEPTIBLE)},
        // From Quarantined Prodromal
        {S(QUARANTINED, PRODROMAL), S(QUARANTINED, PRODROMAL)}, {S(QUARANTINED, PRODROMAL), S(FREE, PRODROMAL)}, {S(QUARANTINED, PRODROMAL), S(FREE, RASH)}, {S(QUARANTINED, PRODROMAL), S(ISOLATED, RASH)},
        // From Quarantined Recovered
        {S(QUARANTINED, RECOVERED), S(QUARANTINED, RECOVERED)}, {S(QUARANTINED, RECOVERED), S(FREE, RECOVERED)},
        // From Hospitalized
        {S(HOSPITALIZED, RASH), S(HOSPITALIZED, RASH)}, {S(HOSPITALIZED, RASH), S(FREE, RECOVERED)},
        // From Recovered
        {S(FREE, RECOVERED), S(FREE, RECOVERED)}
    };

    auto nstates = model.get_n_states();
    size_t n_nonzero_that_should_be_zero = 0u;
    std::set< std::pair<size_t, size_t> > found_nonzero_that_should_be_zero;
    for (size_t i = 0; i < nstates; ++i)
    {
        for (size_t j = 0; j < nstates; ++j)
        {
            // Finding transition
            if (non_zero_transitions.find({i, j}) == non_zero_transitions.end())
            {
                if (mat(i, j) > 1e-10)
                {
                    n_nonzero_that_should_be_zero++;
                    found_nonzero_that_should_be_zero.insert({i, j});
                }
            }
        }
    }

    // Validating zero transitions
    for (const auto & t : found_nonzero_that_should_be_zero)
        std::cout << "Unexpected transition: " << model.get_states()[t.first] <<
            " -> " << model.get_states()[t.second] << ": " <<
            mat(t.first, t.second) << std::endl;
    REQUIRE(n_nonzero_that_should_be_zero == 0u);

    // Validate core disease progression  
    REQUIRE_FALSE(
        moreless(mat(S(FREE, LATENT), S(FREE, PRODROMAL)) + mat(S(FREE, LATENT), S(QUARANTINED, PRODROMAL)), 1.0/model("Incubation period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(S(FREE, PRODROMAL), S(FREE, RASH)) + mat(S(FREE, PRODROMAL), S(ISOLATED, RASH)), 1.0/model("Prodromal period"), 0.1)
    );

    // Exits from Rash with the drawn rash period (see measles-stages.hpp)
    auto rash = measles_rash_exits(
        model("Rash period"),
        model("Rash period sd"),
        model("Hospitalization rate"),
        1.0/model("Days undetected")
    );

    // Agents move between Rash and Isolated (and back when isolation
    // ends), so the recovery rate of either state depends on the isolation
    // policy. 14k and 14l test the rash period; the share hospitalized is
    // checked below.

    REQUIRE_FALSE(
        moreless(mat(S(FREE, RASH), S(HOSPITALIZED, RASH)), model("Hospitalization rate"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(S(ISOLATED, RASH), S(HOSPITALIZED, RASH)), model("Hospitalization rate"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(S(QUARANTINED, LATENT), S(FREE, PRODROMAL)) + mat(S(QUARANTINED, LATENT), S(QUARANTINED, PRODROMAL)), 1.0/model("Incubation period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(S(QUARANTINED, PRODROMAL), S(FREE, RASH)) + mat(S(QUARANTINED, PRODROMAL), S(ISOLATED, RASH)), 1.0/model("Prodromal period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(S(HOSPITALIZED, RASH), S(FREE, RECOVERED)), 1.0/model("Hospitalization period"), 0.1)
    );

    // Hospitalization probability
    REQUIRE_FALSE(
        moreless(
            rash.p_hospitalized,
            obs_hosp_probability,
            0.1
        )
    );

    #undef mat
    #undef S

    // Hospitalization probability
    std::cout << "Hospitalization probability: " <<
        rash.p_hospitalized << " (observed ~" << obs_hosp_probability << ")" << std::endl;

    std::cout << "\nTransition matrix validation completed" << std::endl;


}
