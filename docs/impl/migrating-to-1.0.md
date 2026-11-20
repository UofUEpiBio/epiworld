## Overview

epiworld 1.0.0 changes how a model gets its contacts, how a disease runs its course, and how exposures combine. Code written for 0.x needs changes, and **results change** too: the main reason is that exposures are now independent. This page lists the changes, with the 0.x code next to its 1.0 equivalent. The R, Python and JavaScript packages follow the same changes; their NEWS files list the functions they renamed.

In short:

1. **No template parameter.** `Model<>`, `Virus<>`, `Tool<>` and `epimodels::ModelSIR<>` are `Model`, `Virus`, `Tool` and `epimodels::ModelSIR`.
2. **Contacts come from layers.** A model has no network of its own. Networks, groups and mixing are [contact layers](contact-layers.md), added with `add_layer()`.
3. **The population size goes to the constructor.**
4. **A virus carries its natural history.** Stages and their transitions replace states and update functions; model statuses (quarantined, isolated, ...) are separate from the stages.
5. **Placement uses one vocabulary**, `assign::`, for viruses, tools and groups.
6. **Exposures are independent.** An agent with several infectious contacts in a day is more likely to be infected than in 0.x.

## Networks are layers

```cpp
// 0.x
epimodels::ModelSIR<> model("flu", 0.01, 0.1, 0.3);
model.agents_smallworld(100000, 10L, false, .01);

// 1.0
epimodels::ModelSIR model("flu", 100000, 0.01, 0.1, 0.3);
model.add_layer(layer::network("contacts", graph::smallworld(10, .01)));
```

`graph::smallworld()` returns a *generator*: the layer draws a new network at every run, and each replicate of `run_multiple()` gets its own. To keep one network for every run, pass the graph itself (an `AdjList`).

| 0.x (`Model`) | 1.0 |
|---|---|
| `agents_smallworld(n, k, d, p)` | `add_layer(layer::network(name, graph::smallworld(k, p)))` |
| `agents_from_edgelist(source, target, n, directed)` | `add_layer(layer::network(name, AdjList(source, target, n, directed)))` |
| `agents_from_adjlist(adjlist)` | `add_layer(layer::network(name, adjlist))` |
| `agents_bernoulli(...)`, `agents_sbm(...)` | `graph::bernoulli(...)`, `graph::sbm(...)` |
| `add_edge()`, `rm_edge()`, `has_edge()`, `write_edgelist()` | the same methods on the layer: `model.get_layer_as< layer::Network >(name)` |
| `set_rewire_fun()`, `set_rewire_prop()`, `rewire()` | `set_rewiring(graph::rewire_degseq(param))` on the layer |
| `is_directed()` | `is_directed()` on the layer |

A model can have several networks (home, work, school), and mixing layers next to them. All of them feed one draw per agent.

## Removed models

The models that existed only to pair a disease with a way of mixing are gone: the disease model takes a layer instead.

| 0.x | 1.0 |
|---|---|
| `ModelSIRCONN`, `ModelSEIRCONN`, `ModelSIRDCONN`, `ModelSEIRDCONN` (random mixing at a contact rate) | `ModelSIR`, `ModelSEIR`, `ModelSIRD`, `ModelSEIRD`, plus a mixing layer with one group and a 1 × 1 contact matrix (the contact rate) |
| `ModelSIRMixing`, `ModelSEIRMixing` (entities and a contact matrix) | `ModelSIR`, `ModelSEIR`, plus a mixing layer with the groups and the matrix |
| `ModelSEIRMixingQuarantine`, `ModelSEIRNetworkQuarantine` | `ModelSEIRQuarantine`, on any layers |

```cpp
// 0.x: ModelSIRCONN with 10 contacts a day
epimodels::ModelSIRCONN<> model("flu", 10000, 0.01, 10.0, 0.1, 0.3);

// 1.0
epimodels::ModelSIR model("flu", 10000, 0.01, 0.1, 0.3);
auto mixing = layer::mixing("mixing");
mixing->set_groups(assign::labels(std::vector< size_t >(10000, 0u)), 1u);
mixing->set_contact_matrix({10.0});
model.add_layer(mixing);
```

The mixing layer draws *distinct* contacts: an agent meets each infectious agent at most once a day.

## Entities are groups of a mixing layer

`Entity`, `add_entity()` and the entity distribution functions are gone. Groups live in a mixing layer, which owns the grouping, fixed or drawn at every run:

```cpp
auto mixing = layer::mixing("mixing");
mixing->set_groups(assign::labels(groups), 3u);            // fixed
// or: mixing->set_groups(grouping::random({2000, 3000, 1000}));  // drawn every run
mixing->set_contact_matrix(contact_matrix);
model.add_layer(mixing);
```

