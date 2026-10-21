#include "tests.hpp"

using namespace epiworld;

// A layer is a shared pointer: the model and the caller hold the same layer,
// so an edit through the caller's pointer is an edit of the model. A layer
// belongs to one model at most; a copy of the model clones its layers, and a
// layer that outlives its model is detached from it.
EPIWORLD_TEST_CASE("Contact layers - ownership", "[layer-ownership]") {

    const size_t n = 500u;

    epimodels::ModelSIR<> draw("flu", n, 0.0, 0.0, 0.0);
    AdjList graph = rgraph_smallworld(n, 4, 0.0, false, draw);

    auto history = [](Model<> & m) -> std::vector< int > {
        std::vector< int > out;
        m.get_db().get_hist_total(nullptr, nullptr, &out);
        return out;
    };

    // -- one layer, two holders ------------------------------------------------
    // Ties added through the caller's pointer are the model's ties
    auto net = layer::network("contacts", graph);

    epimodels::ModelSIR<> model("flu", n, 0.02, 0.3, 0.2);
    model.verbose_off();
    auto & ref = model.add_layer(net);
    REQUIRE(&ref == net.get());

    for (size_t i = 0u; i < 50u; ++i)
        REQUIRE(net->add_edge(i, i + 250u));
    model.run(30, 11);

    // The same edits made through the model
    epimodels::ModelSIR<> expected("flu", n, 0.02, 0.3, 0.2);
    expected.verbose_off();
    expected.add_layer(layer::network("contacts", graph));
    auto & theirs = expected.get_layer_as< layer::Network<> >("contacts");
    for (size_t i = 0u; i < 50u; ++i)
        theirs.add_edge(i, i + 250u);
    expected.run(30, 11);

    epimodels::ModelSIR<> unedited("flu", n, 0.02, 0.3, 0.2);
    unedited.verbose_off();
    unedited.add_layer(layer::network("contacts", graph));
    unedited.run(30, 11);

    REQUIRE(history(model) != history(unedited));
    REQUIRE(history(model) == history(expected));

    // -- one model per layer ---------------------------------------------------
    epimodels::ModelSIR<> other("flu", n, 0.02, 0.3, 0.2);
    other.verbose_off();
    REQUIRE_THROWS_AS(other.add_layer(net), std::logic_error);

    // A copy of the layer is detached, so it can go into another model
    other.add_layer(std::make_shared< layer::Network<> >(*net));
    other.run(30, 11);
    REQUIRE(history(other) == history(model));

    // -- copies of the model clone the layers ----------------------------------
    // The caller's pointer stays with the original
    Model<> copy(model);
    REQUIRE(net->add_edge(0u, 400u));
    REQUIRE(net->has_edge(0u, 400u));
    auto & copied = copy.get_layer_as< layer::Network<> >("contacts");
    REQUIRE_FALSE(copied.has_edge(0u, 400u));

    // -- a layer can outlive its model -----------------------------------------
    std::shared_ptr< layer::Network<> > kept;
    {
        epimodels::ModelSIR<> brief("flu", n, 0.02, 0.3, 0.2);
        kept = layer::network("contacts", graph);
        brief.add_layer(kept);
        REQUIRE_NOTHROW(kept->neighbors_view(0u));
    }
    REQUIRE_THROWS_AS(kept->neighbors_view(0u), std::logic_error);
    REQUIRE(kept->add_edge(0u, 250u));

    // ... and, detached, it goes into a new one
    epimodels::ModelSIR<> later("flu", n, 0.02, 0.3, 0.2);
    later.verbose_off();
    later.add_layer(kept);
    later.run(10, 1);
    REQUIRE(kept->has_edge(0u, 250u));

}
