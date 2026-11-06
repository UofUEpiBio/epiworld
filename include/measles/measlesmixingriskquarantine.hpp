#ifndef EPIWORLD_MODELS_MEASLESMIXINGRISKQUARANTINE_HPP
#define EPIWORLD_MODELS_MEASLESMIXINGRISKQUARANTINE_HPP

#include "measlesbase.hpp"

using namespace epiworld;

/**
 * @file measlesmixingriskquarantine.hpp
 * @brief Template for a Measles model with population mixing and risk-based quarantine
 */

/**
 * @brief Measles model with population mixing and risk-based quarantine strategies
 *
 * @details The disease, the statuses and the rules linking them are those of
 * `ModelMeaslesBase`: stages Susceptible, Latent, Prodromal, Rash, Severe and
 * Recovered; statuses Free, Quarantined, Isolated and Hospitalized. Agents
 * with rash do not transmit.
 *
 * A case is detected, isolated ("Isolated") and triggers contact tracing:
 *
 * - each day with rash, for a free agent, with probability 1 / "Days
 *   undetected", from the day after its onset (if "Isolation period" is not
 *   negative; "Detected");
 * - at its rash onset, for a free agent, with probability "Detection rate
 *   quarantine" once a quarantine has been triggered;
 * - at its rash onset in quarantine (isolated by the base rules).
 *
 * The day's cases quarantine the free agents without tools who are willing to
 * (and isolate those with rash willing to isolate), for a period that depends
 * on the agent's exposure risk:
 *
 * - **High Risk**: agents in the same group as a case ("Quarantine period
 *   high");
 * - **Medium Risk**: traced contacts of a case, within "Contact tracing days
 *   window" days of its rash onset, each found with probability "Contact
 *   tracing success rate" ("Quarantine period medium");
 * - **Low Risk**: the other agents ("Quarantine period low").
 *
 * A negative period releases the agent the next day.
 *
 * The contacts come from the model's mixing layer, which the user adds, as for
 * ModelMeaslesMixing: a `layer::Mixing` with the groups and a contact matrix.
 * The model samples from it with its own sampler and logs the contacts in it.
 *
 * ![Model Diagram](../assets/img/measlesmixingriskquarantine.png)
 *
 * @tparam TSeq Type for genetic sequences (default: EPI_DEFAULT_TSEQ)
 * @ingroup disease_specific
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelMeaslesMixingRiskQuarantine final : public ModelMeaslesBase<TSeq>
{
private:

    std::vector< size_t > agents_triggered_contact_tracing; ///< The day's cases
    std::vector< int > quarantine_risk_level; ///< Risk level of each agent (0=low, 1=medium, 2=high)
    std::vector< size_t > days_quarantine_triggered; ///< Days when quarantine was triggered

    /// Days of quarantine of an agent, from its risk level
    int risk_period(size_t agent_id);

    static void _quarantine_process(Model<TSeq> * m); ///< Overall quarantine process

public:

    // Risk levels for quarantine
    static constexpr int RISK_LOW    = 0;
    static constexpr int RISK_MEDIUM = 1;
    static constexpr int RISK_HIGH   = 2;

    ModelMeaslesMixingRiskQuarantine() = delete;

    /**
     * @brief Constructs a ModelMeaslesMixingRiskQuarantine object.
     *
     * @param n The number of agents in the model.
     * @param prevalence The initial share of latent agents.
     * @param transmission_rate The probability of transmission per contact.
     * @param vax_efficacy The efficacy of the vaccine.
     * @param incubation_period Mean days latent.
     * @param prodromal_period Mean days prodromal.
     * @param rash_period Mean days with rash.
     * @param hospitalization_rate Daily probability that an agent with rash
     * becomes severe (and is hospitalized).
     * @param hospitalization_period Mean days severe (in hospital).
     * @param days_undetected Mean days until an agent with rash is detected.
     * @param quarantine_period_high Days of quarantine for high-risk contacts.
     * @param quarantine_period_medium Days of quarantine for medium-risk contacts.
     * @param quarantine_period_low Days of quarantine for low-risk contacts.
     * @param quarantine_willingness Share of agents willing to quarantine.
     * @param isolation_willingness Share of agents willing to isolate when
     * quarantined with rash.
     * @param isolation_period Days of isolation from the rash onset
     * (negative: no detection by "Days undetected").
     * @param prop_vaccinated The proportion of vaccinated agents.
     * @param detection_rate_quarantine The probability of detection at the
     * rash onset once a quarantine has been triggered.
     * @param contact_tracing_success_rate The probability of tracing a
     * contact (default: 1.0).
     * @param contact_tracing_days_window The number of days before or after
     * the rash onset within which contacts are traced (default: 4).
     * @param incubation_period_sd Standard deviation of the incubation
     * period in days (default: 2.5).
     * @param prodromal_period_sd Standard deviation of the prodromal period
     * in days (default: 1.7).
     * @param rash_period_sd Standard deviation of the rash period in days
     * (default: 1.0).
     */
    ModelMeaslesMixingRiskQuarantine(
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double vax_efficacy,
        epiworld_double incubation_period,
        epiworld_double prodromal_period,
        epiworld_double rash_period,
        epiworld_double hospitalization_rate,
        epiworld_double hospitalization_period,
        // Policy parameters
        epiworld_double days_undetected,
        epiworld_fast_int quarantine_period_high,
        epiworld_fast_int quarantine_period_medium,
        epiworld_fast_int quarantine_period_low,
        epiworld_double quarantine_willingness,
        epiworld_double isolation_willingness,
        epiworld_fast_int isolation_period,
        epiworld_double prop_vaccinated,
        epiworld_double detection_rate_quarantine,
        epiworld_double contact_tracing_success_rate = 1.0,
        epiworld_fast_uint contact_tracing_days_window = 4u,
        epiworld_double incubation_period_sd = 2.5,
        epiworld_double prodromal_period_sd = 1.7,
        epiworld_double rash_period_sd = 1.0
    );

    void reset() override;
    std::unique_ptr< Model<TSeq> > clone_ptr() override;

    /**
     * @brief Get the risk level assigned to each agent for quarantine purposes
     * @return Vector of integers representing risk levels (0=low, 1=medium, 2=high)
     */
    const auto & get_quarantine_risk_level() const
    {
        return quarantine_risk_level;
    };

    /**
     * @brief The days a quarantine was triggered
     */
    auto get_days_quarantine_triggered() const
    {
        return days_quarantine_triggered;
    };

};

