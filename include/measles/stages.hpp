#ifndef MEASLES_STAGES_HPP
#define MEASLES_STAGES_HPP

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace epiworld;

/**
 * @brief Draws the number of days of a measles stage.
 *
 * @details A lognormal draw with the given mean and standard deviation (in
 * days, on the natural scale), rounded to whole days, with a minimum of one.
 * A standard deviation of zero gives a fixed duration.
 *
 * @param m Model (for its random number generator).
 * @param mean Mean duration in days.
 * @param sd Standard deviation of the duration in days.
 * @return The duration, in whole days (at least one).
 */
template<typename TSeq>
inline int measles_draw_days(
    Model<TSeq> * m,
    epiworld_double mean,
    epiworld_double sd
)
{

    epiworld_double days = mean;
    if ((sd > 0.0) && (mean > 0.0))
    {
        // A standard normal from two uniforms (Box-Muller). Unlike
        // Model::rlognormal(), whose distribution keeps a spare normal
        // between calls (and across runs, as seeding does not reset it),
        // this keeps no state, so a run's draws depend only on its seed.
        epiworld_double u1 = 1.0 - m->runif(); // in (0, 1]
        epiworld_double u2 = m->runif();
        epiworld_double z  = std::sqrt(-2.0 * std::log(u1)) *
            std::cos(2.0 * M_PI * u2);

        epiworld_double s2 = std::log(1.0 + (sd * sd) / (mean * mean));
        days = std::exp(std::log(mean) - s2 / 2.0 + std::sqrt(s2) * z);
    }

    return std::max(1, static_cast<int>(std::lround(days)));

}

/**
 * @brief The duration of a measles stage, for `NaturalHistory::set_duration()`.
 * @param mean Name of the parameter with the mean duration in days.
 * @param sd Name of the parameter with its standard deviation.
 */
template<typename TSeq>
inline StageDurationFun<TSeq> measles_stage_days(
    std::string mean,
    std::string sd
)
{
    return [mean, sd](Agent<TSeq> *, Model<TSeq> * m) -> int {
        return measles_draw_days(m, m->par(mean), m->par(sd));
    };
}

/// Checks the standard deviations of the measles stage durations.
inline void measles_check_stage_sds(
    epiworld_double incubation_period_sd,
    epiworld_double prodromal_period_sd,
    epiworld_double rash_period_sd,
    const char * model_name
)
{
    auto max_double = std::numeric_limits< double >::max();
    EpiAssert::check_bounds(incubation_period_sd, 0.0, max_double, "incubation_period_sd", model_name);
    EpiAssert::check_bounds(prodromal_period_sd, 0.0, max_double, "prodromal_period_sd", model_name);
    EpiAssert::check_bounds(rash_period_sd, 0.0, max_double, "rash_period_sd", model_name);
}

#endif
