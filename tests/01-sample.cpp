#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("AgentsSample", "[model-methods]") {

    // Creating a model
    Model<> model;

    // Declaring the three statuses in the model
    auto susceptible_state = model.add_state("Susceptible", epiworld::default_update_susceptible<>);
    auto exposed_state = model.add_state("Exposed", epiworld::default_update_exposed<>);
    auto removed_state = model.add_state("Removed");

    // Adding the tool and virus. Transmission is certain, recovery takes one
    // day, nobody dies, and recovered agents go back to susceptible but are
    // immune: the outbreak sweeps the network once.
    model.add_param(1.0, "Infectiousness");
    model.add_param(1.0, "Immunity");
    model.add_param(1.0, "Death reduction");

    Virus<> virus("covid 19", 10, false);
    virus.set_post_immunity("Immunity");
    virus.set_state(exposed_state, susceptible_state, removed_state);
    virus.set_prob_death(.01);
    virus.set_prob_infecting_fun(
        [](Agent<> *, Virus<> &, Model<> * m) { return m->par("Infectiousness"); }
    );
    virus.set_incubation(3.0);
    model.add_virus(virus);

    epiworld::Tool<> tool("vaccine", 1.0, true);
    tool.set_death_reduction("Death reduction");
    tool.set_recovery_enhancer_fun(
        [](Tool<> &, Agent<> *, VirusPtr<> &, Model<> *) { return 1.0; }
    );
    model.add_tool(tool);

    // Generating a random pop
    model.agents_smallworld(40);

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
    std::vector< std::string > state_from, state_to;
    std::vector< int > hist_date, hist_counts;
    model.get_db().get_hist_transition_matrix(
        state_from, state_to, hist_date, hist_counts, false
    );

    const auto & labels = model.get_states();
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

    REQUIRE(model.get_virus(0).get_incubation(&model) == 3.0);
    REQUIRE(moves(exposed_state, exposed_state) == 0.0);
    REQUIRE(moves(exposed_state, removed_state) == 0.0);

    // Everyone gets infected exactly once (the seeded agents are listed with
    // source -1), and keeps the immunity
    std::vector< int > date, source, target, virus_id, source_exposure_date;
    model.get_db().get_transmissions(
        date, source, target, virus_id, source_exposure_date
    );
    std::sort(target.begin(), target.end());
    REQUIRE(std::adjacent_find(target.begin(), target.end()) == target.end());
    REQUIRE(target.size() == model.size());
    REQUIRE(model.get_agents_states() == std::vector< epiworld_fast_uint >(
        model.size(), susceptible_state
    ));
    for (const auto & agent : model.get_agents())
        REQUIRE(agent.get_n_tools() == 2u);

    AgentsSample<> agents(model, .05 * 40);

    printf_epiworld("Total sampled: %lu\n", agents.size());
    for (auto & a: agents)
        a->print(model);


}
