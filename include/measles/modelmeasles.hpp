#ifndef MEASLES_MODELMEASLES_HPP
#define MEASLES_MODELMEASLES_HPP

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "samplermixing.hpp"

using namespace epiworld;

/**
 * @brief What the measles models share: the disease, the statuses, how the
 * two are linked, and the contact sampler.
 *
 * @details Each agent has a stage of the disease (the virus's natural
 * history) and a status (the model's):
 *
 * | Stages      | Exits                                                       |
 * |-------------|-------------------------------------------------------------|
 * | Susceptible | infected by a contact (the model's sampler), to Latent      |
 * | Latent      | after "Incubation period" (lognormal, sd "Incubation period sd") |
 * | Prodromal   | after "Prodromal period" (lognormal), to Rash; infectious   |
 * | Rash        | to Severe at the daily "Hospitalization rate", else to Recovered after "Rash period" (lognormal); infectious at the reduced contact rate |
 * | Severe      | to Recovered at the daily rate 1 / "Hospitalization period" |
 * | Recovered   |                                                             |
 *
 * Severe stops the rash clock. Latent and Prodromal also have an external
 * transition, "PEP cleared", to Recovered (see `InterventionMeaslesPEP`).
 *
 * The statuses are Free, Quarantined, Isolated and Hospitalized; the last
 * three take the agent out of the contacts. The model's rules move agents
 * between them (fire()), together with these declared ones:
 *
 * - "Quarantined" (Free to Quarantined), which ends after the quarantine
 *   period ("Quarantine ends").
 * - "Isolated" (Free to Isolated), which ends "Isolation period" days after
 *   the rash onset ("Isolation ends").
 * - Optionally, "Detected" (Free to Isolated): a free agent with rash at the
 *   start of the day is detected that day with probability 1 / "Days
 *   undetected" (from the day after its rash onset), if "Isolation period" is
 *   not negative.
 * - An agent quarantined at the start of the day of its rash onset is
 *   isolated that day ("Onset in quarantine").
 * - Entering Severe admits the agent ("Admitted", recorded as a
 *   hospitalization), and leaving it discharges the agent ("Discharged").
 * - "PEP release" (Quarantined to Free), for post-exposure prophylaxis.
 *
 * The lognormal durations are rounded to whole days, with a minimum of one; a
 * standard deviation of zero gives a fixed duration.
 *
 * @ingroup disease_specific
 */
class ModelMeasles : public Model
{
protected:

    SamplerMixing sampler;

    std::vector< bool > quarantine_willingness; ///< Drawn at reset (some models)
    std::vector< bool > isolation_willingness;  ///< Drawn at reset (some models)

    /// Tools that exempt a traced contact from quarantine (by name).
    std::vector< std::string > quarantine_exempt_tools = {"MMR", "PEP MMR"};

    /// Whether the agent holds one of the quarantine-exempt tools.
    bool is_quarantine_exempt(const Agent & agent) const;

    /**
     * @brief Builds the disease, the statuses and their links.
     * @param seed Where the initial cases go.
     * @param quarantine_period The quarantine's duration.
     * @param reduction_param The parameter reducing the contacts of agents with
     * rash ("" for none: rash is not infectious).
     * @param detected Whether free agents with rash are detected (and
     * isolated) at the daily rate 1 / "Days undetected" ("Detected").
     */
    void measles_build(
        Assign seed,
        Duration quarantine_period,
        std::string reduction_param,
        bool detected
    );

    /// Draws who is willing to quarantine and to isolate.
    void draw_willingness();

    /// The infection of a free susceptible agent by its sampled contacts.
    Virus * infect(Agent & p);

    /// Checks the standard deviations of the stage durations.
    static void check_stage_sds(
        epiworld_double incubation_period_sd,
        epiworld_double prodromal_period_sd,
        epiworld_double rash_period_sd,
        const char * model_name
    );

public:

    // Stages
    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int LATENT      = 1;
    static constexpr int PRODROMAL   = 2;
    static constexpr int RASH        = 3;
    static constexpr int SEVERE      = 4;
    static constexpr int RECOVERED   = 5;

    // Statuses
    static constexpr int FREE         = 0;
    static constexpr int QUARANTINED  = 1;
    static constexpr int ISOLATED     = 2;
    static constexpr int HOSPITALIZED = 3;

    /// Who is willing to quarantine (models that draw it).
    std::vector< bool > get_quarantine_willingness() const
    {
        return quarantine_willingness;
    }

    /// Who is willing to isolate (models that draw it).
    std::vector< bool > get_isolation_willingness() const
    {
        return isolation_willingness;
    }

    /**
     * @name Who is exempt from quarantine
     * @brief The tools, by name, whose holders are not quarantined.
     * @details An agent holding any of these tools is not quarantined when
     * traced or when their group is exposed, whether or not the tool
     * protects them: documented MMR vaccination, routine or as
     * post-exposure prophylaxis, is presumptive evidence of immunity (CDC),
     * while immunoglobulin is not. The default is `{"MMR", "PEP MMR"}`: the
     * models' vaccine and `InterventionMeaslesPEP`'s MMR. A name that no
     * tool has exempts no one.
     */
    ///@{
    void set_quarantine_exempt_tools(std::vector< std::string > names)
    {
        quarantine_exempt_tools = std::move(names);
    }

    const std::vector< std::string > & get_quarantine_exempt_tools() const
    {
        return quarantine_exempt_tools;
    }
    ///@}

    #ifdef EPI_DEBUG
    const std::vector< int > & get_sampled_sizes() const
    {
        return sampler.get_sampled_sizes();
    }
    #endif

    void next() override
    {
        sampler.update(*this);
        Model::next();
    }

};

