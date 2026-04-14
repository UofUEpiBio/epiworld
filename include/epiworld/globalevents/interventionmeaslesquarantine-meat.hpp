#ifndef EPIWORLD_INTERVENTIONMEASLESQUARANTINE_MEAT_HPP
#define EPIWORLD_INTERVENTIONMEASLESQUARANTINE_MEAT_HPP

#include "interventionmeaslesquarantine-bones.hpp"

template<typename TSeq>
inline InterventionMeaslesQuarantine<TSeq>::InterventionMeaslesQuarantine(
    std::string name,
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    std::vector<int> quarantinable_states,
    std::vector<int> quarantine_targets,
    int isolatable_state,
    int isolation_state,
    int max_base_state
) {

    this->set_name(name);

    // Validate matching lengths
    if (quarantinable_states.size() != quarantine_targets.size())
        throw std::logic_error(
            "The length of quarantinable_states (" +
            std::to_string(quarantinable_states.size()) +
            ") must equal the length of quarantine_targets (" +
            std::to_string(quarantine_targets.size()) + ")."
        );

    _quarantinable_states = std::move(quarantinable_states);
    _quarantine_targets   = std::move(quarantine_targets);
    _isolatable_state     = isolatable_state;
    _isolation_state      = isolation_state;
    _max_base_state       = max_base_state;

    _quarantine_willingness = quarantine_willingness;
    _isolation_willingness  = isolation_willingness;

}

template<typename TSeq>
inline void InterventionMeaslesQuarantine<TSeq>::_setup(
    Model<TSeq> * model
) {

    // Register parameters (skip if already present)
    model->add_param(
        _quarantine_willingness,
        _par_quarantine_willingness,
        true
    );
    model->add_param(
        _isolation_willingness,
        _par_isolation_willingness,
        true
    );

    // Initialise data vectors if not already done
    if (_quarantine_willing.empty())
        init(model->size());

}

template<typename TSeq>
inline void InterventionMeaslesQuarantine<TSeq>::init(
    size_t n_agents,
    size_t max_contacts
) {

    _contact_tracing.reset(n_agents, max_contacts);
    _quarantine_willing.assign(n_agents, false);
    _isolation_willing.assign(n_agents, false);
    _agent_quarantine_triggered.assign(n_agents, QUARANTINE_INACTIVE);
    _day_flagged.assign(n_agents, 0);
    _exposure_date.assign(n_agents, 0);
    _system_quarantine_triggered = false;

}

template<typename TSeq>
inline void InterventionMeaslesQuarantine<TSeq>::_apply_quarantine(
    Agent<TSeq> & agent,
    Model<TSeq> & model
) {

    auto agent_state = static_cast<int>(agent.get_state());
    size_t agent_id  = agent.get_id();

    // Already quarantined / isolated / hospitalised?
    if (agent_state > _max_base_state)
        return;

    // Vaccinated (has a tool)?
    if (agent.get_n_tools() != 0u)
        return;

    // Check if agent is in the isolatable state
    if (agent_state == _isolatable_state)
    {
        if (_isolation_willing[agent_id])
        {
            agent.change_state(model, _isolation_state);
            _day_flagged[agent_id] = model.today();
        }
        return;
    }

    // Check quarantine willingness
    if (!_quarantine_willing[agent_id])
        return;

    // Find the matching quarantinable state
    for (size_t s = 0u; s < _quarantinable_states.size(); ++s)
    {
        if (agent_state == _quarantinable_states[s])
        {
            agent.change_state(model, _quarantine_targets[s]);
            _day_flagged[agent_id] = model.today();
            return;
        }
    }

    // State not mapped – no quarantine action

}

template<typename TSeq>
inline void InterventionMeaslesQuarantine<TSeq>::operator()(
    Model<TSeq> * model,
    int
) {

    // Lazy per-simulation setup
    if (static_cast<int>(model->get_sim_id()) != _model_id)
    {
        _model_id = static_cast<int>(model->get_sim_id());
        _setup(model);
    }

    // ---------------------------------------------------------------
    // System-wide quarantine mode (e.g. school-wide)
    // ---------------------------------------------------------------
    if (_system_quarantine_triggered)
    {
        for (auto & agent : model->get_agents())
            _apply_quarantine(agent, *model);

        _system_quarantine_triggered = false;
        return;
    }

    // ---------------------------------------------------------------
    // Per-agent contact-tracing mode
    // ---------------------------------------------------------------
    bool has_ct_days_prior =
        model->has_param("Contact tracing days prior");
    bool has_ct_success_rate =
        model->has_param("Contact tracing success rate");

    double ct_days_prior  = has_ct_days_prior
        ? model->par("Contact tracing days prior") : 0.0;
    double ct_success_rate = has_ct_success_rate
        ? model->par("Contact tracing success rate") : 1.0;

    for (size_t agent_i = 0u; agent_i < model->size(); ++agent_i)
    {

        if (_agent_quarantine_triggered[agent_i] != QUARANTINE_ACTIVE)
            continue;

        // Retrieve contacts for this agent
        size_t n_contacts = _contact_tracing.get_n_contacts(agent_i);
        if (n_contacts >= EPI_MAX_TRACKING)
            n_contacts = EPI_MAX_TRACKING;

        // Reference date for the days-prior window
        int ref_date = _exposure_date[agent_i];

        for (size_t ci = 0u; ci < n_contacts; ++ci)
        {

            auto [contact_id, contact_date] = _contact_tracing.get_contact(
                agent_i, ci
            );

            // Check days-prior window (if the parameter exists)
            if (has_ct_days_prior)
            {
                double days_since = static_cast<double>(ref_date) -
                    static_cast<double>(contact_date);

                if (days_since > ct_days_prior)
                    continue;
            }

            // Check tracing success
            if (model->runif() > ct_success_rate)
                continue;

            auto & contact = model->get_agent(contact_id);
            _apply_quarantine(contact, *model);

        }

        // Mark as processed
        _agent_quarantine_triggered[agent_i] = QUARANTINE_DONE;

    }

}

