#ifndef EPIWORLD_INTERVENTIONQUARANTINE_MEAT_HPP
#define EPIWORLD_INTERVENTIONQUARANTINE_MEAT_HPP

#include "interventionquarantine-bones.hpp"

template<typename TSeq>
inline InterventionQuarantine<TSeq>::InterventionQuarantine(
    std::string name,
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    epiworld_double contact_tracing_success_rate,
    epiworld_fast_uint contact_tracing_days_prior,
    std::vector< int > trigger_states,
    std::vector< int > quarantine_source_states,
    std::vector< int > quarantine_target_states,
    std::vector< int > isolation_source_states,
    std::vector< int > isolation_target_states
) {

    this->set_name(name);

    // Validate parallel arrays have matching lengths
    if (quarantine_source_states.size() != quarantine_target_states.size())
        throw std::logic_error(
            "InterventionQuarantine: quarantine_source_states and "
            "quarantine_target_states must have the same length. Got " +
            std::to_string(quarantine_source_states.size()) + " and " +
            std::to_string(quarantine_target_states.size()) + "."
        );

    if (isolation_source_states.size() != isolation_target_states.size())
        throw std::logic_error(
            "InterventionQuarantine: isolation_source_states and "
            "isolation_target_states must have the same length. Got " +
            std::to_string(isolation_source_states.size()) + " and " +
            std::to_string(isolation_target_states.size()) + "."
        );

    _quarantine_willingness = quarantine_willingness;
    _isolation_willingness  = isolation_willingness;
    _ct_success_rate        = contact_tracing_success_rate;
    _ct_days_prior          = contact_tracing_days_prior;

    _trigger_states             = trigger_states;
    _quarantine_source_states   = quarantine_source_states;
    _quarantine_target_states   = quarantine_target_states;
    _isolation_source_states    = isolation_source_states;
    _isolation_target_states    = isolation_target_states;

}

template<typename TSeq>
inline void InterventionQuarantine<TSeq>::_setup(
    Model<TSeq> * model
) {

    // Contact tracing is required
    if (!model->is_contact_tracing_on())
        throw std::logic_error(
            "InterventionQuarantine requires contact tracing to be "
            "enabled. Call model.contact_tracing_on() before running."
        );

    size_t n = model->size();

    // Register parameters (skip if already registered)
    model->add_param(
        _quarantine_willingness, _par_quarantine_willingness, true);
    model->add_param(
        _isolation_willingness, _par_isolation_willingness, true);
    model->add_param(
        _ct_success_rate, _par_ct_success_rate, true);
    model->add_param(
        static_cast<epiworld_double>(_ct_days_prior),
        _par_ct_days_prior, true);

    // Randomize per-agent willingness
    auto q_prob = model->par(_par_quarantine_willingness);
    auto i_prob = model->par(_par_isolation_willingness);

    _willing_to_quarantine.assign(n, false);
    _willing_to_isolate.assign(n, false);
    _day_last_triggered.assign(n, -1);
    _day_flagged.assign(n, -1);

    for (size_t i = 0u; i < n; ++i)
    {
        if (model->runif() < q_prob)
            _willing_to_quarantine[i] = true;

        if (model->runif() < i_prob)
            _willing_to_isolate[i] = true;
    }

}

/**
 * @brief Main quarantine processing logic.
 *
 * @details
 * This operator runs as a global event at the end of each simulation day.
 * The algorithm proceeds as follows:
 *
 * **Step 1 — Setup check:**
 * On the first invocation of each simulation (detected via `get_sim_id()`),
 * the internal state is initialized: per-agent willingness arrays are
 * randomized and tracking arrays are reset.
 *
 * **Step 2 — Identify newly triggered agents:**
 * Iterate over all agents. An agent triggers the quarantine process if:
 * - Its current state is in `trigger_states`, AND
 * - It has not already been processed today (tracked via
 *   `_day_last_triggered`). This allows agents who return to the system
 *   after quarantine to be re-processed in subsequent outbreaks.
 *   Specifically, an agent can trigger again on any day after its last
 *   trigger day, enabling re-quarantine when the agent re-enters a
 *   trigger state (e.g., gets re-infected and isolated again).
 *
 * **Step 3 — Contact tracing and quarantine:**
 * For each newly triggered agent:
 * - Retrieve contacts from the model's `ContactTracing` object.
 * - For each contact made within `contact_tracing_days_prior` days of
 *   today:
 *   - Apply `contact_tracing_success_rate` probabilistic filter.
 *   - Skip contacts that already have a tool (e.g., vaccinated).
 *   - Skip contacts already flagged today (avoid double-quarantine).
 *   - If the contact's state matches a `quarantine_source_state` and
 *     the contact is willing to quarantine, move them to the
 *     corresponding `quarantine_target_state`.
 *   - If the contact's state matches an `isolation_source_state` and
 *     the contact is willing to both quarantine and isolate, move them
 *     to the corresponding `isolation_target_state`.
 *   - Record the day flagged for quarantine release calculations.
 */
