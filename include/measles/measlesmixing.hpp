#ifndef EPIWORLD_MODELS_MEASLESMIXING_HPP
#define EPIWORLD_MODELS_MEASLESMIXING_HPP

#include "samplermixing.hpp"
#include "stages.hpp"

using namespace epiworld;

/**
 * @file measlesmixing.hpp
 * @brief Template for a Measles model with population mixing, quarantine, and contact tracing
 */

/**
 * @brief Measles model with population mixing, quarantine, and contact tracing
 *
 * This class implements a Measles epidemiological model based on the SEIR framework
 * with additional features including:
 *
 * - Population mixing based on contact matrices
 * - Measles-specific disease progression: Susceptible → Latent → Prodromal → Rash
 * - Prodromal individuals are infectious (replace the "Infected" state in SEIR)
 * - Rash individuals can have reduced infectious contact and can be detected for isolation
 * - Quarantine measures for contacts found by contact tracing
 * - Isolation policies for detected individuals during the rash state
 * - Contact tracing with configurable success rates
 * - Hospitalization of severe cases
 * - Individual willingness to comply with public health measures
 *
 * Each agent has a **status** (the model's states) and a **stage** of the
 * measles natural history (see `NaturalHistory`):
 *
 * | Statuses (states) | Stages (natural history "Measles") |
 * |---|---|
 * | Free         | Susceptible |
 * | Quarantined  | Latent |
 * | Isolated     | Prodromal (infectious) |
 * | Hospitalized | Rash (infectious at a reduced contact rate) |
 * |              | Recovered |
 *
 * The status says where the agent is, and who it can meet: only free agents
 * are in contact. The stage says what the disease does: the latent,
 * prodromal and rash stages last a number of days drawn from a lognormal
 * distribution on entry (`measles_stage_days()`), whatever the status.
 *
 * - A free susceptible agent can be infected by its contacts (unless immune).
 * - A free agent with rash may be detected (which starts contact tracing,
 *   and isolates the agent) or hospitalized, each day.
 * - Quarantine (traced contacts) lasts "Quarantine period" days from the
 *   day the agent was flagged; a quarantined agent who develops rash is
 *   isolated.
 * - Isolation lasts "Isolation period" days from rash onset. An isolated
 *   agent with rash may be hospitalized each day.
 * - Hospitalized agents are discharged, free and recovered, with daily
 *   probability 1 / "Hospitalization period". The rash stage does not end in
 *   hospital: hospitalized agents recover at discharge.
 *
 * The contacts come from the model's mixing layer, which the user adds: a
 * `layer::Mixing` with the groups and a contact matrix (entry `(i, j)` is the
 * expected number of contacts per day an agent in group `i` has with agents
 * in group `j`), e.g.,
 *
 * ```cpp
 * auto mix = layer::mixing<>("mixing");
 * mix->set_groups(grouping::random<>({300, 700}));
 * mix->set_contact_matrix({10.0, 2.0, 2.0, 5.0});
 * model.add_layer(mix);
 * ```
 *
 * The model samples from it with its own sampler (contacts drawn with
 * replacement, Rash agents at the reduced contact rate) and logs the contacts
 * in it, which the quarantine process reads. It has no other layers (see
 * `measles_mixing_layer()`).
 *
 * ![Model Diagram](../assets/img/measlesmixing.png)
 *
 * **Implementation details:**
 * <a href="../impl/mixing-and-entity-distribution.md">Mixing and Entity Distribution</a>,
 * <a href="../impl/quarantine-isolation-and-contact-tracing.md">Quarantine, Isolation, and Contact Tracing</a>,
 * <a href="../impl/sampling-contacts.md">Sampling Contacts</a>
 *
 * @tparam TSeq Type for genetic sequences (default: EPI_DEFAULT_TSEQ)
 * @ingroup disease_specific
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelMeaslesMixing final : public Model<TSeq>
{
private:

    SamplerMixing<TSeq> sampler;

    // Statuses
    static void _update_free(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_quarantined(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_isolated(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_hospitalized(Agent<TSeq> * p, Model<TSeq> * m);

    // Stages (Latent and Prodromal just last their drawn duration)
    static void _update_rash(Agent<TSeq> * p, Model<TSeq> * m);

    static void _on_stage_change(
        Agent<TSeq> * p, size_t k, int from, int to, Model<TSeq> * m
    );

    static void _quarantine_process(Model<TSeq> * m);

    /// Hospitalizes the agent with the daily probability (agents with rash)
    static bool _hospitalize(Agent<TSeq> * p, Model<TSeq> * m);

    // Data about the quarantine process
    std::vector< bool > quarantine_willingness; ///< Indicator for quarantine willingness
    std::vector< bool > isolation_willingness; ///< Indicator for isolation willingness
    std::vector< size_t > agent_quarantine_triggered; ///< Whether the quarantine process has started
    std::vector< int > day_flagged; ///< Day the agent was quarantined or isolated

public:

    /// @name Statuses (the model's states)
    ///@{
    static const int FREE         = 0;
    static const int QUARANTINED  = 1;
    static const int ISOLATED     = 2;
    static const int HOSPITALIZED = 3;
    ///@}

    /// @name Stages of the natural history (`get_natural_history(0)`)
    ///@{
    static const int SUSCEPTIBLE = 0;
    static const int LATENT      = 1;
    static const int PRODROMAL   = 2;
    static const int RASH        = 3;
    static const int RECOVERED   = 4;
    ///@}

    static const size_t QUARANTINE_PROCESS_INACTIVE = 0u;
    static const size_t QUARANTINE_PROCESS_ACTIVE   = 1u;
    static const size_t QUARANTINE_PROCESS_DONE     = 2u;

    ModelMeaslesMixing() = delete;

    /**
     * @brief Constructs a ModelMeaslesMixing object.
     *
     * @param n The number of agents in the model.
     * @param prevalence The initial prevalence of the disease in the model.
     * @param transmission_rate The transmission rate of the disease in the
     * model.
     * @param vax_efficacy The efficacy of the vaccine.
     * @param vax_reduction_recovery_rate The reduction in recovery rate due
     * to the vaccine.
     * @param incubation_period The incubation period of the disease in the
     * model.
     * @param prodromal_period The prodromal period of the disease in the
     * model.
     * @param rash_period The rash period of the disease in the model.
     * @param hospitalization_rate The rate at which infected individuals are
     * hospitalized.
     * @param hospitalization_period The average duration of hospitalization in
     * days.
     * @param days_undetected The average number of days an infected individual
     * remains undetected.
     * @param quarantine_period The duration of quarantine in days for latent
     * contacts.
     * @param quarantine_willingness The proportion of individuals willing to
     * comply with quarantine measures.
     * @param isolation_willingness The proportion of individuals willing to
     * self-isolate when detected.
     * @param isolation_period The duration of isolation in days for detected
     * infected individuals.
     * @param prop_vaccinated The proportion of vaccinated agents.
     * @param contact_tracing_success_rate The probability of successfully
     * identifying and tracing contacts (default: 1.0).
     * @param contact_tracing_days_window The number of days prior or after
     * rash onset for which contacts are traced (default: 4).
     * @param rash_reduction_contact_rate The reduction in contacts of agents
     * with rash (default: 1.0, no contacts).
     * @param incubation_period_sd Standard deviation of the incubation
     * period in days (default: 2.5).
     * @param prodromal_period_sd Standard deviation of the prodromal period
     * in days (default: 1.7).
     * @param rash_period_sd Standard deviation of the rash period in days
     * (default: 1.0).
     */
    ModelMeaslesMixing(
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double vax_efficacy,
        epiworld_double vax_reduction_recovery_rate,
        epiworld_double incubation_period,
        epiworld_double prodromal_period,
        epiworld_double rash_period,
        epiworld_double hospitalization_rate,
        epiworld_double hospitalization_period,
        // Policy parameters
        epiworld_double days_undetected,
        epiworld_fast_int quarantine_period,
        epiworld_double quarantine_willingness,
        epiworld_double isolation_willingness,
        epiworld_fast_int isolation_period,
        epiworld_double prop_vaccinated,
        epiworld_double contact_tracing_success_rate = 1.0,
        epiworld_fast_uint contact_tracing_days_window = 4u,
        epiworld_double rash_reduction_contact_rate = 1.0,
        epiworld_double incubation_period_sd = 2.5,
        epiworld_double prodromal_period_sd = 1.7,
        epiworld_double rash_period_sd = 1.0
    );

    /**
     * @brief Reset the model to initial state
     */
    void reset() override;

    /**
     * @brief Create a clone of this model
     * @return Pointer to a new model instance with the same configuration
     */
    std::unique_ptr< Model<TSeq> > clone_ptr() override;

    /**
     * @brief Set the initial states of the model
     * @param proportions_ Double vector with two elements:
     * - [0]: The proportion of the initial cases who start in the prodromal
     *   stage (the rest start latent).
     * - [1]: The proportion of the other agents who start recovered (immune).
     * @param queue_ Ignored.
     */
    ModelMeaslesMixing<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    ) override;

    /**
     * @brief Get the quarantine trigger status for all agents
     * @return Vector indicating quarantine process status for each agent
     */
    std::vector< size_t > get_agent_quarantine_triggered() const
    {
        return agent_quarantine_triggered;
    };

    /**
     * @brief Get the quarantine willingness for all agents
     * @return Vector of boolean values indicating each agent's willingness to quarantine
     */
    std::vector< bool > get_quarantine_willingness() const
    {
        return quarantine_willingness;
    };

    /**
     * @brief Get the isolation willingness for all agents
     * @return Vector of boolean values indicating each agent's willingness to self-isolate
     */
    std::vector< bool > get_isolation_willingness() const
    {
        return isolation_willingness;
    };

    // Overriding the next() function to include the model update
    void next() override;

};

