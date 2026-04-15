#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles PEP separate MMR/IG deactivation",
    "[ModelMeaslesPEPDeactivation]"
) {

    using MS = epimodels::ModelMeaslesSchool<>;

    int n_seeds = 5;
    size_t nsims = 200;

    // Helper lambda: create model, attach PEP with given willingness,
    // run, and return average counts of agents with PEP MMR and PEP IG.
    auto run_pep_scenario = [&](
        double mmr_will,
        double ig_will,
        double mmr_window,
        double ig_window,
        double & avg_mmr,
        double & avg_ig
    ) {
        epimodels::ModelMeaslesSchool<> model(
            1000,    // Number of agents
            n_seeds, // Number of initial cases
            12.0,    // Contact rate
            0.2,     // Transmission rate
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

        epimodels::InterventionMeaslesPEP<> pep(
            "Post-exposure prophylaxis for measles",
            1.0,       // MMR efficacy
            1.0,       // IG efficacy
            4.0 * 7.0, // IG half-life mean
            7.0 / 2.0, // IG half-life sd
            mmr_will,  // MMR willingness
            ig_will,   // IG willingness
            mmr_window,// MMR window
            ig_window, // IG window
            MS::ISOLATED,
            {MS::QUARANTINED_LATENT, MS::QUARANTINED_SUSCEPTIBLE},
            {MS::SUSCEPTIBLE, MS::SUSCEPTIBLE},
            {MS::LATENT, MS::SUSCEPTIBLE}
        );

        model.add_globalevent(pep);

        model.get_virus(0).set_distribution(
            [&n_seeds](Virus<> & v, Model<> * m) -> void {
            for (int i = 0; i < n_seeds; ++i)
                m->get_agents()[i].set_virus(*m, v);
            return;
        });

        std::vector<double> got_mmr(nsims, 0.0);
        std::vector<double> got_ig(nsims, 0.0);

        auto saver = [&](size_t n, Model<>* m) -> void {
            for (auto & agent : m->get_agents())
            {
                if (agent.has_tool("PEP MMR"))
                    got_mmr[n]++;
                if (agent.has_tool("PEP IG"))
                    got_ig[n]++;
            }
        };

        model.run_multiple(200, nsims, 1231, saver, true, true, 2);

        avg_mmr = 0.0;
        avg_ig = 0.0;
        for (size_t i = 0; i < nsims; ++i)
        {
            avg_mmr += got_mmr[i];
            avg_ig += got_ig[i];
        }
        avg_mmr /= nsims;
        avg_ig /= nsims;
    };

    // =================================================================
    // Scenario 1: Both MMR and IG active (willingness = 1.0 for both)
    // =================================================================
    double avg_mmr_both, avg_ig_both;
    run_pep_scenario(1.0, 1.0, 3.0, 6.0, avg_mmr_both, avg_ig_both);

    std::cout << "\n=== Both active ===" << std::endl;
    std::cout << "Avg MMR: " << avg_mmr_both << std::endl;
    std::cout << "Avg IG:  " << avg_ig_both << std::endl;

    // Both should be distributed
    REQUIRE(avg_mmr_both > 0.0);
    REQUIRE(avg_ig_both > 0.0);

    // =================================================================
    // Scenario 2: MMR deactivated (willingness = 0 for MMR)
    // =================================================================
    double avg_mmr_nommr, avg_ig_nommr;
    run_pep_scenario(0.0, 1.0, 3.0, 6.0, avg_mmr_nommr, avg_ig_nommr);

    std::cout << "\n=== MMR deactivated (willingness=0) ===" << std::endl;
    std::cout << "Avg MMR: " << avg_mmr_nommr << std::endl;
    std::cout << "Avg IG:  " << avg_ig_nommr << std::endl;

    // No agent should receive MMR when willingness is 0
    REQUIRE(avg_mmr_nommr < 1e-10);
    // IG should still be given (and likely more than the both-active
    // scenario since agents who would have gotten MMR now get IG)
    REQUIRE(avg_ig_nommr > 0.0);

    // =================================================================
    // Scenario 3: IG deactivated (willingness = 0 for IG)
    // =================================================================
    double avg_mmr_noig, avg_ig_noig;
    run_pep_scenario(1.0, 0.0, 3.0, 6.0, avg_mmr_noig, avg_ig_noig);

    std::cout << "\n=== IG deactivated (willingness=0) ===" << std::endl;
    std::cout << "Avg MMR: " << avg_mmr_noig << std::endl;
    std::cout << "Avg IG:  " << avg_ig_noig << std::endl;

    // No agent should receive IG when willingness is 0
    REQUIRE(avg_ig_noig < 1e-10);
    // MMR should still be given
    REQUIRE(avg_mmr_noig > 0.0);

    // =================================================================
    // Scenario 4: Both deactivated (willingness = 0 for both)
    // =================================================================
    double avg_mmr_none, avg_ig_none;
    run_pep_scenario(0.0, 0.0, 3.0, 6.0, avg_mmr_none, avg_ig_none);

    std::cout << "\n=== Both deactivated (willingness=0) ===" << std::endl;
    std::cout << "Avg MMR: " << avg_mmr_none << std::endl;
    std::cout << "Avg IG:  " << avg_ig_none << std::endl;

    // No agent should receive any PEP
    REQUIRE(avg_mmr_none < 1e-10);
    REQUIRE(avg_ig_none < 1e-10);

    // =================================================================
    // Scenario 5: Agents get either MMR or IG, never both
    // =================================================================
    // Run with both active and check no agent has both tools
    {
        epimodels::ModelMeaslesSchool<> model(
            1000, n_seeds, 12.0, 0.2, 0.9, 0.3,
            7.0, 4.0, 5.0, 3, 0.1, 7.0,
            0.1, 21u, 1.0, 4u
        );

        epimodels::InterventionMeaslesPEP<> pep(
            "PEP",
            1.0, 1.0, 4.0 * 7.0, 7.0 / 2.0,
            1.0, 1.0, // both willing
            3.0, 6.0, // windows
            MS::ISOLATED,
            {MS::QUARANTINED_LATENT, MS::QUARANTINED_SUSCEPTIBLE},
            {MS::SUSCEPTIBLE, MS::SUSCEPTIBLE},
            {MS::LATENT, MS::SUSCEPTIBLE}
        );

        model.add_globalevent(pep);

        model.get_virus(0).set_distribution(
            [&n_seeds](Virus<> & v, Model<> * m) -> void {
            for (int i = 0; i < n_seeds; ++i)
                m->get_agents()[i].set_virus(*m, v);
            return;
        });

        size_t both_count = 0;
        auto saver = [&](size_t, Model<>* m) -> void {
            for (auto & agent : m->get_agents())
            {
                if (agent.has_tool("PEP MMR") && agent.has_tool("PEP IG"))
                    both_count++;
            }
        };

        model.run_multiple(200, nsims, 1231, saver, true, true, 2);

        std::cout << "\n=== Exclusivity check ===" << std::endl;
        std::cout << "Agents with BOTH tools: " << both_count << std::endl;

        // No agent should have both tools simultaneously
        REQUIRE(both_count == 0);
    }

}
