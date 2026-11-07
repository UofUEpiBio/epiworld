#ifndef EPIWORLD_MODELS_MEASLESMIXING_HPP
#define EPIWORLD_MODELS_MEASLESMIXING_HPP

#include "modelmeasles.hpp"

using namespace epiworld;

/**
 * @file measlesmixing.hpp
 * @brief Template for a Measles model with population mixing, quarantine, and contact tracing
 */

/**
 * @brief Measles model with population mixing, quarantine, and contact tracing
 *
 * @details The disease, the statuses and the rules linking them are those of
 * `ModelMeasles`: stages Susceptible, Latent, Prodromal, Rash, Severe and
 * Recovered; statuses Free, Quarantined, Isolated and Hospitalized.
 *
 * A free agent with rash is detected and isolated with probability 1 / "Days
 * undetected" each day, from the day after its rash onset ("Detected", if
 * "Isolation period" is not negative). Every day, the quarantine process (a
 * global event) traces the contacts of the agents detected that day in the
 * layer's log, within
 * "Contact tracing days window" days of the rash onset, each found with
 * probability "Contact tracing success rate". A free contact without tools
 * who is willing to quarantine is quarantined ("Quarantined") if susceptible,
 * latent or prodromal, or isolated if it has rash and is willing to isolate.
 * Quarantine lasts "Quarantine period" days (negative: no quarantine).
 *
 * The contacts come from the model's mixing layer, which the user adds: a
 * `layer::Mixing` with the groups and a contact matrix (entry `(i, j)` is the
 * expected number of contacts per day an agent in group `i` has with agents
 * in group `j`), e.g.,
 *
 * ```cpp
 * auto mix = layer::mixing("mixing");
 * mix->set_groups(assign::counts({{0, 300}, {1, 700}}), 2);
 * mix->set_contact_matrix({10.0, 2.0, 2.0, 5.0});
 * model.add_layer(mix);
 * ```
 *
 * The model samples from it with its own sampler (contacts drawn with
 * replacement, agents with rash at the reduced contact rate) and logs the
 * contacts in it, which the quarantine process reads. It has no other layers
 * (see `measles_mixing_layer()`).
 *
 * ![Model Diagram](../assets/img/measlesmixing.png)
 *
 * **Implementation details:**
 * <a href="../impl/mixing-and-entity-distribution.md">Mixing and Entity Distribution</a>,
 * <a href="../impl/quarantine-isolation-and-contact-tracing.md">Quarantine, Isolation, and Contact Tracing</a>,
 * <a href="../impl/sampling-contacts.md">Sampling Contacts</a>
 *
 * @ingroup disease_specific
 */
class ModelMeaslesMixing final : public ModelMeasles
{
private:

    std::vector< size_t > agent_quarantine_triggered; ///< Whether the quarantine process has started

    static void _quarantine_process(Model * m);

public:

    static const size_t QUARANTINE_PROCESS_INACTIVE = 0u;
    static const size_t QUARANTINE_PROCESS_ACTIVE   = 1u;
    static const size_t QUARANTINE_PROCESS_DONE     = 2u;

    ModelMeaslesMixing() = delete;

    /**
     * @brief Constructs a ModelMeaslesMixing object.
     *
     * @param n The number of agents in the model.
     * @param prevalence The initial share of latent agents.
     * @param transmission_rate The probability of transmission per contact.
     * @param vax_efficacy The efficacy of the vaccine.
     * @param vax_reduction_recovery_rate Ignored.
     * @param incubation_period Mean days latent.
     * @param prodromal_period Mean days prodromal.
     * @param rash_period Mean days with rash.
     * @param hospitalization_rate Daily probability that an agent with rash
     * becomes severe (and is hospitalized).
     * @param hospitalization_period Mean days severe (in hospital).
     * @param days_undetected Mean days until an agent with rash is detected.
     * @param quarantine_period Days of quarantine (negative: no quarantine).
     * @param quarantine_willingness Share of agents willing to quarantine.
     * @param isolation_willingness Share of agents willing to isolate when
     * traced.
     * @param isolation_period Days of isolation from the rash onset
     * (negative: no detection, no isolation).
     * @param prop_vaccinated The proportion of vaccinated agents.
     * @param contact_tracing_success_rate The probability of tracing a
     * contact (default: 1.0).
     * @param contact_tracing_days_window The number of days before or after
     * the rash onset within which contacts are traced (default: 4).
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

    void reset() override;
    std::unique_ptr< Model > clone_ptr() override;

    /**
     * @brief Get the quarantine trigger status for all agents
     * @return Vector indicating quarantine process status for each agent
     */
    std::vector< size_t > get_agent_quarantine_triggered() const
    {
        return agent_quarantine_triggered;
    };


