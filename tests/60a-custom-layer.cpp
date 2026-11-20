#include "tests.hpp"

using namespace epiworld;

// A contact layer of the user's own: each agent meets `k` agents drawn at
// random every day. It derives from ContactLayer and implements reset(),
// update(), sample() and clone(), and nothing else (the example of
// docs/impl/contact-layers.md). End to end:
//
// - On day 1, with one carrier among N agents, the infections are
//   1 - (1 - p / N)^k per agent, about k p in all.
// - Agents that carry the virus but are not infectious (exposed, in an SEIR
//   with a latent period longer than the run) are not sources:
//   ContactLayer::is_source() applies the mask sample() receives.
// - The layer cannot push, so the model pulls even when asked to push.
// - Transmissions record the layer, the post-sampling callback and the
//   layer's log see its contacts, and run_multiple() clones it.

namespace {

class RandomEncounters : public ContactLayer {
private:
    size_t k;

public:
    RandomEncounters(std::string name, size_t k_) :
        ContactLayer(std::move(name)), k(k_) {}

    // The layer keeps nothing across days: nothing to restore
    void reset(Model &) override { mark_clean(); }
    void update(Model &) override {}

    size_t sample(
        size_t agent,
        Model & model,
        const std::vector< bool > * exclude,
        std::vector< size_t > & out
    ) override
    {
        const size_t n_agents = model.size();
        size_t n = 0u;
        for (size_t draw = 0u; draw < k; ++draw)
        {
            const size_t j = std::min(
                static_cast< size_t >(model.runif() * n_agents), n_agents - 1u
            );
            if ((j == agent) || !is_source(j, exclude))
                continue;
            if (model.has_post_sampling())
                model.register_sampled_contact(j, agent);
            trace(j, agent);
            out.push_back(j);
            ++n;
        }
        return n;
    }

    std::shared_ptr< ContactLayer > clone() const override
    {
        return std::make_shared< RandomEncounters >(*this);
    }
};

constexpr size_t N = EPI_TEST_SIZE(2000u, 500u);
constexpr size_t REPS = EPI_TEST_SIZE(400u, 100u);

}

EPIWORLD_TEST_CASE("A contact layer of your own", "[layers]") {

    const size_t k = 10u;
    const double p = 0.5;

    // Day 1, one carrier: about k p infections, pulled even when pushing is
    // asked for
    double total = 0.0;
    for (size_t r = 0u; r < REPS; ++r)
    {
        epimodels::ModelSIR model(
            "flu", N, 1.5 / static_cast< double >(N), p, 0.0
        );
        model.verbose_off();
        model.add_layer(std::make_shared< RandomEncounters >("encounters", k));
        model.set_transmission_mode("push");
        model.run(1, static_cast< int >(r + 1u));
        REQUIRE(model.get_last_transmission_mode() == TransmissionMode::pull);

        std::vector< int > date, source, target, virus, expo, layer;
        model.get_db().get_transmissions(date, source, target, virus, expo, layer);
        for (size_t t = 0u; t < source.size(); ++t)
            if (source[t] >= 0)
            {
                REQUIRE(layer[t] == 0);
                total += 1.0;
            }
    }

    const double expected = static_cast< double >(N - 1u) *
        (1.0 - std::pow(1.0 - p / static_cast< double >(N), static_cast< double >(k)));
    REQUIRE_THAT(
        total / static_cast< double >(REPS),
        Catch::Matchers::WithinAbs(expected, 4.0 * std::sqrt(expected / REPS))
    );

    // Exposed agents carry the virus but are not infectious: with a latent
    // period far longer than the run, nobody is infected through the layer
    {
        epimodels::ModelSEIR model("flu", N, 0.1, 1.0, 1e6, 0.0);
        model.verbose_off();
        model.add_layer(std::make_shared< RandomEncounters >("encounters", k));
        model.run(10, 3);
        std::vector< int > date, source, target, virus, expo, layer;
        model.get_db().get_transmissions(date, source, target, virus, expo, layer);
        for (int s : source)
            REQUIRE(s < 0);
    }

    // A full run: the callback and the layer's log see the contacts, and
    // run_multiple() clones the layer into each replicate
    {
        epimodels::ModelSIR model("flu", N, 0.01, 0.1, 1.0 / 7.0);
        model.verbose_off();
        auto layer = std::make_shared< RandomEncounters >("encounters", k);
        layer->enable_tracing(50u);
        model.add_layer(layer);

        size_t reported = 0u;
        model.set_post_sampling(
            [&reported](Agent *, const SampledContactsView & c, Model *) {
                reported += c.size();
            }
        );
        model.run(30, 7);
        REQUIRE(reported > 0u);

        auto & log = model.get_layer("encounters").get_tracing();
        size_t logged = 0u;
        for (size_t a = 0u; a < N; ++a)
            logged += log.get_n_contacts(a);
        REQUIRE(logged > 0u);

        model.clear_post_sampling();
        std::vector< double > sizes;
        model.run_multiple(30, 4, 11, [&sizes](size_t, Model * m) {
            std::vector< int > counts;
            m->get_db().get_today_total(nullptr, nullptr, &counts);
            sizes.push_back(static_cast< double >(counts[2]));
        }, true, false, 1);
        REQUIRE(sizes.size() == 4u);
        for (double s : sizes)
            REQUIRE(s > 0.0);
    }

}
