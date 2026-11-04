#ifndef MEASLESSCHOOL_HPP
#define MEASLESSCHOOL_HPP

#include <cassert>
#include "samplermixing.hpp"
#include "stages.hpp"

using namespace epiworld;

/**
 * @brief Template for a Measles model with quarantine
 *
 * @param TSeq The type of the sequence to be used.
 * @details
 * This model can be described as a SEIHR model with isolation and quarantine.
 * The infectious period is divided into prodromal and rash stages.
 *
 * Each agent has a **status** and a **stage** of the measles natural history
 * (see `NaturalHistory`):
 *
 * | Statuses (states) | Stages (natural history "Measles") |
 * |---|---|
 * | Free         | Susceptible |
 * | Quarantined  | Latent |
 * | Isolated     | Prodromal (infectious) |
 * | Hospitalized | Rash |
 * |              | Recovered |
 *
 * The model's states are the pairs (`state()`). Only free agents are in
 * contact. The latent, prodromal and rash stages last a number of days drawn
 * from a lognormal distribution on entry (`measles_stage_days()`), whatever
 * the status.
 *
 * The quarantine process is triggered any time that an agent with rash is
 * detected. The agent is then isolated and all free agents who are
 * unvaccinated and have not had rash are quarantined. Isolated agents are
 * released "Isolation period" days after rash onset; quarantined agents
 * "Quarantine period" days after they were quarantined. A quarantined agent
 * who develops rash is isolated. Agents with rash may be hospitalized each
 * day; they are discharged, free and recovered, with daily probability
 * 1 / "Hospitalization period" (the rash stage does not end in hospital).
 *
 * Everyone mixes with everyone, each agent meeting "Contact rate" others a day
 * (drawn with replacement by the model's own sampler). The model builds its
 * one layer, a mixing layer named "school" with everyone in one group, a
 * contact matrix of 1 and "Contact rate" as its contact rate, and logs the
 * contacts in it (e.g., for InterventionMeaslesPEP).
 *
 * ![Model Diagram](../assets/img/measlesschool.png)
 *
 * @ingroup disease_specific
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelMeaslesSchool final :
    public Model<TSeq>,
    public QuarantineTrigger<TSeq>
{

private:

    // Statuses
    static void _update_free(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_quarantined(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_isolated(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_hospitalized(Agent<TSeq> * p, Model<TSeq> * m);

    // Stages (the others last their drawn duration; the statuses decide
    // what happens to agents with rash)
    static void _update_susceptible(Agent<TSeq> * p, Model<TSeq> * m);

    static void _on_stage_change(
        Agent<TSeq> * p, size_t k, int from, int to, Model<TSeq> * m
    );

    /// Hospitalizes the agent with the daily probability (agents with rash)
    static bool _hospitalize(Agent<TSeq> * p, Model<TSeq> * m);

    /**
     * @brief Quarantine agents that are in the system.
     *
     * The flow should be:
     * - The function only runs if quarantine or isolation is on.
     *
     * - Free agents with rash are detected with daily probability
     *   1 / "Days undetected", and isolated; agents hospitalized today are
     *   detected too. Each detection triggers the quarantine.
     *
     * - Then, free agents who have not had rash and hold no tool (e.g., are
     *   not vaccinated) are quarantined, if willing.
     */
    static void _quarantine_agents(Model<TSeq> * m);

    void _update_infectious();

    layer::Mixing<TSeq> * school = nullptr; ///< The model's layer (for its log)

