#ifndef MEASLES_STAGES_HPP
#define MEASLES_STAGES_HPP

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace epiworld;

/**
 * @brief Durations of the disease stages of the measles models
 *
 * @details
 * The latent, prodromal and rash stages last a number of days drawn when the
 * agent enters the stage, from a lognormal distribution with the stage's mean
 * and standard deviation (in days, on the natural scale). The draw is rounded
 * to whole days, with a minimum of one, and the agent leaves the stage on the
 * update of that day. A standard deviation of zero gives a fixed duration.
 *
 * | Stage     | States                          | Parameters                                     |
 * |-----------|---------------------------------|------------------------------------------------|
 * | Latent    | Latent, Quarantined Latent      | `Incubation period`, `Incubation period sd`    |
 * | Prodromal | Prodromal, Quarantined Prodromal| `Prodromal period`, `Prodromal period sd`      |
 * | Rash      | Rash, Isolated                  | `Rash period`, `Rash period sd`                |
 *
 * Moving between the states of a stage (for example, from Latent to
 * Quarantined Latent) keeps the day the stage ends: quarantine and isolation
 * do not change the course of the disease. Agents that start a run in a stage
 * draw its duration on day 0.
 *
 * @tparam TSeq Type for genetic sequences.
 * @tparam TModel The measles model (for its state codes).
 */
template<typename TSeq, typename TModel>
class MeaslesStages {
protected:

    std::vector< int > stage_end; ///< Day each agent leaves its disease stage

    /**
     * @brief Draws the number of days of a stage.
     * @param m Model (for its random number generator).
     * @param mean Mean duration in days.
     * @param sd Standard deviation of the duration in days.
     * @return The duration, in whole days (at least one).
     */
    static int draw_days(
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
     * @brief Starts a stage for an agent: draws when it ends.
     * @param p Agent entering the stage.
     * @param m Model.
     * @param par_mean Name of the parameter with the mean duration.
     * @param par_sd Name of the parameter with the standard deviation.
     */
    void start_stage(
        Agent<TSeq> * p,
        Model<TSeq> * m,
        const std::string & par_mean,
        const std::string & par_sd
    )
    {
        stage_end[p->get_id()] =
            m->today() + draw_days(m, m->par(par_mean), m->par(par_sd));
    }

    void start_latent(Agent<TSeq> * p, Model<TSeq> * m)
    {
        start_stage(p, m, "Incubation period", "Incubation period sd");
    }

    void start_prodromal(Agent<TSeq> * p, Model<TSeq> * m)
    {
        start_stage(p, m, "Prodromal period", "Prodromal period sd");
    }

    void start_rash(Agent<TSeq> * p, Model<TSeq> * m)
    {
        start_stage(p, m, "Rash period", "Rash period sd");
    }

    /// Whether the agent's stage ends today.
    bool stage_over(Agent<TSeq> * p, Model<TSeq> * m) const
    {
        return m->today() >= stage_end[p->get_id()];
    }

    /**
     * @brief Draws the stage durations of the agents that start a run in a
     * stage (called at the end of the model's `reset()`).
     */
    void reset_stages(Model<TSeq> * m)
    {

        stage_end.assign(m->size(), 0);
        for (auto & agent : m->get_agents())
        {

            if (agent.get_virus() == nullptr)
                continue;

            const auto s = agent.get_state();
            if ((s == TModel::LATENT) || (s == TModel::QUARANTINED_LATENT))
                start_latent(&agent, m);
            else if ((s == TModel::PRODROMAL) || (s == TModel::QUARANTINED_PRODROMAL))
                start_prodromal(&agent, m);
            else if ((s == TModel::RASH) || (s == TModel::ISOLATED))
                start_rash(&agent, m);

        }

    }

    /// Checks the parameters of the stage durations.
    static void check_stage_sds(
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

};

#endif