Tools and viruses are placed by group with `assign::group_shares("mixing", shares)` or `assign::group_counts()`. The contact-matrix accessors moved from the model to the layer (`model.get_layer_as< layer::Mixing >("mixing").set_contact_matrix()`). [Mixing by Group](mixing-and-entity-distribution.md) has the details, including groups that meet only inside (households, classrooms).

## Natural history instead of states

In 0.x a model listed its states with `add_state()`, each with an update function, and a virus said which state it led to (`set_state()`). In 1.0 the virus declares its course: stages, how agents enter them (an infection), and how they leave them (rates or durations, with effects such as recovery).

```cpp
// 0.x
Model<> model;
model.add_state("Susceptible", default_update_susceptible<>);
model.add_state("Infected", default_update_exposed<>);
model.add_state("Recovered");

Virus<> virus("covid 19", 100, false);
virus.set_prob_infecting(.10);
virus.set_prob_recovery(.30);
virus.set_state(1, 2);
model.add_virus(virus);

// 1.0
Model model(100000);

Virus virus("covid 19");
virus.add_stage("Susceptible");
virus.add_stage("Infected", stage::infectious);
virus.add_stage("Recovered");
virus.infection(0, 1, 0.10);
virus.rate(1, 2, 0.30).effect(Effect::recovery);

model.add_virus(virus, assign::count(100));
```

Model statuses (Free, Quarantined, Isolated, Hospitalized, ...) are separate from the stages, and the outputs report both. A model can hold several diseases. `get_agents_states()` is now `get_agents_stages()` and `get_agents_statuses()`. See [Natural History and Statuses](natural-history-and-statuses.md).

## Placement

| 0.x | 1.0 |
|---|---|
| `Virus<>("x", prevalence, as_proportion)`, `set_distribution(distribute_virus_randomly(...))` | `model.add_virus(virus, assign::share(p))` or `assign::count(n)` |
| `distribute_virus_to_set(ids)`, `distribute_tool_to_set(ids)` | `assign::agents(ids)` |
| by entity | `assign::group_shares(layer, shares)`, `assign::group_counts(layer, counts)` |
| a stage per agent | `assign::labels(labels)`, `assign::shares()`, `assign::counts()` |

## Contact tracing

The model no longer keeps one contact log. Each layer keeps its own: `layer.enable_tracing(max_contacts)` replaces `Model::contact_tracing_on()`, and `layer.get_tracing()` replaces `Model::get_contact_tracing()`. A mixing layer within groups can also list who shared a group with an agent (`groupmates_of()`). `ModelSEIRQuarantine` turns tracing on for you.

## Outputs

- `get_transmissions()` gains the layer of each transmission (-1 for an infection not drawn from a layer).
- With several diseases, the database's accessors take a `disease` argument (every disease by default).

## Results change

- **Exposures are independent.** An agent escapes infection with probability $\prod (1 - p)$ over its infectious contacts, in every layer. In 0.x the draw was conditioned on at most one transmission, $P(\text{no infection}) = 1 / (1 + \sum p / (1 - p))$, which lowered the probability of infection as contacts piled up. Two contacts at $p = 0.5$ infect with probability 0.75, not 0.67. Models where agents rarely have more than one infectious contact a day barely change. [Combining Exposures](combining-exposures.md) explains why.
- **A layer's weight multiplies the hazard.** A contact of probability $p$ through a layer of weight $w$ counts as $1 - (1 - p)^w$. In 0.x a weight scaled the odds.
- **Recalibrate.** Parameters fitted to data with 0.x, such as transmission probabilities and contact rates, should be fitted again.
- **Random streams.** Since 0.16, a model pushes transmission from the carriers when that is cheaper. In 1.0 mixing layers can be pushed too. Push and pull give the same distribution of outcomes but different runs for a given seed. `set_transmission_mode("pull")` fixes the mode; see [Push and Pull Transmission](transmission-sampling.md).

## New in 1.0

- Several layers per model, each with a weight, disease targets, excluded statuses, closures and a schedule. Tools whose contact effects act in some layers only (`Tool::add_layer_target()`).
- Mixing within groups (households, classrooms), persistence (familiar strangers), and groups drawn at every run.
- Parameters that follow a cycle (`Model::set_param_cycle()`), e.g., more contacts on weekends.
- Several diseases in one model.
- A registry that builds the built-in models by name (`ModelRegistry::make_model()`).
- Layer types of your own: see [Contact Layers](contact-layers.md#a-layer-of-your-own).

## See Also

- [Contact Layers](contact-layers.md) — the layers, how they combine, and writing your own.
- [Mixing by Group](mixing-and-entity-distribution.md) — groups, contact matrices and persistence.
- [Natural History and Statuses](natural-history-and-statuses.md) — stages, statuses and their transitions.
- [Combining Exposures](combining-exposures.md) — independent exposures and the roulette of 0.x.
