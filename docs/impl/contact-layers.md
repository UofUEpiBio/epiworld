## Overview

A **contact layer** is one source of contacts. It answers one question: whom did agent $i$ meet today? A model can have any number of layers, and it has none by default: a model without layers has no transmission through contacts. Each layer is one setting, such as a household, a workplace or a public space, and each can be a different kind:

| Kind | Created with | Contacts of an agent |
|---|---|---|
| Network | `layer::network(name, graph)` | its ties, fixed or redrawn every run |
| Mixing across groups | `layer::mixing(name)` + `set_contact_matrix()` | a number of agents of each group, drawn every day from a contact matrix |
| Mixing within groups | `layer::mixing(name)` + `set_contact_prob()` | each member of its own group, with a probability per pair |
| Your own | a class derived from `ContactLayer` | whatever `sample()` returns |

All the layers of a model feed **one draw** per susceptible agent, so a person exposed at home and at work has one chance of infection that accounts for both. The transmission records the layer it came through.

```cpp
#include "epiworld/epiworld.hpp"
using namespace epiworld;

int main()
{
    const size_t n = 20000u;
    epimodels::ModelSEIR model("flu", n, 0.001, 0.02, 4.0, 1.0 / 7.0);

    // Households of four: each member meets each housemate with probability
    // 0.5 a day
    std::vector< size_t > household(n);
    for (size_t i = 0u; i < n; ++i)
        household[i] = i / 4u;

    auto home = layer::mixing("household");
    home->set_groups(assign::labels(household), n / 4u);
    home->set_contact_prob(0.5);
    model.add_layer(home);

    // Workplaces: a small-world network, redrawn at every run
    model.add_layer(layer::network("work", graph::smallworld(10, 0.05)));

    // Public spaces: three age groups and a contact matrix, scaled by a
    // parameter that a global event or a calibration can change
    auto public_spaces = layer::mixing("public");
    public_spaces->set_groups(assign::shares({{0, 0.25}, {1, 0.5}, {2, 0.25}}), 3u);
    public_spaces->set_contact_matrix({
        0.6, 0.3, 0.1,
        0.2, 0.6, 0.2,
        0.1, 0.4, 0.5
    });
    model.add_param(4.0, "Public contact rate");
    public_spaces->set_contact_rate("Public contact rate");
    model.add_layer(public_spaces);

    model.run(120, 2131);
    model.print();
}
```

`examples/23-layers` is this model, with a count of the transmissions by layer. `examples/24-transit` adds schedules and persistence, and `examples/25-ergm-rewirer` a network that changes every day.

## One draw over all layers

Every contact that carries a virus in an infectious stage is an exposure with probability

$$
p = \left(1 - \text{susceptibility reduction}\right) \times p_v \times \left(1 - \text{transmission reduction}\right).
$$

The exposures are independent, across contacts and across layers. The agent escapes all of them with probability $\prod (1 - p)$. If it is infected, one infector is named, in proportion to its hazard $-\log(1 - p)$. [Combining Exposures](combining-exposures.md) explains the choice, and [Push and Pull Transmission](transmission-sampling.md) how the draw is sampled.

Each layer adjusts its own exposures:

- **Weight.** `set_weight(param)` makes a model parameter the layer's weight. It multiplies the hazard of each contact, so a contact of probability $p$ counts as $1 - (1 - p)^w$. A weight of 2 is two independent exposures, and 0 switches transmission through the layer off, while its contacts still happen and are logged. A layer without a weight has weight 1. The parameter is read every day, so a global event, or a cycle (`Model::set_param_cycle()`), can change it.
- **Diseases.** `add_target(disease)` restricts the layer to some diseases, by name. Without targets, every disease spreads through every layer.
- **Tools.** A tool's contact effects (its susceptibility and transmission reductions) act in every layer, unless `Tool::add_layer_target(layer)` restricts them to some, like a mask worn at work and school but not at home. Its recovery and death effects are not about contacts, so layer targets leave them alone.

`get_transmissions()` returns the layer of each transmission (its id, the position in `add_layer()` order; -1 for an infection not drawn from a layer, such as the seeded ones).

## Who takes part

- **Statuses out of a layer.** `exclude_status(status)` takes the agents in a status out of this layer only: the school layer excludes "Quarantined", while the household layer keeps them. An agent out of a layer neither samples it nor is sampled through it. A mixing layer then counts only the agents present: its contacts per day stay the same, and within groups, so does the probability per pair. A stage or status of contact weight 0 (`set_contact()` in its natural history) is out of every layer. See [Natural History and Statuses](natural-history-and-statuses.md).
- **Closing.** `close()` takes the whole layer out, as in a school closure, until `reopen()`. A closed layer samples no contacts, so it logs none and transmits nothing. Closing during a run takes effect the next day and lasts that run only.
- **Schedules.** `set_schedule()` makes a layer sample contacts on some days only, from a cycle (five `true` and two `false` for weekdays) or a function of the day. A day off the schedule is a closed day. The schedule is a setting, made between runs, while `close()` is the intervention a global event makes during one. `is_closed()` says whether the layer has contacts today, for either reason.

