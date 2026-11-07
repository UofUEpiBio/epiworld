#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("AgentsSample", "[model-methods]") {

    // Creating a model
    Model model(40);

    // Adding the tool and virus. Transmission is certain, recovery takes one
    // day (the tool makes it certain), nobody dies (the tool prevents it), and
    // recovered agents go back to susceptible but are immune: the outbreak
    // sweeps the network once.
    model.add_param(1.0, "Infectiousness");
    model.add_param(1.0, "Immunity");
    model.add_param(1.0, "Death reduction");

    Virus virus("covid 19");
    const size_t susceptible_state = virus.add_stage("Susceptible");
    const size_t exposed_state = virus.add_stage("Exposed", stage::infectious);
    const size_t removed_state = virus.add_stage("Removed");
    virus.infection(0, 1, ValueFun(
        [](Agent &, Model & m) { return m.par("Infectiousness"); }
    ));
    const int recovery = virus.rate(1, 0, 0.0).effect(Effect::recovery);
    virus.rate(1, 2, 0.01).effect(Effect::death);
    model.add_virus(virus, assign::count(10));

    // The recovered get a tool that blocks infection (a reaction)
    epiworld::Tool immunity("immunity");
    immunity.set_susceptibility_reduction("Immunity");
    model.add_tool(immunity, assign::count(0));
    model.react(
        Trigger::fired(recovery),
        [](Agent & p, Model & m) { p.add_tool(m, m.get_tool("immunity")); }
    );

    epiworld::Tool tool("vaccine");
    tool.set_death_reduction("Death reduction");
    tool.set_recovery_enhancer_fun(
        [](Tool &, Agent *, VirusPtr &, Model *) { return 1.0; }
    );
    model.add_tool(tool, assign::share(1.0));

    // Generating a random pop
    model.add_layer(layer::network(
        "network", rgraph_smallworld(40, 5, .01, false, model)
    ));

    // Running the model
    model.run(100, 123);
    model.print();

    // Will print the transition matrix
    (void) model.get_db().get_transition_probability();

    // Counts: entry [from + to * n_states]
    auto transitions = model.get_db().get_transition_probability(false, false);
    const size_t n_states = model.get_n_states();
    auto moves = [&](size_t from, size_t to) {
        return transitions[from + to * n_states];
    };

    // The daily history: each day accounts for every agent (staying or moving),
    // and its days add up to the counts above
    std::vector< std::string > status_from, state_from, status_to, state_to;
    std::vector< int > hist_date, hist_counts;
    model.get_db().get_hist_transition_matrix(
        status_from, state_from, status_to, state_to, hist_date, hist_counts,
        false
    );

    const auto labels = tests_state_labels(model);
    auto state_id = [&labels](const std::string & s) {
        return static_cast< size_t >(
            std::find(labels.begin(), labels.end(), s) - labels.begin()
        );
    };

    std::vector< int > per_day(model.get_ndays() + 1u, 0);
    std::vector< epiworld_double > summed(n_states * n_states, 0.0);
    for (size_t i = 0u; i < hist_counts.size(); ++i)
    {
        per_day[hist_date[i]] += hist_counts[i];
        if (hist_date[i] < static_cast< int >(model.get_ndays()))
            summed[state_id(state_from[i]) + state_id(state_to[i]) * n_states] +=
                hist_counts[i];
    }

    REQUIRE(hist_counts.size() == per_day.size() * n_states * n_states);
    REQUIRE(per_day == std::vector< int >(per_day.size(), static_cast< int >(model.size())));
    REQUIRE(summed == transitions);

    REQUIRE(moves(exposed_state, exposed_state) == 0.0);
    REQUIRE(moves(exposed_state, removed_state) == 0.0);

    // Everyone gets infected exactly once (the seeded agents are listed with
    // source -1), and keeps the immunity
    std::vector< int > date, source, target, virus_id, source_exposure_date, source_exposure_date_layer;
    model.get_db().get_transmissions(
        date, source, target, virus_id, source_exposure_date, source_exposure_date_layer
    );
    std::sort(target.begin(), target.end());
    REQUIRE(std::adjacent_find(target.begin(), target.end()) == target.end());
    REQUIRE(target.size() == model.size());
    REQUIRE(model.get_agents_states() == std::vector< epiworld_fast_uint >(
        model.size(), susceptible_state
    ));
    for (const auto & agent : model.get_agents())
        REQUIRE(agent.get_n_tools() == 2u);

    AgentsSample agents(model, .05 * 40);

    printf_epiworld("Total sampled: %lu\n", agents.size());
    for (auto & a: agents)
        a->print(model);


}
