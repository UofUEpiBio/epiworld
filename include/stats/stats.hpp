#ifndef EPIWORLD_STATS_HPP
#define EPIWORLD_STATS_HPP

/**
 * @file stats.hpp
 * @brief Basic summary statistics for tests, examples and benchmarks.
 *
 * @details Functions over `std::vector<T>` for any arithmetic `T` (e.g.,
 * `epiworld_double`, `int`, `size_t`); they accumulate and return `double`. No
 * dependency on epiworld. `mean()` adds the values in order, so it returns
 * exactly what `std::accumulate(..., 0.0) / n` would.
 */

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

namespace stats {

/** Mean of @p x, accumulated in double (NaN if empty). */
template< typename T >
inline double mean(const std::vector< T > & x)
{
    return std::accumulate(x.begin(), x.end(), 0.0) /
        static_cast< double >(x.size());
}

/** Sample variance of @p x, with the `n - 1` denominator (NaN if n < 2). */
template< typename T >
inline double variance(const std::vector< T > & x)
{
    if (x.size() < 2u)
        return std::numeric_limits< double >::quiet_NaN();

    const double m = mean(x);
    return std::accumulate(
        x.begin(), x.end(), 0.0,
        [m](double acc, T y) {
            const double d = static_cast< double >(y) - m;
            return acc + d * d;
        }
    ) / (static_cast< double >(x.size()) - 1.0);
}

/** Sample standard deviation of @p x. */
template< typename T >
inline double sd(const std::vector< T > & x)
{
    return std::sqrt(variance(x));
}

/**
 * Quantile @p q in [0, 1] of @p x, interpolating linearly between the order
 * statistics (R's type 7). NaN if @p x is empty. The vector is taken by value
 * and sorted.
 */
template< typename T >
inline double quantile(std::vector< T > x, double q)
{
    if (x.empty())
        return std::numeric_limits< double >::quiet_NaN();

    std::sort(x.begin(), x.end());
    const double pos = q * static_cast< double >(x.size() - 1u);
    const size_t lo = static_cast< size_t >(std::floor(pos));
    const size_t hi = static_cast< size_t >(std::ceil(pos));
    const double xlo = static_cast< double >(x[lo]);
    const double xhi = static_cast< double >(x[hi]);
    return xlo + (xhi - xlo) * (pos - static_cast< double >(lo));
}

/** Median of @p x (NaN if empty). */
template< typename T >
inline double median(const std::vector< T > & x)
{
    return quantile(x, 0.5);
}

} // namespace stats

#endif
