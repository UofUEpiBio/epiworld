## Overview
A contact matrix says how many contacts per day an agent of one group has with the agents of each group (e.g., age groups). It is square, one row and one column per group, stored as a one-dimensional vector in column-major order: entry $(i, j)$ is the expected number of contacts per day of an agent in group $i$ with agents in group $j$. Its entries must be non-negative; its rows need not sum to anything in particular (a row's sum is the expected contacts per day of an agent of that group). With a contact-rate parameter (`set_contact_rate()`), the contacts are that parameter times the matrix, so a global event or a calibration can scale them.

### Example

```cpp
std::vector<double> contact_matrix = {
    8.0, 2.0, 3.0,  /* Column A: contacts with agents of group A */
    1.0, 7.0, 3.0,  /* Column B */
    1.0, 1.0, 4.0   /* Column C */
};

/* Set the contact matrix of the model's mixing layer. */
model.get_layer_as< layer::Mixing >("mixing").set_contact_matrix(contact_matrix);
```

Here an agent of group $A$ has, on average, 8 contacts a day with group $A$, 1 with group $B$ and 1 with group $C$ (the first entry of each column), 10 in all.

## The Mixing Layer

Mixing by group is a *contact layer*, a `layer::Mixing` added to any model (`model.add_layer(layer::mixing("mixing"))`). The layer holds the contact matrix, the grouping of the agents, and the pools of infectious agents that `sample()` draws from each step. The model has no contact-matrix methods of its own: everything goes through the layer, which `model.get_layer_as< layer::Mixing >("mixing")` returns. Every agent of a group takes part in the mixing (the contacts are spread over all of them), and the agents in the model's infectious states (`Model::set_infectious_states()`) transmit. With `set_contact_rate(param)`, the contacts are the model parameter `param` times the matrix, so a global event or a calibration can change them.

The layer owns its grouping, set with `set_groups()` before the first run, either fixed or drawn at every reset by a generator (as a network layer takes a `graph::` generator), so each run and each replicate of `run_multiple()` gets its own draw:

```cpp
auto & mixing = model.get_layer_as< layer::Mixing >("mixing");

// Fixed: the group of each agent (layer::Mixing::NO_GROUP leaves one out),
// and the number of groups
mixing.set_groups(groups, 3u);

// Or random groups of 2,000, 3,000 and 1,000 agents, drawn at every reset
// (agents left over are in no group)
mixing.set_groups(grouping::random({2000, 3000, 1000}));
```

Tools and viruses can be given by group: `model.add_tool(tool, assign::group_shares("mixing", shares))` (or `assign::group_counts()`) gives the tool to a share (or a number) of each group of the layer, drawn at every reset, e.g., vaccination rates by school (see [Virus and Tool Distribution Functions](virus-and-tool-distribution.md)).

Whether an edit is permanent depends only on when it is made. Edits made during a run (in a global event, say), such as `mixing.set_group(agent, g)`, `mixing.set_contact_matrix(cmat)` or `mixing.set_contact_prob(q)`, apply to that run only: they set the layer's dirty flag, and the next run starts from the baseline again. A move made during a step takes effect at the end of the step, when the pools are rebuilt, so every agent of that step samples from the grouping the step started with. The same calls made between runs set the baseline, so a calibration (ABC, LFMCMC) can reuse one model. `set_groups()` replaces the whole grouping and can only be called between runs.

### Mixing within groups only

A contact matrix suits a few groups that meet each other. For many small groups that only meet inside -- households, classrooms, transit trips -- give the probability `q` that two members of a group meet on a given day instead:

```cpp
// Households (group of agent i: i / 10) where everyone meets everyone daily
auto households = layer::mixing("households");
households->set_groups(groups, n_households);
households->set_contact_prob(1.0);   // or one probability per household
model.add_layer(households);
```

Each member meets each other member of its group independently with probability `q[g]`, whatever the size of the group: a member of a household of 3 meets `2q` people a day on average, one of a household of 10 meets `9q`. With `q = 1` every group is a clique, with no ties stored; a member who is not available (in a state outside the layer's available states) is simply not met, and the others keep meeting with the same `q`. How much a contact transmits is the layer weight's business (`set_weight()`), not `q`'s. The layer needs its grouping from `set_groups()`, and `members_of(g)` lists a group. With tracing on (`enable_tracing()`), the layer also keeps the run's history of moves and absences: `groupmates_of(agent, d1, d2, out)` fills `out` with who shared a group with `agent`, both present, between days `d1` and `d2`, with the last day and the number of days they shared (contact tracing by membership). Since an infectious agent reaches only its own group, the model's queue follows the layer (see [Queueing System](queueing-system.md)); a layer with a contact matrix makes the model visit every agent instead. Setting contact probabilities between runs drops the contact matrix, and the reverse.

Both forms draw contacts the same way: for each group, a binomial number of *distinct* infectious members (the same as meeting each one independently with a probability `p` per pair). Within groups, `p = q[g]`; with a matrix, `p = min(1, c(g, h) / others(h))`, where `others(h)` counts the available agents of `h` other than the agent itself, so a matrix keeps its contacts per day when agents become unavailable. A diagonal matrix with `c(g, g) = q * others(g)` therefore gives the same run as `q`.

## See Also

- [Core Simulation Models](core-simulation-models.md) — the models, and how to give them a mixing layer.
- [Sampling Contacts](sampling-contacts.md) — the binomial sampling optimization for drawing contacts from the contact matrix.
- [Virus and Tool Distribution Functions](virus-and-tool-distribution.md) — analogous distribution functions for assigning viruses and tools to agents.
- [Library Architecture](library-architecture.md) — how the model, its agents, and its layers fit together.
