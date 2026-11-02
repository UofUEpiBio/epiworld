#include "tests.hpp"
#include "measles-stages.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

// The latent and rash periods follow their discretized lognormal laws. With
// every agent seeded as latent, no transmission, no detection and a fixed
// prodromal period, the day an agent leaves Latent is its latent period, and
// the day it recovers is latent + prodromal + rash.
EPIWORLD_TEST_CASE(
    "Measles models: lognormal stage durations",
    "[MeaslesStagesLaws]"
) {

    size_t n = 20000u;
    double incubation = 12.0, incubation_sd = 2.5;
    double prodromal = 4.0;
    double rash = 3.0, rash_sd = 1.0;

    measles::ModelMeaslesSchool<> model(
        n,           // Number of agents
        n,           // Number of initial (latent) cases: everyone
        2.0,         // Contact rate
        0.0,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        incubation,  // Incubation period
        prodromal,   // Prodromal period
        rash,        // Rash period
        3.0,         // Days undetected
        0.0,         // Hospitalization rate
        7.0,         // Hospitalization duration
        0.0,         // Proportion vaccinated
        -1,          // Quarantine period (off)
        1.0,         // Quarantine willingness
        -1,          // Isolation period (off)
        incubation_sd,
        0.0,         // Prodromal period sd: fixed
        rash_sd
    );

    model.verbose_off();
    model.run(60, 2231);

    auto latent = transitions_by_day(model, "Latent", "Prodromal");
    auto recovered = transitions_by_day(model, "Rash", "Recovered");

    auto pmf_latent = measles_stage_pmf(incubation, incubation_sd);
    auto pmf_rash   = measles_stage_pmf(rash, rash_sd);

    double max_diff_latent = 0.0, max_diff_recovered = 0.0;
    int total_latent = 0, total_recovered = 0;
    for (int d = 1; d <= 60; ++d)
    {

        // Expected share leaving latent on day d
        double p_latent = pmf_latent[d - 1];

        // Expected share recovering on day d
        double p_recovered = 0.0;
        for (int l = 1; l < d; ++l)
        {
            int r = d - l - static_cast<int>(prodromal);
            if (r >= 1)
                p_recovered += pmf_latent[l - 1] * pmf_rash[r - 1];
        }

        max_diff_latent = std::max(
            max_diff_latent,
            std::abs(latent[d] / static_cast<double>(n) - p_latent)
        );
        max_diff_recovered = std::max(
            max_diff_recovered,
            std::abs(recovered[d] / static_cast<double>(n) - p_recovered)
        );

        total_latent    += latent[d];
        total_recovered += recovered[d];

    }

    #ifdef CATCH_CONFIG_MAIN
    std::cout << "Max difference in daily shares, latent: " << max_diff_latent
        << ", recovered: " << max_diff_recovered << std::endl;
    #endif

    // Everyone goes through both stages within 60 days
    REQUIRE(total_latent == static_cast<int>(n));
    REQUIRE(total_recovered == static_cast<int>(n));

    // Binomial sd of a daily share is below 0.0036 at n = 20000
    REQUIRE(max_diff_latent < 0.015);
    REQUIRE(max_diff_recovered < 0.015);

    // About 0.2% of the agents are latent for more than 21 days (the length
    // of a quarantine); the geometric period of mean 12 days left about 16%
    // latent that long
    int late = 0;
    for (int d = 22; d <= 60; ++d)
        late += latent[d];
    REQUIRE(late < static_cast<int>(n) / 100);

}
