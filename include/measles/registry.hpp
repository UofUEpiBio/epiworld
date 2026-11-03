#ifndef MEASLES_REGISTRY_HPP
#define MEASLES_REGISTRY_HPP

/**
 * @brief Adds the measles models to a model registry.
 *
 * @details `ModelMeaslesMixing` ("MeaslesMixing"),
 * `ModelMeaslesMixingRiskQuarantine` ("MeaslesMixingRiskQuarantine") and
 * `ModelMeaslesSchool` ("MeaslesSchool"), with the defaults of the measles R
 * package. They name their own virus ("Measles"). As with every registered
 * model, the model comes without layers: the mixing models need a mixing
 * layer, which the user adds.
 *
 * ```cpp
 * auto registry = epiworld::epimodels::model_registry<>();
 * measles::register_models(registry);
 * auto model = registry.make_model("MeaslesSchool", {{"n", 500}});
 * ```
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline epimodels::ModelRegistry<TSeq> & register_models(
    epimodels::ModelRegistry<TSeq> & registry
)
{

    using epimodels::ParamInfo;
    using epimodels::ModelArgs;
    using epimodels::detail::registry_prob;
    using epimodels::detail::registry_days;
    using epimodels::detail::registry_n;
    using uint_t = epiworld_fast_uint;
    using int_t  = epiworld_fast_int;
    const double inf = std::numeric_limits< double >::infinity();

    auto period = [inf](std::string arg, std::string name, double value,
        std::string description) -> ParamInfo {
        return {std::move(arg), std::move(name), value, -1.0, inf, true,
            std::move(description)};
    };

    const ParamInfo transmission = registry_prob(
        "transmission_rate", "Transmission rate", 0.9,
        "Probability of transmission per contact with an infectious agent."
    );
    const ParamInfo efficacy = registry_prob(
        "vax_efficacy", "Vax efficacy", 0.97,
        "Probability that the vaccine prevents infection."
    );
    const ParamInfo redux_recovery = registry_prob(
        "vax_reduction_recovery_rate", "(IGNORED) Vax improved recovery", 0.5,
        "Not used by the model."
    );
    const ParamInfo vaccinated = registry_prob(
        "prop_vaccinated", "Vaccination rate", 1.0 - 1.0 / 15.0,
        "Proportion of agents vaccinated."
    );
    const ParamInfo incubation = registry_days(
        "incubation_period", "Incubation period", 12.0,
        "Mean days from exposure to the prodromal stage."
    );
    const ParamInfo prodromal = registry_days(
        "prodromal_period", "Prodromal period", 4.0,
        "Mean days infectious before the rash."
    );
    const ParamInfo rash = registry_days(
        "rash_period", "Rash period", 3.0, "Mean days with rash."
    );
    const ParamInfo hosp_rate = registry_prob(
        "hospitalization_rate", "Hospitalization rate", 0.035,
        "Daily probability that an agent with rash is hospitalized (0.035: "
        "about 10% of cases)."
    );
    const ParamInfo hosp_period = registry_days(
        "hospitalization_period", "Hospitalization period", 7.0,
        "Average number of days in the hospital."
    );
    const ParamInfo undetected = registry_days(
        "days_undetected", "Days undetected", 2.0,
        "Average number of days before a case with rash is detected."
    );
    const ParamInfo q_willingness = registry_prob(
        "quarantine_willingness", "Quarantine willingness", 1.0,
        "Probability that a contact complies with quarantine."
    );
    const ParamInfo i_willingness = registry_prob(
        "isolation_willingness", "Isolation willingness", 1.0,
        "Probability that a detected case complies with isolation."
    );
    const ParamInfo i_period = period(
        "isolation_period", "Isolation period", 4.0,
        "Days in isolation for a detected case (negative: no isolation)."
    );
    const ParamInfo tracing = registry_prob(
        "contact_tracing_success_rate", "Contact tracing success rate", 1.0,
        "Probability that a contact is traced."
    );
    const ParamInfo tracing_window = {
        "contact_tracing_days_window", "Contact tracing days window", 4.0, 0.0,
        inf, true, "Days before detection whose contacts are traced."
    };
    const ParamInfo incubation_sd = registry_days(
        "incubation_period_sd", "Incubation period sd", 2.5,
        "Standard deviation of the (lognormal) incubation period, in days; 0 "
        "gives a fixed period."
    );
    const ParamInfo prodromal_sd = registry_days(
        "prodromal_period_sd", "Prodromal period sd", 1.7,
        "Standard deviation of the (lognormal) prodromal period, in days; 0 "
        "gives a fixed period."
    );
    const ParamInfo rash_sd = registry_days(
        "rash_period_sd", "Rash period sd", 1.0,
        "Standard deviation of the (lognormal) rash period, in days; 0 gives "
        "a fixed period."
    );

    // One initial case among 9,000 agents (the measles R package's examples)
    const ParamInfo n_mixing = registry_n(9000);
    const ParamInfo prevalence_mixing = registry_prob(
        "prevalence", "", 1.0 / 9000.0,
        "Proportion of agents infected at the start."
    );

    registry.add({"MeaslesMixing", "Measles with population mixing",
        "Measles with quarantine of traced contacts and isolation of detected "
        "cases, on a mixing layer.",
        "",
        {
            n_mixing, prevalence_mixing, transmission, efficacy,
            redux_recovery, incubation, prodromal, rash, hosp_rate,
            hosp_period, undetected,
            period("quarantine_period", "Quarantine period", 21.0,
                "Days in quarantine for the contacts of a detected case "
                "(negative: no quarantine)."),
            q_willingness, i_willingness, i_period, vaccinated, tracing,
            tracing_window,
            registry_prob("rash_reduction_contact_rate",
                "Rash reduction contact rate", 1.0,
                "Multiplier of the contact rate of agents with rash (1: no "
                "reduction)."),
            incubation_sd, prodromal_sd, rash_sd
        },
        {ModelMeaslesMixing<TSeq>::LATENT, ModelMeaslesMixing<TSeq>::RECOVERED,
            ModelMeaslesMixing<TSeq>::RECOVERED},
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelMeaslesMixing<TSeq> >(
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
        }});

    registry.add({"MeaslesMixingRiskQuarantine",
        "Measles with population mixing and risk-based quarantine",
        "Measles where the quarantine of a traced contact lasts by the "
        "contact's risk (high, medium, low), on a mixing layer.",
        "",
        {
            n_mixing, prevalence_mixing, transmission, efficacy, incubation,
            prodromal, rash, hosp_rate, hosp_period, undetected,
            period("quarantine_period_high", "Quarantine period high", 21.0,
                "Days in quarantine for high-risk contacts (negative: no "
                "quarantine)."),
            period("quarantine_period_medium", "Quarantine period medium",
                14.0, "Days in quarantine for medium-risk contacts (negative: "
                "no quarantine)."),
            period("quarantine_period_low", "Quarantine period low", 7.0,
                "Days in quarantine for low-risk contacts (negative: no "
                "quarantine)."),
            q_willingness, i_willingness, i_period, vaccinated,
            registry_prob("detection_rate_quarantine",
                "Detection rate quarantine", 0.5,
                "Daily probability that an infected agent in quarantine is "
                "detected."),
            tracing, tracing_window, incubation_sd, prodromal_sd, rash_sd
        },
        {ModelMeaslesMixingRiskQuarantine<TSeq>::LATENT,
            ModelMeaslesMixingRiskQuarantine<TSeq>::RECOVERED,
            ModelMeaslesMixingRiskQuarantine<TSeq>::RECOVERED},
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelMeaslesMixingRiskQuarantine<TSeq> >(
                static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_rate"), a("vax_efficacy"),
                a("incubation_period"), a("prodromal_period"),
                a("rash_period"), a("hospitalization_rate"),
                a("hospitalization_period"), a("days_undetected"),
                static_cast< int_t >(a("quarantine_period_high")),
                static_cast< int_t >(a("quarantine_period_medium")),
                static_cast< int_t >(a("quarantine_period_low")),
                a("quarantine_willingness"), a("isolation_willingness"),
                static_cast< int_t >(a("isolation_period")),
                a("prop_vaccinated"), a("detection_rate_quarantine"),
                a("contact_tracing_success_rate"),
                static_cast< uint_t >(a("contact_tracing_days_window")),
                a("incubation_period_sd"), a("prodromal_period_sd"),
                a("rash_period_sd")
            );
        }});

    // One initial case in a school of 500
    registry.add({"MeaslesSchool", "Measles in a school",
        "Measles in a fully mixed school, with quarantine of unvaccinated "
        "contacts and isolation of detected cases.",
        "",
        {
            registry_n(500),
            {"prevalence", "", 1.0, 0.0,
                static_cast< double >(std::numeric_limits< uint_t >::max()),
                true, "Number of agents infected (latent) at the start."},
            {"contact_rate", "Contact rate", 15.0 / 0.9 / 4.0, 0.0, inf, false,
                "Average number of contacts per agent per day."},
            transmission, efficacy, redux_recovery, incubation, prodromal,
            rash, undetected, hosp_rate, hosp_period, vaccinated,
            period("quarantine_period", "Quarantine period", 21.0,
                "Days in quarantine for the contacts of a detected case "
                "(negative: no quarantine)."),
            q_willingness, i_period, incubation_sd, prodromal_sd, rash_sd
        },
        {static_cast< int_t >(ModelMeaslesSchool<TSeq>::LATENT),
            static_cast< int_t >(ModelMeaslesSchool<TSeq>::RECOVERED), -99},
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelMeaslesSchool<TSeq> >(
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
        }});

    return registry;

}

#endif