template<typename TSeq>
inline void ModelMeaslesMixing<TSeq>::reset()
{

    Model<TSeq>::reset();

    // Whether each vaccinated agent is immune is drawn here, in id order (the
    // susceptible update then skips the immune ones)
    auto & virusptr = Model<TSeq>::viruses[0u];
    for (auto & a: this->get_agents())
        if (a.get_virus() == nullptr)
            a.get_susceptibility_reduction(virusptr, *this);

    // The quarantine process reads the contacts in the layer's log
    auto & mixing = measles_mixing_layer(*this);
    if (!mixing.is_tracing())
        mixing.enable_tracing();

    sampler.reset(*this);

    // Setting up the quarantine parameters
    quarantine_willingness.assign(this->size(), false);
    isolation_willingness.assign(this->size(), false);
    for (size_t idx = 0; idx < quarantine_willingness.size(); ++idx)
    {
        quarantine_willingness[idx] =
            this->runif() < EPI_PAR(this, "Quarantine willingness");
        isolation_willingness[idx] =
            this->runif() < EPI_PAR(this, "Isolation willingness");
    }

    agent_quarantine_triggered.assign(this->size(), 0u);
    day_flagged.assign(this->size(), 0);

    return;

}

template<typename TSeq>
inline std::unique_ptr<Model<TSeq>> ModelMeaslesMixing<TSeq>::clone_ptr()
{

    return std::make_unique<ModelMeaslesMixing<TSeq>>(*this);

}

