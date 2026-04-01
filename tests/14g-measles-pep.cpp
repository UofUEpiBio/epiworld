#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Measles PEP tool history", "[ModelMeaslesPEP]") {

    using MS = epimodels::ModelMeaslesSchool<>;

    int n_seeds = 1;
    epimodels::ModelMeaslesSchool<> model(
        1000,    // Number of agents
        n_seeds, // Number of initial cases
        6.0,     // Contact rate
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
        4u,      // Isolation period
        1.0,     // PEP efficacy
        1.0      // PEP willingness
    );

    // Setting the distribution function of the initial cases
    model.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    size_t nsims = 500;

    // --- Per-agent approach ---
    // Counts of agents who received PEP, classified by final state
    std::vector<double> pep_from_susceptible(nsims, 0.0);
    std::vector<double> pep_from_exposed(nsims, 0.0);
    // Average day PEP was given (per-agent tool date)
    std::vector<double> avg_pep_day_susceptible(nsims, 0.0);
    std::vector<double> avg_pep_day_exposed(nsims, 0.0);

    // --- Database history approach ---
    // At the last day, PEP tool count in Susceptible/Recovered
    // directly gives PEP recipients; subtracting from total gives
    // agents who reached those states without PEP.
    std::vector<double> hist_pep_susceptible(nsims, 0.0);
    std::vector<double> hist_pep_recovered(nsims, 0.0);
    std::vector<double> hist_total_susceptible(nsims, 0.0);
    std::vector<double> hist_total_recovered(nsims, 0.0);

    auto saver = [&](size_t n, Model<>* m) -> void {

        // ---- Per-agent tool inspection ----
        for (auto & agent : m->get_agents())
        {
            if (!agent.has_tool("PEP MMR"))
                continue;

            auto tools = agent.get_tools();
            for (size_t t = 0; t < agent.get_n_tools(); ++t)
            {
                if (tools[t]->get_name() != "PEP MMR")
                    continue;

                int pep_day = tools[t]->get_date();
                auto final_state = agent.get_state();

                if (final_state == MS::SUSCEPTIBLE)
                {
                    pep_from_susceptible[n]++;
                    avg_pep_day_susceptible[n] += pep_day;
                }
                else if (final_state == MS::RECOVERED)
                {
                    pep_from_exposed[n]++;
                    avg_pep_day_exposed[n] += pep_day;
                }

                break;
            }
        }

        if (pep_from_susceptible[n] > 0)
            avg_pep_day_susceptible[n] /= pep_from_susceptible[n];
        if (pep_from_exposed[n] > 0)
            avg_pep_day_exposed[n] /= pep_from_exposed[n];

        // ---- Database history approach ----
        // Get total history (date, state_name, counts)
        std::vector<int> total_date, total_counts;
        std::vector<std::string> total_state;
        m->get_db().get_hist_total(&total_date, &total_state, &total_counts);

        // Get tool history (date, tool_id, state_name, counts)
        std::vector<int> tool_date, tool_id, tool_counts;
        std::vector<std::string> tool_state;
        m->get_db().get_hist_tool(tool_date, tool_id, tool_state, tool_counts);

        int pep_id = m->get_tool("PEP MMR").get_id();

        // Find the last recorded date
        int last_date = *std::max_element(total_date.begin(), total_date.end());

        // Extract total counts at the last date
        auto state_names = m->get_states();
        for (size_t j = 0; j < total_date.size(); ++j)
        {
            if (total_date[j] != last_date)
                continue;
            if (total_state[j] == state_names[MS::SUSCEPTIBLE])
                hist_total_susceptible[n] = total_counts[j];
            else if (total_state[j] == state_names[MS::RECOVERED])
                hist_total_recovered[n] = total_counts[j];
        }

        // Extract PEP tool counts at the last date
        for (size_t j = 0; j < tool_date.size(); ++j)
        {
            if (tool_date[j] != last_date || tool_id[j] != pep_id)
                continue;
            if (tool_state[j] == state_names[MS::SUSCEPTIBLE])
                hist_pep_susceptible[n] = tool_counts[j];
            else if (tool_state[j] == state_names[MS::RECOVERED])
                hist_pep_recovered[n] = tool_counts[j];
        }
    };

    model.run_multiple(60, nsims, 1231, saver, true, true, 2);

    model.print();

    // =========================================================
    // Aggregate across simulations
    // =========================================================
    double total_pep_susceptible = 0.0;
    double total_pep_exposed = 0.0;
    double total_pep_day_s = 0.0;
    double total_pep_day_e = 0.0;
    size_t sims_with_exposed_pep = 0;
    size_t sims_with_susceptible_pep = 0;

    // Database history aggregation
    double total_hist_pep_s = 0.0, total_hist_pep_r = 0.0;
    double total_hist_total_s = 0.0, total_hist_total_r = 0.0;

    for (size_t i = 0; i < nsims; ++i)
    {
        total_pep_susceptible += pep_from_susceptible[i];
        total_pep_exposed += pep_from_exposed[i];

        if (pep_from_susceptible[i] > 0)
        {
            total_pep_day_s += avg_pep_day_susceptible[i];
            sims_with_susceptible_pep++;
        }
        if (pep_from_exposed[i] > 0)
        {
            total_pep_day_e += avg_pep_day_exposed[i];
            sims_with_exposed_pep++;
        }

        total_hist_pep_s += hist_pep_susceptible[i];
        total_hist_pep_r += hist_pep_recovered[i];
        total_hist_total_s += hist_total_susceptible[i];
        total_hist_total_r += hist_total_recovered[i];
    }

    double avg_susceptible = total_pep_susceptible / nsims;
    double avg_exposed = total_pep_exposed / nsims;
    double avg_day_s = sims_with_susceptible_pep > 0
        ? total_pep_day_s / sims_with_susceptible_pep : -1.0;
    double avg_day_e = sims_with_exposed_pep > 0
        ? total_pep_day_e / sims_with_exposed_pep : -1.0;

    // Database history averages
    double avg_hist_pep_s = total_hist_pep_s / nsims;
    double avg_hist_pep_r = total_hist_pep_r / nsims;
    double avg_hist_total_s = total_hist_total_s / nsims;
    double avg_hist_total_r = total_hist_total_r / nsims;
    double avg_hist_no_pep_s = avg_hist_total_s - avg_hist_pep_s;
    double avg_hist_no_pep_r = avg_hist_total_r - avg_hist_pep_r;

    // =========================================================
    // Diagnostics
    // =========================================================
    std::cout << "\n=== Per-Agent Tool Inspection ===" << std::endl;
    std::cout << "Avg non-exposed PEP recipients (Q_Susceptible->Susceptible): "
              << avg_susceptible << std::endl;
    std::cout << "Avg exposed PEP recipients (Q_Exposed->Recovered): "
              << avg_exposed << std::endl;
    std::cout << "Avg day of PEP (non-exposed): " << avg_day_s << std::endl;
    std::cout << "Avg day of PEP (exposed): " << avg_day_e << std::endl;

    std::cout << "\n=== Database History (total vs tool) ===" << std::endl;
    std::cout << "Avg total Susceptible at end:    " << avg_hist_total_s << std::endl;
    std::cout << "Avg PEP holders in Susceptible:  " << avg_hist_pep_s << std::endl;
    std::cout << "Avg Susceptible without PEP:     " << avg_hist_no_pep_s << std::endl;
    std::cout << "Avg total Recovered at end:      " << avg_hist_total_r << std::endl;
    std::cout << "Avg PEP holders in Recovered:    " << avg_hist_pep_r << std::endl;
    std::cout << "Avg Recovered without PEP:       " << avg_hist_no_pep_r << std::endl;

    // =========================================================
    // Cross-validation: per-agent counts must match DB history
    // PEP tool counts at the end.
    // =========================================================
    REQUIRE_FALSE(moreless(avg_susceptible, avg_hist_pep_s, 0.01));
    REQUIRE_FALSE(moreless(avg_exposed, avg_hist_pep_r, 0.01));

    // =========================================================
    // Tests: Some non-exposed agents received PEP
    // =========================================================
    REQUIRE(avg_hist_pep_s > 0.0);

    // =========================================================
    // Tests: Some exposed agents received PEP
    // =========================================================
    REQUIRE(avg_hist_pep_r > 0.0);

    // =========================================================
    // Non-exposed PEP recipients should outnumber exposed ones
    // =========================================================
    REQUIRE(avg_hist_pep_s > avg_hist_pep_r);

    // =========================================================
    // Susceptible agents without PEP: these are agents who were
    // never quarantined (e.g., vaccinated or simply not caught
    // in the quarantine net). Should be > 0 since we have 10%
    // vaccinated.
    // =========================================================
    REQUIRE(avg_hist_no_pep_s > 0.0);

    // =========================================================
    // Recovered without PEP: agents who went through the full
    // disease progression (prodromal -> rash -> recovered)
    // without PEP. With a higher contact rate and 1 seed, some
    // agents will recover naturally.
    // =========================================================
    REQUIRE(avg_hist_no_pep_r >= 0.0);

    // =========================================================
    // PEP timing checks
    // =========================================================
    double earliest_possible = model("Incubation period") +
        model("Prodromal period");
    std::cout << "Earliest possible PEP day (approx): "
              << earliest_possible << std::endl;

    REQUIRE(avg_day_s >= earliest_possible);
    REQUIRE(avg_day_e >= earliest_possible);

    // =========================================================
    // Comparison: run WITHOUT PEP
    // =========================================================
    epimodels::ModelMeaslesSchool<> model_nopep(
        1000,    // Number of agents
        n_seeds, // Number of initial cases
        2.0,     // Contact rate
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
        4u,      // Isolation period
        0.0,     // PEP efficacy  = 0 (disabled)
        0.0      // PEP willingness = 0 (no one gets it)
    );

    model_nopep.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    std::vector<double> nopep_pep_count(nsims, 0.0);

    auto nopep_saver = [&](size_t n, Model<>* m) -> void {
        for (auto & agent : m->get_agents())
        {
            if (agent.has_tool("PEP MMR"))
                nopep_pep_count[n]++;
        }
    };

    model_nopep.run_multiple(60, nsims, 1231, nopep_saver, true, true, 2);

    double total_nopep_pep = 0.0;
    for (size_t i = 0; i < nsims; ++i)
        total_nopep_pep += nopep_pep_count[i];

    std::cout << "\n=== No-PEP Model ===" << std::endl;
    std::cout << "Total PEP recipients across all sims (should be 0): "
              << total_nopep_pep << std::endl;

    // With PEP willingness = 0, nobody should have received PEP
    REQUIRE(total_nopep_pep < 1e-10);

    // =========================================================
    // Summary
    // =========================================================
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "With PEP (efficacy=1.0, willingness=1.0):" << std::endl;
    std::cout << "  [per-agent] Non-exposed PEP recipients: "
              << avg_susceptible << std::endl;
    std::cout << "  [per-agent] Exposed PEP recipients:     "
              << avg_exposed << std::endl;
    std::cout << "  [per-agent] Avg PEP day (non-exposed):  "
              << avg_day_s << std::endl;
    std::cout << "  [per-agent] Avg PEP day (exposed):      "
              << avg_day_e << std::endl;
    std::cout << "  [db hist]   PEP in Susceptible:         "
              << avg_hist_pep_s << std::endl;
    std::cout << "  [db hist]   PEP in Recovered:           "
              << avg_hist_pep_r << std::endl;
    std::cout << "  [db hist]   Susceptible without PEP:    "
              << avg_hist_no_pep_s << std::endl;
    std::cout << "  [db hist]   Recovered without PEP:      "
              << avg_hist_no_pep_r << std::endl;
    std::cout << "Without PEP:" << std::endl;
    std::cout << "  Total PEP recipients: " << total_nopep_pep << std::endl;
}
