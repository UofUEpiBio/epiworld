Who gets a virus or a tool at the start of a run, which stage agents without a virus start in, and which group of a mixing layer an agent belongs to are all placements, written with one vocabulary: `assign` (see `assign.hpp`). An `Assign` gives each eligible agent a **label**, or none, drawn with the model's random numbers when a run starts.

| Form | Labels |
|---|---|
| `assign::share(p)`, `assign::count(n)` | A random share (or number) of the eligible agents |
| `assign::shares({{l, p}, ...})`, `assign::counts({{l, n}, ...})` | Several labels: each takes its share (or number) of the eligible agents, in order, from those left |
| `assign::group_shares(layer, {p_g})`, `assign::group_counts(layer, {n_g})` | A share (or number) of each group of a mixing layer |
| `assign::agents(ids)` | The given agents |
| `assign::labels(v)` | `v[i]` for agent `i` (-1: none); the same every run |
| `assign::by(fun)` | `fun(agent, model)` for each eligible agent |

What a label means, and who is eligible, depends on the use:

| Use | Label | Eligible |
|---|---|---|
| `Model::add_virus(v, a)` | The stage the agent starts in (a carrier stage); a form without labels uses the infection's target | Agents without a virus whose stage can be infected |
| `Model::add_tool(t, a)` | -- (has the tool) | Everyone |
| `Model::set_initial(a)` | A stage without the virus | Agents left without a virus, in the first stage |
| `layer::Mixing::set_groups(a, n)` | A group, in `[0, n)` | Everyone |

The placements are made in this order at every reset: the layers (so a placement can use their groups), the viruses, the tools, the initial stages, then the global events' set-up. The random forms are drawn again in each run; `agents()` and `labels()` are fixed.

## Examples

```cpp
// An SEIR virus: 1% of the agents start Exposed, 0.5% Infected
model.add_virus(seir_virus, assign::shares({{EXPOSED, 0.01}, {INFECTED, 0.005}}));

// 25% start Recovered (immune), the rest Susceptible
model.set_initial(assign::shares({{RECOVERED, 0.25}}));

// A vaccine for 90% of the agents in group 0 of the "school" layer and 50%
// of those in group 1
model.add_tool(vaccine, assign::group_shares("school", {0.9, 0.5}));

// Ten given agents get a second virus
model.add_virus(other, assign::agents({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
```

A virus or a tool added without a placement (`model.add_virus(v)`) is recorded but given to no one; `Model::assign_virus(id, a)` and `Model::assign_tool(id, a)` change the placement of one already added. During a run, a global event can give a tool to an agent with `agent.add_tool(model, tool)` and move agents along declared transitions with `Model::fire()`.

Each `add_virus()` records a new variant with its own id (the founder of a lineage); a tool can target the viruses of some lineages only (`Tool::add_target()`).

## See Also

- [Core Simulation Models](core-simulation-models.md) — the models' `prevalence` arguments are `assign::share()` placements.
- [Mixing by Group](mixing-and-entity-distribution.md) — the groups of a mixing layer.
- [Extending the Library with Models](extending-the-library-with-models.md) — declaring a natural history.