    /// This model's entry in a `ModelRegistry`: arguments and builder
    static ModelInfo model_info();

};

inline void ModelMeaslesMixing::reset()
{

    Model::reset();

    // The quarantine process reads the contacts in the layer's log
    auto & mixing = measles_mixing_layer(*this);
    if (!mixing.is_tracing())
        mixing.enable_tracing();

    this->sampler.reset(*this);
    this->draw_willingness();
    agent_quarantine_triggered.assign(this->size(), 0u);

}

inline std::unique_ptr<Model> ModelMeaslesMixing::clone_ptr()
{
    return std::make_unique<ModelMeaslesMixing>(*this);
}

inline void ModelMeaslesMixing::_quarantine_process(Model * m) {

    auto * model = model_cast<ModelMeaslesMixing>(m);
    const int isolated = m->transition_id("Isolated");
    const int quarantined = m->transition_id("Quarantined");
    constexpr int RASH = ModelMeasles::RASH;
    constexpr int FREE = ModelMeasles::FREE;

    // Contact tracing from the detected agents
    auto & ct = measles_mixing_layer(*m).get_tracing();
    for (size_t agent_i = 0u; agent_i < m->size(); ++agent_i)
    {

        if (model->agent_quarantine_triggered[agent_i] != QUARANTINE_PROCESS_ACTIVE)
            continue;

        model->agent_quarantine_triggered[agent_i] = QUARANTINE_PROCESS_DONE;

        if (EPI_PAR(m, "Quarantine period") < 0)
            continue;

        // A log keeps each agent's last contacts only
        const size_t n_contacts = std::min(
            ct.get_n_contacts(agent_i), ct.get_max_contacts()
        );

        const int onset = m->get_stage_entered(agent_i, RASH);

        for (size_t contact_i = 0u; contact_i < n_contacts; ++contact_i)
        {

            auto [contact_id, contact_date] = ct.get_contact(agent_i, contact_i);

            if (std::abs(onset - contact_date) > EPI_PAR(m, "Contact tracing days window"))
                continue;

            if (m->runif() > EPI_PAR(m, "Contact tracing success rate"))
                continue;

            auto & agent = m->get_agent(contact_id);
            const int s = m->get_stage(agent);
            if ((m->get_status(agent) != FREE) || (s > RASH))
                continue;

            // Agents with some tool are not quarantined
            if (agent.get_n_tools() != 0u)
                continue;

            if (!model->quarantine_willingness[contact_id])
                continue;

            if (s != RASH)
                m->fire(quarantined, agent);
            else if (model->isolation_willingness[contact_id])
                m->fire(isolated, agent);

        }

    }

}

inline ModelMeaslesMixing::ModelMeaslesMixing(
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
    this->check_stage_sds(incubation_period_sd, prodromal_period_sd, rash_period_sd, "ModelMeaslesMixing");

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

    this->measles_build(
        assign::share(prevalence),
        Duration::fixed("Quarantine period"),
        "Rash reduction contact rate",
        true
    );

    // A detection starts the contact tracing (once per agent)
    this->react(
        Trigger::fired(this->transition_id("Detected")),
        [](Agent & p, Model & m) {
            auto & mm = dynamic_cast< ModelMeaslesMixing & >(m);
            auto & state = mm.agent_quarantine_triggered[static_cast< size_t >(p.get_id())];
            if (state == QUARANTINE_PROCESS_INACTIVE)
                state = QUARANTINE_PROCESS_ACTIVE;
        }
    );

    // The vaccine
    ToolVaccine vaccine("MMR");
    vaccine.set_susceptibility_reduction(this->get_param("Vax efficacy"));
    this->add_tool(vaccine, assign::share(prop_vaccinated));

    this->add_globalevent(_quarantine_process, "Quarantine process");

    this->set_population_size(n);

    this->set_name("Measles with Mixing and Quarantine");

}