template<typename TSeq>
inline bool ModelMeaslesMixing<TSeq>::_hospitalize(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    if (m->runif() >= EPI_PAR(m, "Hospitalization rate"))
        return false;

    m->record_hospitalization(*p);
    p->change_state(*m, HOSPITALIZED);
    return true;

}

template<typename TSeq>
inline void ModelMeaslesMixing<TSeq>::_update_free(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    auto * model = model_cast<ModelMeaslesMixing<TSeq>, TSeq>(m);
    const int stage = p->get_stage(*m);

    if (stage == SUSCEPTIBLE)
    {

        // An immune agent cannot be infected: no contacts drawn
        if (p->get_susceptibility_reduction(model->viruses[0u], *m) >= 1.0)
            return;

        size_t ndraws = model->sampler.sample(p, *model);

        #ifdef EPI_DEBUG
        model->sampler.record_sampled_size(ndraws);
        #endif

        if (ndraws == 0u)
            return;

        Virus<TSeq> * virus = m->sample_virus(
            p, model->sampler.get_sampled_agents().data(), ndraws
        );
        if (virus != nullptr)
            p->set_virus(*m, *virus);

    }
    else if (stage == RASH)
    {

        // Detection starts contact tracing and isolates the agent, unless
        // it is hospitalized
        bool detected =
            (EPI_PAR(m, "Isolation period") >= 0) &&
            (m->runif() < 1.0/EPI_PAR(m, "Days undetected"));

        if (detected)
            model->agent_quarantine_triggered[p->get_id()] =
                QUARANTINE_PROCESS_ACTIVE;

        if (!_hospitalize(p, m) && detected)
        {
            p->change_state(*m, ISOLATED);
            model->day_flagged[p->get_id()] = m->today();
        }

    }

}

