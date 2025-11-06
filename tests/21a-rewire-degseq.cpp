#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Rewire degseq preserves degree and changes network", "[rewire_degseq]") {

    // Create two SEIR models with smallworld networks
    // Model 1: p=0 (no rewiring, pure ring lattice)
    epimodels::ModelSEIR<> model_0(
        "a virus", 0.01, 0.5, 7.0, 0.3
    );
    
    model_0.seed(1231);
    model_0.agents_smallworld(100, 4, false, 0.0); // p=0: no rewiring
    model_0.verbose_off();
    
    // Model 2: p=0.5 (50% rewiring)
    epimodels::ModelSEIR<> model_1(
        "a virus", 0.01, 0.5, 7.0, 0.3
    );
    
    model_1.seed(1231);
    model_1.agents_smallworld(100, 4, false, 0.5); // p=0.5: 50% rewiring
    model_1.verbose_off();
    
    // Get edgelists from both models
    std::vector<int> source_0, target_0;
    std::vector<int> source_1, target_1;
    
    model_0.write_edgelist(source_0, target_0);
    model_1.write_edgelist(source_1, target_1);
    
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
    
    if (!same_num_edges) {
        FAIL("Different number of edges!\nModel 0 (p=0): " << source_0.size() << " edges\nModel 1 (p=0.5): " << source_1.size() << " edges");
    }
    
    if (!degrees_preserved) {
        FAIL("Degrees were not preserved!");
    }
    
    if (!structure_changed) {
        FAIL("Network structure did not change!");
    }
    
    std::cout << "PASS: Rewire degseq test successful" << std::endl;
    std::cout << "  - Same number of edges: " << source_0.size() << std::endl;
    std::cout << "  - Degrees preserved: YES" << std::endl;
    std::cout << "  - Different edges: " << different_edges << " out of " << edges_0.size() << std::endl;

}
