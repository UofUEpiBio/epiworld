
#ifndef VARICELLASCHOOL_HPP
#define VARICELLASCHOOL_HPP

#include <cassert>

using namespace epiworld;

#define LOCAL_UPDATE_FUN(name) \
    template<typename TSeq> \
    inline void ModelVaricellaSchool<TSeq>:: name \
    (Agent<TSeq> * p, Model<TSeq> * m)

/**
 * @brief Varicella (chickenpox) model with quarantine
 *
 * @param TSeq The type of the sequence to be used.
 * @details
 * This model adapts the Measles school model for varicella (chickenpox).
 * Like measles, it is an SEIHR model with isolation and quarantine where
 * the infectious state is divided into prodromal and rash phases.
 *
 * Key differences from the Measles model:
 *
 * - The MMRV (Measles, Mumps, Rubella, Varicella) vaccine is used instead
 *   of MMR.
 * - Vaccination shortens both the prodromal and rash periods. The
 *   `vax_improved_recovery` parameter specifies the proportional reduction
 *   in duration for vaccinated breakthrough cases.
 * - Hospitalization risk differs by vaccination status: two separate
 *   parameters are provided for vaccinated and unvaccinated agents.
 *
 * Epidemiological background:
 *
 * - Incubation period: ~14 days (range 10–21 days).
 * - Prodromal period: ~2 days (1–4 days); 1 day in vaccinated breakthrough.
 * - Rash (vesicular) period: ~5–7 days; shorter in vaccinated breakthrough.
 * - Hospitalization: ~5–7 per 1,000 unvaccinated; much lower in vaccinated.
 * - Transmission occurs during the late prodromal phase.
 *
 * The quarantine and isolation logic (including contact tracing) is
 * inherited unchanged from the Measles school model.
 *
 * @ingroup disease_specific
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelVaricellaSchool final :
    public Model<TSeq>,
    public QuarantineTrigger<TSeq>
{

private:

    /**
     * @brief State update functions.
     */
    ///@{
    static void _update_susceptible(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_latent(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_prodromal(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_rash(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_isolated(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_isolated_recovered(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_q_latent(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_q_susceptible(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_q_prodromal(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_q_recovered(Agent<TSeq> * p, Model<TSeq> * m);
    static void _update_hospitalized(Agent<TSeq> * p, Model<TSeq> * m);
    ///@}

    /**
     * @brief Quarantine agents that are in the system.
     *
     * The flow should be:
     * - The function only runs if the quarantine status is active.
     *
     * - Agents who are in quarantine, isolation, removed, or
     *   hospitalized are ignored.
     *
     * - Agents who are in the RASH state are isolated.
     *
     * - Vaccinated agents are ignored.
     *
     * - Susceptible, Latent, and Prodromal agents are moved to the
     *   QUARANTINED_* state.
     *
     * - At the end of the function, the quarantine status is set false.
     */
    static void _quarantine_agents(Model<TSeq> * m);
    
    // Update which agents are infectious for contact
    void _update_infectious();

public:

    /**
     * @name Model States
     * @brief The different states of the model.
     */
    ///@{
    static constexpr epiworld_fast_uint SUSCEPTIBLE             = 0u;
    static constexpr epiworld_fast_uint LATENT                  = 1u;
    static constexpr epiworld_fast_uint PRODROMAL               = 2u;
    static constexpr epiworld_fast_uint RASH                    = 3u;
    static constexpr epiworld_fast_uint ISOLATED                = 4u;
    static constexpr epiworld_fast_uint ISOLATED_RECOVERED      = 5u;
    static constexpr epiworld_fast_uint QUARANTINED_LATENT      = 6u;
    static constexpr epiworld_fast_uint QUARANTINED_SUSCEPTIBLE = 7u;
    static constexpr epiworld_fast_uint QUARANTINED_PRODROMAL   = 8u;
    static constexpr epiworld_fast_uint QUARANTINED_RECOVERED   = 9u;
    static constexpr epiworld_fast_uint HOSPITALIZED            = 10u;
    static constexpr epiworld_fast_uint RECOVERED               = 11u;
    ///@}
    
    // Default constructor
    ModelVaricellaSchool() = delete;

    /**
     * @brief Construct a ModelVaricellaSchool object.
     *
     * @param n Total number of agents.
     * @param n_latent Number of agents that start in the latent state.
     * @param contact_rate Average number of contacts per day.
     * @param transmission_rate Daily probability of transmission per contact
     *   with a prodromal individual.
     * @param vax_efficacy Probability that the MMRV vaccine fully prevents
     *   infection in a vaccinated agent.
     * @param vax_improved_recovery Proportional reduction in both the
     *   prodromal and rash period durations for vaccinated breakthrough cases.
     *   For example, 0.5 means the periods are 50 % shorter for vaccinated
     *   individuals.
     * @param incubation_period Mean latent period in days.
     * @param prodromal_period Mean prodromal period in days (unvaccinated
     *   baseline).
     * @param rash_period Mean rash period in days (unvaccinated baseline).
     * @param days_undetected Mean number of days before a rash case is
     *   detected and isolated.
     * @param hospitalization_rate_unvax Daily probability of hospitalization
     *   for unvaccinated agents in the rash state.
     * @param hospitalization_rate_vax Daily probability of hospitalization
     *   for vaccinated breakthrough cases in the rash state.
     * @param hospitalization_period Mean duration of hospitalization in days.
     * @param prop_vaccinated Proportion of the population that receives the
     *   MMRV vaccine at the start of the simulation.
     * @param quarantine_period Duration of quarantine in days.  Pass -1 to
     *   disable quarantine.
     * @param quarantine_willingness Probability that an agent complies with
     *   quarantine.
     * @param isolation_period Duration of isolation in days.  Pass -1 to
     *   disable isolation.
     */
    ModelVaricellaSchool(
        epiworld_fast_uint n,
        epiworld_fast_uint n_latent,
        // Disease parameters
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double vax_efficacy,
        epiworld_double vax_improved_recovery,
        epiworld_double incubation_period,
        epiworld_double prodromal_period,
        epiworld_double rash_period,
        epiworld_double days_undetected,
        epiworld_double hospitalization_rate_unvax,
        epiworld_double hospitalization_rate_vax,
        epiworld_double hospitalization_period,
        // Policy parameters
        epiworld_double prop_vaccinated,
        epiworld_fast_int quarantine_period,
        epiworld_double quarantine_willingness,
        epiworld_fast_int isolation_period
    );

    std::vector<Agent<TSeq> *> infectious; ///< Agents infectious for contact

    std::vector< int > day_flagged; ///< Either detected or started quarantine
    std::vector< int > day_rash_onset; ///< Day of rash onset
    std::vector< int > has_pep;

    void reset() override;

    std::unique_ptr< Model<TSeq> > clone_ptr() override;
    void next() override;

};

template<typename TSeq>
inline void ModelVaricellaSchool<TSeq>::_quarantine_agents(Model<TSeq> * m) {

    auto * model = model_cast<ModelVaricellaSchool<TSeq>,TSeq>(m);

    // Quarantine and isolation can be shut off if negative
    if (
        (model->par("Quarantine period") < 0) &&
        (model->par("Isolation period") < 0)
    )
        return;

    // Capturing the days that matter and the probability of success
    epiworld_double willingness = model->par("Quarantine willingness");
    epiworld_double p_detection = 1.0/(model->par("Days undetected"));
    int prodromal_period = static_cast<int>(model->par("Prodromal period"));

    bool triggered_today = false;

    // Iterating through the agents to detect new cases
    for (size_t i = 0u; i < model->size(); ++i) {

        auto & agent = model->get_agent(i);
        auto agent_id = agent.get_id();
        auto agent_state = agent.get_state();

        // Checking if detection takes place
        if ((agent_state == RASH) && (model->runif() < p_detection))
        {
            agent.change_state(*model, ISOLATED);
            model->day_flagged[agent_id] = model->today();
            model->add_triggering_agent(
                *model,
                agent,
                model->day_rash_onset[agent_id] - prodromal_period
            );
            triggered_today = true;
        }
        // Also trigger if the agent just became hospitalized today
        else if ((agent_state == HOSPITALIZED) && (agent.get_state_last_changed() == model->today()))
        {
            model->day_flagged[agent_id] = model->today();
            model->add_triggering_agent(
                *model,
                agent,
                model->day_rash_onset[agent_id] - prodromal_period
            );
            triggered_today = true;
        }

    }

    if (!triggered_today)
        return;

    // Quarantining other agents
    for (size_t i = 0u; i < model->size(); ++i) {

        auto & agent = model->get_agent(i);
        auto agent_id = agent.get_id();
        auto agent_state = agent.get_state();

        // Already quarantined or isolated
        if (agent_state >= RASH)
            continue;

        // If the agent has a vaccine, then no need for quarantine
        if (agent.get_n_tools() != 0u)
            continue;

        // Quarantine will depend on the willingness of the agent
        // to be quarantined. If negative, then quarantine never happens.
        if (
            (model->par("Quarantine period") >= 0) &&
            (model->runif() < willingness)
        )
        {

            if (agent_state == SUSCEPTIBLE)
                agent.change_state(*model, QUARANTINED_SUSCEPTIBLE);
            else if (agent_state == LATENT)
                agent.change_state(*model, QUARANTINED_LATENT);
            else if (agent_state == PRODROMAL)
                agent.change_state(*model, QUARANTINED_PRODROMAL);

            // And we add the day of quarantine
            model->day_flagged[agent_id] = model->today();

        }

    }

    return;

}

template<typename TSeq>
inline void ModelVaricellaSchool<TSeq>::reset() {

    Model<TSeq>::reset();

    this->day_flagged.assign(this->size(), 0);
    this->day_rash_onset.assign(this->size(), 0);
    this->has_pep.assign(this->size(), false);

    this->_update_infectious();
    return;

}

template<typename TSeq>
inline void ModelVaricellaSchool<TSeq>::_update_infectious() {

    this->infectious.clear();
    int n_available = 0;
    for (auto & agent: this->get_agents())
    {
        const auto & s = agent.get_state();
        if (s == PRODROMAL)
            this->infectious.push_back(&agent);

        if ((s < RASH) || (s == RECOVERED))
            ++n_available;

    }

    // Assumes fixed contact rate throughout the simulation
    // but corrects for the number of available agents.
    double p_contact = 0.0;
    if (n_available > 0)
    {
        p_contact = this->par("Contact rate")/
            static_cast< epiworld_double >(n_available);
    }

    this->set_rand_binom(
        static_cast<int>(this->infectious.size()),
        p_contact > 1.0 ? 1.0 : p_contact
    );

}

template<typename TSeq>
inline std::unique_ptr<Model<TSeq>> ModelVaricellaSchool<TSeq>::clone_ptr()
{

    return std::make_unique<ModelVaricellaSchool<TSeq>>(*this);

}

LOCAL_UPDATE_FUN(_update_susceptible) {

    // How many contacts to draw
    int ndraw = m->rbinom();

    if (ndraw == 0)
        return;

    auto* model = model_cast<ModelVaricellaSchool<TSeq>,TSeq>(m);
    size_t n_infectious = model->infectious.size();

    if (n_infectious == 0)
        return;

    // Drawing from the set
    int nviruses_tmp = 0;
    int i = 0;
    auto & _ref = *m;
    while (i < ndraw)
    {
        // Picking the actual contacts
        auto which = m->runif_index(n_infectious);

        Agent<> & neighbor = *model->infectious[which];

        // Can't sample itself
        if (neighbor.get_id() == p->get_id())
            continue;

        m->get_contact_tracing().add_contact(
            neighbor.get_id(),
            p->get_id(),
            m->today()
        );

        // We successfully drew a contact, so we increment the counter
        i++;

        if (neighbor.get_virus() == nullptr)
            throw std::logic_error("The neighbor has no virus.");

        if (neighbor.get_state() != model->PRODROMAL)
            throw std::logic_error(
                "The neighbor is not in the prodromal state. The state is: " +
                std::to_string(neighbor.get_state())
            );

        auto & v = neighbor.get_virus();

        #ifdef EPI_DEBUG
        if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
            throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
        #endif

        m->array_double_tmp[nviruses_tmp] =
            (1.0 - p->get_susceptibility_reduction(v, _ref)) *
            v->get_prob_infecting(m) *
            (1.0 - neighbor.get_transmission_reduction(v, _ref))
            ;

        m->array_virus_tmp[nviruses_tmp++] = &(*v);

    }

    if (nviruses_tmp == 0u)
        return;

    int which = roulette(nviruses_tmp, m);

    if (which < 0)
        return;

    p->set_virus(*m, *m->array_virus_tmp[which]);

    return;

};

LOCAL_UPDATE_FUN(_update_latent) {

    if (m->runif() < (1.0/p->get_virus()->get_incubation(m)))
        p->change_state(*m, ModelVaricellaSchool<TSeq>::PRODROMAL);

    return;

};

LOCAL_UPDATE_FUN(_update_prodromal) {

    // Vaccinated breakthrough cases have a shorter prodromal period
    bool is_vaccinated = (p->get_n_tools() != 0u);

    double prod_period = m->par("Prodromal period");
    if (is_vaccinated)
        prod_period *= (1.0 - m->par("Vax improved recovery"));

    if (m->runif() < (1.0/prod_period))
    {

        auto* model = model_cast<ModelVaricellaSchool<TSeq>,TSeq>(m);
        model->day_rash_onset[p->get_id()] = m->today();
        p->change_state(*m, ModelVaricellaSchool<TSeq>::RASH);

    }

    return;

};

LOCAL_UPDATE_FUN(_update_rash) {

    auto* model = model_cast<ModelVaricellaSchool<TSeq>,TSeq>(m);

    #ifdef EPI_DEBUG
    if (static_cast<int>(model->day_flagged.size()) <= p->get_id())
        throw std::logic_error(
            "The agent is not in the list of quarantined or isolated agents: " +
            std::to_string(p->get_id()) +
            " vs " +
            std::to_string(model->day_flagged.size()) +
            ". The model has " + std::to_string(model->size()) + " agents."
        );
    #endif

    // Vaccinated breakthrough cases have a shorter rash period and
    // lower hospitalization risk
    bool is_vaccinated = (p->get_n_tools() != 0u);

    double rash_period = m->par("Rash period");
    if (is_vaccinated)
        rash_period *= (1.0 - m->par("Vax improved recovery"));

    double hosp_rate = is_vaccinated ?
        m->par("Hospitalization rate (vaccinated)") :
        m->par("Hospitalization rate (unvaccinated)");

    m->array_double_tmp[0] = 1.0/rash_period;
    m->array_double_tmp[1] = hosp_rate;

    auto which = m->sample_from_probs(2);

    if (which == 0)
    {
        p->rm_virus(*m, RECOVERED);
    }
    else if (which == 1)
    {
        model->record_hospitalization(*p);
        p->change_state(*m, HOSPITALIZED);
    }
    else if (which > 2)
    {
        throw std::logic_error("The roulette returned an unexpected value.");
    }

};

LOCAL_UPDATE_FUN(_update_isolated) {

    auto* model = model_cast<ModelVaricellaSchool<TSeq>,TSeq>(m);

    // Figuring out if the agent can be released from isolation
    int days_since = m->today() - model->day_rash_onset[p->get_id()];

    bool unisolate =
        (m->par("Isolation period") <= days_since) ?
        true: false;

    // Vaccinated breakthrough cases have a shorter rash period and
    // lower hospitalization risk
    bool is_vaccinated = (p->get_n_tools() != 0u);

    double rash_period = m->par("Rash period");
    if (is_vaccinated)
        rash_period *= (1.0 - m->par("Vax improved recovery"));

    double hosp_rate = is_vaccinated ?
        m->par("Hospitalization rate (vaccinated)") :
        m->par("Hospitalization rate (unvaccinated)");

    m->array_double_tmp[0] = 1.0/rash_period;
    m->array_double_tmp[1] = hosp_rate;

    auto which = m->sample_from_probs(2);

    if (which == 0u)
    {
        p->rm_virus(*m, unisolate? RECOVERED: ISOLATED_RECOVERED);
    }
    else if (which == 1u)
    {
        model->record_hospitalization(*p);
        p->change_state(*m, HOSPITALIZED);
    }
    else if (unisolate)
    {
        p->change_state(*m, RASH);
    }

}

LOCAL_UPDATE_FUN(_update_isolated_recovered) {

    auto* model = model_cast<ModelVaricellaSchool<TSeq>,TSeq>(m);

    int days_since = m->today() - model->day_rash_onset[p->get_id()];

    bool unisolate =
        (m->par("Isolation period") <= days_since) ?
        true: false;

    if (unisolate)
        p->change_state(*m, RECOVERED);

}

LOCAL_UPDATE_FUN(_update_q_latent) {

    auto* model = model_cast<ModelVaricellaSchool<TSeq>,TSeq>(m);
    int days_since =
        m->today() - model->day_flagged[p->get_id()];

    bool unquarantine =
        (m->par("Quarantine period") <= days_since) ?
        true: false;

    if (m->runif() < (1.0/p->get_virus()->get_incubation(m)))
    {
        p->change_state(*m, unquarantine ? PRODROMAL : QUARANTINED_PRODROMAL);

    }
    else if (unquarantine)
    {
        p->change_state(*m, 
            LATENT
        );
    }

}

LOCAL_UPDATE_FUN(_update_q_susceptible) {

    auto* model = model_cast<ModelVaricellaSchool<TSeq>,TSeq>(m);
    int days_since =
        m->today() - model->day_flagged[p->get_id()];

    if (days_since >= m->par("Quarantine period"))
        p->change_state(*m, SUSCEPTIBLE);

}

LOCAL_UPDATE_FUN(_update_q_prodromal) {

    auto* model = model_cast<ModelVaricellaSchool<TSeq>,TSeq>(m);

    int days_since = m->today() - model->day_flagged[p->get_id()];

    bool unquarantine =
        (m->par("Quarantine period") <= days_since) ?
        true: false;

    // Vaccinated breakthrough cases have a shorter prodromal period
    bool is_vaccinated = (p->get_n_tools() != 0u);

    double prod_period = m->par("Prodromal period");
    if (is_vaccinated)
        prod_period *= (1.0 - m->par("Vax improved recovery"));

    if (m->runif() < (1.0/prod_period))
    {
        model->day_rash_onset[p->get_id()] = m->today();
        p->change_state(*m, ISOLATED);
    }
    else
    {

        if (unquarantine)
            p->change_state(*m, PRODROMAL);

    }

}

LOCAL_UPDATE_FUN(_update_q_recovered) {

    auto* model = model_cast<ModelVaricellaSchool<TSeq>,TSeq>(m);
    int days_since = m->today() - model->day_flagged[p->get_id()];

    if (days_since >= m->par("Quarantine period"))
        p->change_state(*m, RECOVERED);

}

LOCAL_UPDATE_FUN(_update_hospitalized) {

    if (m->runif() < 1.0/m->par("Hospitalization period"))
        p->rm_virus(*m, RECOVERED);

    return;

}


template<typename TSeq>
inline ModelVaricellaSchool<TSeq>::ModelVaricellaSchool(
    epiworld_fast_uint n,
    epiworld_fast_uint n_latent,
    // Disease parameters
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double vax_efficacy,
    epiworld_double vax_improved_recovery,
    epiworld_double incubation_period,
    epiworld_double prodromal_period,
    epiworld_double rash_period,
    epiworld_double days_undetected,
    epiworld_double hospitalization_rate_unvax,
    epiworld_double hospitalization_rate_vax,
    epiworld_double hospitalization_period,
    // Policy parameters
    epiworld_double prop_vaccinated,
    epiworld_fast_int quarantine_period,
    epiworld_double quarantine_willingness,
    epiworld_fast_int isolation_period
) {

    // Assertions
    auto max_uint = std::numeric_limits< size_t >::max();
    auto max_double = std::numeric_limits< double >::max();
    auto max_int = std::numeric_limits< int >::max();
    EpiAssert::check_bounds(n, static_cast<size_t>(1), max_uint, "n", "ModelVaricellaSchool");
    EpiAssert::check_bounds(n_latent, static_cast<size_t>(0), static_cast<size_t>(n), "n_latent", "ModelVaricellaSchool");
    EpiAssert::check_bounds(contact_rate, 0.0, max_double, "contact_rate", "ModelVaricellaSchool");
    EpiAssert::check_bounds(transmission_rate, 0.0, 1.0, "transmission_rate", "ModelVaricellaSchool");
    EpiAssert::check_bounds(vax_efficacy, 0.0, 1.0, "vax_efficacy", "ModelVaricellaSchool");
    EpiAssert::check_bounds(vax_improved_recovery, 0.0, 1.0, "vax_improved_recovery", "ModelVaricellaSchool");
    EpiAssert::check_bounds(incubation_period, 0.0, max_double, "incubation_period", "ModelVaricellaSchool");
    EpiAssert::check_bounds(prodromal_period, 0.0, max_double, "prodromal_period", "ModelVaricellaSchool");
    EpiAssert::check_bounds(rash_period, 0.0, max_double, "rash_period", "ModelVaricellaSchool");
    EpiAssert::check_bounds(days_undetected, 0.0, max_double, "days_undetected", "ModelVaricellaSchool");
    EpiAssert::check_bounds(hospitalization_rate_unvax, 0.0, 1.0, "hospitalization_rate_unvax", "ModelVaricellaSchool");
    EpiAssert::check_bounds(hospitalization_rate_vax, 0.0, 1.0, "hospitalization_rate_vax", "ModelVaricellaSchool");
    EpiAssert::check_bounds(hospitalization_period, 0.0, max_double, "hospitalization_period", "ModelVaricellaSchool");
    EpiAssert::check_bounds(prop_vaccinated, 0.0, 1.0, "prop_vaccinated", "ModelVaricellaSchool");
    EpiAssert::check_bounds(quarantine_period, -1, max_int, "quarantine_period", "ModelVaricellaSchool");
    EpiAssert::check_bounds(quarantine_willingness, 0.0, 1.0, "quarantine_willingness", "ModelVaricellaSchool");
    EpiAssert::check_bounds(isolation_period, -1, max_int, "isolation_period", "ModelVaricellaSchool");

    this->add_state("Susceptible",             this->_update_susceptible);
    this->add_state("Latent",                  this->_update_latent);
    this->add_state("Prodromal",               this->_update_prodromal);
    this->add_state("Rash",                    this->_update_rash);
    this->add_state("Isolated",                this->_update_isolated);
    this->add_state("Isolated Recovered",      this->_update_isolated_recovered);
    this->add_state("Quarantined Latent",      this->_update_q_latent);
    this->add_state("Quarantined Susceptible", this->_update_q_susceptible);
    this->add_state("Quarantined Prodromal",   this->_update_q_prodromal);
    this->add_state("Quarantined Recovered",   this->_update_q_recovered);
    this->add_state("Hospitalized",            this->_update_hospitalized);
    this->add_state("Recovered");

    // Adding the model parameters
    this->add_param(contact_rate, "Contact rate");
    this->add_param(transmission_rate, "Transmission rate");
    this->add_param(incubation_period, "Incubation period");
    this->add_param(prodromal_period, "Prodromal period");
    this->add_param(rash_period, "Rash period");
    this->add_param(days_undetected, "Days undetected");
    this->add_param(quarantine_period, "Quarantine period");
    this->add_param(quarantine_willingness, "Quarantine willingness");
    this->add_param(isolation_period, "Isolation period");
    this->add_param(hospitalization_rate_unvax, "Hospitalization rate (unvaccinated)");
    this->add_param(hospitalization_rate_vax, "Hospitalization rate (vaccinated)");
    this->add_param(hospitalization_period, "Hospitalization period");
    this->add_param(prop_vaccinated, "Vaccination rate");
    this->add_param(vax_efficacy, "Vax efficacy");
    this->add_param(vax_improved_recovery, "Vax improved recovery");

    // Designing the disease
    Virus<> varicella("Varicella");
    varicella.set_state(LATENT, RECOVERED);
    varicella.set_prob_infecting("Transmission rate");
    varicella.set_prob_recovery("Rash period");
    varicella.set_incubation("Incubation period");
    varicella.set_distribution(
        distribute_virus_randomly(n_latent, false)
    );

    this->add_virus(varicella);

    // Designing the MMRV vaccine
    ToolVaccine<TSeq> vaccine("MMRV");
    vaccine.set_susceptibility_reduction(this->par("Vax efficacy"));
    vaccine.set_distribution(distribute_tool_randomly(prop_vaccinated, true));
    this->add_tool(vaccine);

    this->queuing_off();

    // Quarantine process will be automatically triggered
    // at the end of the day
    auto quarantine_event = GlobalEvent<TSeq>(
        this->_quarantine_agents, "Quarantine process"
    );
    this->add_globalevent(quarantine_event);

    // Setting the population
    this->agents_empty_graph(n);

    // Turning on the contact tracing
    this->contact_tracing_on();

}

template<typename TSeq>
inline void ModelVaricellaSchool<TSeq>::next() {

    this->_update_infectious();
    Model<TSeq>::next();

}

#undef LOCAL_UPDATE_FUN
#endif
