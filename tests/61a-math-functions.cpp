#include "tests.hpp"

using namespace epiworld;

// The elementary functions of math/functions.hpp, against the standard
// library and their definitions:
//
// - math::log1m(p) = log(1 - p): relative error below 1e-14 against
//   std::log1p(-p) for p from 1e-300 to 1 - 1e-16 and for negative p, on both
//   sides of the two range boundaries (1 - sqrt(2) and 1 - sqrt(2) / 2); the
//   special values (p = 0, 1, > 1, -inf, NaN) and subnormal p (exactly -p).
// - math::logistic() and math::logit() are inverses.
// - math::geometric_skip() has the geometric mean q / (1 - q).
// - math::log_factorial() matches std::lgamma(n + 1).

EPIWORLD_TEST_CASE("Math functions", "[math]") {

    // -- log1m ---------------------------------------------------------------
    std::vector< double > ps = {
        0.0, 1e-300, 1e-200, 1e-100, 1e-30, 1e-16, 1e-12, 1e-8, 1e-6, 1e-4,
        1e-3, 0.01, 0.05, 0.1, 0.2, 0.25, 0.5, 0.75, 0.9, 0.99, 0.999,
        1.0 - 1e-6, 1.0 - 1e-12, 1.0 - 1e-16,
        -1e-12, -0.01, -0.1, -0.4, -1.0, -10.0, -1e10
    };

    // Both sides of the boundaries of the direct branch
    for (double b : {1.0 - std::sqrt(2.0), 1.0 - std::sqrt(2.0) / 2.0})
        for (double d : {-1e-9, -1e-15, 0.0, 1e-15, 1e-9})
            ps.push_back(b + d);

    // A log-spaced sweep of (0, 1)
    for (double x = 1e-12; x < 1.0; x *= 1.07)
    {
        ps.push_back(x);
        ps.push_back(1.0 - x);
    }

    double worst = 0.0;
    for (double p : ps)
    {
        const double want = std::log1p(-p);
        const double got = math::log1m(p);
        const double rel = (want == 0.0) ?
            std::abs(got) : std::abs(got - want) / std::abs(want);
        INFO("p = " << p << ": " << got << " vs " << want);
        REQUIRE(rel < 1e-14);
        worst = std::max(worst, rel);
    }
    INFO("worst relative error " << worst);
    REQUIRE(worst < 1e-14);

    REQUIRE(math::log1m(0.0) == 0.0);
    REQUIRE(std::isinf(math::log1m(1.0)));
    REQUIRE(math::log1m(1.0) < 0.0);
    REQUIRE(std::isnan(math::log1m(1.5)));
    REQUIRE(std::isnan(math::log1m(std::numeric_limits< double >::quiet_NaN())));
    REQUIRE(math::log1m(-std::numeric_limits< double >::infinity()) ==
        std::numeric_limits< double >::infinity());

    // Subnormal p: log(1 - p) = -p exactly
    const double tiny = std::numeric_limits< double >::denorm_min();
    for (double p : {tiny, 3.0 * tiny, -tiny, 1000.0 * tiny})
        REQUIRE(math::log1m(p) == -p);

    // -- logistic and logit ---------------------------------------------------
    // (Beyond |x| = 10, logistic(x) is so close to 0 or 1 that the spacing
    // of doubles there limits how well logit() can invert it.)
    for (double x : {-10.0, -3.0, -0.5, 0.0, 0.5, 3.0, 10.0})
        REQUIRE_THAT(
            math::logit(math::logistic(x)), Catch::Matchers::WithinAbs(x, 1e-9)
        );
    REQUIRE(math::logistic(0.0) == 0.5);
    REQUIRE_THAT(math::logit(0.5), Catch::Matchers::WithinAbs(0.0, 1e-15));

    // -- geometric_skip --------------------------------------------------------
    // Failures before the first success, success probability 1 - q: mean
    // q / (1 - q). Uniforms on a fine grid of (0, 1] stand in for draws.
    for (double q : {0.5, 0.9, 0.99})
    {
        const size_t n = 200000u;
        double mean = 0.0;
        for (size_t k = 1u; k <= n; ++k)
            mean += math::geometric_skip(
                (static_cast< double >(k) - 0.5) / static_cast< double >(n),
                std::log(q)
            ) / static_cast< double >(n);
        REQUIRE_THAT(mean, Catch::Matchers::WithinRel(q / (1.0 - q), 1e-3));
    }

    // -- log_factorial ---------------------------------------------------------
    for (int n : {0, 1, 2, 5, 10, 50, 170})
        REQUIRE_THAT(
            math::log_factorial(n),
            Catch::Matchers::WithinRel(std::lgamma(n + 1.0), 1e-12)
        );

}