template<typename TSeq>
inline void ModelMeaslesMixing<TSeq>::_update_quarantined(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    auto * model = model_cast<ModelMeaslesMixing<TSeq>, TSeq>(m);

    int days_since = m->today() - model->day_flagged[p->get_id()];
    if (EPI_PAR(m, "Quarantine period") <= days_since)
        p->change_state(*m, FREE);

}

template<typename TSeq>
inline void ModelMeaslesMixing<TSeq>::_update_isolated(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // Isolation lasts from rash onset, also for agents who recovered
    const auto & nh = m->get_natural_history();
    if ((p->get_stage(*m) == RASH) && _hospitalize(p, m))
        return;

    int days_since = m->today() - nh.entered(*p, RASH);
    if (EPI_PAR(m, "Isolation period") <= days_since)
        p->change_state(*m, FREE);

}

template<typename TSeq>
inline void ModelMeaslesMixing<TSeq>::_update_hospitalized(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // Discharged free; agents still with rash recover then
    if (m->runif() < 1.0/EPI_PAR(m, "Hospitalization period"))
    {
        p->change_state(*m, FREE);
        if (p->get_stage(*m) == RASH)
            p->rm_virus(*m);
    }

}

template<typename TSeq>
inline void ModelMeaslesMixing<TSeq>::_update_rash(
    Agent<TSeq> * p, Model<TSeq> * m
) {

    // The rash stage does not end in hospital (see _update_hospitalized())
    if (p->get_state() == HOSPITALIZED)
        return;

    if (m->get_natural_history().due(*p, *m))
        p->rm_virus(*m);

}

template<typename TSeq>
inline void ModelMeaslesMixing<TSeq>::_on_stage_change(
    Agent<TSeq> * p, size_t, int, int to, Model<TSeq> * m
) {

    // A quarantined agent who develops rash is isolated
    if ((to == RASH) && (p->get_state() == QUARANTINED))
        p->change_state(*m, ISOLATED);

}

template<typename TSeq>
inline void ModelMeaslesMixing<TSeq>::_quarantine_process(Model<TSeq> * m) {

    auto * model = model_cast<ModelMeaslesMixing<TSeq>,TSeq>(m);
    const auto & nh = m->get_natural_history();

    // The contacts in the log of the mixing layer
    auto & ct = measles_mixing_layer(*m).get_tracing();
    for (size_t agent_i = 0u; agent_i < m->size(); ++agent_i)
    {

        // Checking if the quarantine in the agent was triggered
        // or not
        if (model->agent_quarantine_triggered[agent_i] != QUARANTINE_PROCESS_ACTIVE)
            continue;

        if (EPI_PAR(m, "Quarantine period") < 0)
            continue;

        // Getting the number of contacts, if it is greater
        // than the log's capacity, it means that we overflowed, so
        // we will only quarantine the first get_max_contacts()
        size_t n_contacts = ct.get_n_contacts(agent_i);
        if (n_contacts > ct.get_max_contacts())
            n_contacts = ct.get_max_contacts();

        // When the rash onset started (this is for contact tracing)
        int day_rash_onset_agent_i = nh.entered(m->get_agent(agent_i), RASH);

        for (size_t contact_i = 0u; contact_i < n_contacts; ++contact_i)
        {

            // Checking if the contact is within the contact tracing days prior
            auto [contact_id, contact_date] = ct.get_contact(agent_i, contact_i);

            bool within_days =
                std::abs(day_rash_onset_agent_i - contact_date) <=
                EPI_PAR(m, "Contact tracing days window");

            if (!within_days)
                continue;

            // Checking if we will detect the contact
            if (m->runif() > EPI_PAR(m, "Contact tracing success rate"))
                continue;

            auto & agent = m->get_agent(contact_id);

            // Only free agents who have not recovered
            const int stage = agent.get_stage(*m);
            if ((agent.get_state() != FREE) || (stage == RECOVERED))
                continue;

            // Agents with some tool won't be quarantined
            if (agent.get_n_tools() != 0u)
                continue;

            if (!model->quarantine_willingness[contact_id])
                continue;

            // Contacts with rash are isolated instead, if willing
            if (stage != RASH)
                agent.change_state(*m, QUARANTINED);
            else if (model->isolation_willingness[contact_id])
                agent.change_state(*m, ISOLATED);
            else
                continue;

            model->day_flagged[contact_id] = m->today();

        }

        // Setting the quarantine process off
        model->agent_quarantine_triggered[agent_i] = QUARANTINE_PROCESS_DONE;
    }

    return;
}

