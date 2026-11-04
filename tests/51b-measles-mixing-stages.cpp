#include "tests.hpp"
#include "measles-stages.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

// Paired deterministic scenarios of the measles mixing model, with statuses
// and stages: fixed stage durations (standard deviations of zero), no
// transmission, and probabilities of 0 or 1. Every initial case goes through
// its stages on the same days (latent 5 days, prodromal 3, rash 2), and the
// days of quarantine, isolation, admission, discharge and release are the
// ones the 12-state model gave.
EPIWORLD_TEST_CASE(
    "Measles mixing: stages and statuses on fixed days",
    "[MeaslesMixingStages]"
) {

    using M = measles::ModelMeaslesMixing<>;
    const size_t n = 200u;

    auto make = [n](
        double prevalence,
        double hosp_rate,
        double days_undetected,
        int isolation_period
    ) {
        auto model = std::make_unique< M >(
            n,                // Number of agents
            prevalence,       // Initial prevalence
            0.0,              // Transmission rate
            0.9,              // Vaccination efficacy
            0.3,              // Vaccination reduction recovery rate
            5.0,              // Incubation period
            3.0,              // Prodromal period
            2.0,              // Rash period
            hosp_rate,        // Hospitalization rate
            1.0,              // Hospitalization period (discharge next day)
            days_undetected,  // Days undetected
            21,               // Quarantine period
            1.0,              // Quarantine willingness
            1.0,              // Isolation willingness
            isolation_period, // Isolation period (-1: no detection)
            0.0,              // Proportion vaccinated
            1.0,              // Contact tracing success rate
            4u,               // Contact tracing days window
            1.0,              // Rash reduction contact rate
            0.0,              // Incubation period sd
            0.0,              // Prodromal period sd
            0.0               // Rash period sd
        );
        add_group_mixing(*model, {1.0}, blocks({0u, n}));
        model->verbose_off();
        return model;
    };

    // Only day `day` has transitions, and all n agents make them
    auto all_on = [n](const std::vector< int > & counts, int day) {
        int total = 0;
        for (auto c : counts)
            total += c;
        return (total == static_cast< int >(n)) &&
            (counts[day] == static_cast< int >(n));
    };

    auto stages_on = [&](const Model<> & m, const std::string & from,
        const std::string & to, int day) {
        return all_on(stage_transitions_by_day(m, from, to), day);
    };

    auto statuses_on = [&](const Model<> & m, const std::string & from,
        const std::string & to, int day) {
        return all_on(status_transitions_by_day(m, from, to), day);
    };

    // (a) The course of the disease ------------------------------------------
    auto model = make(1.0, 0.0, 2.0, -1);
    model->run(20, 123);
    REQUIRE(stages_on(*model, "Latent", "Prodromal", 5));
    REQUIRE(stages_on(*model, "Prodromal", "Rash", 8));
    REQUIRE(stages_on(*model, "Rash", "Recovered", 10));
    REQUIRE(model->get_db().get_today_status("Free") == static_cast< int >(n));

    // (b) Quarantine does not change it. Quarantining every agent on day 2
    // (latent): the stages change on the same days. The quarantined agents
    // are isolated the day of rash onset (the stage hook, same step), and
    // released the next day (isolation is off)
    model->add_globalevent(
        [](Model<> * m) -> void {
            for (auto & agent : m->get_agents())
                agent.change_status(*m, M::QUARANTINED);
        },
        "Quarantine everyone",
        2
    );

    model->run(20, 123);
    REQUIRE(statuses_on(*model, "Free", "Quarantined", 2));
    REQUIRE(stages_on(*model, "Latent", "Prodromal", 5));
    REQUIRE(stages_on(*model, "Prodromal", "Rash", 8));
    REQUIRE(statuses_on(*model, "Quarantined", "Isolated", 8));
    REQUIRE(statuses_on(*model, "Isolated", "Free", 9));
    REQUIRE(stages_on(*model, "Rash", "Recovered", 10));

    // No quarantined agent ever had rash
    std::vector< int > date, counts;
    std::vector< std::string > status, stage;
    model->get_db().get_hist_joint(0u, date, status, stage, counts);
    for (size_t i = 0u; i < counts.size(); ++i)
        if ((status[i] == "Quarantined") && (stage[i] == "Rash"))
            REQUIRE(counts[i] == 0);

    // The duration drawn on entry is kept: all left Latent 5 days after
    // entering it, quarantined or not
    const auto & nh = model->get_natural_history();
    for (const auto & a : model->get_agents())
    {
        REQUIRE(nh.entered(a, M::LATENT) == 0);
        REQUIRE(nh.entered(a, M::RASH) == 8);
    }

    // (c) Detection: every free agent with rash is detected the next day and
    // isolated. Isolation lasts 4 days from rash onset, so the agents
    // recover in isolation (day 10) and are released on day 12
    model = make(1.0, 0.0, 1.0, 4);
    model->run(20, 123);
    REQUIRE(statuses_on(*model, "Free", "Isolated", 9));
    REQUIRE(stages_on(*model, "Rash", "Recovered", 10));
    REQUIRE(statuses_on(*model, "Isolated", "Free", 12));

    // (d) Hospitalization: every agent with rash is admitted the next day,
    // and discharged (recovered) the day after
    model = make(1.0, 1.0, 2.0, -1);
    model->run(20, 123);
    REQUIRE(statuses_on(*model, "Free", "Hospitalized", 9));
    REQUIRE(statuses_on(*model, "Hospitalized", "Free", 10));
    REQUIRE(stages_on(*model, "Rash", "Recovered", 10));

    // (e) A case imported on day 3 by a global event (not by contact) counts
    // its latent period from that day: prodromal on day 8, rash on day 11
    model = make(0.0, 0.0, 2.0, -1);
    model->add_globalevent(
        [](Model<> * m) -> void {
            m->get_agent(0).set_virus(*m, m->get_virus(0));
        },
        "Import a case",
        3
    );
    model->run(20, 123);

    auto days = [&](const std::string & from, const std::string & to) {
        auto c = stage_transitions_by_day(*model, from, to);
        std::vector< int > out;
        for (size_t d = 0u; d < c.size(); ++d)
            for (int k = 0; k < c[d]; ++k)
                out.push_back(static_cast< int >(d));
        return out;
    };

    REQUIRE(days("Latent", "Prodromal") == std::vector< int >{8});
    REQUIRE(days("Prodromal", "Rash") == std::vector< int >{11});
    REQUIRE(days("Rash", "Recovered") == std::vector< int >{13});

}
