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
 * do not change the course of the disease.
 *
 * The duration is drawn at the agent's first update in the stage, whatever
 * put the agent there (an infection by contact, an initial case, a case
 * imported with `Agent::set_virus()`, or the end of the previous stage). Each
 * agent records the stage and the infection (its virus's infection date) the
 * end day belongs to; a mismatch means a new stage. The latent stage is
 * counted from the infection date; the others from the day before the first
 * update, the day the agent entered them (agents with a virus are updated
 * every day).
 *
 * @tparam TSeq Type for genetic sequences.
 * @tparam TModel The measles model (for its state codes).
 */
template<typename TSeq, typename TModel>
class MeaslesStages {
protected:

    std::vector< int > stage_end;       ///< Day each agent leaves its stage
    std::vector< int > stage_kind;      ///< Stage that `stage_end` belongs to
    std::vector< int > stage_infection; ///< Infection date it belongs to

    enum Stage { NO_STAGE = 0, LATENT_STAGE, PRODROMAL_STAGE, RASH_STAGE };

    /// The stage of a state.
    static Stage stage_of(int s)
    {
        if ((s == TModel::LATENT) || (s == TModel::QUARANTINED_LATENT))
            return LATENT_STAGE;
        if ((s == TModel::PRODROMAL) || (s == TModel::QUARANTINED_PRODROMAL))
            return PRODROMAL_STAGE;
        if ((s == TModel::RASH) || (s == TModel::ISOLATED))
            return RASH_STAGE;
        return NO_STAGE;
    }

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
     * @brief Whether the agent's stage ends today. Draws the stage's
     * duration first if the agent has just entered it.
     * @param p Agent, with a virus, in a state of one of the three stages.
     * @param m Model.
     */
    bool stage_over(Agent<TSeq> * p, Model<TSeq> * m)
    {

        const auto id = p->get_id();
        const Stage stage = stage_of(static_cast<int>(p->get_state()));
        const int infection = p->get_virus()->get_date();

        if ((stage_kind[id] != stage) || (stage_infection[id] != infection))
        {

            int start;
            epiworld_double mean, sd;
            if (stage == LATENT_STAGE)
            {
                start = infection;
                mean  = m->par("Incubation period");
                sd    = m->par("Incubation period sd");
            }
            else if (stage == PRODROMAL_STAGE)
            {
                start = m->today() - 1;
                mean  = m->par("Prodromal period");
                sd    = m->par("Prodromal period sd");
            }
            else
            {
                start = m->today() - 1;
                mean  = m->par("Rash period");
                sd    = m->par("Rash period sd");
            }

            stage_end[id]       = start + draw_days(m, mean, sd);
            stage_kind[id]      = stage;
            stage_infection[id] = infection;

        }

        return m->today() >= stage_end[id];

    }

    /// Clears the stages (called by the model's `reset()`).
    void reset_stages(Model<TSeq> * m)
    {
        stage_end.assign(m->size(), 0);
        stage_kind.assign(m->size(), NO_STAGE);
        stage_infection.assign(m->size(), -1);
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