Mixing layers have more options: groups drawn at every run, persistence (an agent's usual group, or another of the same pool), and assignments from data. [Mixing by Group](mixing-and-entity-distribution.md) covers them.

## Layers and runs

**Ownership.** Layers are shared pointers. `add_layer()` shares the pointer, so the model and whoever else holds it see the same layer, and an edit through either is an edit of the model. `model.get_layer(name)` returns the layer, and `model.get_layer_as< layer::Mixing >(name)` returns it as its kind. A copy of the model, including each replicate of `run_multiple()`, clones its layers, so replicates never share layer state.

**Edits.** Whether an edit is permanent depends only on when it is made. An edit made during a run, from an update function or a global event, applies to that run only: the layer is marked *dirty*, and the next run starts from the baseline again. An edit made between runs becomes the new baseline, so a calibration can reuse one model. Settings, like schedules, excluded statuses and tracing, are made between runs.

**The day.** At the start of each run, the layers are reset (a fixed network restores its ties, a generated one is redrawn, groups are drawn) before viruses and tools are placed, so the placements can follow the layers (`assign::group_shares()`). At the end of each day, after the agents' updates and the global events, the layers are updated, so whatever changed during a day applies to the next day's contacts.

## Contact tracing

`enable_tracing(max_contacts)` makes a layer keep a log of the contacts it samples: each contact with a carrier is recorded under the carrier, with the day, keeping the last `max_contacts` per agent. `get_tracing()` returns this run's log. A traced mixing layer within groups also keeps the history of moves and absences, and `groupmates_of(agent, d1, d2, out)` lists who shared a group with an agent between two days, by membership rather than by sampled contact. See [Quarantine, Isolation, and Contact Tracing](quarantine-isolation-and-contact-tracing.md).

## Push, pull and the queue

A step can be **pulled** (each susceptible agent asks every layer for its contacts) or **pushed** (each carrier asks every layer whom it reaches). The two give the same distribution of outcomes, and the model picks the cheaper one each day (see [Push and Pull Transmission](transmission-sampling.md)). A step pushes or pulls every layer, so it pushes only if every layer can push. Separately, the model's queue skips agents that cannot change, but only if every layer lets it see whom a carrier can reach (see [Queueing System](queueing-system.md)).

| Kind | Push | Queue |
|---|---|---|
| Undirected network | yes | yes |
| Directed network | no (a tie is stored by its source only) | no |
| Mixing within groups | yes | yes |
| Mixing across groups (contact matrix) | yes | no (every group can reach every other) |
| Your own | if it overrides `supports_push()` and `sample_reverse()` | if it overrides `supports_queue()` |

A layer that cannot push or use the queue still works: the model then pulls, or visits every agent.

## A layer of your own

Derive from `ContactLayer` and implement four functions:

- `sample(agent, model, exclude, out)` appends the contacts of `agent` today that are **sources**: they carry a virus, and their state is not flagged in `exclude`. `is_source(contact, exclude)` applies both checks. It reports each contact to the post-sampling callback, if the model has one, and to the layer's log (`trace()`).
- `reset(model)` restores the baseline if the layer is dirty (`is_dirty()`), sizes its buffers, and calls `mark_clean()`.
- `update(model)` rebuilds what depends on the population, at the end of each day.
- `clone()` returns a deep copy.

A layer whose state can be edited calls `edits_baseline()` from every function that edits it; that is what makes an edit during a run last that run only. Here is a layer where each agent meets $k$ agents drawn at random every day (`tests/60a-custom-layer.cpp` runs it):

```cpp
class RandomEncounters : public ContactLayer {
private:
    size_t k;

public:
    RandomEncounters(std::string name, size_t k_) :
        ContactLayer(std::move(name)), k(k_) {}

    // The layer keeps nothing across days: nothing to restore
    void reset(Model &) override { mark_clean(); }
    void update(Model &) override {}

    size_t sample(
        size_t agent,
        Model & model,
        const std::vector< bool > * exclude,
        std::vector< size_t > & out
    ) override
    {
        const size_t n_agents = model.size();
        size_t n = 0u;
        for (size_t draw = 0u; draw < k; ++draw)
        {
            const size_t j = std::min(
                static_cast< size_t >(model.runif() * n_agents), n_agents - 1u
            );
            if ((j == agent) || !is_source(j, exclude))
                continue;
            if (model.has_post_sampling())
                model.register_sampled_contact(j, agent);
            trace(j, agent);
            out.push_back(j);
            ++n;
        }
        return n;
    }

    std::shared_ptr< ContactLayer > clone() const override
    {
        return std::make_shared< RandomEncounters >(*this);
    }
};

// ...
model.add_layer(std::make_shared< RandomEncounters >("encounters", 8u));
```

Weights, disease targets, excluded statuses, closing, schedules and tracing come with the base class. To let the model push, override `supports_push()` to return `true`, and `sample_reverse(source, model, out)` to append the agents that `source` meets, each with the probability that its own `sample()` would meet `source`, independently. (`RandomEncounters` is not symmetric: each agent draws its own encounters, so it pulls.)

## See Also

- [Mixing by Group](mixing-and-entity-distribution.md) — contact matrices, groups within groups, persistence, schedules and parameter cycles.
- [Push and Pull Transmission](transmission-sampling.md) — how one draw over all layers is sampled from either side.
- [Combining Exposures](combining-exposures.md) — why exposures are independent.
- [Queueing System](queueing-system.md) — which agents the model visits each day.
- [Migrating to 1.0](migrating-to-1.0.md) — from the networks, entities and mixing models of 0.x to layers.
