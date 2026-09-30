#ifndef EPIWORLD_STATS_HPP
#define EPIWORLD_STATS_HPP

/**
 * @file stats.hpp
 * @brief Basic summary statistics for tests, examples and benchmarks.
 *
 * @details Plain functions over `std::vector<double>`; no dependency on
 * epiworld. `mean()` and `variance()` add the values in order, so they return
 * exactly what `std::accumulate` would.
 */

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace stats {

/** Mean of @p x (NaN if empty). */
inline double mean(const std::vector< double > & x)
{
    return std::accumulate(x.begin(), x.end(), 0.0) /
        static_cast< double >(x.size());
}

/** Sample variance of @p x, with the `n - 1` denominator. */
inline double variance(const std::vector< double > & x)
{
    const double m = mean(x);
    return std::accumulate(
        x.begin(), x.end(), 0.0,
        [m](double acc, double y) { return acc + (y - m) * (y - m); }
    ) / (static_cast< double >(x.size()) - 1.0);
}

/** Sample standard deviation of @p x. */
inline double sd(const std::vector< double > & x)
{
    return std::sqrt(variance(x));
}

/**
 * Quantile @p q in [0, 1] of @p x, interpolating linearly between the order
 * statistics (R's type 7). The vector is taken by value and sorted.
 */
inline double quantile(std::vector< double > x, double q)
{
    std::sort(x.begin(), x.end());
    const double pos = q * static_cast< double >(x.size() - 1u);
    const size_t lo = static_cast< size_t >(std::floor(pos));
    const size_t hi = static_cast< size_t >(std::ceil(pos));
    return x[lo] + (x[hi] - x[lo]) * (pos - static_cast< double >(lo));
}

/** Median of @p x. */
inline double median(const std::vector< double > & x)
{
    return quantile(x, 0.5);
}

} // namespace stats

#endif
