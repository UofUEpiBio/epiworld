#include "tests.hpp"
#include "../include/varicella/varicella.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Varicella model (no quarantine)",
    "[ModelVaricellaSchoolOff]"
) {

    // Queuing doesn't matter and get results that are meaningful
    int n_seeds = 1;
    varicella::ModelVaricellaSchool<> model(
        500,    // Number of agents
        n_seeds, // Number of initial cases
        2.0,     // Contact rate
        0.2,     // Transmission rate
        0.9,     // Vaccination efficacy
        0.5,     // Vax improved recovery (50 % shorter duration)
        14.0,    // Incubation period
        2.0,     // Prodromal period
        6.0,     // Rash period
        3.0,     // Days undetected
        0.005,   // Hospitalization rate (unvaccinated)
        0.001,   // Hospitalization rate (vaccinated)
        7.0,     // Hospitalization duration
        0.0,     // Proportion vaccinated
        -1,      // Quarantine period (disabled)
        0.0,     // Quarantine willingness
        -1       // Isolation period (disabled)
    );

    // Fix seed placement so the test is fully deterministic
    model.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    size_t nsims = 500;
    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * n_seeds, -1.0);
    std::vector< double > outbreak_sizes(nsims, 0.0);
    std::vector< double > hospitalizations(nsims, 0.0);

    auto saver = tests_create_saver(transitions, R0s, n_seeds, nullptr,
                                    &outbreak_sizes, &hospitalizations);

    model.run_multiple(200, nsims, 42, saver, true, true, 4);

    model.print(false);

    auto avg_transitions = tests_calculate_avg_transitions(transitions, model);
    tests_print_avg_transitions(avg_transitions, model);

    size_t n_states = model.get_n_states();
    #define mat(i, j) avg_transitions[j*n_states + i]

    double p_recovered = 1.0/model("Rash period");
    double R0_theo = model("Contact rate") * model("Transmission rate") *
        model("Prodromal period");

    // Average R0 should be close to the theoretical value
    double R0_observed = 0.0;
    for (auto & r: R0s)
    {
        if (r >= 0.0)
            R0_observed += r;
        else
            throw std::range_error("The R0 value is negative.");
    }
    R0_observed /= static_cast<epiworld_double>(nsims * n_seeds);

    REQUIRE_FALSE(moreless(R0_observed, R0_theo, 0.25));

    // Transition from Latent to Prodromal
    REQUIRE_FALSE(
        moreless(mat(1, 2), 1.0/model("Incubation period"), 0.05)
    );

    // Transition from Prodromal to Rash (no quarantine, no vaccination,
    // so all prodromal agents should use the baseline prodromal period)
    REQUIRE_FALSE(
        moreless(mat(2, 3), 1.0/model("Prodromal period"), 0.05)
    );

    // Transition from Rash to Recovered
    REQUIRE_FALSE(
        moreless(mat(3, 11), p_recovered, 0.05)
    );

    // Transition from Hospitalized to Recovered
    REQUIRE_FALSE(
        moreless(mat(10, 11), 1.0/model("Hospitalization period"), 0.05)
    );

    // Hospitalization probability from rash state
    double hosp_rate = model("Hospitalization rate (unvaccinated)");
    REQUIRE_FALSE(
        moreless(
            hosp_rate / (hosp_rate + p_recovered),
            std::accumulate(hospitalizations.begin(), hospitalizations.end(), 0.0) /
                std::accumulate(outbreak_sizes.begin(), outbreak_sizes.end(), 0.0),
            0.05
        )
    );

    std::cout << "R0 (observed): " << R0_observed
              << " (expected ~" << R0_theo << ")" << std::endl;
    std::cout << "Transition Latent→Prodromal: " << mat(1, 2)
              << " (expected ~" << 1.0/model("Incubation period") << ")" << std::endl;
    std::cout << "Transition Prodromal→Rash: " << mat(2, 3)
              << " (expected ~" << 1.0/model("Prodromal period") << ")" << std::endl;
    std::cout << "Transition Rash→Recovered: " << mat(3, 11)
              << " (expected ~" << p_recovered << ")" << std::endl;

    #undef mat

}
