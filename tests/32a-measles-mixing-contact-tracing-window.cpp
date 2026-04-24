#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

/**
 * @brief Validate the contact tracing window filter in ModelMeaslesMixing.
 *
 * The window check uses:
 *   std::abs(day_rash_onset - contact_date) <= window
 *
 * This test verifies that a larger contact tracing window leads to more
 * effective quarantine and therefore a smaller outbreak compared to a
 * narrower window, validating the symmetric absolute-value comparison.
 *
 * With the buggy code (abs applied to bool instead of the difference),
 * the comparison was equivalent to a one-sided check (diff <= window),
 * meaning any contact after rash onset would always pass. The fix ensures
 * both sides of the window are correctly filtered.
 */
EPIWORLD_TEST_CASE(
    "Contact tracing window correctly filters quarantine contacts",
    "[ModelMeaslesMixing-contact-tracing-window]"
) {

    size_t n_agents = 3'000u;
    int    n_seeds  = 3;
    size_t nsims    = 200u;

    // Single fully-mixing group
    std::vector<double> contact_matrix = {8.0};

    auto make_model = [&](epiworld_fast_uint window) {
        measles::ModelMeaslesMixing<> m(
            n_agents,
            static_cast<double>(n_seeds) / static_cast<double>(n_agents),
            0.15,    // transmission_rate
            0.0,     // vax_efficacy (no vaccine)
            0.0,     // vax_reduction_recovery_rate
            7.0,     // incubation_period
            4.0,     // prodromal_period (contacts occur over ~4 days)
            5.0,     // rash_period
            contact_matrix,
            0.0,     // hospitalization_rate (disabled)
            7.0,     // hospitalization_period
            2.0,     // days_undetected
            21,      // quarantine_period
            1.0,     // quarantine_willingness  (all willing)
            1.0,     // isolation_willingness   (all willing)
            7,       // isolation_period
            0.0,     // prop_vaccinated
            1.0,     // contact_tracing_success_rate (100%)
            window   // contact_tracing_days_window  ← key parameter
        );

        m.add_entity(Entity<>("Population", dist_factory<>(0, n_agents)));

        m.get_virus(0).set_distribution(
            [n_seeds](Virus<> & v, Model<> * mdl) -> void {
                for (int i = 0; i < n_seeds; ++i)
                    mdl->get_agents()[i].set_virus(*mdl, v);
            }
        );

        return m;
    };

    // States that represent individuals who were never infected
    std::vector<size_t> not_infected_states = {
        measles::ModelMeaslesMixing<>::SUSCEPTIBLE,
        measles::ModelMeaslesMixing<>::QUARANTINED_SUSCEPTIBLE
    };

    // -------------------------------------------------------------------
    // Scenario A: window = 1 (very narrow — only traces same-day and
    // 1-day-before contacts)
    // -------------------------------------------------------------------
    auto model_narrow = make_model(1u);
    std::vector<std::vector<int>> dist_narrow(nsims);

    auto saver_narrow = [&dist_narrow](size_t n, Model<> * m) {
        m->get_db().get_today_total(nullptr, &dist_narrow[n]);
    };

    model_narrow.run_multiple(80, nsims, 1234, saver_narrow, true, true, 1);

    // -------------------------------------------------------------------
    // Scenario B: window = 20 (covers entire prodromal period many times
    // over — effectively traces all contacts)
    // -------------------------------------------------------------------
    auto model_wide = make_model(20u);
    std::vector<std::vector<int>> dist_wide(nsims);

    auto saver_wide = [&dist_wide](size_t n, Model<> * m) {
        m->get_db().get_today_total(nullptr, &dist_wide[n]);
    };

    model_wide.run_multiple(80, nsims, 1234, saver_wide, true, true, 1);

    // -------------------------------------------------------------------
    // Compare outbreak sizes.
    // With a wider window, more contacts are traced and quarantined, so
    // the outbreak should be smaller (fewer ever-infected agents).
    // -------------------------------------------------------------------
    const std::vector<double> thresholds = {100.0, 500.0, 1000.0};

    auto p_narrow = test_compute_prob_outbreak_gt_k(
        dist_narrow, not_infected_states, thresholds, nsims, 0u, false
    );
    auto p_wide = test_compute_prob_outbreak_gt_k(
        dist_wide, not_infected_states, thresholds, nsims, 0u, false
    );

    double sum_narrow = std::accumulate(p_narrow.begin(), p_narrow.end(), 0.0);
    double sum_wide   = std::accumulate(p_wide.begin(),   p_wide.end(),   0.0);

    std::cout << "========================================================\n";
    std::cout << "Contact tracing window test (ModelMeaslesMixing)\n";
    std::cout << "  window=1  : P(outbreak > threshold) sum = " << sum_narrow << "\n";
    std::cout << "  window=20 : P(outbreak > threshold) sum = " << sum_wide   << "\n";
    std::cout << "========================================================\n";

    // A wider window traces more contacts → more effective quarantine
    // → smaller outbreaks → lower exceedance probabilities.
    REQUIRE(sum_narrow > sum_wide);

}