template<typename TSeq>
inline void InterventionQuarantine<TSeq>::operator()(
    Model<TSeq> * model, int
) {

    // Setup on first call or new simulation
    if (static_cast<int>(model->get_sim_id()) != _model_id)
    {
        _model_id = static_cast<int>(model->get_sim_id());
        _setup(model);
    }

    int today = model->today();
    auto ct_days_prior = static_cast<int>(
        model->par(_par_ct_days_prior));
    auto ct_success_rate = model->par(_par_ct_success_rate);

    auto & ct = model->get_contact_tracing();

    // Scan for newly triggered agents
    for (auto & agent : model->get_agents())
    {

        size_t agent_id = agent.get_id();

        // Already processed today — skip to avoid duplicate processing
        // within the same day
        if (_day_last_triggered[agent_id] == today)
            continue;

        // Check if agent is in a trigger state
        int agent_state = static_cast<int>(agent.get_state());
        if (!IN(agent_state, _trigger_states))
            continue;

        // Record the day of this trigger so we don't re-process today,
        // but allow re-triggering on a future day if the agent returns
        // to the system and another outbreak occurs.
        _day_last_triggered[agent_id] = today;

        // Retrieve this agent's contacts
        size_t n_contacts = ct.get_n_contacts(agent_id);
        if (n_contacts > EPI_MAX_TRACKING)
            n_contacts = EPI_MAX_TRACKING;

        for (size_t ci = 0u; ci < n_contacts; ++ci)
        {

            auto [contact_id, contact_date] =
                ct.get_contact(agent_id, ci);

            // Check if the contact is within the tracing window
            int days_since = today - static_cast<int>(contact_date);
            if (days_since > ct_days_prior)
                continue;

            // Probabilistic tracing success
            if (model->runif() > ct_success_rate)
                continue;

            auto & contact = model->get_agent(contact_id);

            // Skip agents who already have a tool (e.g., vaccinated)
            if (contact.get_n_tools() != 0u)
                continue;

            // Skip contacts already flagged today
            if (_day_flagged[contact_id] == today)
                continue;

            int contact_state = static_cast<int>(contact.get_state());

            // Try quarantine mapping first
            bool handled = false;
            for (size_t qi = 0u; qi < _quarantine_source_states.size(); ++qi)
            {
                if (contact_state == _quarantine_source_states[qi])
                {
                    if (_willing_to_quarantine[contact_id])
                    {
                        contact.change_state(
                            *model,
                            _quarantine_target_states[qi]
                        );
                        _day_flagged[contact_id] = today;
                    }
                    handled = true;
                    break;
                }
            }

            if (handled)
                continue;

            // Try isolation mapping
            for (size_t ii = 0u; ii < _isolation_source_states.size(); ++ii)
            {
                if (contact_state == _isolation_source_states[ii])
                {
                    if (_willing_to_quarantine[contact_id] &&
                        _willing_to_isolate[contact_id])
                    {
                        contact.change_state(
                            *model,
                            _isolation_target_states[ii]
                        );
                        _day_flagged[contact_id] = today;
                    }
                    break;
                }
            }

        }

    }

}

template<typename TSeq>
inline std::unique_ptr< GlobalEvent<TSeq> >
InterventionQuarantine<TSeq>::clone_ptr() const
{
    return std::make_unique< InterventionQuarantine<TSeq> >(*this);
}

template<typename TSeq>
inline const std::vector< int > &
InterventionQuarantine<TSeq>::get_day_flagged() const
{
    return _day_flagged;
}

template<typename TSeq>
inline int
InterventionQuarantine<TSeq>::get_day_flagged(size_t agent_id) const
{
    return _day_flagged[agent_id];
}

template<typename TSeq>
inline const std::vector< bool > &
InterventionQuarantine<TSeq>::get_quarantine_willingness() const
{
    return _willing_to_quarantine;
}

template<typename TSeq>
inline const std::vector< bool > &
InterventionQuarantine<TSeq>::get_isolation_willingness() const
{
    return _willing_to_isolate;
}

#endif
