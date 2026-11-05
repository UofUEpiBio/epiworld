#ifndef EPIWORLD_NATURALHISTORY_BONES_HPP
#define EPIWORLD_NATURALHISTORY_BONES_HPP

/**
 * @file naturalhistory-bones.hpp
 * @brief The natural history of a disease: its stages and the declared
 * transitions between them (see `natural-history.md`).
 *
 * @details A natural history is a small graph. Its nodes are **stages**
 * (susceptible, latent, infectious, recovered, ...) and its edges are
 * **transitions**, each of one kind:
 *
 * - `infection()`: by contact, drawn from the model's layers (or a custom
 *   sampler). Taking it gives the agent the virus.
 * - `rate()`: a daily probability. The rates out of a stage compete: one
 *   draw (`roulette()`) picks one of them, or none.
 * - `after()`: a duration, drawn when the agent enters the stage. It is a
 *   deadline: the agent leaves on the update of the day it falls, unless a
 *   rate fired first.
 * - `external()`: fired by code, through `Model::fire()`.
 *
 * A stage is a **carrier** (its agents hold the virus) and may be
 * **infectious** (they transmit; this implies carrier). Entering a
 * non-carrier stage releases the virus. There are no fixed roles: a stage is
 * susceptible because an infection transition leaves it, and a death is a
 * stage with contact weight 0 and no exits.
 *
 * Every number a transition uses is a `Value`: a number, a model parameter
 * (by name) or a function. A number given directly becomes a model parameter
 * when the natural history is set on a model.
 */

class Nodes;

template<typename TSeq>
class NaturalHistory;

template<typename TSeq>
class TransitionRef;

/// Flags of a stage (see `NaturalHistory::add_stage()`).
namespace stage {
    constexpr unsigned int carrier    = 1u;      ///< Agents in it hold the virus
    constexpr unsigned int infectious = 1u | 2u; ///< They transmit (implies carrier)
}

/// Which tool effect a rate takes (see `TransitionRef::effect()`).
enum class Effect : uint8_t {
    none,     ///< No tool effect
    recovery, ///< `p` becomes `1 - (1 - p) * (1 - recovery enhancer)`
    death     ///< `p` becomes `p * (1 - death reduction)`
};

/// One or more stages (an `int` or a list of them).
class Nodes {
public:
    std::vector< int > ids;
    Nodes(int id) : ids({id}) {}
    Nodes(std::initializer_list< int > l) : ids(l) {}
    Nodes(std::vector< int > v) : ids(std::move(v)) {}
};

template<typename TSeq = EPI_DEFAULT_TSEQ>
using ValueFun = std::function< epiworld_double(Agent<TSeq> &, Model<TSeq> &) >;

template<typename TSeq = EPI_DEFAULT_TSEQ>
using DurationFun = std::function< int(Agent<TSeq> &, Model<TSeq> &) >;

/// A custom infection sampler: the virus the agent catches, or `nullptr`.
template<typename TSeq = EPI_DEFAULT_TSEQ>
using InfectionSampler = std::function< Virus<TSeq> * (Agent<TSeq> &, Model<TSeq> &) >;

