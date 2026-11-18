#include "tests.hpp"

using namespace epiworld;

// A layer's schedule (Phase 4c). A traced school (within-group mixing) on
// weekdays only (five days on, two off, from day 1) logs no contacts on days
// 6, 7, 13, 14, ... and transmits nothing then, while the home layer goes on;
// a schedule from a function (every third day) does the same. A closure made
// by a global event takes the layer out on top of its schedule, and reopening
// gives the schedule back. The schedule is a setting: it lasts across runs,
// and setting it during a run is an error. A parameter cycle (a weekly
// contact rate, a weight on a three-day cycle) gives each day its value, and
// the same run as global events that set the parameters.

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

// The days of the transmissions (not the seeds) through each layer
std::array< std::set< int >, 2 > transmission_days(Model & model)
{
    std::vector< int > date, source, target, virus, expo, layer;
    model.get_db().get_transmissions(date, source, target, virus, expo, layer);

    std::array< std::set< int >, 2 > days;
    for (size_t k = 0u; k < source.size(); ++k)
        if (source[k] >= 0)
            days[static_cast< size_t >(layer[k])].insert(date[k]);
    return days;
}

bool weekday(int d) { return ((d - 1) % 7) < 5; }

// A model with a public layer whose contact rate and weight are parameters
std::unique_ptr< epimodels::ModelSIR > build_public()
{
    auto model = std::make_unique< epimodels::ModelSIR >(
        "Virus", N, 0.01, 0.05, 0.1
    );
    model->verbose_off();
    model->add_param(2.0, "Public contact rate");
    model->add_param(1.0, "Public weight");
    model->add_layer(layer::network("home", graph::smallworld(4, 0.1)));

    auto p = layer::mixing("public");
    p->set_groups(assign::labels(std::vector< size_t >(N, 0u)), 1u);
    p->set_contact_matrix({1.0});
    p->set_contact_rate("Public contact rate");
    p->set_weight("Public weight");
    model->add_layer(p);
    return model;
}

const std::vector< double > RATE = {2, 2, 2, 2, 2, 5, 5};
const std::vector< double > WEIGHT = {1.0, 0.5, 0.25};

void close_school(Model * m) { m->get_layer("school").close(); }
void reopen_school(Model * m) { m->get_layer("school").reopen(); }

}