inline void ModelMeasles::measles_build(
    Assign seed,
    Duration quarantine_period,
    std::string reduction_param,
    bool detected
)
{

    // The disease
    Virus virus("Measles");
    virus.add_stage("Susceptible");
    virus.add_stage("Latent", stage::carrier);
    virus.add_stage("Prodromal", stage::infectious);
    virus.add_stage("Rash", stage::infectious);
    virus.add_stage("Severe", stage::carrier);
    virus.add_stage("Recovered");

    virus.infection(SUSCEPTIBLE, LATENT, "Transmission rate").sampler(
        [](Agent & p, Model & m) -> Virus * {
            auto * mm = dynamic_cast< ModelMeasles * >(&m);
            if (mm == nullptr)
                throw std::logic_error(
                    "The measles virus samples its contacts with a measles "
                    "model's sampler; it cannot be used in another model."
                );
            return mm->infect(p);
        }
    );
    virus.after(LATENT, PRODROMAL, Duration::lognormal(
        "Incubation period", "Incubation period sd"
    ));
    virus.after(PRODROMAL, RASH, Duration::lognormal(
        "Prodromal period", "Prodromal period sd"
    ));
    virus.rate(RASH, SEVERE, "Hospitalization rate");
    virus.after(RASH, RECOVERED, Duration::lognormal(
        "Rash period", "Rash period sd"
    ));
    const int out_severe = virus.rate(
        SEVERE, RECOVERED, Value::inverse("Hospitalization period")
    );
    virus.external({LATENT, PRODROMAL}, RECOVERED, "PEP cleared");
    this->add_virus(virus, std::move(seed));

    // Where agents are
    Statuses s;
    s.add_status("Free");
    s.add_status("Quarantined");
    s.add_status("Isolated");
    s.add_status("Hospitalized");
    s.set_contact(QUARANTINED, 0.0);
    s.set_contact(ISOLATED, 0.0);
    s.set_contact(HOSPITALIZED, 0.0);

    s.external(FREE, QUARANTINED, "Quarantined");
    s.after(QUARANTINED, FREE, std::move(quarantine_period))
        .name("Quarantine ends");
    s.external(FREE, ISOLATED, "Isolated");
    if (detected)
        s.rate(FREE, ISOLATED, ValueFun(
            [](Agent &, Model & m) -> epiworld_double {
                if (m.par("Isolation period") < 0)
                    return 0.0;
                const epiworld_double days = m.par("Days undetected");
                return (days <= 1.0) ? 1.0 : (1.0 / days);
            }
        )).when({RASH}).name("Detected");
    const int onset_q = s.external(
        {FREE, QUARANTINED}, ISOLATED, "Onset in quarantine"
    );
    s.after(
        ISOLATED, FREE,
        Duration::fixed("Isolation period").since(RASH)
    ).name("Isolation ends");
    const int admit = s.external(
        {FREE, QUARANTINED, ISOLATED}, HOSPITALIZED, "Admitted"
    );
    const int discharge = s.external(HOSPITALIZED, FREE, "Discharged");
    s.external(QUARANTINED, FREE, "PEP release");
    this->set_statuses(std::move(s));

    // How the two are linked
    this->react(Trigger::entered(RASH).was({QUARANTINED}), onset_q);
    this->react(Trigger::entered(SEVERE), admit);
    this->react(
        Trigger::entered(SEVERE),
        [](Agent & p, Model & m) { m.record_hospitalization(p); }
    );
    this->react(Trigger::fired(out_severe), discharge);

    // Free agents meet the prodromal ones, and those with rash at the reduced
    // rate; every susceptible agent samples its contacts every day
    sampler.configure(
        PRODROMAL, RASH, std::move(reduction_param),
        {SUSCEPTIBLE, LATENT, PRODROMAL, RECOVERED}, FREE
    );
    this->queuing_off();

}

inline void ModelMeasles::draw_willingness()
{
    quarantine_willingness.assign(this->size(), false);
    isolation_willingness.assign(this->size(), false);
    for (size_t idx = 0; idx < this->size(); ++idx)
    {
        quarantine_willingness[idx] =
            this->runif() < EPI_PAR(this, "Quarantine willingness");
        isolation_willingness[idx] =
            this->runif() < EPI_PAR(this, "Isolation willingness");
    }
}

inline bool ModelMeasles::is_quarantine_exempt(const Agent & agent) const
{
    for (const auto & name : quarantine_exempt_tools)
        if (agent.has_tool(std::string_view(name)))
            return true;

    return false;
}

inline Virus * ModelMeasles::infect(Agent & p)
{

    // An immune agent cannot be infected: no contacts drawn
    if (p.get_susceptibility_reduction(this->viruses[0u], *this) >= 1.0)
        return nullptr;

    const size_t n = sampler.sample(&p, *this);

    #ifdef EPI_DEBUG
    sampler.record_sampled_size(n);
    #endif

    if (n == 0u)
        return nullptr;

    return this->sample_virus(&p, sampler.get_sampled_agents().data(), n);

}

inline void ModelMeasles::check_stage_sds(
    epiworld_double incubation_period_sd,
    epiworld_double prodromal_period_sd,
    epiworld_double rash_period_sd,
    const char * model_name
)
{
    auto max_double = std::numeric_limits< double >::max();
    EpiAssert::check_bounds(incubation_period_sd, 0.0, max_double, "incubation_period_sd", model_name);
    EpiAssert::check_bounds(prodromal_period_sd, 0.0, max_double, "prodromal_period_sd", model_name);
    EpiAssert::check_bounds(rash_period_sd, 0.0, max_double, "rash_period_sd", model_name);
}

#endif
