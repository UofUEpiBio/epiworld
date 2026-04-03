#define EPI_DEBUG
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
        4u       // Isolation period
    );

    // Creating the PEP intervention and 
    // setting it up so we can call it as a global event.
    epimodels::InterventionMeaslesPEP<> pep(
        "Post-exposure prophylaxis for measles", // Name of the intervention
        1.0,       // "PEP MMR efficacy"
        1.0,       // "PEP IG efficacy"
        4.0 * 7.0, // "PEP IG half-life (mean)"
        7.0/2.0,   // "PEP IG half-life (sd)"
        1.0,       // "PEP willingness"
        0.0,       // "PEP MMR window"
        {MS::QUARANTINED_EXPOSED, MS::QUARANTINED_SUSCEPTIBLE},
        {MS::RECOVERED, MS::SUSCEPTIBLE}
    );
    
    model.add_globalevent(pep);

    // Setting the distribution function of the initial cases
    model.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    size_t nsims = 500;

    // --- Per-agent counts ---
    // Agents classified by which PEP tool they received
    std::vector<double> got_mmr(nsims, 0.0);   // PEP MMR holders
    std::vector<double> got_ig(nsims, 0.0);     // PEP IG holders
    std::vector<double> got_nothing(nsims, 0.0); // No PEP at all
    // Average PEP day
    std::vector<double> avg_day_mmr(nsims, 0.0);
    std::vector<double> avg_day_ig(nsims, 0.0);

    // --- DB history counts (tool hist vs total hist) ---
    // PEP MMR tool counts by final state
    std::vector<double> hist_mmr_susceptible(nsims, 0.0);
    std::vector<double> hist_mmr_recovered(nsims, 0.0);
    // PEP IG tool counts by final state
    std::vector<double> hist_ig_susceptible(nsims, 0.0);
    std::vector<double> hist_ig_recovered(nsims, 0.0);
    // Total counts for subtraction
    std::vector<double> hist_total_susceptible(nsims, 0.0);
    std::vector<double> hist_total_recovered(nsims, 0.0);

    auto saver = [&](size_t n, Model<>* m) -> void {

        // ---- Per-agent tool inspection ----
        for (auto & agent : m->get_agents())
        {
            bool has_mmr = agent.has_tool("PEP MMR");
            bool has_ig  = agent.has_tool("PEP IG");

            if (!has_mmr && !has_ig)
            {
                got_nothing[n]++;
                continue;
            }

            auto tools = agent.get_tools();
            for (size_t t = 0; t < agent.get_n_tools(); ++t)
            {
                auto name = tools[t]->get_name();
                if (name == "PEP MMR")
                {
                    got_mmr[n]++;
                    avg_day_mmr[n] += tools[t]->get_date();
                    break;
                }
                else if (name == "PEP IG")
                {
                    got_ig[n]++;
                    avg_day_ig[n] += tools[t]->get_date();
                    break;
                }
            }
        }

        if (got_mmr[n] > 0)
            avg_day_mmr[n] /= got_mmr[n];
        if (got_ig[n] > 0)
            avg_day_ig[n] /= got_ig[n];

        // ---- Database history approach ----
        std::vector<int> total_date, total_counts;
        std::vector<std::string> total_state;
        m->get_db().get_hist_total(&total_date, &total_state, &total_counts);

        std::vector<int> tool_date, tool_id_vec, tool_counts;
        std::vector<std::string> tool_state;
        m->get_db().get_hist_tool(tool_date, tool_id_vec, tool_state, tool_counts);

        int mmr_id = m->get_tool("PEP MMR").get_id();
        int ig_id  = m->get_tool("PEP IG").get_id();

        int last_date = *std::max_element(total_date.begin(), total_date.end());

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

        for (size_t j = 0; j < tool_date.size(); ++j)
        {
            if (tool_date[j] != last_date)
                continue;
            int tid = tool_id_vec[j];
            if (tid == mmr_id)
            {
                if (tool_state[j] == state_names[MS::SUSCEPTIBLE])
                    hist_mmr_susceptible[n] = tool_counts[j];
                else if (tool_state[j] == state_names[MS::RECOVERED])
                    hist_mmr_recovered[n] = tool_counts[j];
            }
            else if (tid == ig_id)
            {
                if (tool_state[j] == state_names[MS::SUSCEPTIBLE])
                    hist_ig_susceptible[n] = tool_counts[j];
                else if (tool_state[j] == state_names[MS::RECOVERED])
                    hist_ig_recovered[n] = tool_counts[j];
            }
        }
    };

    model.run_multiple(200, nsims, 1231, saver, true, true, 2);

    model.print();

    // =========================================================
    // Aggregate across simulations
    // =========================================================
    double tot_mmr = 0, tot_ig = 0, tot_nothing = 0;
    double tot_day_mmr = 0, tot_day_ig = 0;
    size_t sims_mmr = 0, sims_ig = 0;

    double tot_hist_mmr_s = 0, tot_hist_mmr_r = 0;
    double tot_hist_ig_s = 0, tot_hist_ig_r = 0;
    double tot_hist_total_s = 0, tot_hist_total_r = 0;

    for (size_t i = 0; i < nsims; ++i)
    {
        tot_mmr += got_mmr[i];
        tot_ig += got_ig[i];
        tot_nothing += got_nothing[i];
        if (got_mmr[i] > 0) { tot_day_mmr += avg_day_mmr[i]; sims_mmr++; }
        if (got_ig[i] > 0) { tot_day_ig += avg_day_ig[i]; sims_ig++; }

        tot_hist_mmr_s += hist_mmr_susceptible[i];
        tot_hist_mmr_r += hist_mmr_recovered[i];
        tot_hist_ig_s += hist_ig_susceptible[i];
        tot_hist_ig_r += hist_ig_recovered[i];
        tot_hist_total_s += hist_total_susceptible[i];
        tot_hist_total_r += hist_total_recovered[i];
    }

    double avg_mmr = tot_mmr / nsims;
    double avg_ig = tot_ig / nsims;
    double avg_nothing = tot_nothing / nsims;
    double avg_day_mmr_all = sims_mmr > 0 ? tot_day_mmr / sims_mmr : -1.0;
    double avg_day_ig_all = sims_ig > 0 ? tot_day_ig / sims_ig : -1.0;

    double avg_h_mmr_s = tot_hist_mmr_s / nsims;
    double avg_h_mmr_r = tot_hist_mmr_r / nsims;
    double avg_h_ig_s  = tot_hist_ig_s / nsims;
    double avg_h_ig_r  = tot_hist_ig_r / nsims;
    double avg_h_total_s = tot_hist_total_s / nsims;
    double avg_h_total_r = tot_hist_total_r / nsims;
    double avg_h_no_pep_s = avg_h_total_s - avg_h_mmr_s - avg_h_ig_s;
    double avg_h_no_pep_r = avg_h_total_r - avg_h_mmr_r - avg_h_ig_r;

    // =========================================================
    // Diagnostics
    // =========================================================
    std::cout << "\n=== Per-Agent Tool Inspection ===" << std::endl;
    std::cout << "Avg agents with PEP MMR:  " << avg_mmr << std::endl;
    std::cout << "Avg agents with PEP IG:   " << avg_ig << std::endl;
    std::cout << "Avg agents with no PEP:   " << avg_nothing << std::endl;
    std::cout << "Avg day of PEP MMR:       " << avg_day_mmr_all << std::endl;
    std::cout << "Avg day of PEP IG:        " << avg_day_ig_all << std::endl;

    std::cout << "\n=== DB History: Tool vs Total ===" << std::endl;
    std::cout << "Avg total Susceptible:       " << avg_h_total_s << std::endl;
    std::cout << "  PEP MMR in Susceptible:    " << avg_h_mmr_s << std::endl;
    std::cout << "  PEP IG  in Susceptible:    " << avg_h_ig_s << std::endl;
    std::cout << "  No PEP  in Susceptible:    " << avg_h_no_pep_s << std::endl;
    std::cout << "Avg total Recovered:         " << avg_h_total_r << std::endl;
    std::cout << "  PEP MMR in Recovered:      " << avg_h_mmr_r << std::endl;
    std::cout << "  PEP IG  in Recovered:      " << avg_h_ig_r << std::endl;
    std::cout << "  No PEP  in Recovered:      " << avg_h_no_pep_r << std::endl;

    // =========================================================
    // Tests: PEP MMR recipients
    // =========================================================
    // Susceptible agents in quarantine always get MMR (no virus,
    // so they fall through the virus check in the intervention).
    REQUIRE(avg_h_mmr_s > 0.0);

    // Some exposed agents infected within the MMR window get MMR.
    REQUIRE(avg_h_mmr_r > 0.0);

    // =========================================================
    // Tests: PEP IG recipients
    // =========================================================
    // Exposed agents infected beyond the MMR window get IG instead.
    // With incubation=7 and window=3, agents exposed early enough
    // that (quarantine_day - infection_day) > 3 should get IG.
    REQUIRE(avg_h_ig_r > 0.0);

    // Susceptible agents should NOT get IG (they have no virus,
    // so the intervention always gives them MMR).
    REQUIRE(avg_h_ig_s < 1e-10);

    // =========================================================
    // Cross-validation: per-agent totals should match DB history
    // sums across Susceptible + Recovered states.
    // =========================================================
    REQUIRE_FALSE(moreless(avg_mmr, avg_h_mmr_s + avg_h_mmr_r, 0.01));
    REQUIRE_FALSE(moreless(avg_ig, avg_h_ig_s + avg_h_ig_r, 0.01));

    // =========================================================
    // Non-exposed PEP (MMR) recipients should outnumber exposed
    // =========================================================
    REQUIRE(avg_h_mmr_s > avg_h_mmr_r);

    // =========================================================
    // Agents without any PEP: vaccinated or not quarantined
    // =========================================================
    REQUIRE(avg_h_no_pep_s > 0.0);
    REQUIRE(avg_h_no_pep_r >= 0.0);

    // =========================================================
    // PEP timing checks
    // =========================================================
    double earliest_possible = model("Incubation period") +
        model("Prodromal period");
    std::cout << "Earliest possible PEP day (approx): "
              << earliest_possible << std::endl;

    REQUIRE(avg_day_mmr_all >= earliest_possible);
    if (sims_ig > 0)
        REQUIRE(avg_day_ig_all >= earliest_possible);

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
        4u       // Isolation period
    );

    model_nopep.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    std::vector<double> nopep_mmr(nsims, 0.0);
    std::vector<double> nopep_ig(nsims, 0.0);

    auto nopep_saver = [&](size_t n, Model<>* m) -> void {
        for (auto & agent : m->get_agents())
        {
            if (agent.has_tool("PEP MMR"))
                nopep_mmr[n]++;
            if (agent.has_tool("PEP IG"))
                nopep_ig[n]++;
        }
    };

    model_nopep.run_multiple(60, nsims, 1231, nopep_saver, true, true, 2);

    double total_nopep_mmr = 0.0, total_nopep_ig = 0.0;
    for (size_t i = 0; i < nsims; ++i)
    {
        total_nopep_mmr += nopep_mmr[i];
        total_nopep_ig += nopep_ig[i];
    }

    std::cout << "\n=== No-PEP Model ===" << std::endl;
    std::cout << "Total PEP MMR recipients (should be 0): "
              << total_nopep_mmr << std::endl;
    std::cout << "Total PEP IG recipients (should be 0): "
              << total_nopep_ig << std::endl;

    REQUIRE(total_nopep_mmr < 1e-10);
    REQUIRE(total_nopep_ig < 1e-10);

    // =========================================================
    // Summary
    // =========================================================
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "With PEP (MMR eff=1, IG eff=1, willingness=1, window=3):"
              << std::endl;
    std::cout << "  [per-agent] PEP MMR holders:       " << avg_mmr << std::endl;
    std::cout << "  [per-agent] PEP IG holders:        " << avg_ig << std::endl;
    std::cout << "  [per-agent] No PEP:                " << avg_nothing << std::endl;
    std::cout << "  [per-agent] Avg PEP MMR day:       " << avg_day_mmr_all << std::endl;
    std::cout << "  [per-agent] Avg PEP IG day:        " << avg_day_ig_all << std::endl;
    std::cout << "  [db hist]   MMR in Susceptible:    " << avg_h_mmr_s << std::endl;
    std::cout << "  [db hist]   MMR in Recovered:      " << avg_h_mmr_r << std::endl;
    std::cout << "  [db hist]   IG  in Susceptible:    " << avg_h_ig_s << std::endl;
    std::cout << "  [db hist]   IG  in Recovered:      " << avg_h_ig_r << std::endl;
    std::cout << "  [db hist]   No PEP Susceptible:    " << avg_h_no_pep_s << std::endl;
    std::cout << "  [db hist]   No PEP Recovered:      " << avg_h_no_pep_r << std::endl;
    std::cout << "Without PEP:" << std::endl;
    std::cout << "  Total MMR: " << total_nopep_mmr << std::endl;
    std::cout << "  Total IG:  " << total_nopep_ig << std::endl;
}
