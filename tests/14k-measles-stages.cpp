#include "tests.hpp"
#include "measles-stages.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

// With fixed stage durations (standard deviations of zero) and no
// transmission, every initial case goes through its stages on the same days:
// latent for 5 days, prodromal for 3 and rash for 2. Moving agents to
// quarantine part-way through their latent period does not restart it.
EPIWORLD_TEST_CASE(
    "Measles models: fixed stage durations",
    "[MeaslesStages]"
) {

    size_t n = 200u;

    measles::ModelMeaslesMixing<> model(
        n,           // Number of agents
        1.0,         // Initial prevalence: everyone
        0.0,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        5.0,         // Incubation period
        3.0,         // Prodromal period
        2.0,         // Rash period
        {1.0},       // Contact matrix
        0.0,         // Hospitalization rate
        7.0,         // Hospitalization duration
        2.0,         // Days undetected
        21,          // Quarantine period
        1.0,         // Quarantine willingness
        1.0,         // Isolation willingness
        -1,          // Isolation period (no detection)
        0.0,         // Proportion vaccinated
        1.0,         // Contact tracing success rate
        4u,          // Contact tracing days window
        1.0,         // Rash reduction contact rate
        0.0,         // Incubation period sd
        0.0,         // Prodromal period sd
        0.0          // Rash period sd
    );

    model.add_entity(Entity<>("Population", distribute_entity_to_range<>(0, n)));
    model.verbose_off();

    // Only day `day` has transitions, and all n agents make them
    auto all_on = [&](const std::string & from, const std::string & to, int day) {
        auto counts = transitions_by_day(model, from, to);
        int total = 0;
        for (auto c : counts)
            total += c;
        return (total == static_cast<int>(n)) &&
            (counts[day] == static_cast<int>(n));
    };

    model.run(20, 123);
    REQUIRE(all_on("Latent", "Prodromal", 5));
    REQUIRE(all_on("Prodromal", "Rash", 8));
    REQUIRE(all_on("Rash", "Recovered", 10));

    // Quarantining every latent agent on day 2: the latent period still ends
    // on day 5. The rash starts in isolation on day 8; isolation is off, so
    // agents are released to Rash the next day, and the rash period still
    // ends on day 10
    model.add_globalevent(
        [](Model<> * m) -> void {
            for (auto & agent : m->get_agents())
                if (agent.get_state() == measles::ModelMeaslesMixing<>::LATENT)
                    agent.change_state(
                        *m, measles::ModelMeaslesMixing<>::QUARANTINED_LATENT
                    );
        },
        "Quarantine everyone",
        2
    );

    model.run(20, 123);
    REQUIRE(all_on("Latent", "Quarantined Latent", 2));
    REQUIRE(all_on("Quarantined Latent", "Quarantined Prodromal", 5));
    REQUIRE(all_on("Quarantined Prodromal", "Isolated", 8));
    REQUIRE(all_on("Isolated", "Rash", 9));
    REQUIRE(all_on("Rash", "Recovered", 10));

    // The same course in the risk-based quarantine model
    measles::ModelMeaslesMixingRiskQuarantine<> model_risk(
        n,           // Number of agents
        1.0,         // Initial prevalence: everyone
        0.0,         // Transmission rate
        0.9,         // Vaccination efficacy
        5.0,         // Incubation period
        3.0,         // Prodromal period
        2.0,         // Rash period
        {1.0},       // Contact matrix
        0.0,         // Hospitalization rate
        7.0,         // Hospitalization duration
        2.0,         // Days undetected
        21,          // Quarantine period high risk
        14,          // Quarantine period medium risk
        7,           // Quarantine period low risk
        1.0,         // Quarantine willingness
        1.0,         // Isolation willingness
        -1,          // Isolation period (no detection)
        0.0,         // Proportion vaccinated
        0.0,         // Detection rate during quarantine
        1.0,         // Contact tracing success rate
        4u,          // Contact tracing days window
        0.0,         // Incubation period sd
        0.0,         // Prodromal period sd
        0.0          // Rash period sd
    );

    model_risk.add_entity(
        Entity<>("Population", distribute_entity_to_range<>(0, n))
    );
    model_risk.verbose_off();
    model_risk.run(20, 123);

    auto risk_on = [&](const std::string & from, const std::string & to, int day) {
        auto counts = transitions_by_day(model_risk, from, to);
        return counts[day] == static_cast<int>(n);
    };

    REQUIRE(risk_on("Latent", "Prodromal", 5));
    REQUIRE(risk_on("Prodromal", "Rash", 8));
    REQUIRE(risk_on("Rash", "Recovered", 10));

    // A case imported on day 3 by a global event (not by contact) counts its
    // latent period from that day: prodromal on day 8, rash on day 11
    measles::ModelMeaslesMixing<> model_import(
        n,           // Number of agents
        0.0,         // Initial prevalence: none
        0.0,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        5.0,         // Incubation period
        3.0,         // Prodromal period
        2.0,         // Rash period
        {1.0},       // Contact matrix
        0.0,         // Hospitalization rate
        7.0,         // Hospitalization duration
        2.0,         // Days undetected
        21,          // Quarantine period
        1.0,         // Quarantine willingness
        1.0,         // Isolation willingness
        -1,          // Isolation period (no detection)
        0.0,         // Proportion vaccinated
        1.0,         // Contact tracing success rate
        4u,          // Contact tracing days window
        1.0,         // Rash reduction contact rate
        0.0,         // Incubation period sd
        0.0,         // Prodromal period sd
        0.0          // Rash period sd
    );

    model_import.add_entity(
        Entity<>("Population", distribute_entity_to_range<>(0, n))
    );
    model_import.verbose_off();
    model_import.add_globalevent(
        [](Model<> * m) -> void {
            m->get_agent(0).set_virus(
                *m, m->get_virus(0), measles::ModelMeaslesMixing<>::LATENT
            );
        },
        "Import a case",
        3
    );
    model_import.run(20, 123);

    auto import_on = [&](const std::string & from, const std::string & to) {
        auto counts = transitions_by_day(model_import, from, to);
        std::vector< int > days;
        for (size_t d = 0u; d < counts.size(); ++d)
            for (int k = 0; k < counts[d]; ++k)
                days.push_back(static_cast<int>(d));
        return days;
    };

    REQUIRE(import_on("Latent", "Prodromal") == std::vector< int >{8});
    REQUIRE(import_on("Prodromal", "Rash") == std::vector< int >{11});
    REQUIRE(import_on("Rash", "Recovered") == std::vector< int >{13});

    // With drawn (lognormal) durations, a run depends only on its seed: the
    // same seed gives the same history after a run with another seed
    model_risk.set_param("Incubation period sd", 2.5);
    model_risk.set_param("Prodromal period sd", 1.7);
    model_risk.set_param("Rash period sd", 1.0);

    auto history = [&](int seed) {
        model_risk.run(30, seed);
        std::vector< std::string > from, to;
        std::vector< int > date, counts;
        model_risk.get_db().get_hist_transition_matrix(from, to, date, counts, true);
        return std::make_pair(date, counts);
    };

    auto first = history(7);
    history(99);
    REQUIRE(history(7) == first);

}
