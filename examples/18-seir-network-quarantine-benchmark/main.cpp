#include "../../include/epiworld/epiworld.hpp"

#include <chrono>
#include <cstdio>
#include <vector>

using namespace epiworld;

int main() {

    // -------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------
    const size_t n_groups      = 20;
    const size_t group_size    = 250;
    const size_t n_total       = n_groups * group_size;
    const double mean_contacts = 50.0;
    const int    n_days        = 100;
    const int    n_reps        = 50;

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

    // Build a column-major contact matrix for the mixing layer.
    // Each entry stores the expected number of contacts per day from a
    // source group to a target group.
    std::vector< double > contact_matrix(n_groups * n_groups, 0.0);
    for (size_t g = 0; g < n_groups; ++g)
    {
        for (size_t h = 0; h < n_groups; ++h)
        {
            contact_matrix[h * n_groups + g] = sbm_mixing[g * n_groups + h];
        }
    }

    // Common disease parameters
    const double prevalence             = 1.0/static_cast<double>(n_total);
    const double transmission_prob      = 0.025;
    const double latent_period          = 7.0;
    const double recovery_rate          = 1.0/5.0;
    const double hospitalization_rate   = 0.05;
    const double hospitalization_period = 7.0;
    const double detection_delay        = 3.0;
    const int    quarantine_period      = 7;
    const double quarantine_willingness = 0.8;
    const double isolation_willingness  = 0.9;
    const int    isolation_period       = 10;

    // -------------------------------------------------------------------
    // Benchmark: ModelSEIRQuarantine on a mixing layer and on a network
    // -------------------------------------------------------------------
    std::printf("=== Benchmark: SEIRQuarantine, mixing layer vs network ===\n");
    std::printf("Population: %zu agents in %zu groups of %zu\n",
        n_total, n_groups, group_size);
    std::printf("Mean contacts: %.1f, Days: %d, Reps: %d\n\n",
        mean_contacts, n_days, n_reps);

    double avg_outbreak_size_mixing = 0.0;
    double avg_outbreak_size_network = 0.0;

    auto get_os = [](const Model<> & m) -> double {

        std::vector< int > date, virus_id, size;

        m.get_db().get_outbreak_size(date, virus_id, size);

        return static_cast<double>(size.back());

    };

    // Mixing layer: groups of consecutive agents
    {
        epimodels::ModelSEIRQuarantine<> model_mix(
            "Flu",
            static_cast<epiworld_fast_uint>(n_total),
            prevalence,
            transmission_prob * .9,
            latent_period,
            recovery_rate,
            hospitalization_rate,
            hospitalization_period,
            detection_delay,
            quarantine_period,
            quarantine_willingness,
            isolation_willingness,
            isolation_period
        );

        std::vector< size_t > groups(n_total);
        for (size_t i = 0; i < n_total; ++i)
            groups[i] = i / group_size;

        auto mixing = layer::mixing<>("mixing");
        mixing->set_groups(groups, n_groups);
        mixing->set_contact_matrix(contact_matrix);
        model_mix.add_layer(mixing);

        model_mix.verbose_off();

        // Warm-up run
        model_mix.run(n_days, 999);

        double total_ms = 0.0;
        for (int rep = 0; rep < n_reps; ++rep)
        {
            auto t0 = std::chrono::high_resolution_clock::now();
            model_mix.run(n_days, static_cast<size_t>(rep + 1));
            auto t1 = std::chrono::high_resolution_clock::now();

            avg_outbreak_size_mixing += get_os(model_mix);

            total_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        std::printf("Mixing layer: avg time per run = %.2f ms\n",
            total_ms / n_reps);
        model_mix.print(true);
    }

    std::printf("\n");

    // -------------------------------------------------------------------
    // Network layer: a stochastic block model with the same mixing
    // -------------------------------------------------------------------
    {
        epimodels::ModelSEIRQuarantine<> model_net(
            "Flu",
            n_groups * group_size,
            prevalence,
            transmission_prob,
            latent_period,
            recovery_rate,
            hospitalization_rate,
            hospitalization_period,
            detection_delay,
            quarantine_period,
            quarantine_willingness,
            isolation_willingness,
            isolation_period
        );

        // Set up SBM network (this creates the agent connections)
        model_net.add_layer(layer::network(
            "network",
            rgraph_sbm(std::vector<size_t>(n_groups, group_size), sbm_mixing, true, model_net)
        ));

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
            avg_outbreak_size_network += get_os(model_net);
        }

        std::printf("Network:      avg time per run = %.2f ms\n",
            total_ms / n_reps);
        model_net.print(false);
    }

    std::printf("\n");
    std::printf("Average outbreak size - Mixing: %.2f\n", avg_outbreak_size_mixing / n_reps);
    std::printf("Average outbreak size - Network: %.2f\n", avg_outbreak_size_network / n_reps);

    return 0;
}