public:

    /// @name Statuses
    ///@{
    static constexpr int FREE         = 0;
    static constexpr int QUARANTINED  = 1;
    static constexpr int ISOLATED     = 2;
    static constexpr int HOSPITALIZED = 3;
    ///@}

    /// @name Stages of the natural history (`get_natural_history(0)`)
    ///@{
    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int LATENT      = 1;
    static constexpr int PRODROMAL   = 2;
    static constexpr int RASH        = 3;
    static constexpr int RECOVERED   = 4;
    ///@}

    /// The model's state with this status and stage.
    static constexpr epiworld_fast_uint state(int status, int stage)
    {
        return static_cast< epiworld_fast_uint >(status + 4 * stage);
    }

    ModelMeaslesSchool() = delete;

    /**
     * @param n The number of entities in the model.
     * @param n_latent The number of initial infected agents in the model.
     * @param contact_rate The contact rate between entities in the model.
     * @param transmission_rate The transmission rate of the disease in the model.
     * @param vax_efficacy The efficacy of the vaccine.
     * @param vax_reduction_recovery_rate The reduction in recovery rate due to the vaccine.
     * @param incubation_period The incubation period of the disease in the model.
     * @param prodromal_period The prodromal period of the disease in the model.
     * @param rash_period The rash period of the disease in the model.
     * @param days_undetected The number of days an infected agent remains undetected.
     * @param hospitalization_rate The rate of hospitalization.
     * @param hospitalization_period The period of hospitalization.
     * @param prop_vaccinated The proportion of vaccinated agents.
     * @param quarantine_period The period of quarantine.
     * @param quarantine_willingness The willingness of agents to be quarantined.
     * @param isolation_period The period of isolation.
     * @param incubation_period_sd Standard deviation of the incubation
     * period in days (default: 2.5).
     * @param prodromal_period_sd Standard deviation of the prodromal period
     * in days (default: 1.7).
     * @param rash_period_sd Standard deviation of the rash period in days
     * (default: 1.0).
     */
    ModelMeaslesSchool(
        epiworld_fast_uint n,
        epiworld_fast_uint n_latent,
        // Disease parameters
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double vax_efficacy,
        epiworld_double vax_reduction_recovery_rate,
        epiworld_double incubation_period,
        epiworld_double prodromal_period,
        epiworld_double rash_period,
        epiworld_double days_undetected,
        epiworld_double hospitalization_rate,
        epiworld_double hospitalization_period,
        // Policy parameters
        epiworld_double prop_vaccinated,
        epiworld_fast_int quarantine_period,
        epiworld_double quarantine_willingness,
        epiworld_fast_int isolation_period,
        epiworld_double incubation_period_sd = 2.5,
        epiworld_double prodromal_period_sd = 1.7,
        epiworld_double rash_period_sd = 1.0
    );

    std::vector<Agent<TSeq> *> infectious; ///< Agents infectious for contact

    std::vector< int > day_flagged; ///< Day the agent was quarantined or isolated

    void reset() override;

    std::unique_ptr< Model<TSeq> > clone_ptr() override;
    void next() override;

};

template<typename TSeq>
inline void ModelMeaslesSchool<TSeq>::_quarantine_agents(Model<TSeq> * m) {

    auto * model = model_cast<ModelMeaslesSchool<TSeq>,TSeq>(m);
    const auto & nh = m->get_natural_history();

    // Quarantine and isolation can be shut off if negative
    if (
        (EPI_PAR(model, "Quarantine period") < 0) &&
        (EPI_PAR(model, "Isolation period") < 0)
    )
        return;

    // Capturing the days that matter and the probability of success
    epiworld_double willingness = EPI_PAR(model, "Quarantine willingness");
    epiworld_double p_detection = 1.0/(EPI_PAR(model, "Days undetected"));
    int prodromal_period = static_cast<int>(EPI_PAR(model, "Prodromal period"));

    bool triggered_today = false;

    // Detecting new cases: free agents with rash, and agents hospitalized
    // today. Public health considers them infectious from a prodromal
    // period before rash onset.
    for (size_t i = 0u; i < model->size(); ++i) {

        auto & agent = model->get_agent(i);
        const int status = agent.get_status(*m);

        bool detected = false;
        if ((status == FREE) && (agent.get_stage(*m) == RASH))
        {
            if (model->runif() < p_detection)
            {
                agent.change_status(*model, ISOLATED);
                model->day_flagged[agent.get_id()] = model->today();
                detected = true;
            }
        }
        else if (
            (status == HOSPITALIZED) &&
            (agent.get_state_last_changed() == model->today())
        )
        {
            model->day_flagged[agent.get_id()] = model->today();
            detected = true;
        }

        if (detected)
        {
            model->add_triggering_agent(
                *model, agent, nh.entered(agent, RASH) - prodromal_period
            );
            triggered_today = true;
        }

    }

    if (!triggered_today)
        return;

    // Quarantining free agents who have not had rash, hold no tool, and
    // are willing
    for (size_t i = 0u; i < model->size(); ++i) {

        auto & agent = model->get_agent(i);
        const int stage = agent.get_stage(*m);
        if ((agent.get_status(*m) != FREE) || (stage >= RASH))
            continue;

        if (agent.get_n_tools() != 0u)
            continue;

        if (
            (EPI_PAR(model, "Quarantine period") >= 0) &&
            (model->runif() < willingness)
        )
        {
            agent.change_status(*model, QUARANTINED);
            model->day_flagged[agent.get_id()] = model->today();
        }

    }

    return;

}

