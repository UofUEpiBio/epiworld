#include "tests.hpp"

using namespace epiworld;

// Edits made to a layer during a run are dropped by the next reset(); edits
// made between runs become the new baseline, also on a model copied during a
// run.
EPIWORLD_TEST_CASE("Contact layer edits and the dirty flag", "[layer-dirty]") {

    const size_t n = 600u;
    const std::vector< double > cm = {
        15.0, 1.0,
        1.0, 15.0
    };
    const std::vector< double > closed = {0.0, 0.0, 0.0, 0.0};

    epimodels::ModelSIRMixing<> model("Flu", n, 0.0, 0.5, 0.1, cm);

    Virus<> v = model.get_virus(0);
    model.rm_virus(0);
    v.set_distribution(dist_virus<>(0));
    model.add_virus(v);

    model.add_entity(Entity<>("G1", dist_factory<>(0, 300)));
    model.add_entity(Entity<>("G2", dist_factory<>(300, 600)));
    model.verbose_off();

    auto counts = [&model]() -> std::vector< int > {
        std::vector< int > out;
        model.get_db().get_hist_total(nullptr, nullptr, &out);
        return out;
    };

    // Baseline: the epidemic takes off
    model.run(30, 77);
    const auto baseline = counts();
    REQUIRE_FALSE(model.get_layer("mixing").is_dirty());

    size_t susceptible_end = 0u;
    for (auto & a : model.get_agents())
        if (a.get_state() == epimodels::ModelSIRMixing<>::SUSCEPTIBLE)
            ++susceptible_end;
    REQUIRE(susceptible_end < n / 2u);

    // The same run with the matrix closed on day 1, for this run only
    model.add_globalevent(
        [&closed](Model<> * m) -> void {
            m->get_layer_as< layer::Mixing<> >("mixing").set_contact_matrix(closed);
        },
        "close"
    );
    model.run(30, 77);
    const auto with_edit = counts();
    REQUIRE(with_edit != baseline);
    REQUIRE(model.get_layer("mixing").is_dirty());

    size_t susceptible_edit = 0u;
    for (auto & a : model.get_agents())
        if (a.get_state() == epimodels::ModelSIRMixing<>::SUSCEPTIBLE)
            ++susceptible_edit;
    // (the matrix closes after day 1, so only that day's contacts got through)
    REQUIRE(susceptible_edit > n - 40u);

    // Without the event, the next run starts from the baseline again
    model.rm_globalevent("close");
    model.run(30, 77);
    REQUIRE(counts() == baseline);
    REQUIRE_FALSE(model.get_layer("mixing").is_dirty());

    // An edit between runs becomes the new baseline: nothing spreads, run after run
    model.get_layer_as< layer::Mixing<> >("mixing").set_contact_matrix(closed);
    model.run(30, 77);
    const auto closed_counts = counts();

    size_t susceptible_closed = 0u;
    for (auto & a : model.get_agents())
        if (a.get_state() == epimodels::ModelSIRMixing<>::SUSCEPTIBLE)
            ++susceptible_closed;
    REQUIRE(susceptible_closed == n - 1u);

    model.run(30, 77);
    REQUIRE(counts() == closed_counts);

    // A copy made during a run is not in a run: an edit on it, after the
    // source's run, sets its baseline
    model.get_layer_as< layer::Mixing<> >("mixing").set_contact_matrix(cm);
    std::unique_ptr< epimodels::ModelSIRMixing<> > copy;
    model.add_globalevent(
        [&copy](Model<> * m) -> void {
            copy = std::make_unique< epimodels::ModelSIRMixing<> >(
                *model_cast< epimodels::ModelSIRMixing<>, int >(m)
            );
        },
        "copy", 1
    );
    model.run(30, 77);
    REQUIRE(copy != nullptr);
    REQUIRE_FALSE(copy->is_running());

    copy->rm_globalevent("copy");
    copy->get_layer_as< layer::Mixing<> >("mixing").set_contact_matrix(closed);
    REQUIRE_FALSE(copy->get_layer("mixing").is_dirty());

    copy->run(30, 77);
    size_t susceptible_copy = 0u;
    for (auto & a : copy->get_agents())
        if (a.get_state() == epimodels::ModelSIRMixing<>::SUSCEPTIBLE)
            ++susceptible_copy;
    REQUIRE(susceptible_copy == n - 1u);

}
