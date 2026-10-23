## Overview
Mixing matrices, often referred to as contact matrices, are important when defining interaction dynamics between groups or entities in a simulation. These matrices are represented as square matrices, where each entry specifies the interaction rate between two groups. The rows of the matrix correspond to the source group, and the columns correspond to the target group. The values in each row must sum to $1$, since interaction rates are normalized and probabilistically meaningful.

The contact matrix is typically stored as a one-dimensional vector in column-major order. This representation is efficient for matrix operations and ensures compatibility with the underlying simulation engine. During initialization or runtime, the matrix can be validated to ensure that it adheres to the required constraints: it must be square (i.e., the number of rows equals the number of columns), all entries must be non-negative, and the sum of each row must equal 1. These constraints ensure that the matrix represents a valid stochastic process for interactions.

The contact matrix is used to adjust interaction probabilities between entities. For example, if an agent belongs to group $A$, the matrix entry at row $A$ and column $B$ determines the probability of interaction with agents in group $B$. This adjustment is applied dynamically during the simulation, allowing for heterogeneous interaction patterns across groups.

### Example
Epiworld has no in-built functionality for contact matrices, but they're easy enough to write yourself.

```cpp
std::vector<double> contact_matrix = {
    0.8, 0.1, 0.1,  /* Group A interacts mostly within itself. */
    0.2, 0.7, 0.1,  /* Group B interacts mostly within itself, with some interaction with A. */
    0.3, 0.3, 0.4   /* Group C has more balanced interactions across groups. */
};

/* Set the contact matrix of the model's mixing layer. */
model.get_layer_as< layer::Mixing<> >("mixing").set_contact_matrix(contact_matrix);
```

In this example, the contact matrix specifies that agents in group $A$ interact with agents in group $A$ $80 \%$ of the time, with smaller probabilities of interacting with groups $B$ and $C$. Similarly, group $B$ has a $70 \%$ self-interaction rate, and group $C$ has a more balanced interaction pattern.

## The Mixing Layer

The mixing models (`ModelSIRMixing`, `ModelSEIRMixing`, `ModelSEIRMixingQuarantine`) draw their contacts from a *contact layer*, a `layer::Mixing` named `"mixing"` that the model owns as its layer 0. The layer holds the contact matrix, the grouping of the agents, and the pools of infectious agents that `sample()` draws from each step. The model has no contact-matrix methods of its own: everything goes through the layer, which `model.get_layer_as< layer::Mixing<> >("mixing")` returns.

By default the grouping comes from the entities: at the start of each run the layer assigns every agent to the group of its first entity (agents without entities take no part). From then on the layer works on its own table, so a grouping can also be given directly, without entities:

```cpp
// Group of each agent (layer::Mixing<>::NO_GROUP leaves one out), and the
// number of groups
auto & mixing = model.get_layer_as< layer::Mixing<> >("mixing");
mixing.set_groups(groups, 3u);
```

Whether an edit is permanent depends only on when it is made. Edits made during a run (in a global event, say), such as `mixing.set_group(agent, g)`, `mixing.set_contact_matrix(cmat)` or `mixing.set_rates(r)`, apply to that run only: they set the layer's dirty flag, and the next run starts from the baseline again. A move made during a step takes effect at the end of the step, when the pools are rebuilt, so every agent of that step samples from the grouping the step started with. The same calls made between runs set the baseline, so a calibration (ABC, LFMCMC) can reuse one model. `set_groups()` replaces the whole grouping and can only be called between runs.

### Mixing within groups only

A contact matrix suits a few groups that meet each other. For many small groups that only meet inside -- households, classrooms, transit trips -- give a contact rate per group instead, the diagonal of a matrix that is never stored:

```cpp
// Households of 10 (group of agent i: i / 10), 3 contacts a day within each
auto households = layer::mixing<>("households", {infected}, {s, infected, r});
households->set_groups(groups, n_households);
households->set_rates(3.0);   // or one rate per household
model.add_layer(households);
```

