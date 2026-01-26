#ifndef EPIWORLD_MATH_LFMCMC_MEAT_PRINT_HPP
#define EPIWORLD_MATH_LFMCMC_MEAT_PRINT_HPP

#include <stdexcept>
#include <algorithm>
#include <string>

#include "epiworld/math/distributions.hpp"
#include "epiworld/math/lfmcmc/lfmcmc-bones.hpp"

namespace epiworld::math::lfmcmc {
namespace {
/// @brief Compute the width needed to print a vector of numbers, as specific to the LFMCMC.
[[maybe_unused]]
size_t compute_width(std::vector<epiworld_double> const& numbers) {
    size_t nchar = 0;
    for (const auto& number : numbers)
    {
        if (number == 0.0) {
            continue;
        }

        const epiworld_double absv = std::abs(number);
        if (absv >= 1.0)
        {
            const auto tmp = static_cast<size_t>(std::log10(absv));
            nchar = std::max(nchar, tmp);
        }
    }

    // Padding: sign, decimal point, decimals
    return nchar + 2;
}
}

template<typename TData>
inline void LFMCMC<TData>::print(size_t burnin) const
{
    ////////////////////////////////////////////////////////////////////////////
    // Validate burn-in and define sample range explicitly
    ////////////////////////////////////////////////////////////////////////////

    if (burnin >= m_n_samples) {
        throw std::length_error(
            "The burnin is greater than or equal to the number of samples."
        );
    }

    const size_t first = burnin;
    const size_t last  = m_n_samples;
    const size_t n_samples_print = last - first;

    const auto n_samples_dbl =
        static_cast<epiworld_double>(n_samples_print);

    ////////////////////////////////////////////////////////////////////////////
    // Storage for summaries: [mean, lower, upper]
    ////////////////////////////////////////////////////////////////////////////

    std::vector<epiworld_double> summ_params(m_n_params * 3, 0.0);
    std::vector<epiworld_double> summ_stats(m_n_stats * 3, 0.0);

    std::vector<epiworld_double> summary_buffer;
    summary_buffer.reserve(n_samples_print);

    ////////////////////////////////////////////////////////////////////////////
    // Compute parameter summaries
    ////////////////////////////////////////////////////////////////////////////

    for (size_t k = 0; k < m_n_params; ++k)
    {
        summary_buffer.clear();

        // Copy samples and compute mean separately for clarity
        epiworld_double mean = 0.0;
        for (size_t i = first; i < last; ++i)
        {
            const epiworld_double value =
                m_all_accepted_params[i * m_n_params + k];
            summary_buffer.push_back(value);
            mean += value;
        }

        mean /= n_samples_dbl;

        // Compute quantiles using nth_element (no full sort needed)
        const size_t idx_low = quantile_index_of(CI95_LOWER_QUANTILE, n_samples_print);
        const size_t idx_upp = quantile_index_of(CI95_UPPER_QUANTILE, n_samples_print);

        std::nth_element(summary_buffer.begin(), summary_buffer.begin() + idx_low, summary_buffer.end());
        const epiworld_double q_low = summary_buffer[idx_low];

        std::nth_element(summary_buffer.begin(), summary_buffer.begin() + idx_upp, summary_buffer.end());
        const epiworld_double q_upp = summary_buffer[idx_upp];

        summ_params[k * 3 + 0] = mean;
        summ_params[k * 3 + 1] = q_low;
        summ_params[k * 3 + 2] = q_upp;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Compute statistic summaries (same structure as parameters)
    ////////////////////////////////////////////////////////////////////////////

    for (size_t k = 0; k < m_n_stats; ++k)
    {
        summary_buffer.clear();

        epiworld_double mean = 0.0;
        for (size_t i = first; i < last; ++i)
        {
            const epiworld_double value =
                m_all_accepted_stats[i * m_n_stats + k];
            summary_buffer.push_back(value);
            mean += value;
        }

        mean /= n_samples_dbl;

        const size_t idx_low = quantile_index_of(CI95_LOWER_QUANTILE, n_samples_print);
        const size_t idx_upp = quantile_index_of(CI95_UPPER_QUANTILE, n_samples_print);

        std::nth_element(summary_buffer.begin(), summary_buffer.begin() + idx_low, summary_buffer.end());
        const epiworld_double q_low = summary_buffer[idx_low];

        std::nth_element(summary_buffer.begin(), summary_buffer.begin() + idx_upp, summary_buffer.end());
        const epiworld_double q_upp = summary_buffer[idx_upp];

        summ_stats[k * 3 + 0] = mean;
        summ_stats[k * 3 + 1] = q_low;
        summ_stats[k * 3 + 2] = q_upp;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Printing header
    ////////////////////////////////////////////////////////////////////////////

    printf_epiworld("___________________________________________\n\n");
    printf_epiworld("LIKELIHOOD-FREE MARKOV CHAIN MONTE CARLO\n\n");
    printf_epiworld("N Samples (total) : %zu\n", m_n_samples);
    printf_epiworld(
        "N Samples (after burn-in period) : %zu\n",
        n_samples_print
    );

    std::string abbr;
    epiworld_double elapsed = 0.0;
    get_elapsed_time("auto", &elapsed, &abbr, false);
    printf_epiworld("Elapsed t : %.2f%s\n\n", elapsed, abbr.c_str());

    ////////////////////////////////////////////////////////////////////////////
    // PARAMETERS
    ////////////////////////////////////////////////////////////////////////////

    printf_epiworld("Parameters:\n");

    const size_t width_params = compute_width(summ_params);
    const std::string charlen = std::to_string(width_params);

    if (!m_param_names.empty())
    {
        size_t name_width = 0;
        for (const auto& name : m_param_names) {
            name_width = std::max(name_width, name.size());
        }

        const std::string fmt =
            "  -%-" + std::to_string(name_width) +
            "s : % " + charlen + ".2f [% " + charlen +
            ".2f, % " + charlen +
            ".2f] (initial : % " + charlen + ".2f)\n";

        for (size_t k = 0; k < m_n_params; ++k)
        {
            printf_epiworld(
                fmt.c_str(),
                m_param_names[k].c_str(),
                summ_params[k * 3 + 0],
                summ_params[k * 3 + 1],
                summ_params[k * 3 + 2],
                m_initial_params[k]
            );
        }
    }
    else
    {
        const std::string fmt =
            "  [%-2zu] : % " + charlen + ".2f [% " + charlen +
            ".2f, % " + charlen +
            ".2f] (initial : % " + charlen + ".2f)\n";

        for (size_t k = 0; k < m_n_params; ++k)
        {
            printf_epiworld(
                fmt.c_str(),
                k,
                summ_params[k * 3 + 0],
                summ_params[k * 3 + 1],
                summ_params[k * 3 + 2],
                m_initial_params[k]
            );
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // STATISTICS
    ////////////////////////////////////////////////////////////////////////////

    printf_epiworld("\nStatistics:\n");

    const size_t width_stats = compute_width(summ_stats);
    const std::string statlen = std::to_string(width_stats);

    if (!m_stat_names.empty())
    {
        size_t name_width = 0;
        for (const auto& name : m_stat_names) {
            name_width = std::max(name_width, name.size());
        }

        const std::string fmt =
            "  -%-" + std::to_string(name_width) +
            "s : % " + statlen + ".2f [% " + statlen +
            ".2f, % " + statlen +
            ".2f] (Observed: % " + statlen + ".2f)\n";

        for (size_t k = 0; k < m_n_stats; ++k)
        {
            printf_epiworld(
                fmt.c_str(),
                m_stat_names[k].c_str(),
                summ_stats[k * 3 + 0],
                summ_stats[k * 3 + 1],
                summ_stats[k * 3 + 2],
                m_observed_stats[k]
            );
        }
    }
    else
    {
        const std::string fmt =
            "  [%-2zu] : % " + statlen + ".2f [% " + statlen +
            ".2f, % " + statlen +
            ".2f] (Observed: % " + statlen + ".2f)\n";

        for (size_t k = 0; k < m_n_stats; ++k)
        {
            printf_epiworld(
                fmt.c_str(),
                k,
                summ_stats[k * 3 + 0],
                summ_stats[k * 3 + 1],
                summ_stats[k * 3 + 2],
                m_observed_stats[k]
            );
        }
    }

    printf_epiworld("___________________________________________\n\n");
}
}

#endif /* !EPIWORLD_MATH_LFMCMC_MEAT_PRINT_HPP */