#include "tests.hpp"

using namespace epiworld;

// The post-sampling callback sees the same contacts whether the network pulls
// or pushes. With no transmission the model never changes, so every day must
// give the same sorted batches in both modes -- including the contacts that
// cannot transmit. With certain transmission on a ring the whole run is
// deterministic, and the modes must agree too.

namespace {

using Batches = std::vector< std::pair< size_t, std::vector< size_t > > >;

// Batches of each day, in dispatch order, with the contacts sorted
std::vector< Batches > collect(
    epimodels::ModelSIR<> & model,
    TransmissionMode mode,
    int ndays,
    TransmissionMode * used
)
{

    std::vector< Batches > days(static_cast< size_t >(ndays));

    model.set_transmission_mode(mode);
    model.verbose_off();
    model.set_post_sampling(
        [&days](Agent<> * p, const SampledContactsView & c, Model<> * m) -> void {
            std::vector< size_t > ids(c.begin(), c.end());
            std::sort(ids.begin(), ids.end());
            days[static_cast< size_t >(m->today() - 1)].emplace_back(
                p->get_id(), std::move(ids)
            );
        }
    );

    model.run(ndays, 331);
    *used = model.get_last_transmission_mode();

    return days;

}

}

EPIWORLD_TEST_CASE("Post-sampling callback - push and pull agree", "[post-sampling-push]") {

    // No transmission and no recovery: the state of the model never changes
    {

        auto run = [](TransmissionMode mode, TransmissionMode * used) {
            epimodels::ModelSIR<> model("Virus", 0.05, 0.0, 0.0);
            model.agents_smallworld(400, 6, false, 0.1);
            return collect(model, mode, 6, used);
        };

        TransmissionMode used_pull, used_push;
        auto pull = run(TransmissionMode::pull, &used_pull);
        auto push = run(TransmissionMode::push, &used_push);

        REQUIRE(used_pull == TransmissionMode::pull);
        REQUIRE(used_push == TransmissionMode::push);

        size_t total = 0u;
        for (size_t d = 0u; d < pull.size(); ++d)
        {
            INFO("day " << d + 1);
            REQUIRE(pull[d] == push[d]);
            total += pull[d].size();
        }

        // Carriers do have susceptible neighbors
        REQUIRE(total > 0u);

        // Dispatched in ascending infectious-agent order
        for (const auto & day : push)
            for (size_t k = 1u; k < day.size(); ++k)
                REQUIRE(day[k - 1u].first < day[k].first);

    }

    // Certain transmission on a ring: fully deterministic
    {

        auto run = [](TransmissionMode mode, TransmissionMode * used) {

            epimodels::ModelSIR<> model("Virus", 0.0, 1.0, 0.0);

            Virus<> v = model.get_virus(0);
            model.rm_virus(0);
            v.set_distribution(dist_virus<>(0));
            model.add_virus(v);

            std::vector< int > source, target;
            for (int i = 0; i < 12; ++i)
            {
                source.push_back(i);
                target.push_back((i + 1) % 12);
            }
            model.agents_from_edgelist(source, target, 12, false);

            return collect(model, mode, 8, used);

        };

        TransmissionMode used_pull, used_push;
        auto pull = run(TransmissionMode::pull, &used_pull);
        auto push = run(TransmissionMode::push, &used_push);

        REQUIRE(used_push == TransmissionMode::push);

        for (size_t d = 0u; d < pull.size(); ++d)
        {
            INFO("day " << d + 1);
            REQUIRE(pull[d] == push[d]);
        }

        // The first day: the seed meets both neighbors
        REQUIRE(push[0].size() == 1u);
        REQUIRE(push[0][0].first == 0u);
        REQUIRE(push[0][0].second == std::vector< size_t >({1u, 11u}));

    }

}
