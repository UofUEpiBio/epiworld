#include "tests.hpp"

using namespace epiworld;

// A within-group mixing layer keeps the history of who was in which group,
// and who was absent, and answers groupmates_of(agent, d1, d2). Moves made by
// a global event on day d count from day d + 1, as do absences (agents in the
// model's excluded states). In a quarantine model with households, the
// history matches presence recomputed from the agents' states day by day, and
// every contact in the layer's log is a groupmate that day.
namespace {

using Mates = std::map< size_t, std::pair< int, int > >; // last day, days

Mates mates(const layer::Mixing<> & l, size_t agent, int d1, int d2)
{
    Mates out;
    for (const auto & m : l.groupmates_of(agent, d1, d2))
        out[m.agent] = {m.last_day, m.n_days};
    return out;
}

}

EPIWORLD_TEST_CASE("Mixing layer - group history", "[layer-mixing-history]") {

    // -- moves ----------------------------------------------------------------
    // Two households of three; agent 2 moves to the second on day 3, and
    // agent 4 leaves the layer on day 5.
    epimodels::ModelSIR<> sir("flu", 6u, 0.0, 0.5, 0.1);
    sir.verbose_off();

    auto hh = layer::mixing<>("households");
    hh->set_groups({0, 0, 0, 1, 1, 1}, 2u);
    hh->set_contact_prob(1.0);
    sir.add_layer(hh);

    using L = layer::Mixing<>;
    sir.add_globalevent([](Model<> * m) {
        m->get_layer_as< L >("households").set_group(2u, 1u);
    }, "move", 3);
    sir.add_globalevent([](Model<> * m) {
        m->get_layer_as< L >("households").set_group(4u, L::NO_GROUP);
    }, "leave", 5);

    sir.run(8, 1);

    const Mates moved = {
        {0u, {3, 3}}, {1u, {3, 3}}, {3u, {8, 5}}, {4u, {5, 2}}, {5u, {8, 5}}
    };
    const Mates left = {{2u, {5, 2}}, {3u, {5, 5}}, {5u, {5, 5}}};
    const Mates window = {{3u, {6, 2}}, {4u, {5, 1}}, {5u, {6, 2}}};
    const Mates stayed = {{1u, {8, 5}}};

    REQUIRE(mates(*hh, 2u, 1, 8) == moved);
    REQUIRE(mates(*hh, 4u, 1, 8) == left);
    REQUIRE(mates(*hh, 2u, 5, 6) == window);
    REQUIRE(mates(*hh, 0u, 4, 8) == stayed);

    // Days after today are not known yet
    REQUIRE(mates(*hh, 0u, 1, 100) == mates(*hh, 0u, 1, 8));

    // Only within groups
    auto everyone = layer::mixing<>("everyone");
    everyone->set_groups(std::vector< size_t >(6u, 0u), 1u);
    everyone->set_contact_matrix({1.0});
    sir.add_layer(everyone);
    sir.run(2, 1);
    REQUIRE_THROWS(everyone->groupmates_of(0u, 0, 2));

    // -- absences ---------------------------------------------------------------
    // Households of four in a quarantine model: the isolated, quarantined and
    // hospitalized are absent.
    const size_t n = 400u;
    const int ndays = 40;
    using Q = epimodels::ModelSEIRQuarantine<>;

    Q model(
        "v", n, 0.05,
        0.3,     // transmission_prob
        3.0,     // latent_period
        1.0 / 7, // recovery_rate
        0.05,    // hospitalization_rate
        7.0,     // hospitalization_period
        2.0,     // detection_delay
        7,       // quarantine_period
        0.8,     // quarantine_willingness
        0.8,     // isolation_willingness
        7        // isolation_period
    );
    model.verbose_off();

    std::vector< size_t > groups(n);
    for (size_t i = 0u; i < n; ++i)
        groups[i] = i / 4u;

    auto homes = layer::mixing<>("households");
    homes->set_groups(groups, n / 4u);
    homes->set_contact_prob(1.0);
    model.add_layer(homes);

    // Presence on day d + 1, from the states at the end of day d (the
    // model's own quarantine process runs first)
    std::vector< std::vector< bool > > present(ndays + 2, std::vector< bool >(n, true));
    model.add_globalevent([&present](Model<> * m) {
        for (const auto & a : m->get_agents())
        {
            const int s = static_cast< int >(a.get_state());
            present[m->today() + 1][a.get_id()] =
                (s < Q::ISOLATED) || (s > Q::HOSPITALIZED);
        }
    }, "presence");

    model.run(ndays, 2026);

    size_t n_absent_days = 0u;
    for (size_t i = 0u; i < n; ++i)
    {
        Mates expected;
        for (size_t j = (i / 4u) * 4u; j < (i / 4u) * 4u + 4u; ++j)
        {
            if (j == i)
                continue;

            for (int d = 1; d <= ndays; ++d)
                if (present[d][i] && present[d][j])
                {
                    expected[j].first = d;
                    ++expected[j].second;
                }
        }

        for (int d = 1; d <= ndays; ++d)
            n_absent_days += !present[d][i];

        REQUIRE(mates(*homes, i, 1, ndays) == expected);
    }

    // The run had absences
    REQUIRE(n_absent_days > 0u);

    // Every logged contact is a groupmate that day
    auto & log = homes->get_tracing();
    size_t n_logged = 0u;
    for (size_t i = 0u; i < n; ++i)
        for (const auto & rec : log.get_contacts(i))
            for (int d : rec.get_times())
            {
                ++n_logged;
                REQUIRE(mates(*homes, i, d, d).count(rec.get_contact_id()) == 1u);
            }

    REQUIRE(n_logged > 0u);

}