template<typename TSeq>
inline void ModelMeaslesSchool<TSeq>::reset() {

    Model<TSeq>::reset();
    this->day_flagged.assign(this->size(), 0);
    this->_update_infectious();

    return;

}

template<typename TSeq>
inline void ModelMeaslesSchool<TSeq>::_update_infectious() {

    this->school = &measles_mixing_layer(*this);

    // Free prodromal agents are infectious; free agents without rash are
    // in contact
    this->infectious.clear();
    int n_available = 0;
    for (auto & agent: this->get_agents())
    {

        if (agent.get_status(*this) != FREE)
            continue;

        const int stage = agent.get_stage(*this);
        if (stage == PRODROMAL)
            this->infectious.push_back(&agent);

        if (stage != RASH)
            ++n_available;

    }

    double p_contact = 0.0;
    if (n_available > 0)
    {
        p_contact = EPI_PAR(this, "Contact rate")/
            static_cast< epiworld_double >(n_available);
    }

    this->set_rand_binom(
        static_cast<int>(this->infectious.size()),
        p_contact > 1.0 ? 1.0 : p_contact
    );

}

template<typename TSeq>
inline std::unique_ptr<Model<TSeq>> ModelMeaslesSchool<TSeq>::clone_ptr()
{
    return std::make_unique<ModelMeaslesSchool<TSeq>>(*this);
}

template<typename TSeq>
inline bool ModelMeaslesSchool<TSeq>::_hospitalize(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    if (m->runif() >= EPI_PAR(m, "Hospitalization rate"))
        return false;

    m->record_hospitalization(*p);
    p->change_status(*m, HOSPITALIZED);
    return true;

}

template<typename TSeq>
inline void ModelMeaslesSchool<TSeq>::_update_susceptible(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // Only free agents are in contact
    if (p->get_status(*m) != FREE)
        return;

    auto* model = model_cast<ModelMeaslesSchool<TSeq>,TSeq>(m);
    size_t n_infectious = model->infectious.size();
    if (n_infectious == 0)
        return;

    int ndraw = m->rbinom();
    if (ndraw == 0)
        return;

    // The contacts go to the layer's log, and to the post-sampling callback
    const bool record = m->has_post_sampling();

    int nviruses_tmp = 0;
    int i = 0;
    auto & _ref = *m;
    while (i < ndraw)
    {

        auto which = m->runif_index(n_infectious);
        Agent<> & neighbor = *model->infectious[which];
        if (neighbor.get_id() == p->get_id())
            continue;

        model->school->trace(neighbor.get_id(), p->get_id());
        if (record)
            m->register_sampled_contact(neighbor.get_id(), p->get_id());

        i++;

        #ifdef EPI_DEBUG
        if (neighbor.get_virus() == nullptr)
            throw std::logic_error("The neighbor has no virus.");
        if (neighbor.get_stage(*m) != PRODROMAL)
            throw std::logic_error("The neighbor is not prodromal.");
        #endif

        // Adding the current agent to the tmp list
        auto & v = neighbor.get_virus();
        m->reserve_tmp(static_cast< size_t >(nviruses_tmp) + 1u);

        /* And it is a function of susceptibility_reduction as well */
        m->array_double_tmp[nviruses_tmp] =
            (1.0 - p->get_susceptibility_reduction(v, _ref)) *
            v->get_prob_infecting(m) *
            (1.0 - neighbor.get_transmission_reduction(v, _ref))
            ;

        m->array_virus_tmp[nviruses_tmp++] = &(*v);

    }

    // No virus to compute
    if (nviruses_tmp == 0u)
        return;

    // Running the roulette
    int which = roulette(nviruses_tmp, m);
    if (which < 0)
        return;

    p->set_virus(*m, *m->array_virus_tmp[which]);

}

template<typename TSeq>
inline void ModelMeaslesSchool<TSeq>::_update_free(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // Each day with rash, the agent may be hospitalized; otherwise, it
    // recovers at the end of the rash period
    if ((p->get_stage(*m) == RASH) && !_hospitalize(p, m))
        if (m->get_natural_history().due(*p, *m))
            p->rm_virus(*m);

}

