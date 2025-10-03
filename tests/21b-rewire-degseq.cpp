#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Rewire degseq preserves degree and changes network at p rate", "[rewire_degseq]") {

    const double p = 0.3; // Probability of rewiring
    const int n_agents = 1000;
    const int n_iterations = 400;
    const int k = 4; // Degree per node
    const double tolerance = 0.05; // Acceptable deviation from expected rewiring rate

    // Create a baseline model (p=0, no rewiring)
    epimodels::ModelSEIR<> baseline_model(
        "a virus", 0.01, 0.5, 7.0, 0.3
    );
    
    baseline_model.seed(1231);
    baseline_model.agents_smallworld(n_agents, k, false, 0.0); // p=0: pure ring lattice
    baseline_model.verbose_off();
    
    // Get baseline edgelist
    std::vector<int> source_baseline, target_baseline;
    baseline_model.write_edgelist(source_baseline, target_baseline);
    
    // Create a set of baseline edges for comparison
    std::set<std::pair<int,int>> edges_baseline;
    for (size_t i = 0; i < source_baseline.size(); ++i) {
        int s = source_baseline[i];
        int t = target_baseline[i];
        edges_baseline.insert(s < t ? std::make_pair(s, t) : std::make_pair(t, s));
    }

    // Check degrees are preserved
    std::vector<int> degree_baseline(n_agents, 0);
    std::vector<int> degree_rewired(n_agents, 0);
    
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
        // Create a model with p=0.3
        epimodels::ModelSEIR<> rewired_model(
            "a virus", 0.01, 0.5, 7.0, 0.3
        );
        
        rewired_model.seed(1231 + iter); // Different seed each iteration
        rewired_model.agents_smallworld(n_agents, k, false, p);
        rewired_model.verbose_off();
        
        // Get rewired edgelist
        std::vector<int> source_rewired, target_rewired;
        rewired_model.write_edgelist(source_rewired, target_rewired);
        
        
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
    
    // Calculate standard deviation
    double sum_squared_diff = 0.0;
    for (double pct : change_percentages) {
        sum_squared_diff += (pct - avg_change_percent) * (pct - avg_change_percent);
    }
    double std_dev = std::sqrt(sum_squared_diff / n_iterations);
    
    // Check if average change percentage is close to p (within tolerance)
    bool avg_matches_p = std::abs(avg_change_percent - p) <= tolerance;

    // Verify that degrees were preserved
    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(degrees_preserved, n_iterations);
    #endif
    
    #ifdef CATCH_CONFIG_MAIN
    REQUIRE(avg_matches_p);
    INFO("Average change percentage: " << avg_change_percent << ", expected: " << p);
    #else
    if (!avg_matches_p) {
        std::cout << "FAIL: Average edge change rate (" << avg_change_percent << ") is not close enough to p=" << p << std::endl;
        std::cout << "Tolerance was set to ±" << tolerance << std::endl;
        return 1;
    }
    
    std::cout << "PASS: Rewire degseq test successful" << std::endl;
    std::cout << "  - Target rewiring rate (p): " << p << std::endl;
    std::cout << "  - Average rewiring rate: " << avg_change_percent << std::endl;
    std::cout << "  - Standard deviation: " << std_dev << std::endl;
    std::cout << "  - Number of iterations: " << n_iterations << std::endl;
    std::cout << "  - Edges per network: " << edges_baseline.size() << std::endl;
    return 0;
    #endif
}
