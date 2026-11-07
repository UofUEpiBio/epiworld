A model is a `Model` with a population, parameters, and a virus whose natural history says how the disease progresses: its stages and the transitions between them. A model can be built in a function, or as a class that inherits from `Model` and declares everything in its constructor; all the models included in `epiworld` are classes of the second kind.

## Declaring a natural history

The stages and transitions are declared on the virus (see `NaturalHistory`). The first stage is where agents start; a stage flagged `stage::carrier` carries the virus, and one flagged `stage::infectious` also transmits it. The transitions are of four kinds:

| Declaration | Moves an agent |
|---|---|
| `infection(from, to, per_contact)` | after a contact with an infectious agent, with that probability per contact |
| `rate(from, to, daily)` | with that daily probability (several rates out of a stage compete) |
| `after(from, to, duration)` | when a duration, drawn on entering the stage, ends: `Duration::fixed()`, `geometric()`, `lognormal()`, `gamma()`, or a function; `.since(s)` counts from the entry into stage `s` |
| `external(from, to, name)` | when code fires it (`Model::fire()`), e.g., from a global event or a reaction |

A value is a number, the name of a model parameter, `Value::inverse(name)` (`1 / parameter`), `Value::complement(name)` (`1 - parameter`), or a function of the agent and the model. A number becomes a model parameter named after the virus and the transition, so every value can be read and changed with `set_param()`. A transition can be named (`.name()`), can take a tool effect (`.effect(Effect::recovery)`), can branch to several targets with probabilities, and an infection can use its own sampler (`.sampler()`).

Agents can also have a status, such as free, quarantined or isolated (`Statuses`, set with `Model::set_statuses()`), with transitions of their own (`rate()`, `after()`, `external()`); a status transition can be restricted to agents in some stages (`.when()`). Reactions (`Model::react()`) respond to agents entering a stage or a status, for example by quarantining their contacts.

## Example: the ModelSIS class

The SIS (Susceptible-Infected-Susceptible) model is one of the simplest models included in epiworld:

```cpp
class ModelSIS : public Model
{

public:

    static constexpr int SUSCEPTIBLE = 0;
    static constexpr int INFECTED    = 1;

    ModelSIS() = delete;

    ModelSIS(
        const std::string & vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double transmission_prob,
        epiworld_double recovery_rate
    );

};

inline ModelSIS::ModelSIS(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate
    )
{

    this->set_population_size(n);

    this->set_name("Susceptible-Infected-Susceptible (SIS)");

    this->add_param(transmission_prob, "Transmission probability");
    this->add_param(recovery_rate, "Recovery rate");

    Virus virus(vname);
    virus.add_stage("Susceptible");
    virus.add_stage("Infected", stage::infectious);
    virus.infection(SUSCEPTIBLE, INFECTED, "Transmission probability");
    virus.rate(INFECTED, SUSCEPTIBLE, "Recovery rate").effect(Effect::recovery);

    this->add_virus(virus, assign::share(prevalence));

}
```

`add_virus()` gives the model the virus's stages, and `assign::share(prevalence)` places the virus on that share of the agents at the start of each run (see `assign.hpp` for the other forms). Users then add the contacts and run:

```cpp
#include "epiworld.hpp"

using namespace epiworld;

int main() {

    epimodels::ModelSIS flumodel("Flu", 10000, 0.001, 0.1, 0.14);

    flumodel.add_layer(layer::network(
        "contacts", graph::smallworld(20, 0.01)
    ));

    flumodel.run(100, 123);
    flumodel.print();

}
```

`flumodel.draw_natural_history()` returns the model's diagram, drawn from the declarations (a mermaid flowchart). The diagrams of the included models (`include/epiworld/models/<model>.mmd`) are drawn this way by `make diagrams`.

## Execution workflow: main calls

The `Model::run()` method calls, for each day:

```cpp
// Resets the entire system: the layers, then the viruses
// and tools
reset();

// Records the baseline (day 0) and advances to day 1
next();

for (int niter = 0; niter < get_ndays(); ++niter)
{

    // The natural history, the statuses, then the reactions
    this->update_state();     /// <<< events_run() >>>

    // The global events
    this->run_globalevents(); /// <<< events_run() >>>

    // Updates the layers (e.g., rewiring), records the day in the
    // database, and advances the day (step)
    this->next();

    // Mutation happens at the very end
    this->mutate_virus();

}
```

## Execution workflow: events

Changes to agents during a day (adding or removing tools and viruses, moving to another stage or status) are queued as events, which `Model::events_run()` applies after `update_state()` and after `run_globalevents()`. Events are applied in the order they were queued. An event from a declared transition applies only if the agent is still in one of the transition's sources, so if an agent's first event of the pass moved it, a later event from its old stage is skipped. A global event can therefore change the model on the fly: for example, `InterventionMeaslesPEP` (a `GlobalEvent` in the measles models) fires the transitions that take agents out of quarantine when post-exposure prophylaxis is given and effective.

## See Also

- [Library Architecture](library-architecture.md) — the `Agent`/`Model` class hierarchy that new models inherit from.
- [Extending the Library](extending-the-library.md) — defining custom agent behaviors, network structures, and global events.
- [Core Simulation Models](core-simulation-models.md) — the full catalog of pre-built models.
- [Events and Multi-Event Handling](events-and-multi-event-handling.md) — the event queue used during model execution.