template<typename TSeq>
inline void ModelMeaslesSchool<TSeq>::_update_quarantined(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    auto* model = model_cast<ModelMeaslesSchool<TSeq>,TSeq>(m);
    int days_since = m->today() - model->day_flagged[p->get_id()];
    if (days_since >= EPI_PAR(m, "Quarantine period"))
        p->change_status(*m, FREE);

}

template<typename TSeq>
inline void ModelMeaslesSchool<TSeq>::_update_isolated(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // With rash, the agent may be hospitalized, or recover at the end of the
    // rash period. Isolation lasts from rash onset, also for agents who
    // recovered.
    const auto & nh = m->get_natural_history();
    if (p->get_stage(*m) == RASH)
    {
        if (_hospitalize(p, m))
            return;

        if (nh.due(*p, *m))
            p->rm_virus(*m);
    }

    int days_since = m->today() - nh.entered(*p, RASH);
    if (EPI_PAR(m, "Isolation period") <= days_since)
        p->change_status(*m, FREE);

}

template<typename TSeq>
inline void ModelMeaslesSchool<TSeq>::_update_hospitalized(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // Discharged free; agents still with rash recover then
    if (m->runif() < 1.0/EPI_PAR(m, "Hospitalization period"))
    {
        p->change_status(*m, FREE);
        if (p->get_stage(*m) == RASH)
            p->rm_virus(*m);
    }

}

template<typename TSeq>
inline void ModelMeaslesSchool<TSeq>::_on_stage_change(
    Agent<TSeq> * p, size_t, int, int to, Model<TSeq> * m
) {

    // A quarantined agent who develops rash is isolated
    if ((to == RASH) && (p->get_status(*m) == QUARANTINED))
        p->change_status(*m, ISOLATED);

}

