#include "tests.hpp"

using namespace epiworld;

// The outputs by disease, with one disease (several-diseases.md, NH-5b-1):
// every table says the disease of each row, the disease argument selects
// it, and the transition matrices come over the pair, the stage or the
// status, each the other's sum. The agents' stages and statuses agree with
// the model's per-agent answers and with today's totals.
EPIWORLD_TEST_CASE("Outputs by disease", "[outputs-by-disease]") {

    ModelRegistry registry;
    epimodels::register_models(registry);
    auto model = registry.make_model("SEIRQuarantine", {{"n", 2000}}, "flu");
    model->verbose_off();
    model->add_layer(layer::network("network", graph::smallworld(8, 0.1)));
    model->run(60, 42);
    const auto & db = model->get_db();

    // The model's diseases
    REQUIRE(model->get_n_diseases() == 1u);
    REQUIRE(model->get_diseases() == std::vector< std::string >{"flu"});
    REQUIRE(model->disease_id("flu") == 0);
    REQUIRE_THROWS_AS(model->disease_id("measles"), std::range_error);

    // Every table but the tools' starts with the disease of each row
    for (const auto & t : db.get_run_outputs(run_output_names()))
    {
        INFO("output " << t.first);
        if (t.first == "tool_info")
        {
            REQUIRE(t.second.colnames.at(0u) != "disease");
            continue;
        }
        REQUIRE(t.second.colnames.at(0u) == "disease");
        const auto & d = std::get< std::vector< std::string > >(t.second.columns[0u]);
        REQUIRE(d == std::vector< std::string >(t.second.nrow(), "flu"));
    }

    // The disease argument: the one disease is every disease, and an index
    // that is no disease is an error
    std::vector< int > date_all, counts_all, date_0, counts_0;
    std::vector< std::string > status, stage, names;
    db.get_hist_total(&date_all, &status, &stage, &counts_all);
    db.get_hist_total(&date_0, &status, &stage, &counts_0, 0, &names);
    REQUIRE(counts_0 == counts_all);
    REQUIRE(names == std::vector< std::string >(counts_0.size(), "flu"));
    REQUIRE_THROWS_AS(
        db.get_hist_total(&date_0, &status, &stage, &counts_0, 1),
        std::range_error
    );

    std::vector< int > a, b, c, v, e, l;
    db.get_transmissions(a, b, c, v, e, l, all_diseases, &names);
    REQUIRE(std::count_if(b.begin(), b.end(), [](int s) { return s >= 0; }) > 0);
    REQUIRE(names == std::vector< std::string >(a.size(), "flu"));

    // The margins: the stage and status counts are the pair's summed
    auto pair = db.get_transition_probability(Margin::pair, all_diseases, false);
    auto stage_m = db.get_transition_probability(Margin::stage, 0, false);
    auto status_m = db.get_transition_probability(Margin::status, all_diseases, false);

    REQUIRE(pair.disease == "flu");
    REQUIRE(status_m.disease == "");
    REQUIRE(stage_m.stage == model->get_natural_history()->get_stages());
    REQUIRE(status_m.status == model->get_statuses()->get_stages());
    REQUIRE(stage_m.status.empty());
    REQUIRE(pair.size() == stage_m.size() * status_m.size());

    std::vector< double > stage_sum(stage_m.values.size(), 0.0);
    std::vector< double > status_sum(status_m.values.size(), 0.0);
    auto index_of = [](const std::vector< std::string > & labels, const std::string & x) {
        return static_cast< size_t >(std::find(labels.begin(), labels.end(), x) - labels.begin());
    };
    for (size_t i = 0u; i < pair.size(); ++i)
        for (size_t j = 0u; j < pair.size(); ++j)
        {
            stage_sum[index_of(stage_m.stage, pair.stage[i]) +
                index_of(stage_m.stage, pair.stage[j]) * stage_m.size()] += pair(i, j);
            status_sum[index_of(status_m.status, pair.status[i]) +
                index_of(status_m.status, pair.status[j]) * status_m.size()] += pair(i, j);
        }
    REQUIRE(stage_sum == std::vector< double >(stage_m.values.begin(), stage_m.values.end()));
    REQUIRE(status_sum == std::vector< double >(status_m.values.begin(), status_m.values.end()));

    // Someone was quarantined, and an agent changing status within a stage
    // stays put in the stage matrix
    const size_t free_k = index_of(status_m.status, "Free");
    const size_t quar_k = index_of(status_m.status, "Quarantined");
    REQUIRE(status_m(free_k, quar_k) > 0.0);

    // Probabilities: each row with transitions sums to one
    auto probs = db.get_transition_probability(Margin::stage);
    for (size_t i = 0u; i < probs.size(); ++i)
    {
        double total = 0.0;
        for (size_t j = 0u; j < probs.size(); ++j)
            total += probs(i, j);
        if (total > 0.0)
            REQUIRE_THAT(total, Catch::Matchers::WithinAbs(1.0, 1e-9));
    }

    // The agents' stages and statuses
    const auto stages = model->get_agents_stages();
    const auto statuses = model->get_agents_statuses();
    std::vector< int > by_stage(stage_m.size(), 0);
    for (size_t i = 0u; i < model->size(); ++i)
    {
        REQUIRE(stages[i] == model->get_stage(model->get_agent(i)));
        REQUIRE(statuses[i] == model->get_status(model->get_agent(i)));
        ++by_stage[static_cast< size_t >(stages[i])];
    }
    for (size_t s = 0u; s < stage_m.size(); ++s)
        REQUIRE(by_stage[s] == db.get_today_total_stage(stage_m.stage[s], 0));
    REQUIRE_THROWS_AS(model->get_agents_stages(1), std::range_error);

}