template<typename TSeq>
inline ModelMeaslesMixing<TSeq>::ModelMeaslesMixing(
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double vax_efficacy,
    epiworld_double vax_reduction_recovery_rate,
    epiworld_double incubation_period,
    epiworld_double prodromal_period,
    epiworld_double rash_period,
    epiworld_double hospitalization_rate,
    epiworld_double hospitalization_period,
    // Policy parameters
    epiworld_double days_undetected,
    epiworld_fast_int quarantine_period,
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    epiworld_fast_int isolation_period,
    epiworld_double prop_vaccinated,
    epiworld_double contact_tracing_success_rate,
    epiworld_fast_uint contact_tracing_days_window,
    epiworld_double rash_reduction_contact_rate,
    epiworld_double incubation_period_sd,
    epiworld_double prodromal_period_sd,
    epiworld_double rash_period_sd
    )
{

    // Assertions
    auto max_uint = std::numeric_limits< size_t >::max();
    auto max_double = std::numeric_limits< double >::max();
    auto max_int = std::numeric_limits< int >::max();
    EpiAssert::check_probability(prevalence, "prevalence", "ModelMeaslesMixing");
    EpiAssert::check_bounds(n, static_cast<size_t>(1), max_uint, "n", "ModelMeaslesMixing");
    EpiAssert::check_probability(transmission_rate, "transmission_rate", "ModelMeaslesMixing");
    EpiAssert::check_probability(vax_efficacy, "vax_efficacy", "ModelMeaslesMixing");
    EpiAssert::check_bounds(vax_reduction_recovery_rate, 0.0, 1.0, "vax_reduction_recovery_rate", "ModelMeaslesMixing");
    EpiAssert::check_bounds(incubation_period, 0.0, max_double, "incubation_period", "ModelMeaslesMixing");
    EpiAssert::check_bounds(prodromal_period, 0.0, max_double, "prodromal_period", "ModelMeaslesMixing");
    EpiAssert::check_bounds(rash_period, 0.0, max_double, "rash_period", "ModelMeaslesMixing");
    EpiAssert::check_bounds(days_undetected, 0.0, max_double, "days_undetected", "ModelMeaslesMixing");
    EpiAssert::check_probability(hospitalization_rate, "hospitalization_rate", "ModelMeaslesMixing");
    EpiAssert::check_bounds(hospitalization_period, 0.0, max_double, "hospitalization_period", "ModelMeaslesMixing");
    EpiAssert::check_probability(prop_vaccinated, "prop_vaccinated", "ModelMeaslesMixing");
    EpiAssert::check_bounds(quarantine_period, -1, max_int, "quarantine_period", "ModelMeaslesMixing");
    EpiAssert::check_bounds(quarantine_willingness, 0.0, 1.0, "quarantine_willingness", "ModelMeaslesMixing");
    EpiAssert::check_bounds(isolation_period, -1, max_int, "isolation_period", "ModelMeaslesMixing");
    EpiAssert::check_probability(contact_tracing_success_rate, "contact_tracing_success_rate", "ModelMeaslesMixing");
    EpiAssert::check_bounds(rash_reduction_contact_rate, 0.0, 1.0, "rash_reduction_contact_rate", "ModelMeaslesMixing");
    measles_check_stage_sds(
        incubation_period_sd, prodromal_period_sd, rash_period_sd,
        "ModelMeaslesMixing"
    );

    // Setting up parameters
    this->add_param(transmission_rate, "Transmission rate");
    this->add_param(incubation_period, "Incubation period");
    this->add_param(prodromal_period, "Prodromal period");
    this->add_param(rash_period, "Rash period");
    this->add_param(hospitalization_rate, "Hospitalization rate");
    this->add_param(hospitalization_period, "Hospitalization period");
    this->add_param(days_undetected, "Days undetected");
    this->add_param(quarantine_period, "Quarantine period");
    this->add_param(quarantine_willingness, "Quarantine willingness");
    this->add_param(isolation_willingness, "Isolation willingness");
    this->add_param(isolation_period, "Isolation period");
    this->add_param(contact_tracing_success_rate, "Contact tracing success rate");
    this->add_param(contact_tracing_days_window, "Contact tracing days window");
    this->add_param(prop_vaccinated, "Vaccination rate");
    this->add_param(vax_efficacy, "Vax efficacy");
    this->add_param(vax_reduction_recovery_rate, "(IGNORED) Vax improved recovery");
    this->add_param(rash_reduction_contact_rate, "Rash reduction contact rate");
    this->add_param(incubation_period_sd, "Incubation period sd");
    this->add_param(prodromal_period_sd, "Prodromal period sd");
    this->add_param(rash_period_sd, "Rash period sd");

    // Only free agents are in contact; prodromal ones are infectious, and
    // those with rash at the reduced contact rate
    using Sampler = SamplerMixing<TSeq>;
    sampler.configure(
        [](const Agent<TSeq> & a, const Model<TSeq> & m) {
            if (a.get_state() != FREE)
                return Sampler::NONE;
            const int stage = a.get_stage(m);
            return (stage == PRODROMAL) ? Sampler::PRIMARY :
                (stage == RASH) ? Sampler::REDUCED : Sampler::CONTACT;
        },
        "Rash reduction contact rate"
    );

    // Statuses
    this->add_state("Free", _update_free);
    this->add_state("Quarantined", _update_quarantined);
    this->add_state("Isolated", _update_isolated);
    this->add_state("Hospitalized", _update_hospitalized);

    // The natural history
    NaturalHistory<TSeq> nh("Measles");
    nh.add_stage("Susceptible");
    nh.add_stage("Latent");
    nh.add_stage("Prodromal");
    nh.add_stage("Rash", _update_rash);
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
    this->add_natural_history(nh);
    this->set_stage_hook(_on_stage_change);

    // Adding global event
    this->add_globalevent(_quarantine_process, "Quarantine process");

    // Preparing the virus -------------------------------------------
    Virus<TSeq> virus("Measles", prevalence, true);
    virus.set_natural_history(0);
    virus.set_prob_infecting("Transmission rate");

    this->add_virus(virus);

    // Designing the vaccine
    ToolVaccine<TSeq> vaccine("MMR");

    vaccine.set_susceptibility_reduction(this->get_param("Vax efficacy"));

    vaccine.set_distribution(
        distribute_tool_randomly(prop_vaccinated, true)
    );

    this->add_tool(vaccine);

    // Adding the empty population
    this->set_population_size(n);

    this->set_name("Measles with Mixing and Quarantine");

}

template<typename TSeq>
inline ModelMeaslesMixing<TSeq> & ModelMeaslesMixing<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
)
{

    if (proportions_.size() != 2u)
        throw std::invalid_argument("-proportions_- must have two entries.");

    for (auto & v : proportions_)
        if ((v < 0.0) || (v > 1.0))
            throw std::invalid_argument(
                "-proportions_- must have values between 0 and 1."
            );

    // Some initial cases start prodromal, and some of the others recovered
    Model<TSeq>::initial_states_fun = [proportions_](Model<TSeq> * m) -> void {
        for (auto & a : m->get_agents())
        {
            const bool infected = a.get_virus() != nullptr;
            if (m->runif() >= proportions_[infected ? 0u : 1u])
                continue;
            if (infected)
                a.change_stage(*m, PRODROMAL);
            else
                a.change_stage(*m, RECOVERED);
        }
        m->events_run();
    };

    return *this;

}

template<typename TSeq>
inline void ModelMeaslesMixing<TSeq>::next()
{
    sampler.update(*this);
    Model<TSeq>::next();
}

#endif