An agent in group `g` makes about `rates[g]` contacts a day with the members of `g`: the same draw a diagonal contact matrix gives, without walking a row of the matrix. The layer needs its grouping from `set_groups()`, and `members_of(g)` lists a group. Since an infectious agent reaches only its own group, the model's queue follows the layer (see [Queueing System](queueing-system.md)); a layer with a contact matrix makes the model visit every agent instead. Setting rates between runs drops the contact matrix, and the reverse.

## Entity Distribution Functions
Entity distribution functions define how agents are assigned to entities or groups. These assignments influence the contact and transmission dynamics, as agents within the same entity may have higher interaction rates or specific behavioral patterns. The API provides several functions for distributing agents into entities, each tailored to different use cases.

### `distribute_entity_randomly`
This function assigns agents to entities based on a specified prevalence. The prevalence can be interpreted as either a proportion of the total population or an absolute number of agents, depending on the `as_proportion` parameter. Additionally, the `to_unassigned` parameter allows the function to operate only on agents that have not yet been assigned to any entity.

```cpp
auto dist_random = distribute_entity_randomly(0.2, true, false);
model.add_entity("Random Group", dist_random);
```

In this example, $20 \%$ of the agents are randomly assigned to the entity "Random Group."
### `distribute_entity_to_range`
This function assigns agents to entities within a specified range of indices. It is particularly useful for partitioning agents into contiguous groups, such as dividing a population into geographic regions or age cohorts.

```cpp
auto dist_range = distribute_entity_to_range(0, 10);
model.add_entity("Range Group", dist_range);
```

Here, agents with indices $0$ through $10$ are assigned to the entity "Range Group."

### `distribute_entity_to_set`
This function assigns agents to entities based on a predefined set of indices. It is suitable for custom groupings where the agent indices are explicitly specified.

```cpp
std::vector<size_t> custom_indices = {1, 3, 5, 7};
auto dist_set = distribute_entity_to_set(custom_indices);
model.add_entity("Custom Group", dist_set);
```

In this example, agents with indices $1$, $3$, $5$, and $7$ are assigned to the entity "Custom Group."

## Agent Partitioning and Dynamics
Agents are partitioned into entities or groups, which serve as the fundamental units for interaction and transmission dynamics. Each agent can belong to one or more entities, and the group membership determines the agent's interaction probabilities. Entities are represented as objects that maintain a list of their member agents and provide methods for querying and modifying this membership.

The interaction dynamics between agents are governed by the contact matrix and the entity structure. When an agent interacts with others, the simulation engine uses the contact matrix to determine the probabilities of interaction with agents in different groups. These probabilities are adjusted based on the size of the groups and the specified contact rates.

## Sampling Agents
Sampling agents is a key operation in the simulation, used to model interactions and transmissions. The sampling process involves selecting agents from other groups based on the adjusted contact rates and the contact matrix. The number of agents sampled from each group is determined by a binomial random variable, where the probability of sampling is proportional to the contact rate.

```cpp
std::vector<size_t> sampled_agents;
size_t n_sampled = model.sample_agents(&agent, sampled_agents);

for (size_t i = 0; i < n_sampled; ++i) {
    std::cout << "Sampled agent ID: " << sampled_agents[i] << std::endl;
}
```

## See Also

- [Core Simulation Models](core-simulation-models.md) — pre-built mixing models (`SIRMixing`, `SEIRMixing`, etc.) that use contact matrices and entity distribution.
- [Sampling Contacts](sampling-contacts.md) — the binomial sampling optimization for drawing contacts from the contact matrix.
- [Virus and Tool Distribution Functions](virus-and-tool-distribution.md) — analogous distribution functions for assigning viruses and tools to agents.
- [Library Architecture](library-architecture.md) — the `Entity` class and its role in the simulation framework.
