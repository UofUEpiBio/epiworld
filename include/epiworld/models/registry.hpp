#ifndef EPIWORLD_MODELS_REGISTRY_HPP
#define EPIWORLD_MODELS_REGISTRY_HPP

/**
 * @brief One argument of a registered model's constructor.
 * @ingroup epi_models
 *
 * @details `arg` is the argument's name, the one every wrapper uses (R, Python
 * and JS take `transmission_prob`, not each their own). `name` is the model
 * parameter the argument sets, as `Model::get_param()` knows it
 * ("Transmission probability"); it is empty for the arguments that are not
 * parameters (the population size `n`, the prevalence). `min` and `max` are
 * hard limits, checked by `ModelRegistry::make_model()` before the model is
 * built.
 */
struct ParamInfo {
    std::string arg;           ///< Argument name, e.g. "transmission_prob".
    std::string name;          ///< Model parameter it sets, or empty.
    double value;              ///< Default value.
    double min;                ///< Smallest valid value.
    double max;                ///< Largest valid value (may be infinite).
    bool integer;              ///< Whether the value must be a whole number.
    std::string description;   ///< One sentence, for help pages and UIs.
};

/**
 * @brief The states a virus added to a model moves agents through.
 * @ingroup epi_models
 *
 * @details A model's states are shared by every virus it carries. A virus
 * added to a model needs the same three states as the model's own:
 * where it puts an agent it infects (`init`), where the agent goes when it
 * recovers (`post`), and where it goes when it is removed (`removed`), as
 * `Virus::set_state()` takes them. `apply()` sets them on a virus.
 */
struct InfectionStates {
    epiworld_fast_int init;
    epiworld_fast_int post;
    epiworld_fast_int removed;

    template<typename TSeq>
    void apply(Virus<TSeq> & virus) const
    {
        virus.set_state(init, post, removed);
    }
};

/**
 * @brief The values a registered model's builder receives.
 * @ingroup epi_models
 *
 * @details Complete (every argument of the model, defaults filled in) and
 * checked against the model's `ParamInfo`. `a("transmission_prob")` reads a
 * value; `a.virus_name()` the name of the model's virus.
 */
class ModelArgs {
private:
    std::map< std::string, double > values;
    std::string vname;

public:
    ModelArgs(std::map< std::string, double > values_, std::string vname_) :
        values(std::move(values_)), vname(std::move(vname_)) {}

    double operator()(const std::string & arg) const
    {
        auto it = values.find(arg);
        if (it == values.end())
            throw std::logic_error(
                "The model's builder reads \"" + arg +
                "\", which is not one of its arguments."
            );
        return it->second;
    }

    const std::string & virus_name() const noexcept { return vname; }
};

/**
 * @brief A model the registry can build: its arguments, states and builder.
 * @ingroup epi_models
 *
 * @details `params` lists the constructor's arguments in its order. The
 * builder calls the model's constructor and nothing else: the model it returns
 * has no layers, which are added the same way as for a model built directly
 * (`Model::add_layer()`).
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
struct ModelInfo {
    std::string id;                   ///< e.g. "SEIR", for make_model()
    std::string label;                ///< e.g. "Susceptible-Exposed-Infected-Recovered"
    std::string description;          ///< One paragraph
    std::string virus_name;           ///< Default virus name; empty if the model names its own
    std::vector< ParamInfo > params;  ///< Constructor arguments, in order
    InfectionStates virus_states;     ///< States for the viruses added to the model
    std::function< std::unique_ptr< Model<TSeq> >(const ModelArgs &) > build;
};

/**
 * @brief Builds models by name, and lists them with their arguments.
 * @ingroup epi_models
 *
 * @details One place per model: the wrappers bind `list_models()` and
 * `make_model()` once instead of every constructor, and read argument names,
 * defaults, limits and descriptions from `ParamInfo`.
 *
 * ```cpp
 * auto registry = epimodels::model_registry<>();    // the core models
 * measles::register_models(registry);              // and, optionally, measles'
 *
 * auto model = registry.make_model(
 *     "SEIR", {{"n", 5000}, {"transmission_prob", 0.1}}, "flu"
 * );                                               // the rest take defaults
 * model->add_layer(layer::network("contacts", graph::smallworld<>(10, 0.1)));
 * model->run(100, 123);
 * ```
 *
 * A registry is a value: the core's comes from `model_registry()`, and
 * other libraries add their models to it with `add()` (the measles models with
 * `measles::register_models()`). There is no global state.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelRegistry {
private:
    std::vector< ModelInfo<TSeq> > models;

public:
    /**
     * @brief Adds a model.
     * @throws std::invalid_argument if the id is empty or already taken, an
     * argument is repeated, a default is outside its limits, or there is no
     * builder.
     */
    ModelRegistry<TSeq> & add(ModelInfo<TSeq> info);

    /// Every model, in the order they were added.
    const std::vector< ModelInfo<TSeq> > & list_models() const noexcept
    {
        return models;
    }

    /// @throws std::invalid_argument naming the known ids.
    const ModelInfo<TSeq> & get_model_info(const std::string & id) const;

    /**
     * @brief Builds a model from its id and the values of its arguments.
     * @param id The model, as in `list_models()`.
     * @param values Argument values by `ParamInfo::arg`; the arguments left out
     * take their defaults.
     * @param virus_name Name of the model's virus; empty for the default. Only
     * for the models whose `ModelInfo::virus_name` is not empty.
     * @throws std::invalid_argument for an unknown id or argument, a value
     * outside its limits or not whole when it must be, or a virus name for a
     * model that names its own virus.
     */
    std::unique_ptr< Model<TSeq> > make_model(
        const std::string & id,
        const std::map< std::string, double > & values = {},
        const std::string & virus_name = ""
    ) const;
};

