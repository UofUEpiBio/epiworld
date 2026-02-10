#define N_DAYS 120

#include "tests.hpp"

using namespace epiworld;

struct TestResults {
    epimodels::ModelMeaslesMixingRiskQuarantine<> model;
    std::vector<epiworld_double> transitions;
    std::vector<epiworld_double> R0s;
    std::vector< std::vector< int > > final_distribution;
    std::vector< double > sizes;
};

/**
 * @brief Builds a ModelMeaslesMixingRiskQuarantine model for testing
 * @param n Number of agents in the population
 * @param durations Quarantine durations for high, medium, and low risk groups
 * @return Configured ModelMeaslesMixingRiskQuarantine instance
 */
inline TestResults test_model_builder_20d(
    std::string test_name,
    size_t n,
    std::vector< double > contact_matrix,
    std::vector< int > durations = {21, 14, 7},
    size_t nsims = 400
)
{

    double R0       = 10.0;
    double c_rate   = 20.0;
    double p_infect = R0 / (c_rate) * (1.0/4.0);

    epimodels::ModelMeaslesMixingRiskQuarantine<> model(
        n,              // Number of agents
        0.1,            // Initial prevalence
        c_rate,         // Contact rate
        p_infect,       // Transmission rate
        0.95,           // Vaccination efficacy
        7.0,            // Incubation period
        4.0,            // Prodromal period
        5.0,            // Rash period
        contact_matrix, // Contact matrix
        0.2,            // Hospitalization rate
        7.0,            // Hospitalization duration
        2.0,            // Days undetected
        durations[0],   // Quarantine period high risk
        durations[1],   // Quarantine period medium risk (same as high)
        durations[2],   // Quarantine period low risk (same as high)
        1.0,            // Quarantine willingness
        1.0,            // Isolation willingness
        4,              // Isolation period
        0.3,            // Proportion vaccinated
        0.0,            // Detection rate during quarantine
        1.0,            // Contact tracing success rate
        7u              // Contact tracing days prior
    );

    model.add_entity(Entity<>("Population", dist_factory<>(0, n/3)));
    model.add_entity(Entity<>("Population", dist_factory<>(n/3, 2*n/3)));
    model.add_entity(Entity<>("Population", dist_factory<>(2*n/3, n)));

    // Moving the virus to the first agent
    model.get_virus(0).set_distribution(
        distribute_virus_to_set<>({0u})
    );

    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims, -1.0);
    std::vector< std::vector< int > > final_distribution(nsims);

    auto saver = tests_create_saver(
        transitions, R0s, static_cast<int>(1.0),
        &final_distribution
    );

    model.verbose_off();

    model.run_multiple(N_DAYS, nsims, 123, saver, true, false);

    std::cout << test_name << ": " <<
        model("Quarantine period high") << ", " <<
        model("Quarantine period medium") << ", and " <<
        model("Quarantine period low") << " -> ";
    std::vector< double > final_sizes = test_compute_final_sizes(
        final_distribution,
        {
            epimodels::ModelMeaslesMixingRiskQuarantine<>::SUSCEPTIBLE,
            epimodels::ModelMeaslesMixingRiskQuarantine<>::QUARANTINED_SUSCEPTIBLE
        },
        nsims,
        true
    );

    return TestResults{model, transitions[0], R0s, final_distribution, final_sizes};

    // return model;

}

EPIWORLD_TEST_CASE(
    "Measles risk quarantine - different durations comparison",
    "[ModelMeaslesMixingRiskQuarantine_durations]"
) {

    // Contact matrix for 3 groups with equal mixing
    size_t n  = 600;
    
    // More contact within groups
    // According to Toth and others, 83% of contacts are within the same class
    // (for school-aged children). 17% are with other classes.
    double rate_self = 0.83;
    double rate_others = (1.0 - rate_self) / 2.0;
    std::vector<double> contact_matrix = {
        rate_self, rate_others, rate_others,
        rate_others, rate_self, rate_others,
        rate_others, rate_others, rate_self
    };

    /**
     * Prop of outside = .17
     * On average, you see 10 kids.
     * Meaning that of the 10 kids, about 1.7 are not from your class.
     */

    auto model_uniform = test_model_builder_20d("Current ", n, contact_matrix, {21, 21, 21});
    auto model_high    = test_model_builder_20d("High    ", n, contact_matrix, { 0, 21, 21});
    auto model_mid     = test_model_builder_20d("Medium  ", n, contact_matrix, {21,  0, 21});
    auto model_low     = test_model_builder_20d("Low     ", n, contact_matrix, {21, 21,  0});

    std::cout << "## Only one quarantines :" << std::endl;
    auto model_no_quar = test_model_builder_20d("No quarantine", n, contact_matrix, { 0,  0,  0});
    auto model_only_high = test_model_builder_20d("Only high", n, contact_matrix, {21,  0,  0});
    auto model_only_mid = test_model_builder_20d("Only mid", n, contact_matrix, { 0, 21,  0});
    auto model_only_low = test_model_builder_20d("Only low", n, contact_matrix, { 0,  0, 21});
    
    // model_uniform.print();
    // model_varied.print();

    // Should see some difference in outbreak sizes due to different quarantine strategies
    // (This is exploratory - the direction depends on specific parameters)
    REQUIRE(model_no_quar.sizes[0] > model_only_high.sizes[0]);
    REQUIRE(model_no_quar.sizes[0] > model_only_mid.sizes[0]);
    REQUIRE(model_no_quar.sizes[0] > model_only_low.sizes[0]);


}

#undef N_DAYS
#undef N_THREADS