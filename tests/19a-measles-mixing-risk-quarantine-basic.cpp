#define N_THREADS 4

#include "tests.hpp"
#include "measles-stages.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

using MSQ = measles::ModelMeaslesMixingRiskQuarantine;

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
    std::vector<double> contact_matrix = {c_rate};
    
    measles::ModelMeaslesMixingRiskQuarantine model(
        n,           // Number of agents
        0.005,       // Initial prevalence
        p_infect,    // Transmission rate
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

    // Everyone in a single group
    add_group_mixing(model, contact_matrix, blocks({0u, n}));

    // Virus will be distributed to 5 random agents, in each
    // replicate of the model
    model.assign_virus(0, assign::count(5));
    
    std::vector< std::vector< epiworld_double > > transitions(nsims);
    std::vector< epiworld_double > R0s(nsims * 5, -1.0);
    std::vector< double > outbreak_sizes(nsims, 0.0);
    std::vector< double > hospitalizations(nsims, 0.0);

    auto saver = tests_create_saver(transitions, R0s, 5, nullptr, &outbreak_sizes, &hospitalizations);

    // Run simulation
    // Long enough for the outbreaks to end, so the share of cases hospitalized
    // is not cut short
    model.run_multiple(200, nsims, 123, saver, true, true, N_THREADS);
    model.print();

    auto tmat = tests_calculate_avg_transitions(transitions, model);
    tests_print_avg_transitions(tmat, model);

    // Averaging R0
    auto avg_R0 = stats::mean(R0s);

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

    // The engine's states are the (stage, status) pairs, state = stage * 4 +
    // status. Without quarantine and isolation, agents are free, except the
    // severe ones, who are hospitalized
    #define mat(i, j) tmat[(j)*model.get_n_states() + (i)]
    constexpr int NS = 4;
    constexpr int L_FREE   = MSQ::LATENT * NS + MSQ::FREE;
    constexpr int P_FREE   = MSQ::PRODROMAL * NS + MSQ::FREE;
    constexpr int R_FREE   = MSQ::RASH * NS + MSQ::FREE;
    constexpr int SEV_HOSP = MSQ::SEVERE * NS + MSQ::HOSPITALIZED;
    constexpr int REC_FREE = MSQ::RECOVERED * NS + MSQ::FREE;
    std::cout << "Average R0 from index cases: " <<
        avg_R0 <<  " vs expected " << R0 << std::endl;

    // Looking into the transition matrix -----------------------

    // From Exposed
    std::cout << "Transition from Exposed to Prodromal: " <<
        mat(L_FREE, P_FREE) << " (expected: " <<
        1.0/model("Incubation period") << ")" << std::endl;

    // From Prodromal
    std::cout << "Transition from Prodromal to Rash: " <<
        mat(P_FREE, R_FREE) << " (expected: " <<
        1.0/model("Prodromal period") << ")" << std::endl;

    // From Rash
    std::cout << "Transition from Rash to Recovery: " <<
        mat(R_FREE, REC_FREE) << " (expected: " <<
        1.0/model("Rash period") << ")" <<
        std::endl;

    std::cout << "Transition from Rash to Hospitalized: " <<
        mat(R_FREE, SEV_HOSP) << " (expected: " <<
        model("Hospitalization rate") << ")" << std::endl;

    // From hospitalized
    std::cout << "Transition from Hospitalized to Recovery: " <<
        mat(SEV_HOSP, REC_FREE) << " (expected: " <<
        1.0/model("Hospitalization period") << ")" << std::endl;

    // Hospitalization probability
    // Exits from Rash with the drawn rash period (see measles-stages.hpp)
    auto rash = measles_rash_exits(
        model("Rash period"),
        model("Rash period sd"),
        model("Hospitalization rate"),
        0.0 // Isolation is off, so agents with rash are never detected
    );
    double p_recovered = rash.recovery_rate;
    std::cout << "Hospitalization probability: " <<
        rash.p_hospitalized << " (observed ~" << obs_hosp_probability << ")" << std::endl;

    // Validating some transitions -------------------------------
    REQUIRE_FALSE(
        moreless(mat(L_FREE, P_FREE), 1.0/model("Incubation period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(P_FREE, R_FREE), 1.0/model("Prodromal period"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(
            mat(R_FREE, REC_FREE),
            p_recovered,
            0.1
        )
    );

    REQUIRE_FALSE(
        moreless(mat(R_FREE, SEV_HOSP), model("Hospitalization rate"), 0.1)
    );

    REQUIRE_FALSE(
        moreless(mat(SEV_HOSP, REC_FREE), 1.0/model("Hospitalization period"), 0.1)
    );

    // Hospitalization probability
    REQUIRE_FALSE(
        moreless(
            rash.p_hospitalized,
            obs_hosp_probability,
            0.1
        )
    );


}

#undef mat
#undef N_THREADS
