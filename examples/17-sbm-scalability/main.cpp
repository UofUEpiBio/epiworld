/**
 * @file main.cpp
 * @brief SBM scalability benchmark: timing rgraph_sbm for varying group
 *        counts and total population sizes.
 *
 * Generates undirected SBM networks for every combination of:
 *   - n_groups ∈ {3, 5, 10, 50}
 *   - n_total  ∈ {1 000, 10 000, 100 000}
 *
 * Equal-sized blocks; the mixing matrix is constructed so that the
 * expected degree is approximately 10 for every agent regardless of
 * the number of groups:
 *   - within-block  M(g,g) = 6.0
 *   - between-block M(g,h) = 4 / (K - 1) for g ≠ h
 *   - Row sum = 6 + 4 = 10
 *
 * Each configuration is generated `n_reps` times and the mean wall-clock
 * time per graph is reported.
 */

#include "../../include/epiworld/epiworld.hpp"
#include <chrono>
#include <cstdio>
#include <vector>

using namespace epiworld;

int main() {

    std::vector<size_t> group_counts = {3, 5, 10, 50};
    std::vector<size_t> total_sizes  = {1000, 10000, 100000};
    const int n_reps = 10;

    // A lightweight SIR model – we only need its RNG, not its dynamics.
    epimodels::ModelSIR<> model(
        "bench-virus", 0.01, 0.3, 0.5
    );
    model.verbose_off();

    std::printf(
        "%-8s  %-10s  %-8s  %-12s  %-14s\n",
        "groups", "n_total", "n_each", "edges(avg)", "time_ms(avg)"
    );
    std::printf(
        "%-8s  %-10s  %-8s  %-12s  %-14s\n",
        "------", "-------", "------", "----------", "-----------"
    );

    for (auto n_groups : group_counts)
    {
        for (auto n_total : total_sizes)
        {
            size_t n_each = n_total / n_groups;
            if (n_each == 0u)
                continue;

            // Build equal-sized blocks (last block absorbs remainder)
            std::vector<size_t> block_sizes(n_groups, n_each);
            block_sizes.back() += n_total - n_each * n_groups;

            // Build mixing matrix targeting expected degree ~10.
            // Within-block: 6 contacts → p_within = 6 / n_each
            // Between-block: 4 contacts spread evenly among (K-1) groups
            //                → M(g,h) for g≠h is 4/(K-1)
            // Row sum = 6 + 4 = 10
            double within_degree  = 6.0;
            double between_degree = (n_groups > 1u)
                ? 4.0 / static_cast<double>(n_groups - 1u)
                : 0.0;

            std::vector<double> mixing(n_groups * n_groups, 0.0);
            for (size_t g = 0; g < n_groups; ++g)
            {
                for (size_t h = 0; h < n_groups; ++h)
                {
                    if (g == h)
                        mixing[g * n_groups + h] = within_degree;
                    else
                        mixing[g * n_groups + h] = between_degree;
                }
            }

            // Initialise the model with enough agents for this round.
            // We use an empty adjacency list of the right size.
            model.agents_from_adjlist(
                AdjList(
                    std::vector<int>{},
                    std::vector<int>{},
                    static_cast<int>(n_total),
                    false
                )
            );

            // Warm-up run (caches, branch predictors, etc.)
            rgraph_sbm(block_sizes, mixing, true, model);

            // Timed runs
            double total_ms = 0.0;
            double total_edges = 0.0;

            for (int rep = 0; rep < n_reps; ++rep)
            {
                auto t0 = std::chrono::high_resolution_clock::now();
                AdjList al = rgraph_sbm(
                    block_sizes, mixing, true, model
                );
                auto t1 = std::chrono::high_resolution_clock::now();

                total_ms += std::chrono::duration<double, std::milli>(
                    t1 - t0).count();
                total_edges += static_cast<double>(al.ecount());
            }

            std::printf(
                "%-8zu  %-10zu  %-8zu  %-12.0f  %-14.2f\n",
                n_groups,
                n_total,
                n_each,
                total_edges / n_reps,
                total_ms / n_reps
            );
        }
    }

    return 0;
}