/**
 * @brief A number a transition uses: a constant, a model parameter, or a
 * function.
 * @details
 * - `Value(0.1)`: a number. When the natural history is set on a model, it
 *   becomes a model parameter (named after the transition).
 * - `Value("Recovery rate")`: a model parameter, by name.
 * - `Value::inverse("Days")`, `Value::complement("p")`: `1 / p`, `1 - p`.
 * - `Value(fun)`: `fun(agent, model)`.
 *
 * A parameter is read through the agent's virus, so a variant can point it to
 * another parameter (`Virus::use_param()`).
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class Value {
public:
    enum class Kind : uint8_t { none, number, param, inverse, complement, fun };

private:
    Kind kind = Kind::none;
    epiworld_double number = 0.0;
    ParamRef param = ParamRef("");
    ValueFun<TSeq> fun = nullptr;

    friend class NaturalHistory<TSeq>;

public:
    Value() = default;
    Value(epiworld_double x) : kind(Kind::number), number(x) {}
    Value(int x) : Value(static_cast< epiworld_double >(x)) {}
    Value(const char * name) : kind(Kind::param), param(std::string(name)) {}
    Value(std::string name) : kind(Kind::param), param(std::move(name)) {}
    Value(ValueFun<TSeq> f) : kind(Kind::fun), fun(std::move(f)) {}

    static Value inverse(std::string name);    ///< `1 / parameter`
    static Value complement(std::string name); ///< `1 - parameter`

    Kind get_kind() const { return kind; }
    bool is_set() const { return kind != Kind::none; }

    /// The parameter's name ("" for a function).
    const std::string & get_param_name() const { return param.name(); }

    /// The value for this agent (its virus may redirect the parameter).
    epiworld_double eval(Agent<TSeq> & agent, Model<TSeq> & model) const;

    /// The same, with the parameter redirected by `virus` (e.g., the
    /// infector's, for a probability of infection).
    epiworld_double eval(
        Agent<TSeq> & agent,
        const Virus<TSeq> * virus,
        Model<TSeq> & model
    ) const;
};

/**
 * @brief How many days a stage lasts, drawn when the agent enters it.
 * @details Always a whole number of days, at least one.
 * - `fixed(p)`: `p`, rounded.
 * - `geometric(mean)`: a geometric number of days with that mean (the same as
 *   leaving each day with probability `1 / mean`).
 * - `lognormal(mean, sd)`, `gamma(mean, sd)`: the mean and standard deviation
 *   of the duration, in days. A standard deviation of zero is fixed.
 * - `fun`: `fun(agent, model)`.
 *
 * `since(s)` counts the duration from the day the agent entered stage `s`
 * (in the same infection), instead of the day it entered this stage.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class Duration {
public:
    enum class Kind : uint8_t { none, fixed, geometric, lognormal, gamma, fun };

private:
    Kind kind = Kind::none;
    Value<TSeq> mean;
    Value<TSeq> sd;
    DurationFun<TSeq> fun = nullptr;
    int anchor = -1;

    friend class NaturalHistory<TSeq>;

public:
    Duration() = default;
    Duration(DurationFun<TSeq> f) : kind(Kind::fun), fun(std::move(f)) {}

    static Duration fixed(Value<TSeq> days);
    static Duration geometric(Value<TSeq> mean);
    static Duration lognormal(Value<TSeq> mean, Value<TSeq> sd);
    static Duration gamma(Value<TSeq> mean, Value<TSeq> sd);

    /// Counts from the day the agent entered stage `s` in this infection.
    Duration since(int s) const;

    Kind get_kind() const { return kind; }
    int get_anchor() const { return anchor; }

    /// Draws a number of days (at least one).
    int draw(Agent<TSeq> & agent, Model<TSeq> & model) const;
};

/// The kind of a transition (see `naturalhistory-bones.hpp`).
enum class TransitionKind : uint8_t { infection, rate, after, external };

/**
 * @brief A declared transition: from a set of stages to a stage (or a branch
 * of stages), of one kind.
 */
template<typename TSeq>
struct Transition {
    std::string name;
    TransitionKind kind;
    std::vector< int > from;
    int to = -1;

    /// A branch: the target is drawn among these, with these probabilities.
    std::vector< int > branch_to;
    std::vector< Value<TSeq> > branch_prob;

    Value<TSeq> value;          ///< infection (per contact) and rate
    Duration<TSeq> duration;    ///< after
    Effect effect = Effect::none;
    InfectionSampler<TSeq> sampler = nullptr; ///< infection only (optional)
};

/**
 * @brief Refines a transition just declared (chainable); converts to its id.
 */
template<typename TSeq>
class TransitionRef {
private:
    NaturalHistory<TSeq> * nh;
    int id;

public:
    TransitionRef(NaturalHistory<TSeq> * nh_, int id_) : nh(nh_), id(id_) {}
    operator int() const { return id; }
    int get_id() const { return id; }

    TransitionRef & name(std::string n);     ///< Default: "From -> To"
    TransitionRef & effect(Effect e);        ///< The tool effect a rate takes

    /// The target is drawn among `targets`, with their probabilities.
    TransitionRef & to(std::vector< std::pair< int, Value<TSeq> > > targets);

    /// A custom infection sampler (infection transitions only; C++ only).
    TransitionRef & sampler(InfectionSampler<TSeq> f);
};

