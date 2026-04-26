/**
 * @file main.cpp
 * @brief Scalability benchmark: timing rgraph_sbm, rgraph_bernoulli, and
 *        rgraph_smallworld for varying population sizes.
 *
 * All three generators produce undirected networks with an expected
 * degree of approximately 10 so that edge counts are comparable.
 *
 * **SBM** — tested for n_groups ∈ {3, 5, 10, 50} and
 *   n_total in {1 000, 2 500, 5 000}.
 *
 * **Bernoulli** — tested for n_total in {1 000, 2 500, 5 000}
 *   with p = 10 / (n − 1).
 *
 * **Small-world (Watts-Strogatz)** — tested for the same sizes with
 *   k = 10, rewiring probability 0.1.
 *
 * Each configuration is generated `n_reps` times and the mean wall-clock
 * time per graph is reported.
 */

#include "../../include/epiworld/epiworld.hpp"
#include <chrono>
#include <cstdio>
#include <vector>

using namespace epiworld;

/// Helper: initialise the model with n agents and no edges.
static void reset_agents(
    epimodels::ModelSIR<> & model,
    size_t n
)
{
    model.agents_from_adjlist(
        AdjList(
            std::vector<int>{},
            std::vector<int>{},
            static_cast<int>(n),
            false
        )
    );
}

int main() {

    std::vector<size_t> group_counts = {3, 5, 10, 50};
    std::vector<size_t> total_sizes  = {1000, 2500, 5000};
    const int n_reps = 10;

    // A lightweight SIR model – we only need its RNG, not its dynamics.
    epimodels::ModelSIR<> model(
        "bench-virus", 0.01, 0.3, 0.5
    );
    model.verbose_off();

    // -----------------------------------------------------------------
    //  SBM benchmark
    // -----------------------------------------------------------------
    std::printf("=== SBM (Batagelj-Brandes, expected degree ~10) ===\n");
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

            std::vector<size_t> block_sizes(n_groups, n_each);
            block_sizes.back() += n_total - n_each * n_groups;

            double within_degree  = 6.0;
            double between_degree = (n_groups > 1u)
                ? 4.0 / static_cast<double>(n_groups - 1u)
                : 0.0;

            std::vector<double> mixing(n_groups * n_groups, 0.0);
            for (size_t g = 0; g < n_groups; ++g)
                for (size_t h = 0; h < n_groups; ++h)
                    mixing[g * n_groups + h] =
                        (g == h) ? within_degree : between_degree;

            reset_agents(model, n_total);
            rgraph_sbm(block_sizes, mixing, true, model); // warm-up

            double total_ms = 0.0, total_edges = 0.0;
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
                n_groups, n_total, n_each,
                total_edges / n_reps, total_ms / n_reps
            );
        }
    }

    // -----------------------------------------------------------------
    //  Bernoulli benchmark
    // -----------------------------------------------------------------
    std::printf(
        "\n=== Bernoulli (Batagelj-Brandes, expected degree ~10) ===\n"
    );
    std::printf(
        "%-10s  %-12s  %-14s\n",
        "n_total", "edges(avg)", "time_ms(avg)"
    );
    std::printf(
        "%-10s  %-12s  %-14s\n",
        "-------", "----------", "-----------"
    );

    for (auto n_total : total_sizes)
    {
        double p = 10.0 / static_cast<double>(n_total - 1u);

        reset_agents(model, n_total);
        rgraph_bernoulli(n_total, p, false, model); // warm-up

        double total_ms = 0.0, total_edges = 0.0;
        for (int rep = 0; rep < n_reps; ++rep)
        {
            auto t0 = std::chrono::high_resolution_clock::now();
            AdjList al = rgraph_bernoulli(n_total, p, false, model);
            auto t1 = std::chrono::high_resolution_clock::now();

            total_ms += std::chrono::duration<double, std::milli>(
                t1 - t0).count();
            total_edges += static_cast<double>(al.ecount());
        }

        std::printf(
            "%-10zu  %-12.0f  %-14.2f\n",
            n_total, total_edges / n_reps, total_ms / n_reps
        );
    }

    // -----------------------------------------------------------------
    //  Small-world (Watts-Strogatz) benchmark
    // -----------------------------------------------------------------
    std::printf(
        "\n=== Small-world (Watts-Strogatz, k=10, p_rewire=0.1) ===\n"
    );
    std::printf(
        "%-10s  %-12s  %-14s\n",
        "n_total", "edges(avg)", "time_ms(avg)"
    );
    std::printf(
        "%-10s  %-12s  %-14s\n",
        "-------", "----------", "-----------"
    );

    for (auto n_total : total_sizes)
    {
        epiworld_fast_uint k = 10;
        double p_rewire = 0.1;

        reset_agents(model, n_total);
        rgraph_smallworld(n_total, k, p_rewire, false, model); // warm-up

        double total_ms = 0.0, total_edges = 0.0;
        for (int rep = 0; rep < n_reps; ++rep)
        {
            auto t0 = std::chrono::high_resolution_clock::now();
            AdjList al = rgraph_smallworld(
                n_total, k, p_rewire, false, model
            );
            auto t1 = std::chrono::high_resolution_clock::now();

            total_ms += std::chrono::duration<double, std::milli>(
                t1 - t0).count();
            total_edges += static_cast<double>(al.ecount());
        }

        std::printf(
            "%-10zu  %-12.0f  %-14.2f\n",
            n_total, total_edges / n_reps, total_ms / n_reps
        );
    }

    return 0;
}
