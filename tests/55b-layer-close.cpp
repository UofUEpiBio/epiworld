#include "tests.hpp"

using namespace epiworld;

// Closing a layer (Phase 4a). Global events close the school (a traced
// within-group mixing layer) at the end of day 5 and reopen it at the end of
// day 15: it has no contacts on days 6 to 15, so it logs none, transmits
// nothing, and those days are not shared in groupmates_of(). Events on the
// same day run in the order they were added: "close, reopen" on day 20 leaves
// it open, "reopen, close" on day 25 closes it. A closure made during a run
// lasts that run; one made between runs, every run, until reopened.

namespace {

constexpr size_t N = 1000u;
constexpr int DAYS = 30;

std::shared_ptr< layer::Mixing > school()
{
    auto s = layer::mixing("school");
    std::vector< size_t > groups(N);
    for (size_t i = 0u; i < N; ++i)
        groups[i] = i / 25u;
    s->set_groups(assign::labels(groups), N / 25u);
    s->set_contact_prob(0.3);
    s->enable_tracing(1000u);
    return s;
}

// Slow epidemic, so there are carriers all along
std::unique_ptr< epimodels::ModelSIR > build(std::shared_ptr< layer::Mixing > s)
{
    auto model = std::make_unique< epimodels::ModelSIR >(
        "Virus", N, 0.05, 0.05, 0.02
    );
    model->verbose_off();
    model->add_layer(layer::network("home", graph::smallworld(4, 0.1)));
    model->add_layer(s);
    return model;
}

// The days with at least one contact in the layer's log
std::set< int > days_logged(ContactLayer & layer)
{
    std::set< int > days;
    auto & log = layer.get_tracing();
    for (size_t a = 0u; a < log.get_n_agents(); ++a)
        for (size_t k = 0u; k < std::min(log.get_n_contacts(a), log.get_max_contacts()); ++k)
            days.insert(log.get_contact(a, k).second);
    return days;
}

void close_school(Model * m) { m->get_layer("school").close(); }
void reopen_school(Model * m) { m->get_layer("school").reopen(); }

}

EPIWORLD_TEST_CASE("Closing a layer", "[layer-participation]") {

    auto s = school();
    auto model = build(s);
    model->add_globalevent(close_school, "Close", 5);
    model->add_globalevent(reopen_school, "Reopen", 15);
    model->add_globalevent(close_school, "Close, then", 20);
    model->add_globalevent(reopen_school, "reopen", 20);
    model->add_globalevent(reopen_school, "Reopen, then", 25);
    model->add_globalevent(close_school, "close", 25);
    model->run(DAYS, 77);

    // Closed on days 6 to 15 and from day 26; open on the days around
    const std::set< int > days = days_logged(*s);
    for (int d = 6; d <= 15; ++d)
        REQUIRE(days.count(d) == 0u);
    for (int d = 26; d <= DAYS; ++d)
        REQUIRE(days.count(d) == 0u);
    for (int d : {5, 16, 20, 21, 25})
        REQUIRE(days.count(d) == 1u);
    REQUIRE(s->is_closed());

    // No transmission through it while closed, some while open
    std::vector< int > date, source, target, virus, expo, layer;
    model->get_db().get_transmissions(date, source, target, virus, expo, layer);
    size_t n_open = 0u;
    for (size_t k = 0u; k < date.size(); ++k)
        if (layer[k] == static_cast< int >(s->get_id()))
        {
            const bool closed = ((date[k] >= 6) && (date[k] <= 15)) || (date[k] >= 26);
            REQUIRE_FALSE(closed);
            ++n_open;
        }
    REQUIRE(n_open > 0u);

    // Days closed are not days shared: the window of days 6 to 15 has no
    // groupmates, and no one shares more than the 15 open days
    std::vector< layer::Mixing::Groupmate > mates;
    for (size_t a = 0u; a < N; a += 37u)
    {
        s->groupmates_of(a, 6, 15, mates);
        REQUIRE(mates.empty());

        s->groupmates_of(a, 1, DAYS, mates);
        REQUIRE_FALSE(mates.empty());
        for (const auto & m : mates)
        {
            REQUIRE(m.n_days <= 15);
            REQUIRE(m.last_day <= 25);
        }
    }

    // The closure lasted the run: the next one starts open
    model->run(3, 77); // Before day 5, when the events close it again
    REQUIRE_FALSE(s->is_closed());
    REQUIRE(days_logged(*s).count(1) == 1u);

    // Closed between runs: closed in every run, until reopened
    {
        auto s2 = school();
        auto m2 = build(s2);
        s2->close();
        for (int rep = 0; rep < 2; ++rep)
        {
            m2->run(10, 5 + rep);
            REQUIRE(s2->is_closed());
            REQUIRE(days_logged(*s2).empty());
        }

        s2->reopen();
        m2->run(10, 7);
        REQUIRE_FALSE(s2->is_closed());
        REQUIRE_FALSE(days_logged(*s2).empty());
    }

}