template<typename TSeq>
inline ModelRegistry<TSeq> & ModelRegistry<TSeq>::add(ModelInfo<TSeq> info)
{

    if (info.id.empty())
        throw std::invalid_argument("A registered model needs an id.");

    for (const auto & m : models)
        if (m.id == info.id)
            throw std::invalid_argument(
                "The registry already has a model \"" + info.id + "\"."
            );

    if (!info.build)
        throw std::invalid_argument(
            "The model \"" + info.id + "\" has no builder."
        );

    for (size_t i = 0u; i < info.params.size(); ++i)
    {
        const auto & p = info.params[i];

        for (size_t j = 0u; j < i; ++j)
            if (info.params[j].arg == p.arg)
                throw std::invalid_argument(
                    "The model \"" + info.id + "\" lists the argument \"" +
                    p.arg + "\" twice."
                );

        if ((p.value < p.min) || (p.value > p.max))
            throw std::invalid_argument(
                "The default of \"" + p.arg + "\" in the model \"" + info.id +
                "\" is outside its limits."
            );
    }

    models.push_back(std::move(info));
    return *this;

}

template<typename TSeq>
inline const ModelInfo<TSeq> & ModelRegistry<TSeq>::get_model_info(
    const std::string & id
) const
{

    for (const auto & m : models)
        if (m.id == id)
            return m;

    std::string known;
    for (const auto & m : models)
        known += (known.empty() ? "" : ", ") + m.id;

    throw std::invalid_argument(
        "Unknown model \"" + id + "\". The registry has: " + known + "."
    );

}

template<typename TSeq>
inline std::unique_ptr< Model<TSeq> > ModelRegistry<TSeq>::make_model(
    const std::string & id,
    const std::map< std::string, double > & values,
    const std::string & virus_name
) const
{

    const auto & info = get_model_info(id);

    // Every value given must be an argument of the model
    for (const auto & kv : values)
    {
        bool found = false;
        for (const auto & p : info.params)
            if (p.arg == kv.first)
            {
                found = true;
                break;
            }

        if (!found)
        {
            std::string known;
            for (const auto & p : info.params)
                known += (known.empty() ? "" : ", ") + p.arg;

            throw std::invalid_argument(
                "The model \"" + id + "\" has no argument \"" + kv.first +
                "\". Its arguments are: " + known + "."
            );
        }
    }

    // Complete with the defaults, and check
    std::map< std::string, double > complete;
    for (const auto & p : info.params)
    {
        auto it = values.find(p.arg);
        const double v = (it == values.end()) ? p.value : it->second;

        if (!(v >= p.min && v <= p.max))
            throw std::invalid_argument(
                "The argument \"" + p.arg + "\" of the model \"" + id +
                "\" must be in [" + std::to_string(p.min) + ", " +
                std::to_string(p.max) + "]; got " + std::to_string(v) + "."
            );

        if (p.integer && (v != std::floor(v)))
            throw std::invalid_argument(
                "The argument \"" + p.arg + "\" of the model \"" + id +
                "\" must be a whole number; got " + std::to_string(v) + "."
            );

        complete[p.arg] = v;
    }

    if (!virus_name.empty() && info.virus_name.empty())
        throw std::invalid_argument(
            "The model \"" + id + "\" names its own virus; it takes no "
            "virus name."
        );

    return info.build(ModelArgs(
        std::move(complete),
        virus_name.empty() ? info.virus_name : virus_name
    ));

}

namespace detail {

