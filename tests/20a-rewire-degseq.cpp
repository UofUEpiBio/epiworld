#include "tests.hpp"

using namespace epiworld;

// Rewiring the network during a run must not disturb the queueing system.
//
// With a rewiring function set, Model::rewire() runs on every step, and
// rewire_degseq() moves ties with Agent::swap_neighbors(): (i-j) and (k-l)
// become (i-l) and (k-j). The queue counts, for every agent, the registered
// agents among itself and its neighbors. The count is built when an agent is
// registered and unwound when it is deregistered, both walking the neighbors
// the agent has *at that moment*, so a swap in between that the queue is not
// told about leaves the two walks disagreeing. Before swap_neighbors() told
// it, an agent whose count drifted to zero was skipped by update_state(): its
// neighbors' infections never reached it, and an infected agent stranded there
// never recovered.
//
// As in 33b-edges-queue.cpp, the check is that queuing is a pure optimisation:
// a run with it on and a run with it off must agree exactly. That holds in
// every transmission mode (pushing consults the queue too), and in this model
// "auto" switches between the two.

// Degree-sequence rewiring
//
// One test case; each former test file is a block below, with the name it had.
EPIWORLD_TEST_CASE("Degree-sequence rewiring", "[rewire_degseq]") {

    // ---- 20a-rewire-degseq.cpp: Rewire degseq preserves degree and changes network
    {

    // Create two SEIR models with smallworld networks
    // Model 1: p=0 (no rewiring, pure ring lattice)
    epimodels::ModelSEIR model_0(
        "a virus", 100, 0.01, 0.5, 7.0, 0.3
    );
    
    model_0.seed(1231);
    model_0.add_layer(layer::network(
        "network", rgraph_smallworld(100, 4, 0.0, false, model_0)
    )); // p=0: no rewiring
    model_0.verbose_off();
    
    // Model 2: p=0.5 (50% rewiring)
    epimodels::ModelSEIR model_1(
        "a virus", 100, 0.01, 0.5, 7.0, 0.3
    );
    
    model_1.seed(1231);
    model_1.add_layer(layer::network(
        "network", rgraph_smallworld(100, 4, 0.5, false, model_1)
    )); // p=0.5: 50% rewiring
    model_1.verbose_off();
    
    // Get edgelists from both models
    std::vector<int> source_0, target_0;
    std::vector<int> source_1, target_1;
    
    tests_net(model_0).get_edgelist(source_0, target_0);
    tests_net(model_1).get_edgelist(source_1, target_1);
    
    // Check that both networks have the same number of edges
    bool same_num_edges = (source_0.size() == source_1.size());
    
    // Compute degrees for both networks
    const int n = 100;
    std::vector<int> degree_0(n, 0);
    std::vector<int> degree_1(n, 0);
    
    for (size_t i = 0; i < source_0.size(); ++i) {
        degree_0[source_0[i]]++;
        degree_0[target_0[i]]++;
    }
    
    for (size_t i = 0; i < source_1.size(); ++i) {
        degree_1[source_1[i]]++;
        degree_1[target_1[i]]++;
    }
    
    // Check that degree sequences are preserved
    bool degrees_preserved = true;
    for (int i = 0; i < n; ++i) {
        if (degree_0[i] != degree_1[i]) {
            degrees_preserved = false;
            break;
        }
    }
    
    // Check that network structure actually changed
    // Create sets of edges for comparison
    std::set<std::pair<int,int>> edges_0, edges_1;
    for (size_t i = 0; i < source_0.size(); ++i) {
        int s = source_0[i];
        int t = target_0[i];
        edges_0.insert(s < t ? std::make_pair(s, t) : std::make_pair(t, s));
    }
    
    for (size_t i = 0; i < source_1.size(); ++i) {
        int s = source_1[i];
        int t = target_1[i];
        edges_1.insert(s < t ? std::make_pair(s, t) : std::make_pair(t, s));
    }
    
    // Count how many edges are different
    int different_edges = 0;
    for (const auto& edge : edges_0) {
        if (edges_1.find(edge) == edges_1.end()) {
            different_edges++;
        }
    }
    
    bool structure_changed = (different_edges > 0);
    
    REQUIRE(same_num_edges);
    REQUIRE(degrees_preserved);
    REQUIRE(structure_changed);

    std::cout << "PASS: Rewire degseq test successful" << std::endl;
    std::cout << "  - Same number of edges: " << source_0.size() << std::endl;
    std::cout << "  - Degrees preserved: YES" << std::endl;
    std::cout << "  - Different edges: " << different_edges << " out of " << edges_0.size() << std::endl;
    }

    // ---- 20b-rewire-degseq.cpp: Rewire degseq preserves degree and changes network at p rate
    {

    const double p = 0.2; // Probability of rewiring
    const int n_agents = 5000;
    const int n_iterations = 400;
    const int k = 4; // Degree per node
    const double tolerance = 0.05; // Acceptable deviation from expected rewiring rate

    // Create a baseline model (p=0, no rewiring)
    epimodels::ModelSEIR baseline_model(
        "a virus", n_agents, 0.01, 0.5, 7.0, 0.3
    );
    
    baseline_model.seed(1231);
    baseline_model.add_layer(layer::network(
        "network", rgraph_smallworld(n_agents, k, 0.0, false, baseline_model)
    )); // p=0: pure ring lattice
    baseline_model.verbose_off();
    
    // Get baseline edgelist
    std::vector<int> source_baseline, target_baseline;
    tests_net(baseline_model).get_edgelist(source_baseline, target_baseline);
    
    // Create a set of baseline edges for comparison
    std::set<std::pair<int,int>> edges_baseline;
    for (size_t i = 0; i < source_baseline.size(); ++i) {
        int s = source_baseline[i];
        int t = target_baseline[i];
        edges_baseline.insert(s < t ? std::make_pair(s, t) : std::make_pair(t, s));
    }

    // Check degrees are preserved
    std::vector<int> degree_baseline(n_agents, 0);
    
    for (size_t i = 0; i < source_baseline.size(); ++i) {
        degree_baseline[source_baseline[i]]++;
        degree_baseline[target_baseline[i]]++;
    }
    
    // Store percentage of changed edges for each iteration
    std::vector<double> change_percentages;
    change_percentages.reserve(n_iterations);
    
    // Run iterations
    size_t degrees_preserved = n_iterations;
    for (int iter = 0; iter < n_iterations; ++iter) {

        // Reset rewired degree count
        std::vector<int> degree_rewired(n_agents, 0);

        // Create a model with p=0.3
        epimodels::ModelSEIR rewired_model(
            "a virus", n_agents, 0.01, 0.5, 7.0, 0.3
        );
        
        rewired_model.seed(1231 + iter); // Different seed each iteration
        rewired_model.add_layer(layer::network(
            "network", rgraph_smallworld(n_agents, k, p, false, rewired_model)
        ));
        rewired_model.verbose_off();
        
        // Get rewired edgelist
        std::vector<int> source_rewired, target_rewired;
        tests_net(rewired_model).get_edgelist(source_rewired, target_rewired);
        
        
        for (size_t i = 0; i < source_rewired.size(); ++i) {
            degree_rewired[source_rewired[i]]++;
            degree_rewired[target_rewired[i]]++;
        }
        
        for (int i = 0; i < n_agents; ++i) {
            if (degree_baseline[i] != degree_rewired[i]) {
                degrees_preserved--;
                break;
            }
        }
        
        // Create set of rewired edges
        std::set<std::pair<int,int>> edges_rewired;
        for (size_t i = 0; i < source_rewired.size(); ++i) {
            int s = source_rewired[i];
            int t = target_rewired[i];
            edges_rewired.insert(s < t ? std::make_pair(s, t) : std::make_pair(t, s));
        }
        
        // Count different edges
        int different_edges = 0;
        for (const auto& edge : edges_baseline) {
            if (edges_rewired.find(edge) == edges_rewired.end()) {
                different_edges++;
            }
        }
        
        // Calculate percentage of changed edges
        double change_percent = static_cast<double>(different_edges) / edges_baseline.size();
        change_percentages.push_back(change_percent);
        
    }
    
    // Calculate average percentage of changed edges
    double sum = 0.0;
    for (double pct : change_percentages) {
        sum += pct;
    }
    double avg_change_percent = sum / n_iterations;
    
    // Check if average change percentage is close to p (within tolerance)
    bool avg_matches_p = std::abs(avg_change_percent - p) <= tolerance;

    // Verify that degrees were preserved
    REQUIRE(degrees_preserved == n_iterations);
    
    REQUIRE(avg_matches_p);
    INFO("Average change percentage: " << avg_change_percent << ", expected: " << p);
    
    std::cout << "PASS: Rewire degseq test successful" << std::endl;
    std::cout << "  - Target rewiring rate (p): " << p << std::endl;
    std::cout << "  - Average rewiring rate: " << avg_change_percent << std::endl;
    std::cout << "  - Number of iterations: " << n_iterations << std::endl;
    std::cout << "  - Edges per network: " << edges_baseline.size() << std::endl;
    }

    // ---- 20c-rewire-degseq-queue.cpp: Rewire degseq - mid-run rewiring keeps the queue honest
    {

    struct Run {
        std::vector< int > hist_date, hist_counts;
        std::vector< std::string > hist_state;
        std::vector< int > date, source, target, virus, source_exposure_date, layer;
        std::vector< int > net_source_0, net_target_0; // Network before the run
        std::vector< int > net_source, net_target;     // ... and after it
        int n_push = 0, n_pull = 0;
    };

    auto simulate = [](const std::string & mode, bool queuing) -> Run {

        Run r;

        epimodels::ModelSIR model("v", 2000, 0.01, 0.3, 0.2);
        model.verbose_off();
        model.add_layer(layer::network(
            "network", rgraph_smallworld(2000, 6, 0.05, false, model)
        ));
        model.add_param(0.1, "Rewiring proportion");
        tests_net(model).set_rewiring(graph::rewire_degseq("Rewiring proportion"));
        model.set_transmission_mode(mode);

        if (!queuing)
            model.queuing_off();

        // Draws no random numbers, so it leaves the run as it was
        model.add_globalevent([&r](Model * m) -> void {
            if (m->get_last_transmission_mode() == TransmissionMode::push)
                ++r.n_push;
            else
                ++r.n_pull;
        }, "count steps");

        tests_net(model).get_edgelist(r.net_source_0, r.net_target_0);

        model.run(60, 123);

        model.get_db().get_hist_total(&r.hist_date, nullptr, &r.hist_state, &r.hist_counts);
        model.get_db().get_transmissions(
            r.date, r.source, r.target, r.virus, r.source_exposure_date, r.layer
        );
        tests_net(model).get_edgelist(r.net_source, r.net_target);

        return r;

    };

    for (std::string mode : {"pull", "auto", "push"})
    {

        Run on  = simulate(mode, true);
        Run off = simulate(mode, false);

        INFO("transmission mode: " << mode);

        // Not vacuous: an outbreak happened, on a network rewired under it
        bool rewired =
            (on.net_source != on.net_source_0) ||
            (on.net_target != on.net_target_0);

        REQUIRE(on.date.size() > 1000u);
        REQUIRE(rewired);

        if (mode == "auto")
        {
            REQUIRE(on.n_push > 0);
            REQUIRE(on.n_pull > 0);
        }

        REQUIRE_THAT(off.hist_date, Catch::Equals(on.hist_date));
        REQUIRE_THAT(off.hist_state, Catch::Equals(on.hist_state));
        REQUIRE_THAT(off.hist_counts, Catch::Equals(on.hist_counts));

        REQUIRE_THAT(off.date, Catch::Equals(on.date));
        REQUIRE_THAT(off.source, Catch::Equals(on.source));
        REQUIRE_THAT(off.target, Catch::Equals(on.target));
        REQUIRE_THAT(off.virus, Catch::Equals(on.virus));
        REQUIRE_THAT(off.source_exposure_date, Catch::Equals(on.source_exposure_date));

        // Both runs drew the same random numbers, rewiring included
        REQUIRE_THAT(off.net_source, Catch::Equals(on.net_source));
        REQUIRE_THAT(off.net_target, Catch::Equals(on.net_target));

    }
    }

}
