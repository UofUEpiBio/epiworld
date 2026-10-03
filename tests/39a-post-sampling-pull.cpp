#include "tests.hpp"

using namespace epiworld;

// The post-sampling callback in a network that pulls. A ring of 8 agents with
// certain transmission is fully deterministic: the batches, their order, and
// when they are delivered can all be checked. A no-op callback must not change
// a run.

namespace {

struct Batch {
    int day;
    size_t infectious;
    std::vector< size_t > contacts;
    bool state_ok; // Nobody had changed state yet
};

epimodels::ModelSIR<> make_ring()
{

    epimodels::ModelSIR<> model("Virus", 0.0, 1.0, 0.0);

    Virus<> v = model.get_virus(0);
    model.rm_virus(0);
    v.set_distribution([](Virus<> & virus, Model<> * m) -> void {
        m->get_agents()[2].set_virus(*m, virus);
        m->get_agents()[6].set_virus(*m, virus);
    });
    model.add_virus(v);

    std::vector< int > source, target;
    for (int i = 0; i < 8; ++i)
    {
        source.push_back(i);
        target.push_back((i + 1) % 8);
    }
    model.agents_from_edgelist(source, target, 8, false);
    model.set_transmission_mode(TransmissionMode::pull);
    model.verbose_off();

    return model;

}

}

EPIWORLD_TEST_CASE("Post-sampling callback - network pull", "[post-sampling-pull]") {

    auto model = make_ring();

    std::vector< Batch > batches;
    std::vector< std::string > log;

    model.set_post_sampling(
        [&](Agent<> * p, const SampledContactsView & contacts, Model<> * m) -> void {

            Batch b;
            b.day = m->today();
            b.infectious = p->get_id();
            for (auto c : contacts)
                b.contacts.push_back(c);
            std::sort(b.contacts.begin(), b.contacts.end());

            // The state the sampling saw: the contacted agents are still
            // susceptible, since the events were not applied yet.
            b.state_ok = (p->get_state() == 1u);
            for (auto c : b.contacts)
                b.state_ok = b.state_ok && (m->get_agent(c).get_state() == 0u);

            batches.push_back(b);
            log.push_back("cb" + std::to_string(m->today()));

        }
    );

    model.add_globalevent(
        [&](Model<> * m) -> void { log.push_back("ge" + std::to_string(m->today())); },
        "log"
    );

    REQUIRE(model.has_post_sampling());
    model.run(4, 123);

    // Day 1: agents 2 and 6 meet their neighbors. Day 2: the new cases
    // (1, 3, 5, 7) are infectious; 0 and 4 are each reached by two of them.
    // After that nobody is left, and the empty batches are never delivered.
    REQUIRE(batches.size() == 6u);

    const std::vector< size_t > who      = {2, 6, 1, 3, 5, 7};
    const std::vector< int > day         = {1, 1, 2, 2, 2, 2};
    const std::vector< std::vector< size_t > > contacts = {
        {1, 3}, {5, 7}, {0}, {4}, {4}, {0}
    };

    for (size_t k = 0u; k < batches.size(); ++k)
    {
        INFO("batch " << k);
        CHECK(batches[k].infectious == who[k]);
        CHECK(batches[k].day == day[k]);
        CHECK(batches[k].contacts == contacts[k]);
        CHECK(batches[k].state_ok);
    }

    // Callbacks run once per step, before the global events
    const std::vector< std::string > expected = {
        "cb1", "cb1", "ge1",
        "cb2", "cb2", "cb2", "cb2", "ge2",
        "ge3", "ge4"
    };
    CHECK(log == expected);

    // Removing the callback
    model.clear_post_sampling();
    REQUIRE_FALSE(model.has_post_sampling());
    batches.clear();
    model.run(4, 123);
    CHECK(batches.empty());

    // A callback can clear or replace itself: the step in progress finishes on
    // the callback it started with, the change applies from the next step
    {

        auto self_clearing = make_ring();
        size_t calls = 0u;
        self_clearing.set_post_sampling(
            [&calls](Agent<> *, const SampledContactsView &, Model<> * m) -> void {
                calls++;
                m->clear_post_sampling();
            }
        );
        self_clearing.run(1, 123);

        // Both pending batches (agents 2 and 6) were delivered
        CHECK(calls == 2u);
        CHECK_FALSE(self_clearing.has_post_sampling());

        auto replacing = make_ring();
        size_t calls_a = 0u, calls_b = 0u;
        replacing.set_post_sampling(
            [&](Agent<> *, const SampledContactsView &, Model<> * m) -> void {
                calls_a++;
                m->set_post_sampling(
                    [&calls_b](Agent<> *, const SampledContactsView &, Model<> *) -> void {
                        calls_b++;
                    }
                );
            }
        );
        replacing.run(2, 123);

        // Day 1 (two batches) on the original, day 2 (four batches) on the new one
        CHECK(calls_a == 2u);
        CHECK(calls_b == 4u);

    }

    // A no-op callback does not change a run (the observable proxy for "a
    // callback that is not used costs nothing")

    auto run = [](bool hook) -> std::vector< int > {

        epimodels::ModelSEIR<> model("Virus", 0.02, 0.4, 3.0, 0.2);
        model.agents_smallworld(2000, 6, false, 0.1);
        model.set_transmission_mode(TransmissionMode::pull);
        model.verbose_off();

        if (hook)
            model.set_post_sampling(
                [](Agent<> *, const SampledContactsView &, Model<> *) -> void {}
            );

        model.run(40, 331);

        std::vector< int > h;
        model.get_db().get_hist_total(nullptr, nullptr, &h);
        return h;

    };

    // Same history, day by day, with and without a callback installed
    REQUIRE(run(false) == run(true));

}
