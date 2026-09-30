#ifndef EPIWORLD_BENCH_HPP
#define EPIWORLD_BENCH_HPP

/**
 * @file bench.hpp
 * @brief Timing and checksums for the benchmark examples.
 *
 * @details Wall-clock time is unreliable on a loaded machine, so runs are
 * timed in CPU time. `time_runs()` warms a model up, times `reps` runs and
 * hashes each run's daily counts and transmissions. Equal checksums mean
 * bit-identical runs, e.g., across two versions of epiworld.
 *
 * The header only uses long-standing parts of the epiworld API, so the
 * benchmarks that include it still compile against older releases.
 */

#include <cstdint>
#include <ctime>
#include <functional>
#include <vector>

#include "../epiworld/epiworld.hpp"

namespace bench {

/** CPU milliseconds between two `std::clock()` readings. */
inline double cpu_ms(std::clock_t t0, std::clock_t t1)
{
    return 1000.0 * static_cast< double >(t1 - t0) / CLOCKS_PER_SEC;
}

/** FNV-1a over a stream of integers. */
inline void hash_ints(uint64_t & h, const std::vector< int > & x)
{
    for (int v : x)
    {
        uint32_t u = static_cast< uint32_t >(v);
        for (int b = 0; b < 4; ++b)
        {
            h ^= (u >> (8 * b)) & 0xffu;
            h *= 1099511628211ull;
        }
    }
}

/** Hash of the daily counts and the transmissions of the last run. */
inline uint64_t run_checksum(epiworld::Model<> & m)
{
    uint64_t h = 1469598103934665603ull;
    std::vector< int > counts;
    m.get_db().get_hist_total(nullptr, nullptr, &counts);
    hash_ints(h, counts);
    std::vector< int > date, source, target, virus, sexp;
    m.get_db().get_transmissions(date, source, target, virus, sexp);
    hash_ints(h, date);
    hash_ints(h, source);
    hash_ints(h, target);
    hash_ints(h, virus);
    hash_ints(h, sexp);
    return h;
}

/** The outcome of `time_runs()`. */
struct Timing {
    std::vector< double > ms;         ///< CPU milliseconds of each run
    std::vector< double > final_size; ///< Agents not in a susceptible state at the end
    uint64_t checksum = 1469598103934665603ull; ///< Over every run
};

/**
 * Runs @p model once to warm it up (seed 999), then times @p reps runs of
 * @p days days (seeds 1000, 1001, ...).
 *
 * @param n_susceptible_states The states 0, ..., n - 1 are the susceptible
 * ones; the final size is the number of agents outside them.
 * @param after_warmup Called once after the warm-up run, e.g., to reset a
 * counter the model's callbacks keep.
 */
inline Timing time_runs(
    epiworld::Model<> & model,
    int days,
    int reps,
    size_t n_susceptible_states = 1u,
    const std::function< void() > & after_warmup = nullptr
)
{
    Timing res;
    model.verbose_off();

    model.run(static_cast< epiworld_fast_uint >(days), 999);
    if (after_warmup)
        after_warmup();

    for (int r = 0; r < reps; ++r)
    {
        std::clock_t t0 = std::clock();
        model.run(static_cast< epiworld_fast_uint >(days), 1000 + r);
        std::clock_t t1 = std::clock();
        res.ms.push_back(cpu_ms(t0, t1));

        std::vector< int > today;
        model.get_db().get_today_total(nullptr, &today);
        double susceptible = 0.0;
        for (size_t s = 0u; s < n_susceptible_states; ++s)
            susceptible += today[s];
        res.final_size.push_back(
            static_cast< double >(model.size()) - susceptible
        );

        uint64_t h = run_checksum(model);
        res.checksum ^= h + 0x9e3779b97f4a7c15ull +
            (res.checksum << 6) + (res.checksum >> 2);
    }

    return res;
}

} // namespace bench

#endif
