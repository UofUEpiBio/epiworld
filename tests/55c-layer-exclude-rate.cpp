#include "tests.hpp"

using namespace epiworld;

// A mixing layer's contact rate stays constant when agents leave it (D2,
// Phase 4a): half of the population (carriers and susceptibles alike) goes
// "Away", a status the community excludes. A susceptible still present meets
// as many carriers a day as with everybody there (the rate is divided by the
// agents present, not by the population), and the agents away have no
// contacts in the layer at all, in either direction. No one is infected
// (transmission 0), so the counts hold all along.

namespace {

constexpr size_t N = 2000u;
constexpr double RATE = 4.0;
constexpr int DAYS = 10;

struct Measured {
    double rate;     ///< Carriers met per present susceptible per day
    double expected; ///< RATE x the share of carriers among those present
    bool away_seen;  ///< Whether anyone away is in the log
};

// Over days 3 to DAYS (the agents leave at the end of day 1)
Measured contact_rate(bool away)
{

    epimodels::ModelSIR model("Virus", N, 0.5, 0.0, 0.0);
    model.verbose_off();
    model.add_param(RATE, "Contacts");

    auto mix = layer::mixing("community");
    mix->set_groups(assign::labels(std::vector< size_t >(N, 0u)), 1u);
    mix->set_contact_matrix({1.0});
    mix->set_contact_rate("Contacts");
    mix->enable_tracing(1000u);
    mix->exclude_status("Away");
    model.add_layer(mix);

    Statuses st;
    st.add_status("Free");
    st.add_status("Away");
    st.external(0, 1, "Leave");
    model.set_statuses(st);

    // Every other agent leaves at the end of day 1
    if (away)
        model.add_globalevent([](Model * m) {
            const int leave = m->transition_id("Leave");
            for (size_t i = 0u; i < N; i += 2u)
                m->fire(leave, m->get_agent(i));
        }, "Leave", 1);

    model.run(DAYS, 2024);

    const size_t step = away ? 2u : 1u;
    auto & log = mix->get_tracing();
    size_t n_contacts = 0u;
    bool away_seen = false;
    for (size_t a = 0u; a < N; ++a)
        for (size_t k = 0u; k < log.get_n_contacts(a); ++k)
        {
            const auto contact = log.get_contact(a, k);
            if (contact.second < 3)
                continue;

            away_seen = away_seen ||
                (away && (((a % 2u) == 0u) || ((contact.first % 2u) == 0u)));
            ++n_contacts;
        }

    // Those present (no one is infected or recovers)
    size_t n_present = 0u, n_susceptible = 0u;
    for (size_t i = away ? 1u : 0u; i < N; i += step)
    {
        ++n_present;
        n_susceptible += (model.get_agent(i).get_virus() == nullptr) ? 1u : 0u;
    }

    const double n_carriers = static_cast< double >(n_present - n_susceptible);
    return {
        static_cast< double >(n_contacts) /
            static_cast< double >(n_susceptible * (DAYS - 2)),
        RATE * n_carriers / static_cast< double >(n_present),
        away_seen
    };

}

}

EPIWORLD_TEST_CASE("A mixing layer's rate with agents away", "[layer-participation]") {

    const auto everyone = contact_rate(false);
    const auto half = contact_rate(true);

    // RATE contacts a day, of which the carriers' share among those present
    // (dividing by the whole population would give half of it with half away)
    REQUIRE_THAT(everyone.rate, Catch::Matchers::WithinRel(everyone.expected, 0.05));
    REQUIRE_THAT(half.rate, Catch::Matchers::WithinRel(half.expected, 0.05));
    REQUIRE_FALSE(everyone.away_seen);
    REQUIRE_FALSE(half.away_seen);

}
