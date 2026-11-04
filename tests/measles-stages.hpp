#ifndef EPIWORLD_TESTS_MEASLES_STAGES_HPP
#define EPIWORLD_TESTS_MEASLES_STAGES_HPP

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

/**
 * Probability that a measles stage lasts d days, d = 1, ..., dmax (entry
 * d - 1): the lognormal draw of `MeaslesStages::draw_days()`, rounded to whole
 * days with a minimum of one.
 */
inline std::vector< double > measles_stage_pmf(
    double mean,
    double sd,
    int dmax = 200
) {

    std::vector< double > pmf(dmax, 0.0);
    if ((sd <= 0.0) || (mean <= 0.0))
    {
        pmf[std::max(1, static_cast<int>(std::lround(mean))) - 1] = 1.0;
        return pmf;
    }

    double s2 = std::log(1.0 + (sd * sd) / (mean * mean));
    double mu = std::log(mean) - s2 / 2.0;
    double s  = std::sqrt(s2);

    double prev = 0.0;
    for (int d = 1; d <= dmax; ++d)
    {
        double cdf = 0.5 * std::erfc(-(std::log(d + 0.5) - mu) / (s * std::sqrt(2.0)));
        pmf[d - 1] = cdf - prev;
        prev = cdf;
    }

    return pmf;

}

/**
 * Exits from the Rash state of the measles models. Each day with rash, an
 * agent is hospitalized with probability `h`; otherwise, it recovers when its
 * rash period ends; otherwise, it is detected (and isolated) with probability
 * `detection`.
 *
 * - `recovery_rate`: recoveries from Rash per agent-day in Rash, which is what
 *   the transition matrix reports (including agents detected on the day they
 *   recover). It holds only when agents do not come back from Isolated to
 *   Rash, that is, without isolation.
 * - `p_hospitalized`: share of cases hospitalized. Isolation keeps the rash
 *   period and the daily risk, so it does not change it.
 */
struct MeaslesRashExits {
    double recovery_rate;
    double p_hospitalized;
};

inline MeaslesRashExits measles_rash_exits(
    double mean,
    double sd,
    double h,
    double detection
) {

    auto pmf = measles_stage_pmf(mean, sd);
    double stay = (1.0 - h) * (1.0 - detection);

    double surv = 1.0;      // P(rash period >= k)
    double stay_k = 1.0;    // stay^(k - 1)
    double days = 0.0, recoveries = 0.0, not_hospitalized = 0.0;
    for (size_t k = 1u; k <= pmf.size(); ++k)
    {
        days             += surv * stay_k;
        recoveries       += pmf[k - 1] * stay_k * (1.0 - h);
        not_hospitalized += pmf[k - 1] * std::pow(1.0 - h, static_cast<double>(k));
        surv   -= pmf[k - 1];
        stay_k *= stay;
    }

    return {recoveries / days, 1.0 - not_hospitalized};

}

/**
 * Number of agents that moved from state `from` to state `to` on each day of
 * the last run (entry `d` is day `d`).
 */
template<typename TSeq>
inline std::vector< int > transitions_by_day(
    const epiworld::Model<TSeq> & model,
    const std::string & from,
    const std::string & to
) {

    std::vector< std::string > state_from, state_to;
    std::vector< int > date, counts;
    model.get_db().get_hist_transition_matrix(
        state_from, state_to, date, counts, true
    );

    std::vector< int > out(model.get_ndays() + 1, 0);
    for (size_t i = 0u; i < date.size(); ++i)
        if ((state_from[i] == from) && (state_to[i] == to))
            out[date[i]] += counts[i];

    return out;

}

/**
 * Number of agents that moved from stage `from` to stage `to` of natural
 * history `k` on each day of the last run (entry `d` is day `d`).
 */
template<typename TSeq>
inline std::vector< int > stage_transitions_by_day(
    const epiworld::Model<TSeq> & model,
    const std::string & from,
    const std::string & to,
    size_t k = 0u
) {

    std::vector< std::string > stage_from, stage_to;
    std::vector< int > date, counts;
    model.get_db().get_hist_stage_transition_matrix(
        k, stage_from, stage_to, date, counts
    );

    std::vector< int > out(model.get_ndays() + 1, 0);
    for (size_t i = 0u; i < date.size(); ++i)
        if ((stage_from[i] == from) && (stage_to[i] == to))
            out[date[i]] += counts[i];

    return out;

}

/**
 * Pooled daily rates of a model with statuses and stages, over many runs.
 *
 * Each run adds its stage and status transitions, and the agent-days spent
 * in each pair of status and stage (at every record but the last, so each
 * day's transitions have the occupancy at its start). Rates pool the counts
 * of all runs: transitions over agent-days.
 */
struct StatusStageRates {

    size_t nstatus = 0u;
    size_t nstages = 0u;
    std::vector< std::vector< double > > stage, status, occupancy; ///< By run

    StatusStageRates(size_t nruns) :
        stage(nruns), status(nruns), occupancy(nruns) {};

    /// The saver for `run_multiple()` (each run writes its own slot).
    std::function<void(size_t, epiworld::Model<>*)> saver() {
        return [this](size_t n, epiworld::Model<> * m) -> void {
            const auto & db = m->get_db();
            stage[n]  = db.get_stage_transition_probability(0u, false);
            status[n] = db.get_transition_probability(false, false);

            std::vector< int > date, counts;
            std::vector< std::string > st, sg;
            db.get_hist_joint(0u, date, st, sg, counts);
            const size_t cells = m->get_n_states() *
                m->get_natural_history(0u).size();
            occupancy[n].assign(cells, 0.0);
            for (size_t i = 0u; i + cells < counts.size(); ++i)
                occupancy[n][i % cells] += counts[i];
        };
    }

    /// Adds up the runs. Call once, after `run_multiple()`.
    void pool(const epiworld::Model<> & m) {
        nstatus = m.get_n_states();
        nstages = m.get_natural_history(0u).size();
        for (auto * v : {&stage, &status, &occupancy})
        {
            std::vector< double > total((*v)[0].size(), 0.0);
            for (const auto & run : *v)
                for (size_t i = 0u; i < run.size(); ++i)
                    total[i] += run[i];
            v->assign(1u, total);
        }
    }

    /// Daily rate of stage `from` -> `to` (all statuses).
    double stage_rate(int from, int to) const {
        double total = 0.0;
        for (size_t j = 0u; j < nstages; ++j)
            total += stage[0][from + j * nstages];
        return stage[0][from + to * nstages] / total;
    }

    /// Daily rate of status `from` -> `to` (all stages).
    double status_rate(int from, int to) const {
        double total = 0.0;
        for (size_t j = 0u; j < nstatus; ++j)
            total += status[0][from + j * nstatus];
        return status[0][from + to * nstatus] / total;
    }

    /// Daily rate of status `from` -> `to` per agent-day in (`from`,
    /// `stage`), when only agents in that stage make the move.
    double status_rate_in(int from, int to, int stage_) const {
        return status[0][from + to * nstatus] /
            occupancy[0][from + stage_ * nstatus];
    }

};

/// Saves each run's final stage counts (natural history 0).
inline std::function<void(size_t, epiworld::Model<>*)> final_stage_saver(
    std::vector< std::vector< int > > & final_stages
) {
    return [&final_stages](size_t n, epiworld::Model<> * m) -> void {
        final_stages[n] = m->get_db().get_today_stage(0u);
    };
}

#endif
