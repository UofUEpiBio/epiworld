#ifndef EPIWORLD_MATH_FUNCTIONS_HPP
#define EPIWORLD_MATH_FUNCTIONS_HPP

/**
 * @file functions.hpp
 * @brief Elementary functions used across the library: `math::log1m()`,
 * `math::logistic()`, `math::logit()` and `math::geometric_skip()`.
 *
 * @details These are plain functions of numbers: they need no model, so any
 * part of the library (and user code) can call them. The distributions of
 * `math/distributions.hpp` live in the same namespace.
 *
 * The docs page "Math Functions" (docs/impl/math-functions.md) derives
 * `log1m()` and gives its error bound and references.
 */

namespace math {

/**
 * @brief The natural logarithm of one minus `p`, `log(1 - p)`, accurate for
 * every `p <= 1` (in particular for `p` near 0, where `std::log(1 - p)` loses
 * the low digits of `p`).
 *
 * @details The hazard of an exposure of probability `p` is `-log1m(p)`.
 * Write `1 - p = (1 - s) / (1 + s)`, so that
 * `log(1 - p) = -2 atanh(s) = -2 (s + s^3 / 3 + s^5 / 5 + ...)`:
 *
 * - For `p` in `[1 - sqrt(2), 1 - sqrt(2) / 2]`, `s = p / (2 - p)` directly,
 *   with `|s| <= 3 - 2 sqrt(2) = 0.1716`; there is no cancellation, so the
 *   relative error is uniform down to the smallest `p`.
 * - Otherwise `x = 1 - p` (exact for `p >= 1/2`) is split as `x = m 2^e` with
 *   `m` in `[sqrt(2) / 2, sqrt(2))`, so that `log(x) = e log(2) + log(m)` and
 *   `log(m)` uses the same series with `s = (m - 1) / (m + 1)`, again
 *   `|s| <= 0.1716`. `log(2)` is split in two parts so that `e log(2)` is
 *   exact (the range reduction of fdlibm's `log1p`).
 *
 * The series is cut after the `s^17 / 17` term: the rest is below
 * `s^18 / 19 < 1e-15` relative to the result. The function uses only
 * `+`, `-`, `*`, `/` and `std::frexp`, so it gives the same result on every
 * platform, unless the compiler fuses a multiplication and an addition (FMA
 * contraction; build with `-ffp-contract=off` to prevent it).
 *
 * @param p A number no greater than 1.
 * @return `log(1 - p)`: `-inf` for `p = 1`, `inf` for `p = -inf`, NaN for
 * `p > 1` or NaN.
 */
inline double log1m(double p)
{

    #if defined(__clang__)
    #pragma STDC FP_CONTRACT OFF
    #endif

    if (!(p < 1.0))
        return (p == 1.0) ?
            -std::numeric_limits< double >::infinity() :
            std::numeric_limits< double >::quiet_NaN();

    if (std::isinf(p)) // p = -inf
        return std::numeric_limits< double >::infinity();

    // 2 atanh(s) from u = 2 s, by the series in Horner form over z = s^2:
    // u (1 + z / 3 + z^2 / 5 + ... + z^8 / 17). Taking 2 s rather than s keeps
    // tiny p exact: p / (2 - p) is subnormal, and rounds, for p below about
    // 2^-1021, while 2 p / (2 - p) is not (above that the two give the same
    // bits, as doubling is exact).
    auto atanh2 = [](double u) -> double {
        const double s = 0.5 * u;
        const double z = s * s;
        double t = 1.0 / 17.0;
        t = 1.0 / 15.0 + z * t;
        t = 1.0 / 13.0 + z * t;
        t = 1.0 / 11.0 + z * t;
        t = 1.0 / 9.0 + z * t;
        t = 1.0 / 7.0 + z * t;
        t = 1.0 / 5.0 + z * t;
        t = 1.0 / 3.0 + z * t;
        t = 1.0 + z * t;
        return u * t; // 2 atanh(s)
    };

    // 1 - sqrt(2) and 1 - sqrt(2) / 2
    if ((p >= -0.41421356237309503) && (p <= 0.29289321881345248))
        return -atanh2(2.0 * p / (2.0 - p));

    // log(2) = ln2_hi + ln2_lo, where ln2_hi has trailing zero bits, so that
    // e * ln2_hi is exact for every exponent e
    constexpr double ln2_hi = 6.93147180369123816490e-01;
    constexpr double ln2_lo = 1.90821492927058770002e-10;
    constexpr double sqrt2_half = 0.70710678118654752440;

    int e = 0;
    double m = std::frexp(1.0 - p, &e); // 1 - p = m 2^e, m in [1/2, 1)
    if (m < sqrt2_half)
    {
        m *= 2.0;
        --e;
    }

    const double ed = static_cast< double >(e);
    return ed * ln2_hi + (atanh2(2.0 * (m - 1.0) / (m + 1.0)) + ed * ln2_lo);

}

/// The logistic function, `1 / (1 + exp(-x))`.
inline double logistic(double x)
{
    return 1.0 / (1.0 + std::exp(-x));
}

/// The log-odds of `p`, `log(p / (1 - p))` (the inverse of `logistic()`).
inline double logit(double p)
{
    return std::log(p) - log1m(p);
}

/**
 * @brief The number of failures before the first success in Bernoulli trials
 * that fail with probability `q`, drawn by inversion from a uniform `u`.
 * @details `floor(log(u) / log(q))`: geometric skipping, which visits only the
 * successes of a long sequence of trials (Batagelj and Brandes, 2005).
 * @param u A uniform draw in `(0, 1]`.
 * @param log_q `log(q)`, negative (precomputed: e.g., `log1m(p)` for
 * successes of probability `p`).
 * @return The number of failures, as a `double` (it can exceed any integer
 * type when `q` is near 1).
 */
inline double geometric_skip(double u, double log_q)
{
    return std::floor(std::log(u) / log_q);
}

}

#endif