template<typename TSeq>
inline ModelMeaslesSchool<TSeq>::ModelMeaslesSchool(
    epiworld_fast_uint n,
    epiworld_fast_uint n_latent,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double vax_efficacy,
    epiworld_double vax_reduction_recovery_rate,
    epiworld_double incubation_period,
    epiworld_double prodromal_period,
    epiworld_double rash_period,
    epiworld_double days_undetected,
    epiworld_double hospitalization_rate,
    epiworld_double hospitalization_period,
    epiworld_double prop_vaccinated,
    epiworld_fast_int quarantine_period,
    epiworld_double quarantine_willingness,
    epiworld_fast_int isolation_period,
    epiworld_double incubation_period_sd,
    epiworld_double prodromal_period_sd,
    epiworld_double rash_period_sd
) {

    // Assertions
    auto max_uint = std::numeric_limits< size_t >::max();
    auto max_double = std::numeric_limits< double >::max();
    auto max_int = std::numeric_limits< int >::max();
    EpiAssert::check_bounds(n, static_cast<size_t>(1), max_uint, "n", "ModelMeaslesSchool");
    EpiAssert::check_bounds(n_latent, static_cast<size_t>(0), static_cast<size_t>(n), "n_latent", "ModelMeaslesSchool");
    EpiAssert::check_bounds(contact_rate, 0.0, max_double, "contact_rate", "ModelMeaslesSchool");
    EpiAssert::check_bounds(transmission_rate, 0.0, 1.0, "transmission_rate", "ModelMeaslesSchool");
    EpiAssert::check_bounds(vax_efficacy, 0.0, 1.0, "vax_efficacy", "ModelMeaslesSchool");
    EpiAssert::check_bounds(vax_reduction_recovery_rate, 0.0, 1.0, "vax_reduction_recovery_rate", "ModelMeaslesSchool");
    EpiAssert::check_bounds(incubation_period, 0.0, max_double, "incubation_period", "ModelMeaslesSchool");
    EpiAssert::check_bounds(prodromal_period, 0.0, max_double, "prodromal_period", "ModelMeaslesSchool");
    EpiAssert::check_bounds(rash_period, 0.0, max_double, "rash_period", "ModelMeaslesSchool");
    EpiAssert::check_bounds(days_undetected, 0.0, max_double, "days_undetected", "ModelMeaslesSchool");
    EpiAssert::check_bounds(hospitalization_rate, 0.0, 1.0, "hospitalization_rate", "ModelMeaslesSchool");
    EpiAssert::check_bounds(hospitalization_period, 0.0, max_double, "hospitalization_period", "ModelMeaslesSchool");
    EpiAssert::check_bounds(prop_vaccinated, 0.0, 1.0, "prop_vaccinated", "ModelMeaslesSchool");
    EpiAssert::check_bounds(quarantine_period, -1, max_int, "quarantine_period", "ModelMeaslesSchool");
    EpiAssert::check_bounds(quarantine_willingness, 0.0, 1.0, "quarantine_willingness", "ModelMeaslesSchool");
    EpiAssert::check_bounds(isolation_period, -1, max_int, "isolation_period", "ModelMeaslesSchool");
    measles_check_stage_sds(
        incubation_period_sd, prodromal_period_sd, rash_period_sd,
        "ModelMeaslesSchool"
    );

    this->add_param(contact_rate, "Contact rate");
    this->add_param(transmission_rate, "Transmission rate");
    this->add_param(incubation_period, "Incubation period");
    this->add_param(prodromal_period, "Prodromal period");
    this->add_param(rash_period, "Rash period");
    this->add_param(days_undetected, "Days undetected");
    this->add_param(quarantine_period, "Quarantine period");
    this->add_param(quarantine_willingness, "Quarantine willingness");
    this->add_param(isolation_period, "Isolation period");
    this->add_param(hospitalization_rate, "Hospitalization rate");
    this->add_param(hospitalization_period, "Hospitalization period");
    this->add_param(prop_vaccinated, "Vaccination rate");
    this->add_param(vax_efficacy, "Vax efficacy");
    this->add_param(vax_reduction_recovery_rate, "(IGNORED) Vax improved recovery");
    this->add_param(incubation_period_sd, "Incubation period sd");
    this->add_param(prodromal_period_sd, "Prodromal period sd");
    this->add_param(rash_period_sd, "Rash period sd");

    // Statuses
    this->add_state("Free", _update_free);
    this->add_state("Quarantined", _update_quarantined);
    this->add_state("Isolated", _update_isolated);
    this->add_state("Hospitalized", _update_hospitalized);

    // The natural history
    NaturalHistory<TSeq> nh("Measles");
    nh.add_stage("Susceptible", _update_susceptible);
    nh.add_stage("Latent");
    nh.add_stage("Prodromal");
    nh.add_stage("Rash");
    nh.add_stage("Recovered");
    nh.set_duration(
        LATENT, measles_stage_days<TSeq>("Incubation period", "Incubation period sd"),
        PRODROMAL
    );
    nh.set_duration(
        PRODROMAL, measles_stage_days<TSeq>("Prodromal period", "Prodromal period sd"),
        RASH
    );
    nh.set_duration(
        RASH, measles_stage_days<TSeq>("Rash period", "Rash period sd")
    );
    nh.set_infection(LATENT, RECOVERED);
    nh.set_infectious(PRODROMAL);
    this->add_natural_history(nh);
    this->set_stage_hook(_on_stage_change);

    // Virus
    Virus<> measles("Measles");
    measles.set_natural_history(0);
    measles.set_prob_infecting("Transmission rate");
    measles.set_distribution(
        distribute_virus_randomly(n_latent, false)
    );

    this->add_virus(measles);

    // Vaccine
    ToolVaccine<TSeq> vaccine("MMR");
    vaccine.set_susceptibility_reduction(EPI_PAR(this, "Vax efficacy"));
    vaccine.set_distribution(distribute_tool_randomly(prop_vaccinated, true));

    this->add_tool(vaccine);

    this->queuing_off();

    // Quarantine process
    auto quarantine_event = GlobalEvent<TSeq>(
        this->_quarantine_agents, "Quarantine process"
    );
    this->add_globalevent(quarantine_event);

    // Setting the population size
    this->set_population_size(n);

    // The school: everyone in one group; the model samples contacts itself
    auto school = layer::mixing<TSeq>("school");
    school->set_groups(std::vector< size_t >(n, 0u), 1u);
    school->set_contact_matrix({1.0});
    school->set_contact_rate("Contact rate");
    school->enable_tracing();
    this->add_layer(school);

}

template<typename TSeq>
inline void ModelMeaslesSchool<TSeq>::next() {
    this->_update_infectious();
    Model<TSeq>::next();
}

#endif
