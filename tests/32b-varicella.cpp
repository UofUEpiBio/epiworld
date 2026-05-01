#include "tests.hpp"
#include "../include/varicella/varicella.hpp"

using namespace epiworld;

/**
 * @brief Verify that vaccination shortens disease duration and lowers
 * hospitalization risk.
 *
 * We run two scenarios:
 *  - Unvaccinated population (prop_vaccinated = 0).
 *  - Fully vaccinated population (prop_vaccinated = 1, efficacy = 0),
 *    so everyone has the MMRV tool but nobody is immunologically protected
 *    (all are susceptible).  This isolates the disease-severity effect of
 *    vaccination from the infection-prevention effect.
 *
 * In the vaccinated scenario the effective prodromal and rash periods are
 *   period * (1 - vax_improved_recovery)
 * so the transition rates from those states should be faster, and the
 * hospitalization probability should be lower.
 */
EPIWORLD_TEST_CASE(
    "Varicella vaccination shortens duration and lowers hospitalization",
    "[ModelVaricellaSchoolVax]"
) {

    // Shared parameters
    const int    n_agents   = 500;
    const int    n_seeds    = 1;
    const double contact_rate = 20.0;   // High contact to ensure outbreaks
    const double transmission_rate = 1.0;
    const double vax_improved_recovery = 0.5; // 50 % shorter duration
    const double incubation_period  = 14.0;
    const double prodromal_period   = 2.0;
    const double rash_period        = 6.0;
    const double days_undetected    = 3.0;
    const double hosp_rate_unvax    = 0.05; // Use higher rates to accumulate signal
    const double hosp_rate_vax      = 0.01;
    const double hosp_period        = 7.0;
    const size_t nsims              = 500;

    // -----------------------------------------------------------------
    // Scenario A: unvaccinated
    // -----------------------------------------------------------------
    varicella::ModelVaricellaSchool<> model_unvax(
        n_agents,
        n_seeds,
        contact_rate,
        transmission_rate,
        0.0,   // vax_efficacy: irrelevant (prop_vaccinated = 0)
        vax_improved_recovery,
        incubation_period,
        prodromal_period,
        rash_period,
        days_undetected,
        hosp_rate_unvax,
        hosp_rate_vax,
        hosp_period,
        0.0,   // prop_vaccinated = 0
        -1,    // quarantine disabled
        0.0,
        -1     // isolation disabled
    );
    model_unvax.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    std::vector<std::vector<epiworld_double>> trans_unvax(nsims);
    std::vector<epiworld_double> R0s_unvax(nsims * n_seeds, -1.0);
    model_unvax.run_multiple(200, nsims, 123, 
        tests_create_saver(trans_unvax, R0s_unvax, n_seeds),
        true, true, 4);

    auto avg_unvax = tests_calculate_avg_transitions(trans_unvax, model_unvax);

    // -----------------------------------------------------------------
    // Scenario B: vaccinated (efficacy = 0 so everyone gets infected,
    //             but everyone carries the MMRV tool)
    // -----------------------------------------------------------------
    varicella::ModelVaricellaSchool<> model_vax(
        n_agents,
        n_seeds,
        contact_rate,
        transmission_rate,
        0.0,   // vax_efficacy = 0: tool present but no immunity
        vax_improved_recovery,
        incubation_period,
        prodromal_period,
        rash_period,
        days_undetected,
        hosp_rate_unvax,
        hosp_rate_vax,
        hosp_period,
        1.0,   // prop_vaccinated = 1
        -1,    // quarantine disabled
        0.0,
        -1     // isolation disabled
    );
    model_vax.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    std::vector<std::vector<epiworld_double>> trans_vax(nsims);
    std::vector<epiworld_double> R0s_vax(nsims * n_seeds, -1.0);
    std::vector<double> outbreak_sizes_vax(nsims, 0.0);
    std::vector<double> hospitalizations_vax(nsims, 0.0);

    model_vax.run_multiple(200, nsims, 456,
        tests_create_saver(trans_vax, R0s_vax, n_seeds,
                           nullptr, &outbreak_sizes_vax, &hospitalizations_vax),
        true, true, 4);

    auto avg_vax = tests_calculate_avg_transitions(trans_vax, model_vax);

    // -----------------------------------------------------------------
    // Assertions
    // -----------------------------------------------------------------
    size_t n_states = model_unvax.get_n_states();
    #define mat_u(i, j) avg_unvax[j*n_states + i]
    #define mat_v(i, j) avg_vax[j*n_states + i]

    // States: 0=S 1=Latent 2=Prodromal 3=Rash 4=Isolated 5=IsolatedRec
    //         6=QLat 7=QSusc 8=QProd 9=QRec 10=Hosp 11=Rec

    // 1. Vaccinated breakthrough cases should transition faster from
    //    Prodromal → Rash (higher daily probability because shorter period)
    double rate_prod_unvax = mat_u(2, 3);
    double rate_prod_vax   = mat_v(2, 3);

    double expected_rate_unvax = 1.0 / prodromal_period;
    double expected_rate_vax   = 1.0 / (prodromal_period * (1.0 - vax_improved_recovery));

    REQUIRE_FALSE(moreless(rate_prod_unvax, expected_rate_unvax, 0.05));
    REQUIRE_FALSE(moreless(rate_prod_vax,   expected_rate_vax,   0.05));

    // Vaccinated rate should be strictly higher than unvaccinated
    REQUIRE(rate_prod_vax > rate_prod_unvax);

    // 2. Vaccinated breakthrough cases should recover faster from Rash
    double rate_rash_unvax = mat_u(3, 11);
    double rate_rash_vax   = mat_v(3, 11);

    REQUIRE_FALSE(moreless(rate_rash_unvax, 1.0/rash_period, 0.05));
    REQUIRE_FALSE(moreless(rate_rash_vax,
                           1.0/(rash_period * (1.0 - vax_improved_recovery)), 0.05));
    REQUIRE(rate_rash_vax > rate_rash_unvax);

    // 3. Vaccinated breakthrough cases have lower hospitalization rate
    double rate_hosp_unvax = mat_u(3, 10);
    double rate_hosp_vax   = mat_v(3, 10);
    REQUIRE(rate_hosp_vax < rate_hosp_unvax);

    std::cout << "Prodromal→Rash (unvax): " << rate_prod_unvax
              << " (expected ~" << expected_rate_unvax << ")" << std::endl;
    std::cout << "Prodromal→Rash (vax):   " << rate_prod_vax
              << " (expected ~" << expected_rate_vax << ")" << std::endl;
    std::cout << "Rash→Recovered  (unvax): " << rate_rash_unvax
              << " (expected ~" << 1.0/rash_period << ")" << std::endl;
    std::cout << "Rash→Recovered  (vax):   " << rate_rash_vax
              << " (expected ~" << 1.0/(rash_period*(1.0-vax_improved_recovery))
              << ")" << std::endl;
    std::cout << "Rash→Hosp (unvax): " << rate_hosp_unvax << std::endl;
    std::cout << "Rash→Hosp (vax):   " << rate_hosp_vax   << std::endl;

    #undef mat_u
    #undef mat_v

}
