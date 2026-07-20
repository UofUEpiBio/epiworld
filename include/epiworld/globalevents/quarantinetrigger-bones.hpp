#ifndef QUARANTINETRIGGER_BONES_HPP
#define QUARANTINETRIGGER_BONES_HPP

#include <vector>
#include "../config.hpp"

template<typename TSeq = EPI_DEFAULT_TSEQ>
class QuarantineTrigger {
private:
    int _model_sim_id = -1;
    int _day = -1;
    std::vector< size_t > _agents_triggering_quarantine;
    std::vector< int > _date_infectious;
    
    void _setup(const Model<TSeq> & model);
public:
    QuarantineTrigger() = default;
    void add_triggering_agent(
        const Model<TSeq> & model,
        const Agent<TSeq> & agent,
        int date_infectious
    );

    std::vector< size_t > & get_triggering_agents();
    std::vector< int > & get_date_infectious();

    /**
     * @brief Whether a case was identified today.
     *
     * @details
     * The triggering set is only refreshed when `add_triggering_agent` is
     * called, so on days when nothing is detected it still holds the
     * previous detection. Consumers that run every day (such as global
     * events) must use this to tell a fresh detection from a stale one.
     *
     * @param model The model the trigger belongs to.
     * @return true if agents were added to the triggering set during the
     * current day of the current simulation.
     */
    bool has_triggered_today(const Model<TSeq> & model) const;
};

#endif