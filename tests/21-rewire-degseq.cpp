#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include <set>
#include "../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main() {

    // Test 1: Using AdjList with rewire_degseq
    std::cout << "=== Test 1: AdjList rewire_degseq ===" << std::endl;
    
    // Create a ring lattice
    const int n = 20;
    const int k = 4;
    std::vector<int> source;
    std::vector<int> target;
    
    // Build ring lattice
    for (int i = 0; i < n; ++i) {
        for (int j = 1; j <= k/2; ++j) {
            int neighbor = (i + j) % n;
            source.push_back(i);
            target.push_back(neighbor);
        }
    }
    
    AdjList adjlist(source, target, n, false);
    
    // Store original degrees
    std::vector<int> original_degrees(n);
    for (int i = 0; i < n; ++i) {
        original_degrees[i] = adjlist.get_dat()[i].size();
    }
    
    // Store original network structure
    std::vector<std::set<int>> original_neighbors(n);
    for (int i = 0; i < n; ++i) {
        for (auto& p : adjlist.get_dat()[i]) {
            original_neighbors[i].insert(p.first);
        }
    }
    
    // Create a minimal model just for random number generation
    Model<> model;
    model.seed(12345);
    
    // Apply rewiring
    rewire_degseq(&adjlist, &model, 0.5);
    
    // Check degrees after rewiring
    bool degrees_preserved = true;
    for (int i = 0; i < n; ++i) {
        int new_degree = adjlist.get_dat()[i].size();
        if (original_degrees[i] != new_degree) {
            degrees_preserved = false;
            std::cout << "Node " << i << " degree changed from " 
                      << original_degrees[i] << " to " << new_degree << std::endl;
        }
    }
    
    // Check if network structure changed
    int nodes_with_changed_neighbors = 0;
    for (int i = 0; i < n; ++i) {
        std::set<int> new_neighbors;
        for (auto& p : adjlist.get_dat()[i]) {
            new_neighbors.insert(p.first);
        }
        if (original_neighbors[i] != new_neighbors) {
            nodes_with_changed_neighbors++;
        }
    }
    
    std::cout << "Degrees preserved: " << (degrees_preserved ? "YES" : "NO") << std::endl;
    std::cout << "Nodes with changed neighbors: " << nodes_with_changed_neighbors 
              << " out of " << n << std::endl;
    
    if (!degrees_preserved) {
        std::cout << "FAIL: Degrees were not preserved!" << std::endl;
        return 1;
    }
    if (nodes_with_changed_neighbors == 0) {
        std::cout << "FAIL: Network structure did not change!" << std::endl;
        return 1;
    }
    
    std::cout << "PASS: Test 1 successful" << std::endl << std::endl;
    
    return 0;
}