/**
 * @brief What the engine needs about a stage, derived from the declarations.
 */
struct StageInfo {
    int infection = -1;           ///< The infection transition out of it
    std::vector< int > rates;     ///< Its rate transitions, in order
    int after = -1;               ///< Its duration transition
    bool needs_visit = false;     ///< Has a rate or a duration
    bool pushable = false;        ///< Its only drawn exit is a default infection
};

/**
 * @brief How a virus is seeded: how many agents, and in which carrier stages.
 * @details `Seed::proportion(0.01)` infects 1% of the agents, drawn among
 * those whose stage has an infection transition, into that transition's
 * target. `split` places them across carrier stages instead, by shares that
 * add up to 1 (e.g., `{{E, 0.7}, {I, 0.3}}`). Seeded agents enter on day 0
 * and draw their durations then. See `Model::add_virus()`.
 */
struct Seed {
    epiworld_double prevalence = 0.0;
    bool as_proportion = true;
    std::vector< std::pair< int, epiworld_double > > split;

    static Seed proportion(
        epiworld_double p,
        std::vector< std::pair< int, epiworld_double > > split = {}
    );

    static Seed count(
        size_t n,
        std::vector< std::pair< int, epiworld_double > > split = {}
    );
};

/**
 * @brief The natural history of a disease: stages and declared transitions.
 * @details Build it, then hand it to a model (`Model::set_natural_history()`).
 * The first stage is where agents start. Example (SIR):
 *
 * @code{.cpp}
 * NaturalHistory<> nh("Flu");
 * int S = nh.add_stage("Susceptible");
 * int I = nh.add_stage("Infected", stage::infectious);
 * int R = nh.add_stage("Recovered");
 * nh.infection(S, I, "Transmission rate");
 * nh.rate(I, R, "Recovery rate").effect(Effect::recovery);
 * @endcode
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class NaturalHistory {
private:
    std::string name;
    std::vector< std::string > stage_labels;
    std::vector< unsigned int > stage_flags;
    std::vector< epiworld_double > stage_contact;
    std::vector< Transition<TSeq> > transitions;

    // Derived by finalize()
    std::vector< StageInfo > stages;
    bool finalized = false;

    friend class TransitionRef<TSeq>;
    friend class Model<TSeq>;

    int add_transition(Transition<TSeq> t);
    void check_stage(int s, const char * what) const;

    /// Checks the declarations and derives `stages`; turns numbers into model
    /// parameters.
    void finalize(Model<TSeq> & model);

public:
    explicit NaturalHistory(std::string name_);

    /// Adds a stage; returns its id. Flags: `stage::carrier`, `stage::infectious`.
    int add_stage(std::string label, unsigned int flags = 0u);

    /// Contact weight of a stage (default 1). 0 takes its agents out of every
    /// layer: they are not met and do not transmit.
    void set_contact(int s, epiworld_double weight);

    TransitionRef<TSeq> infection(Nodes from, int to, Value<TSeq> per_contact);
    TransitionRef<TSeq> rate(Nodes from, int to, Value<TSeq> daily_probability);
    TransitionRef<TSeq> after(int from, int to, Duration<TSeq> d);
    TransitionRef<TSeq> external(Nodes from, int to, std::string label);

    const std::string & get_name() const { return name; }
    size_t get_n_stages() const { return stage_labels.size(); }
    size_t get_n_transitions() const { return transitions.size(); }
    const std::vector< std::string > & get_stages() const { return stage_labels; }
    const Transition<TSeq> & get_transition(int id) const { return transitions.at(id); }

    bool is_carrier(int s) const { return (stage_flags.at(s) & stage::carrier) != 0u; }
    bool is_infectious(int s) const { return (stage_flags.at(s) & 2u) != 0u; }
    epiworld_double get_contact(int s) const { return stage_contact.at(s); }

    /// The stage's id, by label (throws if absent).
    int stage_id(std::string_view label) const;

    /// The transition's id, by name (throws if absent).
    int transition_id(std::string_view label) const;

    /// Whether `s` is one of the transition's sources.
    bool is_source(int transition, int s) const;

    /// What the engine derived about stage `s` (after `set_natural_history()`).
    const StageInfo & get_stage_info(int s) const { return stages.at(s); }
};

#endif
