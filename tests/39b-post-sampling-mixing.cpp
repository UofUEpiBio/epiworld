#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

// The post-sampling callback in the mixing models. The built-in models with
// contact tracing record contacts through the callback they install: every
// sampled contact must end up in the tracing exactly once, and the tracing
// must still drive the quarantine.

namespace {

struct Delivered {
    std::vector< size_t > per_agent; // Contacts delivered, by infectious agent
    size_t duplicates = 0u;          // Batches that repeat a contacted agent
};

// Wraps the tracing callback of the model so the deliveries can be counted
template<typename TModel>
void count_deliveries(TModel & model, Delivered & d)
{

    d.per_agent.assign(model.size(), 0u);
    auto trace = make_contact_tracing_post_sampling<>();

    model.set_post_sampling(
        [&d, trace](Agent<> * p, const SampledContactsView & c, Model<> * m) -> void {

            trace(p, c, m);

            d.per_agent[p->get_id()] += c.size();

            std::vector< size_t > ids(c.begin(), c.end());
            std::sort(ids.begin(), ids.end());
            if (std::adjacent_find(ids.begin(), ids.end()) != ids.end())
                d.duplicates++;

        }
    );

}

template<typename TModel>
size_t quarantined_ever(TModel & model)
{

    std::vector< std::string > states;
    std::vector< int > counts;
    model.get_db().get_hist_total(nullptr, &states, &counts);

    size_t n = 0u;
    for (size_t i = 0u; i < states.size(); ++i)
        if (states[i].find("Quarantined") != std::string::npos)
            n += static_cast< size_t >(counts[i]);

    return n;

}

}

EPIWORLD_TEST_CASE("Post-sampling callback - mixing and tracing", "[post-sampling-mixing]") {

    // --- SEIR with mixing and quarantine -----------------------------------
    {

        std::vector< double > contact_matrix = {
            20.0, 2.0,
            2.0, 20.0
        };

        epimodels::ModelSEIRMixingQuarantine<> model(
            "Flu", 2000, 0.01, 0.6, 2.0, 0.25, contact_matrix,
            0.1, 5, 2, 4, 0.9, 1.0, 10, 1.0, 4
        );

        model.add_entity(Entity<>("A", dist_factory<>(0, 1000)));
        model.add_entity(Entity<>("B", dist_factory<>(1000, 2000)));

        Delivered d;
        count_deliveries(model, d);

        model.verbose_off();
        model.run(40, 1231);

        // Every contact delivered is in the tracing, once
        auto & ct = model.get_contact_tracing();
        size_t total = 0u;
        for (size_t i = 0u; i < model.size(); ++i)
        {
            REQUIRE(ct.get_n_contacts(i) == d.per_agent[i]);
            total += d.per_agent[i];
        }

        REQUIRE(total > 0u);

        // Contacts are a multiset: repeated draws are kept
        REQUIRE(d.duplicates > 0u);

        // And the tracing drives the quarantine
        REQUIRE(quarantined_ever(model) > 0u);

    }

    // --- Measles with mixing: each contact recorded once -------------------
    {

        const size_t n = 2000u;
        measles::ModelMeaslesMixing<> model(
            n, 10.0 / n, 0.2, 0.9, 0.3, 7.0, 4.0, 5.0, {2.0},
            0.2, 7.0, 3.0, 21, .8, .8, 4, 0.0, 1.0, 4u
        );

        model.add_entity(Entity<>("Population", dist_factory<>(0, n)));
        model.get_virus(0).set_distribution(
            [](Virus<> & v, Model<> * m) -> void {
                for (int i = 0; i < 10; ++i)
                    m->get_agents()[i].set_virus(*m, v);
            }
        );

        Delivered d;
        count_deliveries(model, d);

        model.verbose_off();
        model.run(60, 1231);

        auto & ct = model.get_contact_tracing();
        size_t total = 0u;
        for (size_t i = 0u; i < model.size(); ++i)
        {
            REQUIRE(ct.get_n_contacts(i) == d.per_agent[i]);
            total += d.per_agent[i];
        }

        REQUIRE(total > 0u);

        // The outbreak still spreads and the quarantine still happens
        REQUIRE(quarantined_ever(model) > 0u);

    }

}
