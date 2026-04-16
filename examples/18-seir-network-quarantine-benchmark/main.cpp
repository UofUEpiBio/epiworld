#include "../../include/epiworld/epiworld.hpp"

#include <chrono>
#include <cstdio>
#include <vector>

using namespace epiworld;

/**
 * Helper: create an entity distribution that assigns agents in [from, to).
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline EntityToAgentFun<TSeq> range_dist(int from, int to) {
    return [from, to](Entity<TSeq> & e, Model<TSeq> * m) -> void {
        auto & agents = m->get_agents();
        for (int i = from; i < to; ++i)
            e.add_agent(&agents[i], *m);
    };
}

int main() {

    // -------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------
    const size_t n_groups   = 10;
    const size_t group_size = 500;
    const size_t n_total    = n_groups * group_size;
    const double mean_contacts = 20.0;
    const int    n_days     = 100;
    const int    n_reps     = 50;

    // Build an SBM mixing matrix (row-major):
    // Each row sums to `mean_contacts`. Within-block gets 60%, between-block
    // gets the remaining 40% split evenly across the other groups.
    double within_degree  = mean_contacts * 0.6;
    double between_degree = mean_contacts * 0.4 / static_cast<double>(n_groups - 1);

    std::vector< double > sbm_mixing(n_groups * n_groups, 0.0);
    for (size_t g = 0; g < n_groups; ++g)
    {
        for (size_t h = 0; h < n_groups; ++h)
        {
            sbm_mixing[g * n_groups + h] =
                (g == h) ? within_degree : between_degree;
        }
    }

    // Build a row-stochastic contact matrix for SEIRMixingQuarantine.
    // Each row must sum to 1.0
    std::vector< double > contact_matrix(n_groups * n_groups, 0.0);
    for (size_t g = 0; g < n_groups; ++g)
    {
        for (size_t h = 0; h < n_groups; ++h)
        {
            contact_matrix[g * n_groups + h] =
                sbm_mixing[g * n_groups + h] / mean_contacts;
        }
    }

    // Common disease parameters
    const double prevalence          = 0.01;
    const double transmission_rate   = 0.5;
    const double avg_incubation_days = 3.0;
    const double recovery_rate       = 1.0/5.0;
    const double hospitalization_rate = 0.05;
    const double hospitalization_period = 7.0;
    const double days_undetected     = 3.0;
    const int    quarantine_period   = 7;
    const double quarantine_willingness = 0.8;
    const double isolation_willingness  = 0.9;
    const int    isolation_period     = 10;

    // -------------------------------------------------------------------
    // Benchmark: SEIRMixingQuarantine
    // -------------------------------------------------------------------
    std::printf("=== Benchmark: SEIRMixingQuarantine vs SEIRNetworkQuarantine ===\n");
    std::printf("Population: %zu agents in %zu groups of %zu\n",
        n_total, n_groups, group_size);
    std::printf("Mean contacts: %.1f, Days: %d, Reps: %d\n\n",
        mean_contacts, n_days, n_reps);

    // Create entities for mixing model
    {
        epimodels::ModelSEIRMixingQuarantine<> model_mix(
            "Flu",
            static_cast<epiworld_fast_uint>(n_total),
            prevalence,
            mean_contacts,       // contact_rate
            transmission_rate,
            avg_incubation_days,
            recovery_rate,
            contact_matrix,
            hospitalization_rate,
            hospitalization_period,
            days_undetected,
            quarantine_period,
            quarantine_willingness,
            isolation_willingness,
            isolation_period
        );

        // Create entities (groups) for the mixing model
        for (size_t g = 0; g < n_groups; ++g)
        {
            int from = static_cast<int>(g * group_size);
            int to   = static_cast<int>((g + 1) * group_size);
            Entity<> ent(
                std::string("Group ") + std::to_string(g),
                range_dist<>(from, to)
            );
            model_mix.add_entity(ent);
        }

        model_mix.verbose_off();

        // Warm-up run
        model_mix.run(n_days, 999);

        double total_ms = 0.0;
        for (int rep = 0; rep < n_reps; ++rep)
        {
            auto t0 = std::chrono::high_resolution_clock::now();
            model_mix.run(n_days, static_cast<size_t>(rep + 1));
            auto t1 = std::chrono::high_resolution_clock::now();

            total_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        std::printf("SEIRMixingQuarantine:  avg time per run = %.2f ms\n",
            total_ms / n_reps);
        model_mix.print();
    }

    std::printf("\n");

    // -------------------------------------------------------------------
    // Benchmark: SEIRNetworkQuarantine
    // -------------------------------------------------------------------
    {
        epimodels::ModelSEIRNetworkQuarantine<> model_net(
            "Flu",
            static_cast<epiworld_fast_uint>(n_total),
            prevalence,
            transmission_rate,
            avg_incubation_days,
            recovery_rate,
            hospitalization_rate,
            hospitalization_period,
            days_undetected,
            quarantine_period,
            quarantine_willingness,
            isolation_willingness,
            isolation_period
        );

        // Set up SBM network (this creates the agent connections)
        model_net.agents_sbm(
            std::vector<size_t>(n_groups, group_size),
            sbm_mixing,
            true
        );

        model_net.verbose_off();

        // Warm-up run
        model_net.run(n_days, 999);

        double total_ms = 0.0;
        for (int rep = 0; rep < n_reps; ++rep)
        {
            auto t0 = std::chrono::high_resolution_clock::now();
            model_net.run(n_days, static_cast<size_t>(rep + 1));
            auto t1 = std::chrono::high_resolution_clock::now();

            total_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        std::printf("SEIRNetworkQuarantine: avg time per run = %.2f ms\n",
            total_ms / n_reps);
        model_net.print();
    }

    return 0;
}