inline ModelInfo ModelMeaslesMixing::model_info()
{

    using uint_t = epiworld_fast_uint;
    using int_t  = epiworld_fast_int;

    // One initial case among 9,000 agents (the measles R package's examples)
    return {"MeaslesMixing", "Measles with population mixing",
        "Measles with quarantine of traced contacts and isolation of detected "
        "cases, on a mixing layer (which the user adds).",
        "",
        {
            ParamInfo::agents(9000),
            ParamInfo::probability("prevalence", "", 1.0 / 9000.0,
                "Proportion of agents infected at the start."),
            ParamInfo::probability("transmission_rate", "Transmission rate", 0.9,
                "Probability of transmission per contact with an infectious "
                "agent."),
            ParamInfo::probability("vax_efficacy", "Vax efficacy", 0.97,
                "Probability that the vaccine prevents infection."),
            ParamInfo::probability("vax_reduction_recovery_rate",
                "(IGNORED) Vax improved recovery", 0.5, "Not used by the model."),
            ParamInfo::duration("incubation_period", "Incubation period", 12.0,
                "Mean days from exposure to the prodromal stage."),
            ParamInfo::duration("prodromal_period", "Prodromal period", 4.0,
                "Mean days infectious before the rash."),
            ParamInfo::duration("rash_period", "Rash period", 3.0,
                "Mean days with rash."),
            ParamInfo::probability("hospitalization_rate", "Hospitalization rate",
                0.035, "Daily probability that an agent with rash is "
                "hospitalized (0.035: about 10% of cases)."),
            ParamInfo::duration("hospitalization_period",
                "Hospitalization period", 7.0,
                "Average number of days in the hospital."),
            ParamInfo::duration("days_undetected", "Days undetected", 2.0,
                "Average number of days before a case with rash is detected."),
            ParamInfo::period("quarantine_period", "Quarantine period", 21.0,
                "Days in quarantine for the contacts of a detected case "
                "(negative: no quarantine)."),
            ParamInfo::probability("quarantine_willingness",
                "Quarantine willingness", 1.0,
                "Probability that a contact complies with quarantine."),
            ParamInfo::probability("isolation_willingness",
                "Isolation willingness", 1.0,
                "Probability that a detected case complies with isolation."),
            ParamInfo::period("isolation_period", "Isolation period", 4.0,
                "Days in isolation for a detected case (negative: no "
                "isolation)."),
            ParamInfo::probability("prop_vaccinated", "Vaccination rate",
                1.0 - 1.0 / 15.0, "Proportion of agents vaccinated."),
            ParamInfo::probability("contact_tracing_success_rate",
                "Contact tracing success rate", 1.0,
                "Probability that a contact is traced."),
            ParamInfo::count("contact_tracing_days_window",
                "Contact tracing days window", 4.0,
                "Days before detection whose contacts are traced."),
            ParamInfo::probability("rash_reduction_contact_rate",
                "Rash reduction contact rate", 1.0,
                "Multiplier of the contact rate of agents with rash (1: no "
                "reduction)."),
            ParamInfo::duration("incubation_period_sd", "Incubation period sd",
                2.5, "Standard deviation of the (lognormal) incubation period, "
                "in days; 0 gives a fixed period."),
            ParamInfo::duration("prodromal_period_sd", "Prodromal period sd",
                1.7, "Standard deviation of the (lognormal) prodromal period, "
                "in days; 0 gives a fixed period."),
            ParamInfo::duration("rash_period_sd", "Rash period sd", 1.0,
                "Standard deviation of the (lognormal) rash period, in days; 0 "
                "gives a fixed period.")
        },
        [](const ModelArgs & a) -> std::unique_ptr< Model > {
            return std::make_unique< ModelMeaslesMixing >(
                static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_rate"), a("vax_efficacy"),
                a("vax_reduction_recovery_rate"), a("incubation_period"),
                a("prodromal_period"), a("rash_period"),
                a("hospitalization_rate"), a("hospitalization_period"),
                a("days_undetected"),
                static_cast< int_t >(a("quarantine_period")),
                a("quarantine_willingness"), a("isolation_willingness"),
                static_cast< int_t >(a("isolation_period")),
                a("prop_vaccinated"), a("contact_tracing_success_rate"),
                static_cast< uint_t >(a("contact_tracing_days_window")),
                a("rash_reduction_contact_rate"), a("incubation_period_sd"),
                a("prodromal_period_sd"), a("rash_period_sd")
            );
        }};
}

#endif
