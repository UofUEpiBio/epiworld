#include "tests.hpp"

using namespace epiworld;

// What a network layer looks like from one run to the next. A layer built
// from a generator is redrawn at every reset: each run, and each replicate of
// run_multiple(), gets its own network, and the same seed gives the same one.
// A layer built from a graph is fixed: an edit made during a run (a global
// event, rewiring) is dropped at the next reset, and an edit made between runs
// stays.

namespace {

std::vector< std::pair< int, int > > ties(Model<> & model, const char * name)
{
    std::vector< int > s, t;
    model.get_layer_as< layer::Network<> >(name).get_edgelist(s, t);
    std::vector< std::pair< int, int > > out;
    for (size_t k = 0u; k < s.size(); ++k)
        out.emplace_back(s[k], t[k]);
    return out;
}

}

EPIWORLD_TEST_CASE("Network layers - redrawn and fixed", "[layer-network]") {

    const size_t n = 500u;

    // -- redrawn ---------------------------------------------------------------
    {
        epimodels::ModelSIR<> model("flu", n, 0.02, 0.2, 0.2);
        model.verbose_off();
        auto & net = model.add_layer(std::make_unique< layer::Network<> >(
            "contacts", graph::smallworld(4, 0.2)
        ));

        // Nothing is drawn before a run
        REQUIRE(net.size() == 0u);

        model.run(10, 1);
        auto first = ties(model, "contacts");
        std::vector< int > hist_first;
        model.get_db().get_hist_total(nullptr, nullptr, &hist_first);

        model.run(10, 2);
        auto second = ties(model, "contacts");

        model.run(10, 1);
        auto again = ties(model, "contacts");
        std::vector< int > hist_again;
        model.get_db().get_hist_total(nullptr, nullptr, &hist_again);

        REQUIRE(first.size() == n * 4u / 2u);
        REQUIRE(first != second);
        REQUIRE(first == again);
        REQUIRE(hist_first == hist_again);

        // Every replicate of run_multiple() draws its own network
        std::vector< std::vector< std::pair< int, int > > > drawn;
        model.run_multiple(
            5, 4, 77,
            [&drawn](size_t, Model<> * m) -> void {
                drawn.push_back(ties(*m, "contacts"));
            },
            true, false, 1
        );
        REQUIRE(drawn.size() == 4u);
        for (size_t i = 1u; i < drawn.size(); ++i)
            REQUIRE(drawn[i] != drawn[0]);
    }

    // -- fixed -----------------------------------------------------------------
    {
        epimodels::ModelSIR<> model("flu", n, 0.02, 0.2, 0.2);
        model.verbose_off();
        auto & net = model.add_layer(std::make_unique< layer::Network<> >(
            "contacts", rgraph_smallworld(n, 4, 0.0, false, model)
        ));
        const auto baseline = ties(model, "contacts");
        REQUIRE_FALSE(net.has_edge(0u, 250u));

        // An edit during a run lasts for that run only
        model.add_globalevent([](Model<> * m) -> void {
            m->get_layer_as< layer::Network<> >("contacts").add_edge(0u, 250u);
        }, "tie", 2);
        model.run(5, 3);
        REQUIRE(net.has_edge(0u, 250u));
        REQUIRE(net.is_dirty());

        model.rm_globalevent("tie");
        model.run(5, 3);
        REQUIRE(ties(model, "contacts") == baseline);

        // So does rewiring
        model.add_param(0.2, "Rewiring proportion");
        net.set_rewiring(graph::rewire_degseq("Rewiring proportion"));
        model.run(5, 4);
        REQUIRE(ties(model, "contacts") != baseline);

        net.set_rewiring(nullptr);
        model.run(5, 4);
        REQUIRE(ties(model, "contacts") == baseline);

        // An edit between runs becomes the network of the runs that follow
        REQUIRE(net.add_edge(1u, 300u));
        REQUIRE_FALSE(net.is_dirty());
        model.run(5, 5);
        model.run(5, 6);
        REQUIRE(net.has_edge(1u, 300u));
        REQUIRE(ties(model, "contacts").size() == baseline.size() + 1u);

        // A graph of the wrong size is refused when the model runs
        epimodels::ModelSIR<> wrong("flu", n + 1u, 0.02, 0.2, 0.2);
        wrong.verbose_off();
        wrong.add_layer(std::make_unique< layer::Network<> >(
            "contacts", rgraph_smallworld(n, 4, 0.0, false, wrong)
        ));
        REQUIRE_THROWS_AS(wrong.run(5, 1), std::length_error);
    }

}
