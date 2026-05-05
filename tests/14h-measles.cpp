#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles model: contact rate reduction increases cases",
    "[ModelMeaslesSchoolContactRateReduction]"
) {

    // Setup: unvaccinated population with no quarantine to cleanly test
    // the effect of the Contact Rate Reduction parameter.
    auto make_model = [](double contact_rate_reduction, double hosp_rate) {
        int n_seeds = 1;
        measles::ModelMeaslesSchool<> model(
            500,            // Number of agents
            n_seeds,        // Number of initial cases
            2.0,            // Contact rate
            0.2,            // Transmission rate
            0.9,            // Vaccination efficacy
            0.3,            // Vaccination reduction recovery rate
            7.0,            // Incubation period
            4.0,            // Prodromal period
            5.0,            // Rash period
            3.0,            // Days undetected
            hosp_rate,      // Hospitalization rate
            7.0,            // Hospitalization duration
            0.0,            // Proportion vaccinated (none, for clean R0 test)
            -1,             // Quarantine period (-1 = off)
            0.0,            // Quarantine willingness
            -1,             // Isolation period (-1 = off)
            contact_rate_reduction
        );

        // Fix the index case to agent 0
        model.get_virus(0).set_distribution(
            [n_seeds](Virus<> & v, Model<> * m) -> void {
                for (int i = 0; i < n_seeds; ++i)
                    m->get_agents()[i].set_virus(*m, v);
            }
        );

        return model;
    };

    // ------------------------------------------------------------------
    // Scenario A: contact_rate_reduction = 1.0 (rash agents NOT infectious)
    // ------------------------------------------------------------------
    auto model_reduction_1 = make_model(1.0, 0.04411765);

    size_t nsims = 500u;
    std::vector<double> outbreak_sizes_1(nsims, 0.0);

    auto saver_1 = [&](size_t n, epiworld::Model<>* m) -> void {
        std::vector<int> date, virus, outbreak;
        m->get_db().get_outbreak_size(date, virus, outbreak);
        outbreak_sizes_1[n] = static_cast<double>(outbreak.back());
    };

    model_reduction_1.run_multiple(200, nsims, 1234, saver_1, true, true, 4);

    // ------------------------------------------------------------------
    // Scenario B: contact_rate_reduction = 0.5 (rash at 50% contact rate)
    // ------------------------------------------------------------------
    auto model_reduction_05 = make_model(0.5, 0.04411765);

    std::vector<double> outbreak_sizes_05(nsims, 0.0);

    auto saver_05 = [&](size_t n, epiworld::Model<>* m) -> void {
        std::vector<int> date, virus, outbreak;
        m->get_db().get_outbreak_size(date, virus, outbreak);
        outbreak_sizes_05[n] = static_cast<double>(outbreak.back());
    };

    model_reduction_05.run_multiple(200, nsims, 1234, saver_05, true, true, 4);

    // ------------------------------------------------------------------
    // Compare: mean outbreak size with reduction=0.5 > reduction=1.0
    // ------------------------------------------------------------------
    double mean_1 = std::accumulate(
        outbreak_sizes_1.begin(), outbreak_sizes_1.end(), 0.0
    ) / static_cast<double>(nsims);

    double mean_05 = std::accumulate(
        outbreak_sizes_05.begin(), outbreak_sizes_05.end(), 0.0
    ) / static_cast<double>(nsims);

    std::cout << "Mean outbreak size (reduction=1.0): " << mean_1  << std::endl;
    std::cout << "Mean outbreak size (reduction=0.5): " << mean_05 << std::endl;

    // With 50% contact rate for rash agents, the expected R0 is higher,
    // so the mean outbreak size should be strictly larger.
    REQUIRE(mean_05 > mean_1);

    // ------------------------------------------------------------------
    // Validate R0 with contact_rate_reduction = 0 (rash fully infectious)
    // and hospitalization_rate = 0 (so the simple formula holds exactly):
    //   R0 = p_infect * contact_rate * (prodromal_period + rash_period)
    // ------------------------------------------------------------------
    auto model_reduction_0 = make_model(0.0, 0.0);

    std::vector<std::vector<epiworld_double>> transitions_0(nsims);
    std::vector<epiworld_double> R0s_0(nsims, -1.0);

    int n_seeds = 1;
    auto saver_r0 = tests_create_saver(transitions_0, R0s_0, n_seeds);

    model_reduction_0.run_multiple(200, nsims, 1234, saver_r0, true, true, 4);

    double R0_observed_0 = 0.0;
    for (auto & r : R0s_0)
        R0_observed_0 += r;
    R0_observed_0 /= static_cast<double>(nsims);

    double R0_theo_0 = model_reduction_0("Transmission rate") *
        model_reduction_0("Contact rate") *
        (model_reduction_0("Prodromal period") + model_reduction_0("Rash period"));

    std::cout << "R0 (reduction=0.0, hosp=0): observed=" << R0_observed_0
              << ", expected=" << R0_theo_0 << std::endl;

    REQUIRE_FALSE(moreless(R0_observed_0, R0_theo_0, 0.4));

}