EPIWORLD_TEST_CASE("A layer's schedule", "[layer-participation]") {

    // Weekdays only
    {
        auto s = school();
        s->set_schedule({true, true, true, true, true, false, false});
        REQUIRE(s->has_schedule());
        REQUIRE(s->is_scheduled(5));
        REQUIRE_FALSE(s->is_scheduled(6));
        REQUIRE(s->is_scheduled(8));

        auto model = build(s);
        for (int run = 0; run < 2; ++run)
        {
            model->run(DAYS, 11 + run);

            const std::set< int > logged = days_logged(*s);
            const auto by_layer = transmission_days(*model);
            for (int d = 1; d <= DAYS; ++d)
            {
                REQUIRE(logged.count(d) == (weekday(d) ? 1u : 0u));
                if (!weekday(d))
                    REQUIRE(by_layer[1].count(d) == 0u);
            }
            REQUIRE(by_layer[1].size() > 5u);

            // The home layer has no schedule
            bool home_weekend = false;
            for (int d : by_layer[0])
                home_weekend = home_weekend || !weekday(d);
            REQUIRE(home_weekend);
        }

        // A setting, made between runs
        model->add_globalevent(
            [](Model * m) { m->get_layer("school").clear_schedule(); },
            "Clear", 3
        );
        REQUIRE_THROWS_WITH(
            model->run(5, 1), Catch::Matchers::Contains("between runs")
        );
    }

    // From a function, and a closure on top of it
    {
        auto s = school();
        s->set_schedule([](int d) { return (d % 3) == 0; });
        auto model = build(s);
        model->add_globalevent(close_school, "Close", 10);
        model->add_globalevent(reopen_school, "Reopen", 20);
        model->run(DAYS, 5);

        const std::set< int > logged = days_logged(*s);
        for (int d = 1; d <= DAYS; ++d)
        {
            const bool open = ((d % 3) == 0) && ((d <= 10) || (d > 20));
            REQUIRE(logged.count(d) == (open ? 1u : 0u));
        }
        REQUIRE(s->is_closed() == ((DAYS + 1) % 3 != 0));

        // Without the schedule, every day
        s->clear_schedule();
        model->rm_globalevent("Close");
        model->rm_globalevent("Reopen");
        model->run(DAYS, 5);
        REQUIRE(days_logged(*s).size() == static_cast< size_t >(DAYS));
    }

    // A parameter cycle, and the same run from global events
    {
        auto cycled = build_public();
        cycled->set_param_cycle("Public contact rate", RATE);
        cycled->set_param_cycle("Public weight", WEIGHT);
        REQUIRE(cycled->has_param_cycle("Public weight"));
        REQUIRE_FALSE(cycled->has_param_cycle("Recovery rate"));

        // The value of each day, seen by a global event of that day
        std::vector< double > rate, weight;
        cycled->add_globalevent([&](Model * m) {
            rate.push_back(m->get_param("Public contact rate"));
            weight.push_back(m->get_param("Public weight"));
        }, "Record");

        auto by_hand = build_public();
        by_hand->add_globalevent([](Model * m) {
            // At the end of day d, the values of day d + 1
            const auto d = static_cast< size_t >(m->today());
            m->set_param("Public contact rate", RATE[d % RATE.size()]);
            m->set_param("Public weight", WEIGHT[d % WEIGHT.size()]);
        }, "Cycle");

        for (int run = 0; run < 2; ++run)
        {
            rate.clear();
            weight.clear();
            cycled->run(DAYS, 7 + run);

            // By hand, day 1's values are set before each run
            by_hand->set_param("Public contact rate", RATE[0]);
            by_hand->set_param("Public weight", WEIGHT[0]);
            by_hand->run(DAYS, 7 + run);

            REQUIRE(rate.size() == static_cast< size_t >(DAYS));
            REQUIRE(cycled->get_param("Public contact rate") == rate.back());
            for (int d = 1; d <= DAYS; ++d)
            {
                const auto k = static_cast< size_t >(d - 1);
                REQUIRE(rate[k] == (weekday(d) ? 2.0 : 5.0));
                REQUIRE(weight[k] == WEIGHT[k % 3u]);
            }

            std::vector< int > date, source, target, virus, expo, layer;
            cycled->get_db().get_transmissions(date, source, target, virus, expo, layer);
            std::vector< int > date2, source2, target2, virus2, expo2, layer2;
            by_hand->get_db().get_transmissions(date2, source2, target2, virus2, expo2, layer2);
            REQUIRE(date.size() > 50u);
            REQUIRE(date == date2);
            REQUIRE(source == source2);
            REQUIRE(target == target2);
            REQUIRE(layer == layer2);
        }

        // Without the cycles, the parameters keep their values
        cycled->clear_param_cycle("Public contact rate");
        cycled->clear_param_cycle("Public weight");
        cycled->set_param("Public contact rate", 3.0);
        rate.clear();
        cycled->run(10, 1);
        REQUIRE(rate == std::vector< double >(10u, 3.0));

        // Errors; a cycle is a setting, made between runs
        REQUIRE_THROWS_AS(
            cycled->set_param_cycle("Public weight", {}), std::length_error
        );
        REQUIRE_THROWS_WITH(
            cycled->set_param_cycle("No such parameter", {1.0}),
            Catch::Matchers::Contains("does not exist")
        );
        cycled->add_globalevent([](Model * m) {
            m->set_param_cycle("Public weight", {1.0});
        }, "Set", 3);
        REQUIRE_THROWS_WITH(
            cycled->run(5, 1), Catch::Matchers::Contains("between runs")
        );
    }

    // Errors
    auto s = school();
    REQUIRE_THROWS_AS(s->set_schedule(std::vector< bool >{}), std::length_error);
    REQUIRE_THROWS_AS(s->set_schedule(std::function< bool(int) >()), std::invalid_argument);

}
