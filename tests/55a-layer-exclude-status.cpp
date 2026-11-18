#include "tests.hpp"

using namespace epiworld;

// A layer can exclude the agents in a status (Phase 4a): the infected are
// isolated as soon as they enter Infected (a reaction; the seeds too), and the
// community layer excludes "Isolated" while the households keep them. Every
// infection is then in a household; without the exclusion, the community
// transmits too. Checked with the community as a network (pulled and pushed)
// and as a mixing layer, with and without the queue. A status the model does
// not have is an error when the run starts, and the excluded statuses are set
// between runs only.

namespace {

constexpr size_t N = 900u;

// Households: cliques of three
std::shared_ptr< layer::Network > households()
{
    std::vector< int > src, tgt;
    for (int h = 0; h < static_cast< int >(N); h += 3)
        for (int a = h; a < h + 3; ++a)
            for (int b = a + 1; b < h + 3; ++b)
            {
                src.push_back(a);
                tgt.push_back(b);
            }

    return layer::network(
        "household", AdjList(src, tgt, static_cast< int >(N), false)
    );
}

std::shared_ptr< ContactLayer > community(bool mixing, bool exclude)
{
    std::shared_ptr< ContactLayer > layer;
    if (mixing)
    {
        auto mix = layer::mixing("community");
        mix->set_groups(assign::labels(std::vector< size_t >(N, 0u)), 1u);
        mix->set_contact_matrix({1.0});
        mix->set_contact_rate("Community contacts");
        layer = mix;
    }
    else
        layer = layer::network("community", graph::smallworld(6, 0.2));

    if (exclude)
        layer->exclude_status("Isolated");
    return layer;
}

// SIR with certain transmission; the infected are isolated on infection
std::unique_ptr< epimodels::ModelSIR > build(
    bool mixing,
    const char * mode,
    bool queue,
    bool exclude = true
)
{
    auto model = std::make_unique< epimodels::ModelSIR >(
        "Virus", N, 0.01, 1.0, 0.25
    );
    model->verbose_off();
    model->add_param(4.0, "Community contacts");
    model->add_layer(households());
    model->add_layer(community(mixing, exclude));

    Statuses st;
    st.add_status("Free");
    st.add_status("Isolated");
    st.external(0, 1, "Isolate");
    model->set_statuses(st);
    model->react(Trigger::entered(1), model->transition_id("Isolate"));

    model->set_transmission_mode(mode);
    if (!queue)
        model->queuing_off();

    return model;
}

}

EPIWORLD_TEST_CASE("A layer excludes a status", "[layer-participation]") {

    for (bool mixing : {false, true})
        for (const char * mode : {"pull", "push"})
            for (bool queue : {true, false})
                for (bool exclude : {true, false})
                {

                    // A mixing layer is pulled
                    if (mixing && (std::string(mode) == "push"))
                        continue;

                    auto model = build(mixing, mode, queue, exclude);
                    model->run(40, 123);

                    std::vector< int > date, source, target, virus, expo, layer;
                    model->get_db().get_transmissions(
                        date, source, target, virus, expo, layer
                    );

                    size_t n_household = 0u, n_community = 0u;
                    for (size_t k = 0u; k < source.size(); ++k)
                        if (source[k] >= 0)
                            ++((layer[k] == 0) ? n_household : n_community);

                    // Everyone infected was isolated
                    const auto & db = model->get_db();
                    REQUIRE(
                        db.get_today_total_status("Isolated") ==
                        static_cast< int >(N) - db.get_today_total_stage("Susceptible")
                    );

                    REQUIRE(n_household > 0u);
                    if (exclude)
                        REQUIRE(n_community == 0u);
                    else
                        REQUIRE(n_community > n_household);

                }

    // A status the model does not have
    {
        auto model = build(true, "pull", true);
        model->get_layer("community").exclude_status("Quarantined");
        REQUIRE_THROWS_WITH(
            model->run(5, 1),
            Catch::Matchers::Contains("excludes the status \"Quarantined\"")
        );
    }

    // Set between runs only
    {
        auto model = build(true, "pull", true);
        model->add_globalevent(
            [](Model * m) { m->get_layer("household").exclude_status("Isolated"); },
            "Too late", 2
        );
        REQUIRE_THROWS_WITH(
            model->run(5, 1),
            Catch::Matchers::Contains("set between runs")
        );
    }

}