template<typename TSeq>
inline int ModelMeaslesMixingRiskQuarantine<TSeq>::risk_period(size_t agent_id)
{

    const int level = quarantine_risk_level[agent_id];
    if (level == RISK_HIGH)
        return static_cast< int >(EPI_PAR(this, "Quarantine period high"));
    if (level == RISK_MEDIUM)
        return static_cast< int >(EPI_PAR(this, "Quarantine period medium"));
    return static_cast< int >(EPI_PAR(this, "Quarantine period low"));

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::_quarantine_process(Model<TSeq> * m) {

    auto * model = model_cast<ModelMeaslesMixingRiskQuarantine<TSeq>,TSeq>(m);
    constexpr int RASH = ModelMeaslesBase<TSeq>::RASH;
    constexpr int FREE = ModelMeaslesBase<TSeq>::FREE;
    const int isolated = m->transition_id("Isolated");
    const int quarantined = m->transition_id("Quarantined");
    auto & mixing = measles_mixing_layer(*m);
    auto & cases = model->agents_triggered_contact_tracing;

    // No case, nothing to trace
    if (cases.empty())
        return;

    model->days_quarantine_triggered.push_back(m->today());

    // Free agents before or with rash can be quarantined; they start at a
    // low risk
    std::vector< bool > can_quarantine(m->size(), false);
    for (auto & agent: m->get_agents())
    {
        if ((m->get_stage(agent) <= RASH) && (m->get_status(agent) == FREE))
        {
            model->quarantine_risk_level[agent.get_id()] = RISK_LOW;
            can_quarantine[agent.get_id()] = true;
        }
    }

    // The risk levels
    const double window = EPI_PAR(m, "Contact tracing days window");
    auto & ct = mixing.get_tracing();
    for (size_t agent_i_idx : cases)
    {

        // (A) The case's group: high risk
        const size_t group_i = mixing.get_group(agent_i_idx);
        if (group_i != layer::Mixing<TSeq>::NO_GROUP)
            for (size_t agent_j_idx: mixing.members_of(group_i))
                if (can_quarantine[agent_j_idx])
                    model->quarantine_risk_level[agent_j_idx] = RISK_HIGH;

        // (B) The case's traced contacts: medium risk
        const size_t n_contacts = std::min(
            ct.get_n_contacts(agent_i_idx), ct.get_max_contacts()
        );
        const int onset = m->get_stage_entered(agent_i_idx, RASH);

        for (size_t contact_j_idx = 0u; contact_j_idx < n_contacts; ++contact_j_idx)
        {

            auto [contact_id, contact_date] = ct.get_contact(
                agent_i_idx, contact_j_idx
            );

            if (!can_quarantine[contact_id])
                continue;

            if (m->runif() > EPI_PAR(m, "Contact tracing success rate"))
                continue;

            if (std::abs(static_cast< double >(onset - contact_date)) > window)
                continue;

            if (model->quarantine_risk_level[contact_id] < RISK_HIGH)
                model->quarantine_risk_level[contact_id] = RISK_MEDIUM;

        }

    }

    // Quarantining (or isolating) the agents
    for (auto & agent: m->get_agents())
    {

        const size_t id = static_cast< size_t >(agent.get_id());
        if (!can_quarantine[id] || (agent.get_n_tools() != 0u))
            continue;

        if (!model->quarantine_willingness[id])
            continue;

        if (m->get_stage(agent) != RASH)
            m->fire(quarantined, agent);
        else if (model->isolation_willingness[id])
            m->fire(isolated, agent);

    }

    cases.clear();

}

template<typename TSeq>
inline ModelMeaslesMixingRiskQuarantine<TSeq>::ModelMeaslesMixingRiskQuarantine(
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double vax_efficacy,
    epiworld_double incubation_period,
    epiworld_double prodromal_period,
    epiworld_double rash_period,
    epiworld_double hospitalization_rate,
    epiworld_double hospitalization_period,
    // Policy parameters
    epiworld_double days_undetected,
    epiworld_fast_int quarantine_period_high,
    epiworld_fast_int quarantine_period_medium,
    epiworld_fast_int quarantine_period_low,
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    epiworld_fast_int isolation_period,
    epiworld_double prop_vaccinated,
    epiworld_double detection_rate_quarantine,
    epiworld_double contact_tracing_success_rate,
    epiworld_fast_uint contact_tracing_days_window,
    epiworld_double incubation_period_sd,
    epiworld_double prodromal_period_sd,
    epiworld_double rash_period_sd
    )
{

    this->check_stage_sds(incubation_period_sd, prodromal_period_sd, rash_period_sd, "ModelMeaslesMixingRiskQuarantine");

    // Setting up parameters
    this->add_param(transmission_rate, "Transmission rate");
    this->add_param(incubation_period, "Incubation period");
    this->add_param(prodromal_period, "Prodromal period");
    this->add_param(rash_period, "Rash period");
    this->add_param(hospitalization_rate, "Hospitalization rate");
    this->add_param(hospitalization_period, "Hospitalization period");
    this->add_param(days_undetected, "Days undetected");
    this->add_param(quarantine_period_high, "Quarantine period high");
    this->add_param(quarantine_period_medium, "Quarantine period medium");
    this->add_param(quarantine_period_low, "Quarantine period low");
    this->add_param(quarantine_willingness, "Quarantine willingness");
    this->add_param(isolation_willingness, "Isolation willingness");
    this->add_param(isolation_period, "Isolation period");
    this->add_param(detection_rate_quarantine, "Detection rate quarantine");
    this->add_param(contact_tracing_success_rate, "Contact tracing success rate");
    this->add_param(contact_tracing_days_window, "Contact tracing days window");
    this->add_param(prop_vaccinated, "Vaccination rate");
    this->add_param(vax_efficacy, "Vax efficacy");
    this->add_param(incubation_period_sd, "Incubation period sd");
    this->add_param(prodromal_period_sd, "Prodromal period sd");
    this->add_param(rash_period_sd, "Rash period sd");

    // The quarantine lasts the period of the agent's risk level; agents with
    // rash do not transmit
    this->measles_build(
        assign::share<TSeq>(prevalence),
        Duration<TSeq>([](Agent<TSeq> & p, Model<TSeq> & m) -> int {
            auto & mm = dynamic_cast< ModelMeaslesMixingRiskQuarantine<TSeq> & >(m);
            return mm.risk_period(static_cast< size_t >(p.get_id()));
        }),
        "",
        true
    );

    // A case: detected, or with its rash onset in quarantine (the base rules
    // isolate it)
    auto add_case = [](Agent<TSeq> & p, Model<TSeq> & m) {
        auto & mm = dynamic_cast< ModelMeaslesMixingRiskQuarantine<TSeq> & >(m);
        mm.agents_triggered_contact_tracing.push_back(
            static_cast< size_t >(p.get_id())
        );
    };
    this->react(Trigger::fired(this->transition_id("Detected")), add_case);
    this->react(Trigger::entered(this->RASH).was({this->QUARANTINED}), add_case);

    // Once a quarantine has been triggered, a free agent's rash onset is a
    // case with probability "Detection rate quarantine"
    this->react(
        Trigger::entered(this->RASH).was({this->FREE}),
        [](Agent<TSeq> & p, Model<TSeq> & m) {
            auto & mm = dynamic_cast< ModelMeaslesMixingRiskQuarantine<TSeq> & >(m);
            if (
                mm.days_quarantine_triggered.empty() ||
                (m.get_status(p) != mm.FREE) ||
                (m.runif() >= m.par("Detection rate quarantine"))
            )
                return;
            mm.agents_triggered_contact_tracing.push_back(
                static_cast< size_t >(p.get_id())
            );
            m.fire(m.transition_id("Isolated"), p);
        }
    );

    // The vaccine
    ToolVaccine<TSeq> vaccine("MMR");
    vaccine.set_susceptibility_reduction(this->get_param("Vax efficacy"));
    this->add_tool(vaccine, assign::share<TSeq>(prop_vaccinated));

    this->add_globalevent(this->_quarantine_process, "Update infected individuals");

    this->set_population_size(n);

    this->set_name("Measles with Mixing and Risk-based Quarantine");

}

template<typename TSeq>
inline void ModelMeaslesMixingRiskQuarantine<TSeq>::reset()
{

    Model<TSeq>::reset();

    // The quarantine process reads the contacts in the layer's log
    auto & mix = measles_mixing_layer(*this);
    if (!mix.is_tracing())
        mix.enable_tracing();

    this->sampler.reset(*this);
    this->draw_willingness();

    agents_triggered_contact_tracing.clear();
    quarantine_risk_level.assign(this->size(), RISK_LOW);
    days_quarantine_triggered.clear();

}

template<typename TSeq>
inline std::unique_ptr<Model<TSeq>> ModelMeaslesMixingRiskQuarantine<TSeq>::clone_ptr()
{
    return std::make_unique<ModelMeaslesMixingRiskQuarantine<TSeq>>(*this);
}

#endif
