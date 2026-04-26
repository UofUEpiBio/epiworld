#include <cmath>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

// We disable the fast binomial approximation to Poisson in
// this example, so that we can compare the two distributions
// directly. In practice, the approximation is very good for
// the parameter values used in this example, and the speedup is
// significant.
#define EPI_NO_FAST_BINOM

#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

struct DrawSummary {
    double mean = 0.0;
    double variance = 0.0;
    double elapsed_seconds = 0.0;
    int min = std::numeric_limits<int>::max();
    int max = std::numeric_limits<int>::min();
    std::vector<size_t> counts;
    size_t overflow = 0u;
};

template <typename DrawFun>
DrawSummary summarize_draws(size_t ndraws, int max_k, DrawFun draw_fun)
{
    DrawSummary out;
    out.counts.resize(static_cast<size_t>(max_k + 1), 0u);

    double sum = 0.0;
    double sumsq = 0.0;
    auto start = std::chrono::steady_clock::now();

    for (size_t i = 0u; i < ndraws; ++i)
    {
        int value = draw_fun();

        sum += static_cast<double>(value);
        sumsq += static_cast<double>(value) * static_cast<double>(value);

        if (value < out.min)
            out.min = value;
        if (value > out.max)
            out.max = value;

        if (value <= max_k)
            out.counts[static_cast<size_t>(value)]++;
        else
            out.overflow++;
    }

    auto end = std::chrono::steady_clock::now();

    out.mean = sum / static_cast<double>(ndraws);
    out.variance =
        (sumsq - (sum * sum / static_cast<double>(ndraws))) /
        static_cast<double>(ndraws - 1u);
    out.elapsed_seconds =
        std::chrono::duration<double>(end - start).count();

    return out;
}

void print_summary(
    const std::string & label,
    const DrawSummary & summary,
    size_t ndraws
)
{
    std::cout << label << "\n"
              << "  mean     : " << summary.mean << "\n"
              << "  variance : " << summary.variance << "\n"
              << "  min/max  : " << summary.min << "/" << summary.max << "\n"
              << "  elapsed  : " << summary.elapsed_seconds << " seconds\n"
              << "  draws/s  : "
              << static_cast<double>(ndraws) / summary.elapsed_seconds << "\n"
              << "  tail mass beyond 40: "
              << static_cast<double>(summary.overflow) / static_cast<double>(ndraws)
              << "\n";
}

int main()
{
    constexpr int n = 5000;
    constexpr double p = 20.0 / static_cast<double>(n);
    constexpr double lambda = static_cast<double>(n) * p;
    constexpr size_t ndraws = 100000u;
    constexpr int max_k = 40;

    Model<int> binom_rng;
    Model<int> poiss_rng;

    binom_rng.seed(20260423u);
    poiss_rng.seed(20260424u);

    auto binom = summarize_draws(
        ndraws,
        max_k,
        [&binom_rng]() -> int { return binom_rng.rbinom(n, p); }
    );

    auto poiss = summarize_draws(
        ndraws,
        max_k,
        [&poiss_rng]() -> int { return poiss_rng.rpoiss(lambda); }
    );

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Comparing epiworld::Model::rbinom and epiworld::Model::rpoiss\n";
    std::cout << "Binomial target: n = " << n << ", p = " << p << "\n";
    std::cout << "Poisson approximation: lambda = n * p = " << lambda << "\n";
    std::cout << "Number of draws per distribution: " << ndraws << "\n\n";

    std::cout << "Theoretical moments\n";
    std::cout << "  Binomial mean     : " << static_cast<double>(n) * p << "\n";
    std::cout << "  Binomial variance : "
              << static_cast<double>(n) * p * (1.0 - p) << "\n";
    std::cout << "  Poisson mean      : " << lambda << "\n";
    std::cout << "  Poisson variance  : " << lambda << "\n\n";

    std::cout << "Empirical moments from epiworld draws\n";
    print_summary("Binomial draws", binom, ndraws);
    print_summary("Poisson draws", poiss, ndraws);

    std::cout << "\nPerformance comparison\n";
    std::cout << "  Speedup (Binomial / Poisson): "
              << binom.elapsed_seconds / poiss.elapsed_seconds << "x\n";

    double max_abs_diff = 0.0;
    int max_abs_diff_at = 0;
    for (int k = 0; k <= max_k; ++k)
    {
        double pk_binom =
            static_cast<double>(binom.counts[static_cast<size_t>(k)]) /
            static_cast<double>(ndraws);
        double pk_poiss =
            static_cast<double>(poiss.counts[static_cast<size_t>(k)]) /
            static_cast<double>(ndraws);
        double abs_diff = std::fabs(pk_binom - pk_poiss);
        if (abs_diff > max_abs_diff)
        {
            max_abs_diff = abs_diff;
            max_abs_diff_at = k;
        }
    }

    std::cout << "\nSelected empirical probabilities\n";
    std::cout << "  k      Binomial    Poisson\n";
    for (int k : {10, 15, 20, 25, 30})
    {
        double pk_binom =
            static_cast<double>(binom.counts[static_cast<size_t>(k)]) /
            static_cast<double>(ndraws);
        double pk_poiss =
            static_cast<double>(poiss.counts[static_cast<size_t>(k)]) /
            static_cast<double>(ndraws);

        std::cout << "  " << std::setw(2) << k << "     "
                  << std::setw(8) << pk_binom << "   "
                  << std::setw(8) << pk_poiss << "\n";
    }

    std::cout << "\nLargest empirical |P(X = k) difference| on k = 0,...," << max_k
              << " occurred at k = " << max_abs_diff_at
              << " with difference " << max_abs_diff << "\n";

    return 0;
}
