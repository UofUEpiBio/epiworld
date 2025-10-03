// Minimal test for rewire_degseq fix
// This test directly verifies the core fix without requiring full compilation of all models

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <random>

// Simplified structures for testing
struct SimpleModel {
    std::mt19937 gen;
    
    SimpleModel() : gen(12345) {}
    
    void seed(unsigned int s) { gen.seed(s); }
    
    double runif() {
        std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen);
    }
    
    bool is_directed() const { return false; }
};

struct SimpleAdjList {
    std::vector<std::map<int,int>> dat;
    bool directed;
    
    SimpleAdjList(const std::vector<int>& source, const std::vector<int>& target, int n, bool dir)
        : dat(n), directed(dir) {
        for (size_t i = 0; i < source.size(); ++i) {
            dat[source[i]][target[i]] = 1;
            if (!dir) {
                dat[target[i]][source[i]] = 1;
            }
        }
    }
    
    std::vector<std::map<int,int>>& get_dat() { return dat; }
    bool is_directed() const { return directed; }
    size_t vcount() const { return dat.size(); }
};

// The core rewiring logic that we're testing
void test_rewire_degseq(
    SimpleAdjList * agents,
    SimpleModel * model,
    double proportion
    )
{
    std::vector<int> nties(agents->vcount(), 0);
    std::vector<unsigned int> non_isolates;
    std::vector<double> weights;
    double nedges = 0.0;
    auto & dat = agents->get_dat();

    for (size_t i = 0u; i < dat.size(); ++i)
        nties[i] += dat[i].size();
    
    bool directed = agents->is_directed();
    for (size_t i = 0u; i < dat.size(); ++i) {
        if (nties[i] > 0) {
            non_isolates.push_back(i);
            if (directed) {
                weights.push_back(static_cast<double>(nties[i]));
                nedges += static_cast<double>(nties[i]);
            } else {
                weights.push_back(static_cast<double>(nties[i])/2.0);
                nedges += static_cast<double>(nties[i]) / 2.0;
            }
        }
    }

    if (non_isolates.size() == 0u)
        throw std::logic_error("The graph is completely disconnected.");

    // Cumulative probs
    weights[0u] /= nedges;
    for (unsigned int i = 1u; i < non_isolates.size(); ++i) {
         weights[i] /= nedges;
         weights[i] += weights[i - 1u];
    }

    unsigned int N = non_isolates.size();
    double prob;
    int nrewires = std::floor(proportion * nedges / (agents->is_directed() ? 1.0 : 2.0));

    while (nrewires-- > 0) {
        // Picking egos
        prob = model->runif();
        int id0 = N - 1;
        for (unsigned int i = 0u; i < N; ++i)
            if (prob <= weights[i]) {
                id0 = i;
                break;
            }

        prob = model->runif();
        int id1 = N - 1;
        for (unsigned int i = 0u; i < N; ++i)
            if (prob <= weights[i]) {
                id1 = i;
                break;
            }

        if (id1 == id0)
            id1++;

        if (id1 >= static_cast<int>(N))
            id1 = 0;

        std::map<int,int> & p0 = agents->get_dat()[non_isolates[id0]];
        std::map<int,int> & p1 = agents->get_dat()[non_isolates[id1]];

        int id01 = std::floor(p0.size() * model->runif());
        int id11 = std::floor(p1.size() * model->runif());

        int count = 0;
        for (auto & n : p0)
            if (count++ == id01)
                id01 = n.first;

        count = 0;
        for (auto & n : p1)
            if (count++ == id11)
                id11 = n.first;

        // THE FIX: Properly rewire edges instead of just swapping weights
        // But first, check if the swap would create duplicate edges
        // We want to swap (id0, id01) <-> (id1, id11) to get (id0, id11) and (id1, id01)
        // Skip if id0 is already connected to id11, or id1 is already connected to id01
        if (p0.find(id11) != p0.end() || p1.find(id01) != p1.end()) {
            continue; // Skip this rewire attempt
        }
        
        int weight_0_01 = p0[id01];
        int weight_1_11 = p1[id11];
        
        p0.erase(id01);
        p1.erase(id11);
        
        p0[id11] = weight_0_01;
        p1[id01] = weight_1_11;
        
        if (!agents->is_directed()) {
            std::map<int,int> & p01 = agents->get_dat()[id01];
            std::map<int,int> & p11 = agents->get_dat()[id11];
            
            int weight_01_0 = p01[non_isolates[id0]];
            int weight_11_1 = p11[non_isolates[id1]];
            
            p01.erase(non_isolates[id0]);
            p11.erase(non_isolates[id1]);
            
            p01[non_isolates[id1]] = weight_11_1;
            p11[non_isolates[id0]] = weight_01_0;
        }
    }
}

int main() {
    std::cout << "=== Testing rewire_degseq fix ===" << std::endl;
    
    // Create a ring lattice
    const int n = 20;
    const int k = 4;
    std::vector<int> source;
    std::vector<int> target;
    
    for (int i = 0; i < n; ++i) {
        for (int j = 1; j <= k/2; ++j) {
            int neighbor = (i + j) % n;
            source.push_back(i);
            target.push_back(neighbor);
        }
    }
    
    SimpleAdjList adjlist(source, target, n, false);
    SimpleModel model;
    
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
    
    // Apply rewiring
    test_rewire_degseq(&adjlist, &model, 0.5);
    
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
    
    // Verify symmetry for undirected graph
    bool symmetric = true;
    for (int i = 0; i < n; ++i) {
        for (auto& p : adjlist.get_dat()[i]) {
            int j = p.first;
            if (adjlist.get_dat()[j].find(i) == adjlist.get_dat()[j].end()) {
                symmetric = false;
                std::cout << "Asymmetry found: " << i << " -> " << j 
                          << " but not " << j << " -> " << i << std::endl;
            }
        }
    }
    std::cout << "Graph symmetric: " << (symmetric ? "YES" : "NO") << std::endl;
    
    if (!degrees_preserved) {
        std::cout << "FAIL: Degrees were not preserved!" << std::endl;
        return 1;
    }
    if (nodes_with_changed_neighbors == 0) {
        std::cout << "FAIL: Network structure did not change!" << std::endl;
        return 1;
    }
    if (!symmetric) {
        std::cout << "FAIL: Graph is not symmetric!" << std::endl;
        return 1;
    }
    
    std::cout << "\nPASS: All tests successful!" << std::endl;
    return 0;
}
