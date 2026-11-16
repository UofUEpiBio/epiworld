#ifndef MEASLESSCHOOL_HPP
#define MEASLESSCHOOL_HPP

#include "modelmeasles.hpp"

using namespace epiworld;

/**
 * @brief Measles model of a school, with quarantine and isolation
 *
 * @details The disease, the statuses and the rules linking them are those of
 * `ModelMeasles`: stages Susceptible, Latent, Prodromal, Rash, Severe and
 * Recovered; statuses Free, Quarantined, Isolated and Hospitalized. Agents
 * with rash do not transmit.
 *
 * Every day, the quarantine process (a global event) detects each free agent
 * with rash with probability 1 / "Days undetected" and isolates it
 * ("Isolated", if "Isolation period" is not negative); an agent becoming
 * severe that day is detected too. A detection quarantines every free
 * susceptible, latent or prodromal agent not exempt (see
 * `set_quarantine_exempt_tools()`), each with probability
 * "Quarantine willingness" ("Quarantined", for "Quarantine period" days; a
 * negative period means no quarantine). The detected agents are the model's
 * quarantine triggers (see `QuarantineTrigger` and `InterventionMeaslesPEP`),
 * considered infectious since their rash onset minus "Prodromal period".
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
class ModelMeaslesSchool final :
    public ModelMeasles,
    public QuarantineTrigger
{

private:

    static void _quarantine_agents(Model * m);

public:

    ModelMeaslesSchool() = delete;

    /**
     * @param n Number of agents.
     * @param n_latent Number of initial (latent) cases.
     * @param contact_rate Contacts per agent per day.
     * @param transmission_rate Probability of transmission per contact.
     * @param vax_efficacy Efficacy of the vaccine.
     * @param vax_reduction_recovery_rate Ignored.
     * @param incubation_period Mean days latent.
     * @param prodromal_period Mean days prodromal.
     * @param rash_period Mean days with rash.
     * @param days_undetected Mean days until an agent with rash is detected.
     * @param hospitalization_rate Daily probability that an agent with rash
     * becomes severe (and is hospitalized).
     * @param hospitalization_period Mean days severe (in hospital).
     * @param prop_vaccinated Share of vaccinated agents.
     * @param quarantine_period Days of quarantine (negative: none).
     * @param quarantine_willingness Probability that an agent quarantines.
     * @param isolation_period Days of isolation from the rash onset
     * (negative: none).
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

    void reset() override;
    std::unique_ptr< Model > clone_ptr() override;


    /// This model's entry in a `ModelRegistry`: arguments and builder
    static ModelInfo model_info();
};

inline void ModelMeaslesSchool::_quarantine_agents(Model * m) {

    auto * model = model_cast<ModelMeaslesSchool>(m);
    constexpr int RASH = ModelMeasles::RASH;
    constexpr int SEVERE = ModelMeasles::SEVERE;
    constexpr int FREE = ModelMeasles::FREE;

    // Quarantine and isolation can be shut off if negative
    const bool quarantine_on = EPI_PAR(model, "Quarantine period") >= 0;
    const bool isolation_on = EPI_PAR(model, "Isolation period") >= 0;
    if (!quarantine_on && !isolation_on)
        return;

    const int isolated = m->transition_id("Isolated");
    const int quarantined = m->transition_id("Quarantined");
    const epiworld_double willingness = EPI_PAR(model, "Quarantine willingness");
    const epiworld_double p_detection = 1.0 / EPI_PAR(model, "Days undetected");
    const int prodromal_period = static_cast<int>(EPI_PAR(model, "Prodromal period"));
    const int today = model->today();

    // Detecting new cases: free agents with rash, and agents severe today
    bool triggered_today = false;
    for (auto & agent : model->get_agents())
    {

        const size_t id = static_cast< size_t >(agent.get_id());
        const int s = model->get_stage(agent, 0);

        bool detected = false;
        if ((s == RASH) && (model->get_status(agent) == FREE))
        {
            detected = model->runif() < p_detection;
            if (detected && isolation_on)
                model->fire(isolated, agent);
        }
        else if ((s == SEVERE) && (model->get_stage_entered(id, SEVERE, 0) == today))
            detected = true;

        if (!detected)
            continue;

        // When public health considers the agent infectious
        model->add_triggering_agent(
            *model, agent, model->get_stage_entered(id, RASH, 0) - prodromal_period
        );
        triggered_today = true;

    }

    if (!triggered_today || !quarantine_on)
        return;

    // Quarantining the free agents before their rash, unless exempt
    for (auto & agent : model->get_agents())
    {

        if ((model->get_stage(agent, 0) >= RASH) || (model->get_status(agent) != FREE))
            continue;

        if (model->is_quarantine_exempt(agent))
            continue;

        if (model->runif() < willingness)
            model->fire(quarantined, agent);

    }

}

inline void ModelMeaslesSchool::reset() {

    Model::reset();
    this->sampler.reset(*this);

}

inline std::unique_ptr<Model> ModelMeaslesSchool::clone_ptr()
{
    return std::make_unique<ModelMeaslesSchool>(*this);
}

inline ModelMeaslesSchool::ModelMeaslesSchool(
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
    this->check_stage_sds(incubation_period_sd, prodromal_period_sd, rash_period_sd, "ModelMeaslesSchool");

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
    this->add_param(hospitalization_rate, "Hospitalization rate");
    this->add_param(hospitalization_period, "Hospitalization period");
    this->add_param(prop_vaccinated, "Vaccination rate");
    this->add_param(vax_efficacy, "Vax efficacy");
    this->add_param(vax_reduction_recovery_rate, "(IGNORED) Vax improved recovery");
    this->add_param(incubation_period_sd, "Incubation period sd");
    this->add_param(prodromal_period_sd, "Prodromal period sd");
    this->add_param(rash_period_sd, "Rash period sd");

    // Agents with rash do not transmit
    this->measles_build(
        assign::count(n_latent),
        Duration::fixed("Quarantine period"),
        "",
        false
    );

    // The vaccine
    ToolVaccine vaccine("MMR");
    vaccine.set_susceptibility_reduction(EPI_PAR(this, "Vax efficacy"));
    vaccine.add_target("Measles");
    this->add_tool(vaccine, assign::share(prop_vaccinated));

    this->add_globalevent(this->_quarantine_agents, "Quarantine process");

    this->set_population_size(n);

    // The layer: everyone in one group, meeting "Contact rate" others a day
    auto school = layer::mixing("school");
    school->set_groups(assign::labels(std::vector< size_t >(n, 0u)), 1u);
    school->set_contact_matrix({1.0});
    school->set_contact_rate("Contact rate");
    school->enable_tracing();
    this->add_layer(school);

    this->set_name("Measles in a School with Quarantine");

}

inline ModelInfo ModelMeaslesSchool::model_info()
{

    using uint_t = epiworld_fast_uint;
    using int_t  = epiworld_fast_int;

    // One initial case in a school of 500
    return {"MeaslesSchool", "Measles in a school",
        "Measles in a fully mixed school, with quarantine of unvaccinated "
        "contacts and isolation of detected cases.",
        "",
        {
            ParamInfo::agents(500),
            ParamInfo::count("prevalence", "", 1.0,
                "Number of agents infected (latent) at the start."),
            {"contact_rate", "Contact rate", 15.0 / 0.9 / 4.0, 0.0,
                std::numeric_limits< double >::infinity(), false,
                "Average number of contacts per agent per day."},
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
            ParamInfo::duration("days_undetected", "Days undetected", 2.0,
                "Average number of days before a case with rash is detected."),
            ParamInfo::probability("hospitalization_rate", "Hospitalization rate",
                0.035, "Daily probability that an agent with rash is "
                "hospitalized (0.035: about 10% of cases)."),
            ParamInfo::duration("hospitalization_period",
                "Hospitalization period", 7.0,
                "Average number of days in the hospital."),
            ParamInfo::probability("prop_vaccinated", "Vaccination rate",
                1.0 - 1.0 / 15.0, "Proportion of agents vaccinated."),
            ParamInfo::period("quarantine_period", "Quarantine period", 21.0,
                "Days in quarantine for the contacts of a detected case "
                "(negative: no quarantine)."),
            ParamInfo::probability("quarantine_willingness",
                "Quarantine willingness", 1.0,
                "Probability that a contact complies with quarantine."),
            ParamInfo::period("isolation_period", "Isolation period", 4.0,
                "Days in isolation for a detected case (negative: no "
                "isolation)."),
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
            return std::make_unique< ModelMeaslesSchool >(
                static_cast< uint_t >(a("n")),
                static_cast< uint_t >(a("prevalence")), a("contact_rate"),
                a("transmission_rate"), a("vax_efficacy"),
                a("vax_reduction_recovery_rate"), a("incubation_period"),
                a("prodromal_period"), a("rash_period"), a("days_undetected"),
                a("hospitalization_rate"), a("hospitalization_period"),
                a("prop_vaccinated"),
                static_cast< int_t >(a("quarantine_period")),
                a("quarantine_willingness"),
                static_cast< int_t >(a("isolation_period")),
                a("incubation_period_sd"), a("prodromal_period_sd"),
                a("rash_period_sd")
            );
        }};
}

#endif
