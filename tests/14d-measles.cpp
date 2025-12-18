#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

/**
 * @brief Test that outbreak size matches virus history totals
 * 
 * This test verifies that:
 * 1. The outbreak size computed via get_outbreak_size() matches the total
 *    number of transmission events (including initial seeds).
 * 2. The outbreak size matches the count of "ever infected" agents
 *    (all agents not in Susceptible or Quarantined Susceptible states).
 * 3. All virus history counts are non-negative (regression test for the bug
 *    where counts could become negative due to incorrect state_prev handling).
 * 4. The virus history sum at final day matches active cases.
 * 
 * This is a regression test using the ModelMeaslesSchool which was previously
 * failing due to multiple state changes on the same day.
 */
EPIWORLD_TEST_CASE(
    "Measles outbreak size vs virus history",
    "[ModelMeaslesVirusHistory]"
) {
    
    // Using the same model configuration as 14a-measles.cpp
    int n_seeds = 5;  // More seeds for larger outbreaks
    epimodels::ModelMeaslesSchool<> model(
        500,    // Number of agents
        n_seeds, // Number of initial cases
        2.0,     // Contact rate
        0.2,     // Transmission rate
        0.9,     // Vaccination efficacy
        0.3,     // Vaccination reduction recovery rate
        7.0,     // Incubation period
        4.0,     // Prodromal period
        5.0,     // Rash period
        3.0,     // Days undetected
        0.04411765,     // Hospitalization rate
        7.0,     // Hospitalization duration
        0.0,     // Proportion vaccinated
        21u,     // Quarantine period
        .8,      // Quarantine willingness
        4u       // Isolation period
    );

    // Setting the distribution function of the initial cases
    model.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(v, m);
        return;
    });

    int ndays = 60;

    model.run(ndays, 17273);

    auto & db = model.get_db();

    // =====================================================================
    // Test 1: get_outbreak_size() should match total transmissions
    // =====================================================================
    std::vector<int> outbreak_date;
    std::vector<int> outbreak_virus_id;
    std::vector<int> outbreak_size;
    db.get_outbreak_size(outbreak_date, outbreak_virus_id, outbreak_size);

    // Get the final outbreak size (last day)
    int final_outbreak_size = outbreak_size[ndays];

    // Count total transmissions (including seeds with source=-1)
    std::vector<int> trans_date;
    std::vector<int> trans_source;
    std::vector<int> trans_target;
    std::vector<int> trans_virus;
    std::vector<int> trans_source_exposure;
    db.get_transmissions(trans_date, trans_source, trans_target, 
                            trans_virus, trans_source_exposure);

    int total_transmissions = static_cast<int>(trans_date.size());

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(final_outbreak_size == total_transmissions);
    #else
    if (final_outbreak_size != total_transmissions)
    {
        std::cerr << "Outbreak size mismatch! "
                    << "get_outbreak_size()=" << final_outbreak_size
                    << " vs total_transmissions=" << total_transmissions
                    << std::endl;
        throw std::logic_error("Outbreak size mismatch");
    }
    #endif

    // =====================================================================
    // Test 2: Outbreak size should match "ever infected" count
    // =====================================================================
    std::vector<std::string> states;
    std::vector<int> counts;
    db.get_today_total(&states, &counts);
    
    int ever_infected = 0;
    for (size_t i = 0; i < states.size(); i++) {
        // Everyone except Susceptible and Quarantined Susceptible was infected
        if (states[i] != "Susceptible" && 
            states[i] != "Quarantined Susceptible")
            ever_infected += counts[i];
    }

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(final_outbreak_size == ever_infected);
    #else
    if (final_outbreak_size != ever_infected)
    {
        std::cerr << "Ever infected mismatch! "
                    << "outbreak_size=" << final_outbreak_size
                    << " vs ever_infected=" << ever_infected
                    << std::endl;
        throw std::logic_error("Ever infected mismatch");
    }
    #endif

    // =====================================================================
    // Test 3: All virus history counts must be non-negative
    // =====================================================================
    std::vector<int> hist_date;
    std::vector<int> hist_id;
    std::vector<std::string> hist_state;
    std::vector<int> hist_counts;
    db.get_hist_virus(hist_date, hist_id, hist_state, hist_counts);

    for (size_t i = 0; i < hist_counts.size(); ++i)
    {
        #ifdef CATCH_CONFIG_MAIN
        REQUIRE(hist_counts[i] >= 0);
        #else
        if (hist_counts[i] < 0)
        {
            std::cerr << "Negative virus count at day "
                        << hist_date[i] << ", state " << hist_state[i]
                        << ", count=" << hist_counts[i] << std::endl;
            throw std::logic_error("Negative virus history count");
        }
        #endif
    }

    // =====================================================================
    // Test 4: Virus history sum at final day should match active cases
    // =====================================================================
    int virus_hist_sum_final = 0;
    for (size_t i = 0; i < hist_date.size(); ++i)
    {
        if (hist_date[i] == ndays)
            virus_hist_sum_final += hist_counts[i];
    }

    std::vector<int> active_date;
    std::vector<int> active_virus_id;
    std::vector<int> active_count;
    db.get_active_cases(active_date, active_virus_id, active_count);

    int active_cases_final = active_count[ndays];

    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(virus_hist_sum_final == active_cases_final);
    #else
    if (virus_hist_sum_final != active_cases_final)
    {
        std::cerr << "Active cases mismatch! "
                    << "virus_hist_sum=" << virus_hist_sum_final
                    << " vs get_active_cases()=" << active_cases_final
                    << std::endl;
        throw std::logic_error("Active cases mismatch");
    }
    #endif

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif
    
}