// -------------------------------------------------------------------- //
// clone_ptr                                                             //
// -------------------------------------------------------------------- //

template<typename TSeq>
inline std::unique_ptr<GlobalEvent<TSeq>>
InterventionMeaslesQuarantine<TSeq>::clone_ptr() const
{
    return std::make_unique<InterventionMeaslesQuarantine<TSeq>>(*this);
}

// -------------------------------------------------------------------- //
// Contact tracing                                                       //
// -------------------------------------------------------------------- //

template<typename TSeq>
inline void InterventionMeaslesQuarantine<TSeq>::add_contact(
    size_t agent_a,
    size_t agent_b,
    size_t day
) {
    _contact_tracing.add_contact(agent_a, agent_b, day);
}

template<typename TSeq>
inline ContactTracing &
InterventionMeaslesQuarantine<TSeq>::get_contact_tracing()
{
    return _contact_tracing;
}

template<typename TSeq>
inline const ContactTracing &
InterventionMeaslesQuarantine<TSeq>::get_contact_tracing() const
{
    return _contact_tracing;
}

// -------------------------------------------------------------------- //
// Quarantine triggering                                                 //
// -------------------------------------------------------------------- //

template<typename TSeq>
inline void InterventionMeaslesQuarantine<TSeq>::trigger_quarantine(
    size_t agent_id
) {
    _agent_quarantine_triggered[agent_id] = QUARANTINE_ACTIVE;
}

template<typename TSeq>
inline void InterventionMeaslesQuarantine<TSeq>::trigger_system_quarantine()
{
    _system_quarantine_triggered = true;
}

template<typename TSeq>
inline size_t InterventionMeaslesQuarantine<TSeq>::get_quarantine_status(
    size_t agent_id
) const {
    return _agent_quarantine_triggered[agent_id];
}

// -------------------------------------------------------------------- //
// Exposure-date tracking                                                //
// -------------------------------------------------------------------- //

template<typename TSeq>
inline void InterventionMeaslesQuarantine<TSeq>::set_exposure_date(
    size_t agent_id,
    int day
) {
    _exposure_date[agent_id] = day;
}

template<typename TSeq>
inline int InterventionMeaslesQuarantine<TSeq>::get_exposure_date(
    size_t agent_id
) const {
    return _exposure_date[agent_id];
}

// -------------------------------------------------------------------- //
// Day-flagged                                                           //
// -------------------------------------------------------------------- //

template<typename TSeq>
inline void InterventionMeaslesQuarantine<TSeq>::set_day_flagged(
    size_t agent_id,
    int day
) {
    _day_flagged[agent_id] = day;
}

template<typename TSeq>
inline int InterventionMeaslesQuarantine<TSeq>::get_day_flagged(
    size_t agent_id
) const {
    return _day_flagged[agent_id];
}

// -------------------------------------------------------------------- //
// Willingness                                                           //
// -------------------------------------------------------------------- //

template<typename TSeq>
inline void InterventionMeaslesQuarantine<TSeq>::set_quarantine_willing(
    size_t agent_id,
    bool willing
) {
    _quarantine_willing[agent_id] = willing;
}

template<typename TSeq>
inline void InterventionMeaslesQuarantine<TSeq>::set_isolation_willing(
    size_t agent_id,
    bool willing
) {
    _isolation_willing[agent_id] = willing;
}

template<typename TSeq>
inline bool InterventionMeaslesQuarantine<TSeq>::is_quarantine_willing(
    size_t agent_id
) const {
    return _quarantine_willing[agent_id];
}

template<typename TSeq>
inline bool InterventionMeaslesQuarantine<TSeq>::is_isolation_willing(
    size_t agent_id
) const {
    return _isolation_willing[agent_id];
}

#endif