    inline ParamInfo registry_prob(
        std::string arg, std::string name, double value, std::string description
    )
    {
        return {std::move(arg), std::move(name), value, 0.0, 1.0, false,
            std::move(description)};
    }

    inline ParamInfo registry_days(
        std::string arg, std::string name, double value, std::string description
    )
    {
        return {std::move(arg), std::move(name), value, 0.0,
            std::numeric_limits< double >::infinity(), false,
            std::move(description)};
    }

    inline ParamInfo registry_n(double value)
    {
        return {"n", "", value, 1.0,
            static_cast< double >(std::numeric_limits< epiworld_fast_uint >::max()),
            true, "Number of agents."};
    }

    inline ParamInfo registry_prevalence(double value)
    {
        return registry_prob("prevalence", "", value,
            "Proportion of agents infected at the start.");
    }

}

/**
 * @brief The registry of the core models.
 * @ingroup epi_models
 *
 * @details SIS, SIR, SEIR, SISD, SIRD, SEIRD, SEIRQuarantine and SURV. Models
 * built from data (`ModelSIRLogit`, `ModelDiffNet`) take arrays and are built
 * directly. The defaults are those of the wrappers' examples.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline ModelRegistry<TSeq> model_registry()
{

    using namespace detail;
    using uint_t = epiworld_fast_uint;
    using int_t  = epiworld_fast_int;
    const double inf = std::numeric_limits< double >::infinity();

    const ParamInfo transmission = registry_prob(
        "transmission_prob", "Transmission probability", 0.05,
        "Probability that a contact with an infected agent infects."
    );
    const ParamInfo recovery = registry_prob(
        "recovery_rate", "Recovery rate", 0.14,
        "Daily probability that an infected agent recovers."
    );
    const ParamInfo death = registry_prob(
        "death_rate", "Death rate", 0.01,
        "Daily probability that an infected agent dies."
    );
    const ParamInfo latent = registry_days(
        "latent_period", "Latent period", 7.0,
        "Mean days in the exposed state (infected, not yet infectious)."
    );

    ModelRegistry<TSeq> r;

    r.add({"SIS", "Susceptible-Infected-Susceptible",
        "Infected agents become susceptible again when they recover.",
        "Disease",
        {registry_n(10000), registry_prevalence(0.01), transmission, recovery},
        {ModelSIS<TSeq>::INFECTED, ModelSIS<TSeq>::SUSCEPTIBLE,
            ModelSIS<TSeq>::SUSCEPTIBLE},
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelSIS<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("recovery_rate")
            );
        }});

    r.add({"SIR", "Susceptible-Infected-Recovered",
        "Infected agents recover with lasting immunity.",
        "Disease",
        {registry_n(10000), registry_prevalence(0.01), transmission, recovery},
        {1, 2, 2},
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelSIR<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("recovery_rate")
            );
        }});

    r.add({"SEIR", "Susceptible-Exposed-Infected-Recovered",
        "Infected agents go through a latent (exposed, not infectious) "
        "period before they become infectious.",
        "Disease",
        {registry_n(10000), registry_prevalence(0.01), transmission, latent,
            recovery},
        {ModelSEIR<TSeq>::EXPOSED, ModelSEIR<TSeq>::REMOVED,
            ModelSEIR<TSeq>::REMOVED},
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelSEIR<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("latent_period"), a("recovery_rate")
            );
        }});

    r.add({"SISD", "Susceptible-Infected-Susceptible-Deceased",
        "SIS, where infected agents may also die.",
        "Disease",
        {registry_n(10000), registry_prevalence(0.01), transmission, recovery,
            death},
        {1, 0, 2},
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelSISD<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("recovery_rate"), a("death_rate")
            );
        }});

    r.add({"SIRD", "Susceptible-Infected-Recovered-Deceased",
        "SIR, where infected agents may also die.",
        "Disease",
        {registry_n(10000), registry_prevalence(0.01), transmission, recovery,
            death},
        {1, 2, 3},
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelSIRD<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("recovery_rate"), a("death_rate")
            );
        }});

    r.add({"SEIRD", "Susceptible-Exposed-Infected-Recovered-Deceased",
        "SEIR, where infected agents may also die.",
        "Disease",
        {registry_n(10000), registry_prevalence(0.01), transmission, latent,
            recovery, death},
        {ModelSEIRD<TSeq>::EXPOSED, ModelSEIRD<TSeq>::REMOVED,
            ModelSEIRD<TSeq>::DECEASED},
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelSEIRD<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("latent_period"), a("recovery_rate"),
                a("death_rate")
            );
        }});

    r.add({"SEIRQuarantine",
        "SEIR with hospitalization, isolation and quarantine",
        "SEIR where detected cases are isolated and their contacts, traced "
        "through the layers that trace, quarantined.",
        "Disease",
        {
            registry_n(10000), registry_prevalence(0.01), transmission, latent,
            recovery,
            registry_prob("hospitalization_rate", "Hospitalization rate", 0.05,
                "Daily probability that an infected agent is hospitalized."),
            registry_days("hospitalization_period", "Hospitalization period",
                7.0, "Average number of days in the hospital."),
            {"detection_delay", "Detection delay", 3.0, -1.0, inf, false,
                "Mean days before an infected agent is detected (0: at once; "
                "negative: never)."},
            {"quarantine_period", "Quarantine period", 14.0, -1.0, inf, true,
                "Days in quarantine for the contacts of a detected case "
                "(negative: no quarantine)."},
            registry_prob("quarantine_willingness", "Quarantine willingness",
                0.8, "Probability that a contact complies with quarantine."),
            registry_prob("isolation_willingness", "Isolation willingness", 0.5,
                "Probability that a detected case complies with isolation."),
            {"isolation_period", "Isolation period", 7.0, -1.0, inf, true,
                "Days in isolation for a detected case (negative: no "
                "isolation)."},
            registry_prob("contact_tracing_prob", "Contact tracing probability",
                1.0, "Probability that a contact is traced."),
            {"contact_tracing_window", "Contact tracing window", 4.0, 0.0, inf,
                true, "Days before detection whose contacts are traced."}
        },
        {ModelSEIRQuarantine<TSeq>::EXPOSED,
            ModelSEIRQuarantine<TSeq>::RECOVERED,
            ModelSEIRQuarantine<TSeq>::RECOVERED},
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelSEIRQuarantine<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")), a("prevalence"),
                a("transmission_prob"), a("latent_period"), a("recovery_rate"),
                a("hospitalization_rate"), a("hospitalization_period"),
                a("detection_delay"),
                static_cast< int_t >(a("quarantine_period")),
                a("quarantine_willingness"), a("isolation_willingness"),
                static_cast< int_t >(a("isolation_period")),
                a("contact_tracing_prob"),
                static_cast< uint_t >(a("contact_tracing_window"))
            );
        }});

    r.add({"SURV", "Surveillance (SEIR with symptoms and vaccination)",
        "Latent agents become symptomatic or asymptomatic; surveillance "
        "detects and isolates cases, and a share of agents is vaccinated.",
        "Disease",
        {
            registry_n(10000),
            {"prevalence", "", 50.0, 0.0,
                static_cast< double >(std::numeric_limits< uint_t >::max()),
                true, "Number of agents infected at the start."},
            registry_prob("efficacy_vax", "Vax efficacy", 0.9,
                "Probability that the vaccine prevents infection."),
            registry_days("latent_period", "Latent period", 3.0,
                "Mean days in the latent state."),
            registry_days("infect_period", "Infect period", 6.0,
                "Mean days infectious."),
            registry_prob("prob_symptoms", "Prob of symptoms", 0.6,
                "Probability that an infected agent develops symptoms."),
            registry_prob("prop_vaccinated", "", 0.25,
                "Proportion of agents vaccinated."),
            registry_prob("prop_vax_redux_transm", "Vax redux transmission",
                0.5, "Factor by which the vaccine reduces transmission."),
            registry_prob("prop_vax_redux_infect", "", 0.5,
                "Factor by which the vaccine reduces infection (not used by "
                "the model)."),
            registry_prob("surveillance_prob", "Surveilance prob.", 0.001,
                "Daily probability that an agent is tested."),
            registry_prob("transmission_prob", "Prob of transmission", 1.0,
                "Probability that a contact with an infected agent infects."),
            registry_prob("prob_death", "Prob. death", 0.001,
                "Probability that an infected agent dies."),
            registry_prob("prob_noreinfect", "Prob. no reinfect", 0.9,
                "Probability that a recovered agent is not reinfected.")
        },
        {ModelSURV<TSeq>::LATENT, ModelSURV<TSeq>::RECOVERED,
            ModelSURV<TSeq>::REMOVED},
        [](const ModelArgs & a) -> std::unique_ptr< Model<TSeq> > {
            return std::make_unique< ModelSURV<TSeq> >(
                a.virus_name(), static_cast< uint_t >(a("n")),
                static_cast< uint_t >(a("prevalence")), a("efficacy_vax"),
                a("latent_period"), a("infect_period"), a("prob_symptoms"),
                a("prop_vaccinated"), a("prop_vax_redux_transm"),
                a("prop_vax_redux_infect"), a("surveillance_prob"),
                a("transmission_prob"), a("prob_death"), a("prob_noreinfect")
            );
        }});

    return r;

}

#endif
