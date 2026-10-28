# Contact layers: combining networks and mixing in one model

**Status:** Phases 0-2 and 3a done (#315-#319). Where things stand, and what
to do next, is in **Section 11 (Runbook)** -- an agent resuming this work
starts there. Written 2026-10-08; revised 2026-10-19 after the design review
(layers replace the in-`Agent` network and entities; per-layer queues; layer
weights and virus targets; release as 1.0.0 on `v1-layers`), and again the same
day (no compatibility shims; model parameters are the single source of truth
for scalars). Revised 2026-10-21 before Phase 2 (models create no layers;
population in the constructor; networks come from generators; rewiring in the
layer), and after 3a (wrappers follow each merged phase; runbook). Revised
2026-10-22: no `Affiliation` kind -- `layer::Mixing` also describes contacts
within groups only (5.1, 5.2, D9). Revised 2026-10-23: within groups, a
contact probability per pair (`q = 1`: cliques, no ties stored); one sampler
drawing distinct contacts for both forms; tracing within groups by a history
of moves and absences (D9, D11).
**Goal:** let a single model draw transmission from several contact sources at
once -- e.g., mixing within households, a workplace network, and mixing in
public spaces -- each with its own structure, weight, and participation rules.

---

## 1. Motivation

Today a model transmits through exactly one mechanism:

- **Network models** (`ModelSIR`, `ModelSEIR`, ...): a single network stored in
  `Agent::neighbors`. Susceptibles pull from their neighbors
  (`default_update_susceptible`, `sampler::make_update_susceptible()`) or
  receive pushed infection odds (`include/epiworld/model-meat-transmission.hpp`).
- **Mixing models** (`ModelSIRMixing`, `ModelSEIRMixing`,
  `ModelSEIRMixingQuarantine`, the measles mixing models): a contact matrix
  across entities, sampled by `sampler::Mixing`
  (`include/epiworld/sampler-mixing.hpp`) or `SamplerMixing`
  (`include/measles/samplermixing.hpp`) inside a model-specific susceptible
  update function, with queuing off.

Realistic populations mix in several settings at once. An agent with the flu
who isolates stops going to work, school, and public places, but keeps
contacting their household. Neither engine can express that today.

Some settings also have **recurring** contacts that are neither a fixed network
nor fresh random mixing. A commuter who takes the 7:40 train meets largely the
same people every weekday because they share a schedule ("familiar
strangers"). Redrawing contacts every step misses that recurrence; a fixed
network overstates it.

### Limitations of the current code this plan removes

| Limitation | Where |
|---|---|
| One network per model; it lives inside `Agent` | `agent-bones.hpp` (`neighbors`, `neighbor_pos`, `n_neighbors`) |
| Mixing groups agents by their **first** entity only | `sampler-mixing.hpp:171,190,210` |
| A contact matrix is dense (`groups x groups`), so it cannot describe 100k households | `contactmatrix-bones.hpp` |
| Network and mixing cannot be combined | separate susceptible update functions |
| Participation is all-or-nothing by state, per model (e.g., "available states"); the measles sampler hard-codes one reduced-contact state; no per-layer control | `sampler-mixing.hpp`, `measles/samplermixing.hpp` |
| Transmission records do not say *where* (which setting) an infection happened | `DataBase::record_transmission(i, j, virus, date)` |
| Contact structure cannot change on a schedule (weekdays, shifts) or keep cohorts that persist across days | -- |

---

## 2. What other ABMs do

| Framework | Structure | Combination rule | Per-setting control |
|---|---|---|---|
| **Starsim** | `Network` and `MixingPool` are both `Route`s; any number of each. | Each route draws per edge independently; `finalize_infections()` keeps the first infection of each agent. | `beta` per disease per route (`{'mf': [0.25, 0.15]}`). `MixingPool` exposes agents to the *average* source, so it records no infector. |
| **Covasim** (from memory) | Named layers (`h`, `s`, `w`, `c`), each an edge list; community layer resampled daily. | Per-edge draws, deduplicated. | `beta_layer`, `iso_factor`, `quar_factor` per layer; `change_beta(layers=...)`, `clip_edges(...)`. |
| **OpenABM-Covid19** | Household (complete graphs, daily), workplace (Watts-Strogatz, 50% of ties sampled daily), random (redrawn daily, neg-binomial degree). | Hazard-based. | Network-type factor `B_n`; quarantine stops workplace and cuts random contacts; lockdown cuts work/random by 80% and raises household. |
| **EpiModel** | `multilayer` networks. | Infection modules loop over layers (`discord_edgelist(dat, at, network = k)`). | Per-layer formation models. |
| **FRED / JUNE / FluTE** (from memory) | Places/venues (household, school, workplace, neighborhood); no explicit edges. | Hazard summed over places. | Per-venue contact matrices (JUNE). |
| **seirsplus** (from memory) | One network plus a global-mixing probability `p`. | -- | -- |

Common pattern: **a named list of contact layers, each with its own intensity,
interventions that target layers by name, and infections attributed to a
layer.**

Note on Starsim/Covasim: their per-edge draws already include the source's
relative infectiousness, so P(infection) = 1 - prod(1 - p_j) is right. The weak
spot is attribution: when two contacts succeed in the same step, the first one
in route/edge order is recorded as the infector, regardless of who was more
infectious. epiworld picks the infector in proportion to each contact's weight
(odds today), which is better and should stay.

### Mobility and recurring contacts (transit)

How often you meet *the same* people again is a property of its own, separate
from how many contacts you have. Models place that recurrence at four levels:

| Recurrence | Examples | Mechanism |
|---|---|---|
| **Same individuals** (same seatmate daily) | OpenABM workplace; Covasim/Starsim static or long-lived edges; EpiModel | Fixed ties, a subset active each day (OpenABM: 50%). EpiModel and Starsim's `DynamicNetwork` make edge **duration** a parameter. |
| **Same cohort** (the 7:40 bus crowd) | Activity-based models such as MATSim-EpiSim (Mueller, Nagel) | Contacts are found among people sharing a vehicle or facility at the same time, from a transport simulation; schedules create the recurrence. |
| **Same pool** (riders from your station) | **JUNE** (checked in the source) | Commuters are assigned to a station; at each commute time slot they get a **random** vehicle at that station (`Station.get_commute_subgroup()` -> `randint`). Recurrence only at the station level. |
| **None** | Starsim `MixingPool`, Covasim community layer, epiworld `Mixing` | Contacts redrawn every step. |

Empirically, bus smart-card data (Sun et al. 2013, Singapore) show repeated
encounters that follow reproducible schedules ("familiar strangers"), but
which connect into one large, loose small-world network rather than isolated
cliques.

A mixing layer gets the contact *rate* and the group-level pattern right but
not the recurrence. Without recurrence a model overstates spread (an infectious
rider keeps meeting the same, increasingly infected or immune people;
fresh contacts do not saturate), smooths away cohort clustering, and makes
cohort-level tracing or quarantine meaningless. Mixing is adequate only where
turnover is high relative to contacts.

---

## 3. Settled decisions

1. **Layers are first-class objects of the model.** A layer is network-type or
   mixing-type (Section 5.1). There is **no network
   inside `Agent` and no implicit baseline network**: a model may have any
   number of layers of any kind, or none (no transmission). Models create no
   layers: the user adds the ones the model needs, and the default susceptible
   update draws from every layer the model has.
2. **Each layer owns a sampler** that answers "whom did agent `i` contact this
   step?". A network layer returns the agent's ties; a mixing layer samples
   from the population and returns the same kind of list -- as if it were a
   random graph (a stochastic block model) redrawn every step, of which only the
   edges touching infectious agents are ever realized.
3. **Layers own their groupings and replace entities.** A school layer is the
   school; an age-group mixing layer is the age groups. Entities exist today to
   give mixing models "common places"; once layers do that, entities have no
   job left in the core and are removed in 1.0.0 (by the end of Phase 3, D6).
4. **One draw per susceptible, one source of infection.** Contacts from all
   layers go into a single accumulator, and one draw decides whether the agent
   is infected and by whom (roulette across layers). Layers are independent
   sources; the single draw only enforces "one infector".
5. **Repeated contacts are repeated exposures.** The same pair appearing twice
   (twice in a layer, or once in each of two layers) counts twice. That is
   intended: they see each other more often.
6. **Layer weight (intensity).** Each layer has a scalar weight that encodes how
   much time a contact in it represents (family > transit), so no weighted
   graphs are needed. It multiplies the exposure of every contact in the layer.
   A weight is also what calibration targets.
7. **Viruses can be restricted to layers**, the same way tools target viruses
   (`Tool::add_target()`, bitmask, default "all"): an STI acts only on the
   sexual-contact layer. A layer skips viruses it does not carry.
8. **One queue per layer.** Each layer that supports it keeps its own queue
   (infectious-contact counts over its own structure), so a susceptible samples
   only the layers where it is exposed. A mixing layer that cannot queue does
   not disable the queue of the others.
9. **Contact tracing is per layer, opt-in, and recorded after sampling.** The
   contact log belongs to the layer (`layer.enable_tracing(max_contacts)`), so
   it resets, clones and runs in parallel with the layer, and the model keeps
   no tracer of its own (`use_contact_tracing`, `get_contact_tracing()` go, per
   point 18). Every layer's pull or push path reports each contact with an
   infectious agent to the layer's own log, tagged with the layer, so tracing
   needs no layer-specific logic and a layer that is not traced pays nothing.
   Layers differ in what tracing can mean (a school is traced by membership, a
   random mixing layer by what was sampled), so each has its own capacity and
   the models read each layer's success rate and window from model parameters
   (point 17); one layer's contacts never push another's out of a shared
   budget. It will change the random stream of a run when on; that is
   accepted. See D11 for the interface.
10. **Transmissions record their layer** (`layer` column; -1 for seeded or
    non-contact infections). This is a breaking change to outputs.
11. **Dirty-flag reset.** Every edit to a layer (add/remove a tie, move an agent
    between groups or venues) goes through the layer API. Whether it is
    permanent depends only on *when* it is made, not on an argument: an edit
    during a run marks the layer dirty, and `reset()` restores only dirty
    layers from their baseline; an edit between runs becomes the new baseline
    (`Model::run()` tells its layers when it runs). This also replaces copying
    the whole agent backup (ties included) on every replicate. Layers are never
    closed to edits. A network layer's baseline is its generator (point 20): a
    fixed graph is restored if dirty, and a random graph is simply redrawn, so
    edits to it last until the next draw.
12. **Recurrence is a first-class property of a layer.** Between a fixed network
    and mixing across groups sits *within-group* mixing (Section 5.2): contacts
    drawn fresh every step, but only among the members of an agent's group (a
    household, a classroom, a bus trip), so the same cohort recurs; groups can
    persist or change (persistence `q`, Phase 4).
13. **Layers can change over time**: schedules (weekdays), membership changes.
    All changes take effect at the end of a step (like state changes).
14. **A contact matrix's rate does not shrink when people leave a layer.** If
    classmates are quarantined, the remaining children keep contacting other
    children at the same rate. The denominator policy lives in one function so
    that this can be revisited (D2). Within groups the parameter is per pair
    instead (D9): the members still present keep meeting each other with the
    same probability, so their number of contacts falls with the group.
15. **Versioning: this is epiworld 1.0.0**, with breaking changes allowed
    (layer column in outputs, contact-matrix accessors move to the layer, entities removed).
    Work happens on an integration branch, `v1-layers` (Section 6).
16. **Combination rule.** Refactor phases keep today's roulette so existing
    results must stay identical; a dedicated step (Phase 6) switches the rule
    to independent exposures (D1) and updates expected results.
17. **Model parameters are the single source of truth for scalars.** A layer
    never stores its own copy of a number a user may calibrate or change: its
    weight, a persistence `q`, a reduced-contact factor are named model
    parameters, which the layer reads
    with a `ParamRef` (the same way `Virus::set_prob_infecting("name")` works).
    So calibration (LFMCMC), `set_param()` and global events reach layers with
    no layer-specific code, and parameters keep their usual semantics (they are
    not part of the layer's baseline or dirty flag). What is structure, not a
    scalar, stays in the layer: contact matrices and per-group contact
    probabilities, ties, groupings. Structure is edited with the layer's setters
    (`set_contact_matrix()`, `set_contact_prob()`, `set_group()`, `add_edge()`, ...)
    under the dirty-flag rules (point 11): between runs an edit is the new
    baseline (so ABC or LFMCMC can reuse one model), during a run it lasts for
    that run. The layer's weight, a model parameter, scales all of a layer's
    contacts, so one overall level is calibrated like any parameter.
    A layer may still default a parameter it needs (adding it to the model when
    absent), the way models add theirs in their constructors. The line between
    the two: a single number the layer reads **every step** (weight, `q`, a
    reduced-contact factor, rewiring rate) is a model parameter; values per
    group or per tie, and what is used **once per draw** (the arguments of a
    graph generator), are structure, like a contact matrix. A
    generator that should follow a parameter reads it itself (it receives the
    model).
18. **No compatibility shims.** The 1.0.0 API lives where the data lives: the
    model does not forward layer methods (no `model.set_contact_matrix()`; it
    is `model.get_layer_as<layer::Mixing<>>("mixing").set_contact_matrix()`),
    and agents do not forward network methods (D3). The bindings follow the
    same rule and follow the core: each wrapper has its own `v1-layers`
    branch, which ports each phase after it merges (Section 6). An API the
    core removes becomes a defunct stub in the wrappers (it errors and names
    the replacement), never a working alias.
19. **The population is given to the model's constructor**, and that is the only
    way to set it (`Model<> m(n)`, `ModelSIR<> m("flu", n, ...)`). Nothing
    about the population mentions networks: `agents_empty_graph()`,
    `agents_smallworld()`, `agents_from_edgelist()` and `agents_from_adjlist()`
    are removed. A different size is a new model.
20. **Networks come from generators.** A `layer::Network` holds a generator, a
    function that receives the model and returns a graph (`AdjList` until a
    flat edge list replaces it), and calls it at every reset, so every run and
    replicate draws a new network from its own seed. Generators live outside
    the layer (`graph::smallworld(k, p)`, wrappers of the `rgraph_*`
    functions), so adding one is writing a function. A fixed network is built
    from a graph instead (`layer::network(name, adjlist)`), read from a file or
    drawn once; it is restored at reset if dirty. Rewiring follows the same pattern: the layer holds a
    rewiring function called at the end of each step (degree-preserving
    rewiring is the first; an ERGM-style rewirer can be another), reading its
    rate from a model parameter (point 17).
21. **Layers are shared pointers.** A layer is created by its factory
    (`layer::network()`, `layer::mixing()`; the constructors are protected) or,
    for a user-defined layer type, by `std::make_shared`, and
    `model.add_layer(ptr)` shares it: the caller's pointer and the model's are
    the same layer, so an edit through either is an edit of the model, and
    nothing is copied. Three rules keep that safe: a layer belongs to one
    model at most (`add_layer()` throws otherwise; add a copy); a model
    detaches its layers when it is destroyed or assigned to, so a pointer that
    outlives the model is a detached layer, not a dangling one; and a copy of
    the model (`run_multiple()`) clones its layers, so outside pointers stay
    with the original. The bindings hold the same pointer (pybind11's
    `shared_ptr` holder, an R external pointer to it), so an R or Python layer
    object is the model's layer.

---

## 4. Open decisions

### D1. Combination rule: roulette (odds) or independent exposures (hazard)

Both pool the contacts of all layers into one accumulator and pick at most one
infector; they differ in P(infection). Today `roulette()`
(`include/epiworld/misc.hpp`) treats exposures as independent Bernoullis
**conditioned on at most one success**:

    P(no infection) = 1 / (1 + R),  R = sum_j r_j,  r_j = p_j / (1 - p_j)
    P(infected by j) = r_j / (1 + R)

The alternative keeps a single infector but drops the conditioning (independent
exposures, competing risks):

    lambda_j = -log(1 - p_j)
    P(no infection) = exp(-sum_j lambda_j) = prod_j (1 - p_j)
    P(infected by j) = (lambda_j / sum lambda) * (1 - exp(-sum lambda))

Both are additive (sum of odds or sum of hazards), so composing layers, the
reservoir sampling of the infector, and push all work the same way with
either. They agree for one infectious contact and diverge as contacts pile up,
which is exactly what stacking layers does:

| Infectious contacts | Roulette | Independent |
|---|---|---|
| 1 x p=0.5 | 0.500 | 0.500 |
| 2 x p=0.5 | 0.667 | 0.750 |
| 3 x p=0.3 (household) | 0.563 | 0.657 |
| 10 x p=0.1 | 0.526 | 0.651 |

Independent layers imply independent exposures: with roulette, adding layers
saturates P(infection) (it is `R / (1 + R)`), so household secondary attack
rates are biased downward. A layer weight `s_L` multiplies the hazard
(`lambda_j * s_L`, i.e. "time in contact"), which also allows `s_L > 1`; under
roulette it would scale odds, which has no clean interpretation.

Cost, per (susceptible, infectious contact) pair, microbenchmark (CPU time,
4M values, 20 interleaved rounds, host clang -O2): odds 0.75 ns, hazard
(`log1p`) 4.68 ns, product 2.13 ns. Plus one `exp` per agent with any
infectious contact. These pairs already pay for tool reduction calls,
`get_prob_infecting()`, a uniform draw, and usually a cache miss, so the
difference is probably a few percent at most -- **to be confirmed on real
models** (Phase 0).

Recommendation: roulette during the refactor phases (identical results are the
safety net), then switch to independent exposures in Phase 6 as the only rule
(1.0.0 allows result changes). Keep a flag only if Phase 0 shows a cost that
matters. Add an end-to-end test against the analytic P(infection).

### D2. Participation: leaky isolation and exclusion

Wanted: an agent can be fully out of a layer (quarantine) or only partly out
("leaky" isolation: 90% of work time gone). Options:

- **(a) Exclusion flag + layer-targeted tools (recommended).** A per-agent,
  per-layer on/off flag (bitset) removes the agent from the layer's pools in
  both directions. Partial reduction reuses the tool machinery: a tool
  (`transmission_reduction` / `susceptibility_reduction`) gets *layer targets*,
  exactly like virus targets, so "isolation reduces work and school contacts by
  90%" is a tool that targets those layers. Lifetimes, distribution
  (`distribute_tool_to_set`), and stacking already exist; layers without
  targeted tools pay nothing. Drawback: the reduction thins the agent's
  exposure, it does not change who they meet.
- **(b) A per-agent, per-layer float weight.** More direct, one load and a
  multiply per infectious contact, but 4 bytes x agents x layers (16 MB at 1M
  agents and 4 layers) unless stored sparsely (only agents off the default 1.0),
  and a second place to express what tools already express.

Contact rate when agents leave (settled, point 14): a contact matrix's rate
stays constant, so a mixing layer divides by the number of agents *present*,
not the original count (within groups, the probability per pair stays
constant instead, D9). For network layers the contacts with an excluded agent are simply lost
(the usual ABM behavior, e.g. Covasim's `clip_edges`, from memory). The
denominator is computed in one function so that frequency-dependent
alternatives can be added later.

### D3. Network convenience API (settled 2026-10-19)

With no network in `Agent`, `Agent::get_neighbors()`, `neighbors_view()`,
`Model::add_edge()`, `rm_edge()`, `has_edge()`, `agents_from_edgelist()`,
`agents_from_adjlist()`, `agents_smallworld()`, `rewire()`,
`write_edgelist()` become methods of `layer::Network` or generators (point 20),
and nothing else. No one-argument shorthand for "the only network layer"
(point 18).

### D4. Mixing layers and the queue

A public-space mixing layer connects every group, so its queue would be
"everyone" after the first case. Per-layer queues (point 8) mean it does not
disable the others, but pulling it for every susceptible each step is O(S).
The efficient route is push for mixing layers (Phase 5). Until then a mixing
layer is pulled for every susceptible, as today.

### D5. Directed networks

Today `directed` is model-wide and forces pull. Recommendation: per network
layer; a model pulls if any layer is directed.

### D6. Entities removal (settled 2026-10-19: remove, by the end of Phase 3)

Entities are removed, not kept as a parallel concept: every use moves to a
layer (entity-level tools and events, `Bubbles`, the measles school and mixing
models, #311). Distribution helpers become layer-membership builders; the
random ones keep today's behavior of redrawing at every `reset()`. Until then,
`layer::Mixing` builds its grouping from the first entity when no grouping is
given (the one shim left from Phase 1, because the random entity distributions
the mixing tests use redraw at each reset and have no layer counterpart yet).
The audit is now only a checklist of call sites, done at the start of the
removal.

### D7. Layer targets on tools: mask or list

Reuse the 63-bit mask of `Tool::add_target` (so at most 63 layers/viruses) or a
dynamic bitset. Recommendation: mask, same limits and wording as virus targets.

### D8. Persistence model for group memberships

Section 5.2 proposes "usual group with probability `q`, otherwise a random group
in the same pool", with `q` per layer. Alternatives: `q` per agent (strict vs
irregular commuters), or a user-supplied assignment function
`group(agent, step)` for data-driven schedules. Recommendation: per-layer `q`
plus an optional per-agent override, and the assignment function as the
general escape hatch.

### D9. Within-group contacts (settled 2026-10-22, revised 2026-10-23)

There is no separate affiliation layer: `layer::Mixing` describes contacts
either with a contact matrix (across groups) or with a **contact probability
per pair** `q` (within groups only): each member meets each other member of
its group with probability `q[g]` per day, whatever the size of the group (a
household of 3 meets `2q` people a day, one of 10 meets `9q`; each pair the
same). The probabilities are structure (point 17): a vector, or one number for
every group; bus trips and subway cars in one transit layer simply have
different `q`. `set_contact_prob()` edits them like `set_contact_matrix()`
edits a matrix.

- **Cliques without ties.** `q = 1` is "everyone meets everyone, every day":
  a household as a clique, stored as one group id per agent instead of
  `k(k - 1)` adjacency entries. There is no clique mode or sentinel; a clique
  is the top of the range.
- **Frequency vs intensity.** `q` is how often a pair meets; how much a
  contact transmits is the layer weight (a model parameter that scales the
  odds). `q` in `[0, 1]` is identifiable in calibration (a rate would
  saturate at the group size).
- **Absent members.** An unavailable member is simply not met; the others
  keep the same `q` (unlike a matrix, point 14).
- **One sampler.** Both forms meet each infectious member of a group
  independently with a probability `p` per pair: a binomial number of
  *distinct* members (a partial shuffle of the group's infectious pool; `p = 1`
  takes all of them, with no draws). Within groups `p = q[g]`; with a matrix
  `p = min(1, c(g, h) / others(h))`, `others(h)` being the available agents of
  `h` other than the agent itself. So a diagonal matrix with
  `c(g, g) = q * others(g)` is the same run as `q`. Drawing distinct contacts
  replaced the draws with replacement in 3b, which changed the results of
  matrix layers (the mixing models) then rather than in Phase 6.

### D10. Where schedules live

Either in the layer (`update()` reads a calendar: active days, per-agent
schedules) or in global events that set layer weights and flags. Layers own
routine schedules; global events own one-off interventions (closing schools).
Recommendation: both, with that split.

### D11. Contact tracing by layer

Settled (point 9): the log is owned by the layer, opt-in, with its own
capacity. Open:

- **Rows allocated lazily.** Today's `ContactTracing` is dense, `n_agents x
  max_contacts` slots of 16 bytes (about 320 MB at 1M agents and 20 slots, from
  the class description). A log per traced layer multiplies that. The log is
  keyed by the infectious agent, so rows can be allocated when an agent first
  appears as a source; narrower integer types are the other lever. Measure
  before choosing (memory and the cost of the lookup).
- **Structural tracing (settled 2026-10-23).** Within groups, tracing needs
  no contact log: the layer keeps a **history** of who was in which group, and
  who was absent, on which days, and tracing asks it "who shared a group with
  X, present, between days `d1` and `d2`" (`groupmates_of(agent, d1, d2)`:
  each contact with the days shared, so also the last one). With `q = 1` that
  is exactly who X met; with `q < 1` it is who X could have met, which is what
  "notify the household / class" means. The measles "high risk" branch
  (shares an entity with the case) becomes that query.
  - *What is stored.* Per agent, its spells: `(from day, group)`, appended when
    a move takes effect (end of step, so days are exact; only the step's net
    change is kept), and `(from day, absent / present)` when it enters or
    leaves an unavailable state or is isolated (Phase 4). Per group, who joined
    or left, and when. Agents that never move or leave cost nothing; leaving
    the layer (`NO_GROUP`) is a move like any other. It resets with the layer.
  - *Query.* The agent's spells in the window give `(group, days)`; for each,
    the candidates are the group's current members plus those who left it
    since, filtered by their own spells. Cost: the group sizes plus the moves
    in the window.
  - *Consequence.* Groups need not be immutable: a household move, a bubble
    entered and left, a return home are all traced.

  A network's current ties are not the contacts someone had last week, and
  `ModelSEIRNetworkQuarantine` traces from the log, so for networks the log
  stays the tool; the sampled-contact log stays opt-in for any layer
  (point 9).
- **Meaning of a contact.** The log holds sampled contacts with infectious
  agents. When the combination rule moves to independent exposures (D1,
  Phase 6) the recorder hooks into the individual exposure events so the meaning
  stays the same.
- **A shared tracing policy.** The consumers (`ModelSEIRMixingQuarantine`,
  `ModelSEIRNetworkQuarantine`, the measles mixing models, the PEP
  intervention) repeat one loop: cap at the tracking limit, draw a success per
  contact, apply a date window, filter by state. The order of the success draw
  relative to the other checks differs between them and sets the random stream,
  so a helper can share the iteration but not the draw. Extract a policy object
  (layers to read, per-layer success and window, action) only after the Phase 3
  port shows what is common; "tracing at school and in the community" in one
  model is the first real second user.
- **Per-layer actions** (quarantine from school only, keep household) need the
  per-layer participation of D2. Tracing with per-layer success rates works
  without it; D2 widens what the traced agents can be made to do.

---

## 5. Design

### 5.1 Layer kinds

| Kind | Describes | Storage | Example |
|---|---|---|---|
| `layer::Network` | explicit ties | per-agent tie lists (as today) | workplace, sexual partners |
| `layer::Mixing` | contacts drawn fresh each step, by group: across groups (a contact matrix) or within groups only (a contact probability per pair) | group id per agent + a `G x G` matrix, or `G` probabilities | age groups in public spaces (matrix); households (`q = 1`), classrooms, transit trips (`q < 1`) |

One class, two ways to give the contacts, one sampler: for an agent in group
`i`, each infectious member of group `g` is met with a probability `p` per
pair (a binomial number of distinct members, D9), with `p = min(1, c(i, g) /
others(g))` over every `g` with a matrix and `p = q[i]` over `g = i` only
within groups. A matrix suits a few groups that meet each other; it
cannot hold 100k households, and its sampler walks the whole row. Contact
probabilities suit many small groups: no matrix is stored, sampling costs only the agent's own
group, cohorts recur (Section 5.2), and since an infectious agent reaches only
its own group, the layer supports the queue (5.7). An agent belongs to one group
per layer; several memberships (a household and a classroom, a morning and an
evening trip) are several layers.

`Bubbles` with `BubbleTies::Complete` (cliques added and withdrawn) becomes a
within-group `Mixing` layer with `q = 1` in 3d: entering and leaving a bubble
are moves, so bubbles are traced through the history (D11). An agent is in one
group per layer, so overlapping bubbles need two layers.

Recurrence, from most to least: `Network` (same individuals), within-group
`Mixing` with `q = 1` (same cohort), with `q < 1` or one large group per route
(same pool), `Mixing` across groups (none).

### 5.2 Within-group mixing, recurrence, and schedules

**Groups.** A within-group mixing layer holds many small groups (a household;
the 7:40 departure of route 12), one per agent (or none), and a contact
probability per pair for each group (D9). Membership builders (3b) make the grouping from data (a group id per
agent, or the members of each group); `members_of(group)` and `get_group(agent)`
query it.

**Contacts within a group.** Each step, an agent in group `g` meets each
member of `g` who is present (not excluded) with probability `q[g]`, drawn
fresh: a binomial number of distinct members of the group's infectious pool,
the draw `Mixing` makes for any group (D9). Recurrence is at the cohort level: you meet the same
crowd, not necessarily the same seatmate. Dyad-level recurrence (same seatmate)
belongs in a `Network` layer.

**Persistence (Phase 4).** Each agent has a *usual* group, and groups are
gathered into *pools* (a route, a station, a school). Each step, with
probability `q` the agent attends its usual group; otherwise it attends another
group drawn from the same pool (a `set_group()` for that step):

- `q = 1`: fixed cohorts (households, classrooms, strict commuters);
- `0 < q < 1`: familiar strangers (mostly the same train, sometimes another);
- `q = 0`: random vehicle at your station each step (JUNE);
- one group per pool: route-level mixing.

Only agents who switch group cost anything per step (`O(riders x (1 - q))`
membership updates), plus `O(q x group size)` per infectious member. A membership change
notifies the queue, as `add_edge()`/`rm_edge()` do today
(`Queue::notify_edge_added/removed`).

**Schedules.** A layer can be active on some steps only (transit and work on
weekdays), and an agent's exclusion flag can follow its own schedule (a remote-work
day excludes that agent from the work layer). Both are evaluated in the layer's
`update()`; changes take effect at the end of the step like any other (point 13). With data instead of a
generative model (e.g., a co-rider network from smart-card records), use a
`Network` layer whose ties are each active with probability `a` per step -- the
OpenABM workplace approach, which is one more multiplier on `p` (thinning).

**Same-step exposures.** A step is a day: an agent's bus, workplace, and
household exposures on one day combine as simultaneous. That is fine with an
incubation period and is the same assumption as for all layers; finer time
slots (JUNE) are out of scope.

### 5.3 Interface (sketch)

```cpp
template<typename TSeq>
class ContactLayer {
public:
    virtual ~ContactLayer() = default;

    const std::string & get_name() const;
    size_t get_id() const;                // index in the model's layer list

    // Lifecycle
    virtual void reset(Model<TSeq> & m) = 0;    // restore if dirty (point 11)
    virtual void update(Model<TSeq> & m) = 0;   // once per step, before transmission
    virtual std::shared_ptr<ContactLayer<TSeq>> clone() const = 0; // run_multiple

    // Pull: append the contacts of `i` this step that carry a virus the
    // layer admits, repeats allowed. Returns how many were appended.
    virtual size_t sample(size_t i, Model<TSeq> & m, std::vector<size_t> & out) = 0;

    // Queue (per layer): is `i` exposed through this layer?
    virtual bool supports_queue() const { return false; }

    // Push (Phase 5): whom did carrier `j` contact?
    virtual bool can_push() const { return false; }
    virtual size_t sample_reverse(size_t j, Model<TSeq> & m, std::vector<size_t> & out);

    // Intensity (a model parameter, point 17) and viruses
    void   set_weight(std::string param); // layer weight (point 6)
    void   add_virus_target(const Virus<TSeq> & v);  // default: every virus

    // Dirty flag, set by every mutator of the derived classes
    bool is_dirty() const;
};
```

Only what a phase uses is added in that phase (no `sample_reverse` before
Phase 5, no weights before Phase 3). Virtual dispatch happens once per
(agent, layer) per step, not per contact.

**Reset order.** Layers reset first, right after the agents return to the
baseline state and before the agents-by-state index is built (its degree sums
count ties) and anything is distributed (entities, viruses, tools, initial
states, the global events' set-up): a redrawn network must exist before agents
get a virus (the queue follows ties), before seeding that depends on the
network, and before `Bubbles` sets itself up. `layer::Mixing` draws no random
numbers at reset; its fallback grouping from entities, which are distributed
later, is built at its first update instead, so its results are unchanged.

The model holds `std::vector<std::shared_ptr<ContactLayer<TSeq>>>` (point
21), with `add_layer(ptr)`, `get_layer(name|id)`, `get_layer_id(name)` (resolve names once,
use ids in hot loops). `Model`'s copy constructor and `clone_ptr()` must
deep-copy layers (`run_multiple()` clones models).

### 5.4 Combining layers in one draw

`default_update_susceptible` (and `sampler::UpdateSusceptible`) become
layer-aware: for a susceptible `i`, every layer appends contacts; each contact
`j` with a virus the layer admits contributes
`p_ij = (1 - sus_red_i) * p_v * (1 - trans_red_j)` (tool reductions, including
layer-targeted ones, D2) scaled by the layer weight, to the per-agent
accumulator; one draw decides infection and infector (rule from D1; the layer
weight scales odds under roulette, hazards after Phase 6). This is the same
accumulator the push path already uses (`PushTarget`: summed odds + weighted
reservoir), so pulling and pushing layers can be mixed in one step: reservoir
samples merge exactly.

Keep the arrays-then-`roulette()` code path for models whose only layer is a
single layer with weight 1 until Phase 3 benchmarks show whether it is needed.

### 5.5 Participation

See D2. Excluded agents are removed from a layer's pools and denominators in
both directions; with a matrix, the contact rates of the others stay constant
(point 14), and within groups their probability per pair does (D9). Partial
(leaky) reduction is a layer-targeted tool. Changes take effect at the end of
the step (point 13), so results do not depend on agent update order.

### 5.6 Neighbors per network layer

- `layer::Network::neighbors_view(i)` returns the existing non-allocating
  `NeighborsView`; `Agent` no longer stores ties (`neighbors`, `neighbor_pos`,
  `n_neighbors` are removed).
- `layer::Network` keeps per-agent tie lists so edits (`add_edge`, `rm_edge`,
  `swap_neighbors`/`rewire_degseq`) and **neighbor order** (which fixes the
  random stream) are preserved in the refactor phase.
- One way to read ties: `neighbors_view()` (the allocating `get_neighbors()`
  goes). One way to edit them: the layer's `add_edge()`, `rm_edge()` and the
  rewiring function, which keep the queue and degree sums in step.
- Bindings: a named list of per-layer neighbor vectors; edgelist I/O gets a
  layer column.

### 5.7 Queue

- The model keeps one queue, fed by every layer it can see
  (`ContactLayer::supports_queue()`): undirected networks and within-group
  `Mixing`. A carrier queues its ties and the members of its group in every
  such layer (counts add across layers); a tie or membership change notifies
  the queue (`Queue::notify_ties_changed()`, `notify_group_changed()`).
- A model with a layer the queue cannot see (a directed network, `Mixing` with
  a matrix) visits every agent, until push exists (D4).
- Per-layer queues (sampling a layer only where the agent is queued in it) were
  planned for 3b and dropped: with one queue, a 6,000-agent model with 600
  households visits 614 susceptibles in 30 days instead of 177,331, and a
  layer an agent has no infectious contact in costs almost nothing to sample
  (a within-group layer returns at once). Revisit with push (Phase 5) if
  profiling shows otherwise.
- Excluded agents (flag) simply produce no contacts; the queue stays
  conservative.

### 5.8 Push and pull

- Phases 1-4: push only for a model whose only layer is one undirected
  network layer (today's behavior). Everything else pulls.
- Phase 5: `sample_reverse()` for `Mixing` (for infectious `j` in group `g`,
  each available agent of group `h` is reached with the same probability per
  pair as pulling, `min(1, c(h, g) / others(g))`; within groups, `h = g` only and
  `q[g]`); each layer
  chooses push or pull by its own cost model, and their contributions merge in
  the shared accumulator.

### 5.9 Outputs

- `record_transmission(i, j, virus, date, layer)`; `get_transmissions()` gets a
  `layer` column (-1 for seeded/non-contact).
- The contact-tracing recorder (the layer's own log, point 9) and the
  post-sampling callback receive the layer of each contact.
- `write_edgelist()` / `get_edgelist()` per layer.

### 5.10 Compatibility

None promised across 0.x -> 1.0.0. Refactor phases (1-2) keep model-level
results identical (same seeds, same draws); the breaking surface is the API
(layers explicit, entities removed, contact-matrix accessors only on the layer, `layer`
column) and, in Phase 6, the combination rule. A migration note per phase goes
in the release notes.

---

## 6. Branching and phases

All work goes on the long-lived integration branch **`v1-layers`**, one PR per
phase into it. `master` stays releasable (0.x fixes keep landing there and are
merged into `v1-layers` regularly). `v1-layers` merges into `master` and is
released as 1.0.0 when Phase 6 is done. Version bumps: none per phase on
`v1-layers`; the first commit there sets the development version to 1.0.0.

**Wrappers follow the core, one merged phase at a time.** epiworldR,
epiworldpy, epiworldjs and the measles R package each keep a `v1-layers`
branch (version `1.0.0-0`; measles its own next minor, depending on
epiworldR's branch). Their `main` branches keep tracking epiworld `master`.
The work is sequential:

1. **C++ phase PR** into epiworld `v1-layers`. Before it merges, each wrapper
   is *compiled and tested* against the PR's headers (no wrapper code is
   written yet); a binding that cannot be written cleanly changes the C++ API
   while it still can. `downstream.yml` automates this check once it exists
   (non-blocking).
2. **After the C++ PR merges**, one PR per wrapper ports the phase, vendoring
   the merge commit, in this order: **epiworldR**, then **epiworldpy**, then
   **epiworldjs**, then the **measles** package (after the epiworldR PR has
   merged, since it builds against epiworldR's `v1-layers`). What the R port
   settles (names, defunct messages) carries over to Python and JS.
3. **Lag at most one phase.** The next C++ phase may start while the wrappers
   port the previous one, but not the one after: C++ phase N+1 does not merge
   until every wrapper has merged phase N-1 (in practice, keep them on N).

- The wrappers mirror the core API and add none of their own. What the core
  removes becomes a defunct stub (R `.Defunct()`, collected in one file and
  one help page; Python and JS raise or throw), with one message table per
  wrapper that also feeds the migration notes. The stubs stay through 1.x.
- Every wrapper vendors epiworld and measles headers from the same epiworld
  commit, and records it in a `VERSIONS` file (Section 11.3).
- `master` -> `v1-layers` merges in epiworld are mirrored by `main` ->
  `v1-layers` merges in the wrappers.

Each phase ships on its own and keeps the suite green. Phases 1-2 are refactors:
the test is that existing model-level results stay identical.

### Phase 0 -- benchmark the combination rule (independent; any time)

- Implement the hazard rule behind a flag on a scratch branch.
- Run SEIR network (pull and push) and SEIR mixing at 1e5-1e6 agents; CPU time,
  interleaved, in the devcontainer.
- Output: overhead numbers for D1.

**Result (2026-10-19, devcontainer, g++ -O2, CPU time, variants interleaved,
scratch branch `phase0-hazard-bench`).** Hazard rule behind
`EPI_TRANSMISSION_HAZARD` at every transmission draw (pull in all models, push
accumulator and final draw). Benchmark: `examples/20-transmission-benchmark`
with two new scenarios (F: `ModelSEIRMixing`, three groups; G: `ModelSEIR`,
`p = 0.06`) and a count of infectious contacts evaluated, so the comparison is
CPU ns **per contact** (whole-run time is confounded: the rules produce
different epidemic sizes).

| Scenario | n | Mode | ns/contact roulette | ns/contact hazard | ratio |
|---|---|---|---|---|---|
| B SEIR network | 100k | pull | 124.6 | 126.4 | 1.015 |
| B SEIR network | 100k | push | 68.6 | 72.3 | 1.054 |
| E heterogeneous SEIR | 165k | pull | 409.6 | 423.9 | 1.035 |
| E heterogeneous SEIR | 165k | push | 427.2 | 428.8 | 1.004 |
| F SEIR mixing | 100k | pull | 281.8 | 270.2 | 0.959 |
| G SEIR network | 100k | pull | 203.4 | 206.3 | 1.014 |
| G SEIR network | 100k | push | 212.7 | 210.4 | 0.990 |
| F SEIR mixing | 1M | pull | 843.8 | 765.8 | 0.908 |
| G SEIR network | 1M | pull | 344.4 | 306.8 | 0.891 |
| G SEIR network | 1M | push | 339.4 | 382.7 | 1.128 |

- Medians of 8 rounds (100k-165k) or 3 rounds (1M). The machine is loaded:
  per-cell spread between rounds is 10-70%, so differences below about 5-10%
  cannot be resolved. The 100k-165k cells have a median ratio of 1.014, with
  no consistent direction; the 1M cells (3 rounds) scatter from 0.89 to 1.13.
- Consistent with the microbenchmark: about 4 ns extra per contact on 70-400 ns
  of work per contact, i.e., 1-2%.
- Correctness (star graph, 10 contacts from two viruses, p = 0.3 and 0.05; 40,000
  replicates; `check-rule.cpp`): P(infection) and the infector split match the
  analytic values for both rules in pull and push (roulette 0.707 vs 0.706;
  hazard 0.870 vs 0.870; share of virus A 0.875 vs 0.874 under hazard).
- Decision for D1: the overhead is not measurable on real models, so there is
  no reason to keep two rules beyond the refactor phases. Proceed as planned
  (roulette until Phase 6, then independent exposures as the only rule).

### Phase 1 -- layer interface + `layer::Mixing`

- Add `ContactLayer`, the model's layer list, `add_layer()`/`get_layer()`/
  `get_layer_as<T>()`, and the dirty-flag baseline/reset machinery (as built:
  no model-level forwarders, no `as_baseline` argument; per-layer queue hooks
  deferred to Phase 2, their first user).
- Move `sampler::Mixing` into `layer::Mixing` with its own explicit grouping (no
  `get_entity(0)`).
- Port `ModelSIRMixing`, `ModelSEIRMixing`, `ModelSEIRMixingQuarantine`.
- As built: `layer::Mixing` holds the contact matrix and its baseline itself;
  `ContactMatrix` moved to `include/measles/` (the two measles mixing models
  still inherit it) and goes with the measles port in Phase 3.
  `Model::sample_virus(agent, contacts, n)` is the first piece of the
  accumulator (5.4): exposures of a list of contacts plus the one draw. The
  five mixing models (epiworld and measles) use it.
- Files: new `contactlayer-bones.hpp`, `layer-mixing.hpp`;
  `sampler-mixing.hpp`, `contactmatrix-*.hpp`, `models/*mixing*.hpp`,
  `model-bones.hpp`, `model-meat.hpp`.
- Tests: existing `05*-mixing`, `06*`, `08-mixing-entities`, `17*` pass with
  identical results; new: an agent's group in the layer differs from its first
  entity; a layer edited during a run is restored on `reset()` (dirty flag),
  and an edit between runs becomes the new baseline.

### Phase 2 -- `layer::Network`

1. Population: every model takes `n` in its constructor (point 19).
2. Ties leave `Agent` for `layer::Network`, which holds the generator, the
   ties and `directed` (points 20, D5).
3. Network functions leave `Model`: `add_edge`, `rm_edge`, `has_edge`,
   `neighbors_view`, edgelist output and rewiring are the layer's (D3).
4. The default susceptible update pulls from every network layer through
   `Model::sample_virus()` (with one layer, results are identical).
5. The queue and push follow every network layer (counts and odds add across
   layers; any directed layer means pull and no queue, D5). Layers reset
   before the distributions (5.3).
6. Port the network models, `Bubbles` (it names the layer it edits, until
   Phase 3), `randgraph.hpp`, and the tests.
- The network pull goes through `Model::sample_virus()` too:
  `collect_neighbor_viruses` becomes `layer::Network::sample()` (contact ids,
  like `Mixing::sample()`), and `sample_virus_single` is removed. As built:
  `make_update_susceptible(exclude)` stays, because which states do not
  transmit (SEIR's exposed) is a property of the disease model, not of a
  contact structure; moving it into each layer would make users configure
  every layer they add. It pulls from every network layer.
- Tests: the whole suite unchanged in results (notably `15`, `20*`, `33*`,
  `34*`, `35*`, `36a`; random graphs drawn once become fixed layers);
  new: a network split into two layers matches the single layer in
  distribution; a redrawn network differs across replicates and repeats with
  the seed. Benchmark: no regression in pull or push (CPU time, interleaved).
- Risk: largest phase (about 200 references to `neighbors` in `include/`, 47 in
  `agent-meat.hpp`). Neighbor order must be preserved exactly.

### Phase 3 -- several layers in one model, entities removed

Phase 3 is built as four PRs into `v1-layers`, in this order:

- **3a. The accumulator** (as built): `ContactLayer::sample()` is virtual and
  appends contacts to a vector; `Model::sample_virus(agent)` collects the
  contacts of *every* layer (networks and mixing) into one draw. A layer has a
  weight (`set_weight(param)`; the odds of its contacts are multiplied by it
  under roulette, 0 turns the layer off, 1 changes nothing) and virus targets
  (`add_virus_target()`, a lineage bitmask like the tools'). The layer id of the
  winning contact travels with the infection: `Model` keeps it in
  `sampled_layer` after each draw, `Agent::set_virus()` takes it into the event,
  and `get_transmissions()` / the `transmission` output get a `layer` column
  (-1 for seeds and events). A model with a layer that is not a network, or with
  a directed network, always pulls and visits every agent (no queue) until
  per-layer queues exist (3b). The SIR, SEIR and SEIR-quarantine mixing models
  use the generic path; the measles models keep their own sampler until 3d.
  Draw-for-draw identical to Phase 2 for every model in the identity harness.
  Post-sampling and contact tracing receive the layer in 3c.
  Follow-up (#319, found reviewing epiworldR #216): the sampler's scratch
  arrays (`array_double_tmp`, `array_virus_tmp`, `array_layer_tmp`) were fixed
  at 2,048 slots and only bounds-checked under `EPI_DEBUG`, so an agent with
  more exposures than that across its layers corrupted memory. They now grow
  (`Model::reserve_tmp()`), in the generic path and in every model with its
  own sampler; there is no ceiling on an agent's exposures, and the 1,024
  neighbor limit of `Bubbles` is gone.
- **3b. Within-group `Mixing` (a contact probability per pair), seen by the
  queue; one sampler drawing distinct contacts (D9).**
- **3c. Contact tracing into the layers** (point 9, D11), in two PRs
  (settled 2026-10-24):
  - **3c-1. Layer logs.** `enable_tracing(max_contacts)` on any layer: the
    layer logs the contacts it samples (pull, push and the models' own
    samplers), under the carrier, in a `ContactTracing` that starts empty at
    every run. `ModelSEIRMixingQuarantine` traces its mixing layer and
    `ModelSEIRNetworkQuarantine` its network layers (turned on at reset); both
    read every traced layer's log, layer by layer, and no longer install the
    post-sampling recorder. Draw-for-draw identical for a model with one
    traced layer; with several, each layer keeps its own `max_contacts`
    (before, one log was shared). The measles models do not sample from
    layers until 3d, so they keep the model's log (`contact_tracing_on()`)
    and the PEP intervention reads it; 3d removes it with them.
  - **3c-2. Group history** (as built). A within-group `Mixing` layer keeps,
    for the run, one list of changes `(from day, agent, group, present)`,
    linked per agent and per group left: a move is recorded when `update()`
    applies it, and an absence when the agent enters or leaves one of the
    model's excluded states (`set_excluded_states()`; Phase 4 adds
    isolation), both from the next day. `groupmates_of(agent, d1, d2)`
    returns each groupmate with the last day and the number of days they
    shared a group, both present (days after today are left out). Matrix
    layers keep no history (`groupmates_of()` throws). Agents that never move
    or leave add nothing to the list; the per-agent index is one integer per
    agent.
- **3d. Entities removed, measles models and the example model on layers**
  (the acceptance tests of D11).

The list below is the whole of Phase 3.

- The accumulator (5.4) for every kind of layer (Phase 2 does networks);
  layer weights; virus targets per layer; the queue following within-group
  layers (5.7); layer id in
  transmission records, contact tracing and post-sampling; within-group
  `layer::Mixing` (a contact probability per pair, as a vector or one number;
  `set_contact_prob()` / `get_contact_prob()`; groups from `set_groups()`,
  fixed until persistence in Phase 4; `members_of()` / `get_group()`).
- Contact tracing moves into the layers (point 9, D11): `enable_tracing()` on
  a layer, the model's tracer and its shims removed, the consumers reading each
  layer's log with per-layer parameters. Within-group layers keep the history
  of moves and absences and answer `groupmates_of(agent, d1, d2)` (D11).
- Layer-membership builders (groupings drawn at random or by size) and
  membership I/O replace the entity distribution functions; remove
  entities (D6), including the entity fallback of `layer::Mixing`, the
  entity-level tools and events, and `Bubbles` as an entity (its complete ties
  become a within-group `Mixing` layer with `q = 1`, see 5.1). The measles models move to layers here (their
  reduced-contact state reads a model parameter, point 17).
- An example model: households (within-group `Mixing`) + workplace (`Network`)
  + public spaces (`Mixing` with a matrix).
- Acceptance: `ModelMeaslesMixingRiskQuarantine` (school traced by membership,
  community by log) is rebuilt on layers and stays draw-for-draw identical,
  which also shows the per-layer tracing design holds (the SEIR quarantine
  models are merged into one before 3d, see "Model consolidation"). Then a
  model with tracing at school and in the community with different success
  rates, as the first test of per-layer policies.
- Tests (end-to-end, about three files):
  - a within-group `Mixing` layer is the same run as the same layer built
    with the equivalent diagonal contact matrix; `q = 1` infects every
    housemate of a certain carrier; with `q < 1` a carrier meets
    Binomial(others, `q`) housemates (`48a`);
  - a combined model attributes transmissions to the expected layers (a
    layer with weight 0, or one that excludes the virus, records none).

### Model consolidation -- one model per compartmental structure (after the 3c-1 ports)

Settled 2026-10-25. Contacts now come from layers, so a model's states,
parameters and update rules describe the course of infection and its layers
describe who meets whom. A model can still carry several viruses, which share
its states. Models that differ only in where contacts come from go:

| Removed | Replaced by |
|---|---|
| `ModelSIRMixing`, `ModelSEIRMixing` | `ModelSIR`, `ModelSEIR` + a mixing layer (contact matrix) |
| `ModelSIRCONN`, `ModelSEIRCONN`, `ModelSIRDCONN`, `ModelSEIRDCONN` | `ModelSIR`, `ModelSEIR`, `ModelSIRD`, `ModelSEIRD` + a one-group mixing layer |
| `ModelSEIRMixingQuarantine`, `ModelSEIRNetworkQuarantine` | `ModelSEIRQuarantine`, on any layers, tracing each of them |

About 2,500 of the 5,426 lines in `include/epiworld/models/`, plus their
bindings in every wrapper. It also removes the `*CONN` models from the risk
"Layers a model ignores" (Section 9). SIS, SISD, SIRLogit, SURV, DiffNet and
the measles models stay: they differ in their states and rules, not only in
the contacts.

**Rules (from the user, 2026-10-25).**

- Where the old models differ, keep what agrees with epidemiological theory;
  the old implementations are not a reference to reproduce.
- Every number a user may set or calibrate is a model parameter (point 17).
- Where one model is more flexible than the other, the merged one keeps that
  flexibility.

**What the check found** (scratch harness on `phase3c-layer-tracing`,
2026-10-25; 10,000 agents):

- `ModelSIRMixing` and `ModelSIR` + the same layer are draw-for-draw identical
  (5 seeds).
- `ModelSEIRMixing` and `ModelSEIR` + the same layer differ only because
  `ModelSEIR` recovers with `runif() < "Recovery rate"`: it ignores the virus's
  recovery probability and the tools' recovery enhancers, unlike every other
  model. With that fixed (Infected uses `default_update_exposed`, as in
  `ModelSIR`) they are draw-for-draw identical (5 seeds). This is a bug on
  `master` too, fixed there first in its own PR (#327).
- `*CONN` models sample infectious contacts with replacement (rate `c / n`);
  a one-group mixing layer samples distinct ones (rate `c / (n - 1)`). The
  final sizes agree (contact rate 4, transmission 0.05, recovery 1/7, 300
  runs): SIR mean 4,246 vs 4,245 (sd 380 vs 378), SEIR 1,943 vs 1,950 (sd 410
  vs 396). But the layer costs 1.6 to 2 times the CPU time; in a callgrind
  profile, `Mixing::sample()` / `draw()` and the binomial draw per
  susceptible lead. Close that gap before the `*CONN` models go.

**Sampling with replacement goes** (settled 2026-10-25). A `*CONN` model
draws its contacts from the infectious agents with replacement, and a person
drawn twice enters the draw twice, so their chance of infecting goes up. That
is not a model of meeting someone more often: how many times a pair meets is
left to chance, and the user cannot set it. Repeated or more intense contact is
said explicitly, with a layer's weight or contact probability. Every layer
draws distinct contacts (D9).

**What the core needs first.**

1. **The contact level is a model parameter** (settled 2026-10-25). In the
   `*CONN` models "Contact rate" is a parameter (global events and calibration
   change it); in a layer the matrix is structure (point 17). A mixing layer
   can take its contact level from a parameter: its contacts are the parameter
   times the matrix (the `*CONN` replacement: a 1 x 1 matrix of 1 and "Contact
   rate"). This is the contact-level counterpart of `set_weight(param)`, which
   scales the odds, and it binds the same way: the layer keeps the parameter's
   name as a `ParamRef` (as `weight_ref` does) and reads the model's value
   at the end of every step, after the global events (in `update()`, which
   sets up the day's draws), never a copy taken when the layer is added. A
   global event or a calibration step that changes "Contact rate" therefore
   changes the contacts drawn on the next day. Acceptance test: a run whose global
   event sets "Contact rate" to 0 on day k has no new exposures after day k,
   while the same seed without the event has some.
2. **Which states transmit, said once** (settled 2026-10-25). A mixing layer
   lists its infectious and available states by id (`layer::mixing(name,
   infectious, available)`), and network sampling takes a separate exclude
   list (`make_update_susceptible({EXPOSED})`); a user adding a layer to
   `ModelSEIR` must know those ids. Instead, a model declares once which of
   its states transmit (in SEIR only Infected; Exposed carries the virus but
   does not), and every layer reads that: networks and mixing alike. Every
   other agent can be met. Taking agents out of a layer is participation (D2,
   Phase 4): exclusion, and leaky isolation as layer-targeted tools. Until then
   `ModelSEIRQuarantine` keeps its isolated, quarantined and hospitalized
   agents out of the layers, as the mixing quarantine model does today; D2
   replaces that mechanism.

   The interim contract, so a generic layer holds no eligibility policy:

   - The public constructor loses both state lists: `layer::mixing(name)`.
     A generic layer samples every agent in the group (everyone counts in the
     denominator), and only agents in the model's transmitting states infect.
   - Two cases stay distinct. *Non-transmitting* agents can be met but cannot
     infect (they still take a share of each susceptible's contacts).
     *Excluded* agents are not met at all: they leave the denominator, so the
     contacts go to the others. Only exclusion needs a policy.
   - Until D2, exclusion is private to `ModelSEIRQuarantine`: an internal
     (not public, not bound by the wrappers) setter on its layers lists the
     excluded states. In SEIRQuarantine: Susceptible, Exposed and Recovered
     are met and do not transmit; Infected is met and transmits; Isolated,
     Detected Hospitalized, Hospitalized, Quarantined Susceptible,
     Quarantined Exposed and Isolated Recovered are excluded. On a network
     layer exclusion means the tie is skipped. D2 removes the internal setter
     and the quarantine model moves to exclusion and layer-targeted tools.
3. **The one-group cost** (above). Done in the consolidation PR: a layer
   with a contact matrix sets up each day's binomial draws (one per pair of
   groups) in `update()`, instead of building the distribution at every
   draw. The random numbers are the same (the Mixing replacements stay draw
   for draw identical). One group, 10,000 agents, 100 days, 20 runs: 0.31 s
   vs 0.26 s CPU for `ModelSIRCONN` (was 1.6 to 2 times); the rest is the
   generic layer path (weights, targets, tracing), which the old model did
   not have.

**The merged models.**

- **Parameter names say what the code does**, in the terms epidemiology uses
  (settled 2026-10-25). Most are daily rates: the probability per day of
  leaving a state, so the mean time in it is 1 / rate days. One set for SIR,
  SEIR, SIRD, SEIRD and the quarantine model. Proposed (to confirm in the PR):

  | Now | Proposed | What the code does |
  |---|---|---|
  | "Transmission rate", "Prob. Transmission" | "Transmission probability" | Probability that one contact with an infectious agent infects (per contact, not per day). |
  | "Recovery rate", "Prob. Recovery" | "Recovery rate" | Daily probability of recovering while infectious; mean infectious period 1 / rate. |
  | "Incubation days", "Avg. Incubation days" | "Latent period" | Mean days in Exposed (daily probability 1 / period). Epidemiology calls the time until infectious the latent period; the incubation period ends at symptoms. |
  | "Contact rate" | "Contact rate" | Expected contacts per day. |
  | "Death rate" | "Death rate" | Daily probability of dying while infectious. |
  | "Hospitalization rate", "Hospitalization period" | unchanged | Daily probability; mean days in hospital. |
  | "Days undetected" | "Detection delay" | Mean days infectious before detection (daily probability 1 / delay; 0 detects at once, a negative value never). |
  | "Contact tracing success rate" | "Contact tracing probability" | Probability that a contact is traced (per contact). |
  | "Contact tracing days prior" | "Contact tracing window" | Days before detection whose contacts are traced. |
  | "Quarantine period", "Isolation period", the two willingness parameters | unchanged | Days; the probability that an agent complies (drawn once per agent). |

- `ModelSEIRQuarantine` keeps the network model's ten states ("Detected
  Hospitalized" separate from "Hospitalized"), samples through the generic
  path (so layer weights, virus targets, and push on networks work; the
  network model's own sampler ignored them), with only Infected transmitting
  (item 2). It traces every layer it is given. The mixing model's unused
  `day_exposed` goes.
- The `*CONN`-only `generation_time_expected()` and `get_n_infected()` go
  (no wrapper binds them, no test uses them).

**Order.** The `ModelSEIR` recovery fix on `master` (#327, merged into
`v1-layers`); one epiworld PR with the core items and the consolidation; then
the wrapper ports. Then 3c-2.

**Tests** (end-to-end, in the suite): `ModelSIR`/`ModelSEIR` + a matrix
layer reproduces the histories the Mixing models gave, draw for draw
(recorded before they go); the "Contact rate" global-event test (item 1);
`ModelSEIRQuarantine` on a network plus a mixing layer quarantines contacts
from both.

**Acceptance check** (run once in the consolidation PR, with the old models
still in the tree; the script and its output go in the PR description). For
each replacement -- SIRCONN, SEIRCONN, SIRDCONN, SEIRDCONN, and each old
quarantine model against `ModelSEIRQuarantine` on the equivalent layer:

- 1,000 runs per model, seeds 1 to 1,000, 10,000 agents, 100 days, the
  parameters of the measurement above (quarantine models: their test-suite
  parameters).
- Statistics: final size (agents ever infected) and peak prevalence.
- Pass: for each statistic, the standardized mean difference
  |mean_old - mean_new| / pooled sd is at most 0.1, and the sd ratio is
  between 0.9 and 1.1.
- The CONN replacements must pass (the switch to distinct contacts changes
  the rate from c / n to c / (n - 1), which is within this threshold at these
  sizes). A quarantine model may fail only where the difference is traced to
  a deliberate change listed above (only Infected transmits, the generic
  sampler); the PR reports the failing statistic and its cause, and the user
  decides.

Result (consolidation PR, 2026-10-27): every replacement passes. The
quarantine models ran with realistic parameters rather than the test suite's
(transmission 1, which infects everyone at once): transmission 0.1, latent
period 4, recovery 1/7, hospitalization 0.01 for 7 days, detection delay 3,
quarantine 14 days, willingness 0.8 and 0.8, isolation 10 days, tracing 0.8
over 4 days; one mixing group with 4 contacts a day, or a small-world network
(k = 8, p = 0.1). The largest standardized difference is 0.084 (SIRD final
size); sd ratios 0.945 to 1.012. The mixing quarantine model is draw for draw
identical to `ModelSEIRQuarantine` on the same layer.

### Phase 4 -- participation, tools, schedules, persistence

- Per-agent exclusion flags (end-of-step), layer-targeted tools (D2, D7),
  `isolate(agent, keep = ...)` (which writes an absence to the within-group
  history, D11), global event to scale/close a layer.
- Schedules (D10), persistence (D8), membership changes notified to the layer
  queue.
- Bubbles: exclusion from other layers (the bubble layer itself is Phase 3).
- An example model: the Phase 3 model plus a transit layer (morning and evening
  trips, route pools, `q < 1`, weekdays only). Another: an ERGM-style rewirer
  (tie dissolution plus Metropolis-Hastings toggles on given coefficients;
  estimation stays in R's `ergm`/`tergm`).
- Tests (end-to-end):
  - deterministic -- with `p = 1`, an isolated infectious agent infects only
    household members; a closed layer records no transmission;
  - a tool targeting a layer reduces transmission in that layer only;
  - schedules and persistence -- no transit-layer transmission on inactive
    steps; with `q = 1` every transit transmission is between members of the
    same usual group, with `q = 0` transmissions also cross groups in a pool;
  - when part of a group is excluded, a matrix layer's contact rate stays
    constant and a within-group layer's probability per pair does.

### Phase 5 -- push for mixing layers

- `sample_reverse()` for `Mixing` (both forms); per-layer push/pull choice
  and cost model (revisit `EPI_TRANSMISSION_AGENT_COST` and kappa).
- Benchmark: push vs pull for mixing layers early in an outbreak.

### Model registry (#312) -- after Phase 3

- Once entities are gone, model construction is final: land the registry
  (`make_model()` / `list_models()`) on `v1-layers`, with the measles models
  registered too.
- The wrappers replace their per-model bindings with the registry (one
  generic constructor); epiworldjs drops its own `cpp/registry.hpp`.

### Phase 6 -- independent exposures, 1.0.0, bindings, docs

- Switch the combination rule to independent exposures (D1) in one commit that
  updates the expected results; add the analytic P(infection) test.
- Merge `v1-layers` into `master`; release 1.0.0.
- Wrappers: the port of Phase 6 updates expected values for the new rule; then
  each wrapper's `v1-layers` merges into its `main` and is released after
  epiworld 1.0.0 (measles after epiworldR is on CRAN).
- Docs: new `docs/impl/contact-layers.md`; update
  `mixing-and-entity-distribution.md`, `queueing-system.md`,
  `transmission-sampling.md`; migration guide.

---

## 7. Testing conventions (from AGENTS.md)

- One `EPIWORLD_TEST_CASE` per file; letter suffixes for related cases; list
  new files in `tests/Makefile`.
- End-to-end: build a model, run it, assert on histories, transition matrices,
  transmissions, edgelists. Refactor phases are tested by unchanged results.
- `model.run()` for deterministic tests, `run_multiple()` for statistical ones.
- Run in the devcontainer; also build with `-DEPI_DEBUG`, since `make test`
  skips those checks.

## 8. Benchmarks

- Phase 0: rule overhead on real models.
- Phase 2: no regression for pull/push on network models; `reset()` cost per
  replicate with the dirty flag vs the full agent backup.
- Phase 3: overhead of a single-layer model through the multi-layer path vs the
  Phase 2 path (to decide whether a fast path must stay).
- Phase 3b: cost per step of a within-group layer of 100k households
  (`q = 1`), vs a network of the same households as cliques. Measured
  (2026-10-23, devcontainer, g++ -O2, CPU time, 8 interleaved runs of 60 days;
  households of 1-6, 349,602 agents, SIR with transmission 0.3): storage
  349,602 group ids vs 1,164,448 adjacency entries; per run 12 vs 9 ms at
  0.1% initial prevalence, 145 vs 170 ms at 5%, 488 vs 515 ms at 20%; the same
  final sizes. The within-group layer is the one way to give households.
- Phase 4: cost per step of a within-group layer as `q` falls (membership
  churn and queue notifications), at transit-like sizes (groups of 50-200,
  two transit layers per commuter).
- Phase 5: push vs pull for mixing layers early in an outbreak.
- Use CPU time and interleave variants; wall-clock on the macOS host is
  unreliable.

## 9. Risks

- **Random stream changes.** Any change of draw order changes results. Phases
  1-2 must be draw-for-draw identical; the rule switch is isolated in Phase 6.
  The one exception: 3b's distinct-contact sampler (D9) changed the results of
  matrix mixing layers, and so of the mixing models.
- **Cloning and reset.** Layers behind `shared_ptr` must be deep-copied in
  `Model`'s copy constructor/assignment and `clone_ptr()` of every model, or
  `run_multiple()` shares state across threads. Every mutator must set the
  dirty flag; a path that edits a layer without it would leak state between
  replicates (cf. #311). Test it directly.
- **Hidden single-network assumptions** in models and global events (Phase 2).
- **Entities removal** (end of Phase 3) touches the measles models, Bubbles,
  and the entity-level tools and events; it makes Phase 3 the second-largest
  phase.
- **Integration branch drift.** `v1-layers` must merge `master` regularly, or
  the final merge is painful.
- **Binding churn** across four packages, spread over the phases by the
  per-phase wrapper ports; the `layer` column and moved accessors break positional
  readers.
- **Callables across the language boundary.** Generators, rewirers and
  user-defined layers written in R, Python or JS (calls back into the
  interpreter from inside a run, and from `run_multiple()` threads). The
  wrappers expose only the built-in generators until a phase needs more.
- **Layers a model ignores.** Models that keep their own sampler accept any
  layer but read only some: the connected models (`*CONN`) draw from the list
  of infected agents and ignore every layer (they go in the model
  consolidation); SIRLogit, SURV and the SEIR network-quarantine model read
  network layers only; the measles mixing models
  read their own mixing layer until 3d. An added layer then silently carries
  no transmission. By the end of Phase 3 each such model either moves to the
  generic path or rejects, in `add_layer()`, the layers it does not read; the
  wrappers document the gap until then.
- **Membership churn.** With low `q`, many memberships change every step; the
  queue notifications and group member lists must stay O(changes), not
  O(population).
- **Mixing push (D4).** Until Phase 5, a public mixing layer is pulled for
  every susceptible each step.

## 10. References

- Starsim source: `starsim/networks.py` (`Route`, `MixingPool`) and
  `starsim/diseases.py` (`Infection.infect`, `infect_route`,
  `finalize_infections`) -- <https://github.com/starsimhub/starsim>
- Starsim networks tutorial --
  <https://docs.idmod.org/projects/starsim/en/latest/tutorials/tut_networks.html>
- OpenABM-Covid19 documentation --
  <https://github.com/BDI-pathogens/OpenABM-Covid19/blob/master/documentation/covid19.md>
- Hinch et al. (2021), OpenABM-Covid19, PLOS Computational Biology --
  <https://doi.org/10.1371/journal.pcbi.1009146>
- EpiModel multilayer networks: `R/net.mod.infection.R` (`discord_edgelist`),
  `R/net.inputs.R` -- <https://github.com/EpiModel/EpiModel>
- JUNE commuting (checked in the source): `june/geography/station.py`
  (`get_commute_subgroup`), `june/geography/city.py`,
  `june/activity/activity_manager.py` -- <https://github.com/IDAS-Durham/JUNE>;
  JUNE-Germany -- <https://arxiv.org/abs/2303.05742>
- Sun, Axhausen, Lee, Huang (2013), Understanding metropolitan patterns of daily
  encounters, PNAS 110(34) -- <https://doi.org/10.1073/pnas.1306440110>
- Mueller (2025), From traffic to transmission: adapting an agent-based
  transport model to simulate the spreading of infectious diseases (MATSim-EpiSim),
  TU Berlin --
  <https://depositonce.tu-berlin.de/items/0ea18ef0-742d-4df0-b3fc-b4df4af97ada/full>
- Hiermann et al. (2025), EpiSim coupled with a capacity-constrained public
  transport flow model (Munich) -- <https://arxiv.org/abs/2511.06377>
- Covasim, FRED, FluTE, seirsplus, and the place-based summary of JUNE:
  described from memory; verify before relying on details.

---

## 11. Runbook: resuming the work

This section is for whoever picks the work up next, human or AI agent, in a
fresh session. With this plan, the open pull requests and the repositories,
it says what to do next. Read Sections 3 (settled decisions) and 6 (phases)
first; this section says how to carry them out.

### 11.1 Where the truth lives

- **This plan**: decisions and the order of work. It lives on epiworld's
  `v1-layers` branch as `contact-layers.md` (its review thread is PR #302).
- **GitHub**: what is done and what is in flight. The status table (11.6) is a
  summary; when it disagrees with GitHub, GitHub wins, and the next epiworld
  PR fixes the table.
- **Each wrapper's `VERSIONS` file** (11.3): the epiworld commit it vendors on
  its `v1-layers` branch.

### 11.2 Deciding the next step

Gather the state:

```sh
for r in epiworld epiworldR epiworldpy epiworldjs measles; do
  echo "== $r"; gh pr list -R UofUEpiBio/$r --base v1-layers --state open
done
gh pr list -R UofUEpiBio/epiworld --base v1-layers --state merged --limit 10
git -C <epiworld checkout> log --oneline origin/v1-layers -10

# The epiworld commit each wrapper vendors, on its v1-layers branch
for f in epiworldR:inst/include/VERSIONS epiworldpy:epiworldpy/include/VERSIONS \
         epiworldjs:vendor/VERSIONS measles:inst/include/VERSIONS; do
  echo "-- ${f%%:*}"
  gh api "repos/UofUEpiBio/${f%%:*}/contents/${f#*:}?ref=v1-layers" \
    -q .content 2>/dev/null | base64 -d 2>/dev/null | grep '^epiworld' ||
    echo "no VERSIONS file: not caught up yet"
done
```

Then take the first of these that applies, do it, and stop:

1. **An open PR into `v1-layers`** (any repository) has unanswered review
   comments or failing CI: fix it, push, and reply to each comment with what
   changed. A review finding in vendored code is fixed in epiworld (its own
   PR into `v1-layers`), then re-vendored.
2. **A wrapper is behind.** Let M be the merge commit of the latest phase PR
   merged into epiworld `v1-layers`. A wrapper is behind when its vendored
   commit is not M or a descendant (`git merge-base --is-ancestor M <sha>`
   fails). Go through the wrappers in order -- epiworldR, epiworldpy,
   epiworldjs, measles -- and port the phase to the first one that is behind
   and has no open port PR (11.4). The next wrapper may start once the
   previous one's PR is open with no unanswered comments; measles waits until
   epiworldR's port has **merged**.
3. **Every wrapper is on M** (or has its port open with nothing to address):
   start the next C++ phase (the first row of 11.6 not yet merged), as
   Section 6 describes, within the one-phase lag rule.
4. **Everything waits on the user's merges**: report what is waiting and stop.

Rules for the agent:

- Never merge, close, or force-push a PR or a shared branch; the user merges.
  Never push to `master`/`main`.
- One PR at a time. After opening or updating one, report its link, what it
  does, and what comes next.
- When a step needs a design decision the plan does not settle, stop and ask.
- Follow each repository's `AGENTS.md`/`CLAUDE.md` (devcontainer builds, test
  conventions, amalgam regeneration in epiworld).

### 11.3 The repositories

Each one has a devcontainer (`.devcontainer/`); build and test in it.
Suggested checkouts are siblings of the epiworld checkout; use a worktree per
PR.

| Repository | Vendor epiworld | Records it in | Build and test (in the container) |
|---|---|---|---|
| epiworld | -- | -- | `make test WITH_OPENMP=0`; `make build/epiworld.hpp` (commit it) |
| epiworldR | `make update` (defaults to `v1-layers` on that branch) | `inst/include/VERSIONS` | `Rscript -e 'cpp11::cpp_register(); roxygen2::roxygenise()'`; `R CMD INSTALL .`; tinytest on `inst/tinytest`; `R CMD build` + `R CMD check --no-manual` |
| epiworldpy | `make update EPIWORLD_BRANCH=v1-layers` | `epiworldpy/include/VERSIONS` | `uv pip install .`; `pytest` |
| epiworldjs | `make update-vendors EPIWORLD_REF=v1-layers` (measles from epiworld's `include/measles`) | `vendor/VERSIONS` | `make check-version test`; `npx playwright test` before a release |
| measles | `make update EPIWORLD_BRANCH=v1-layers` (`include/measles`) | `inst/include/VERSIONS` | install epiworldR from its `v1-layers` first; tinytest; `R CMD check --no-manual` |

**The pre-merge check** (Section 6, step 1) is `.github/workflows/downstream.yml`:
on every PR into `v1-layers`, one job per wrapper checks out the wrapper's
`v1-layers`, replaces its vendored headers with the PR's, and builds and
tests it (non-blocking). Read its results before the C++ PR merges and report
what breaks in the PR; a failure that a port can fix is expected, one that
needs a different C++ API is not. No wrapper code is written at this stage.
To reproduce a job locally, vendor the PR's branch
(`EPIWORLD_BRANCH=<pr branch>`), build, test, and discard the changes.

**A wrapper's first PR (catch-up)** also sets it up: on its `v1-layers`, the
Makefile defaults to epiworld `v1-layers`; the version is `1.0.0-0` (measles:
`0.5.0-0`, `epiworldR (>= 1.0.0)`, `Remotes: UofUEpiBio/epiworldR@v1-layers`);
CI runs on `v1-layers` (and `please-bump` does not fire there); the
`VERSIONS` file is written by the vendor target; and its `AGENTS.md` says
that on `v1-layers` this plan is the source of truth.

### 11.4 A wrapper port PR

- **Branch** `port-<epiworld branch>` off the wrapper's `v1-layers` (e.g.,
  `port-phase3b-within-group-mixing`); the catch-up is `v1-catchup`.
- **Title** `Port epiworld <phase> (UofUEpiBio/epiworld#N)`; the body starts
  with `Ports UofUEpiBio/epiworld#N; vendors <sha>.` so the state can be read
  from GitHub.
- **Content**: vendor M; bind what the phase adds, mirroring the C++ API
  (rule 18; no sugar the core lacks); turn what it removes into defunct stubs
  (R: the table in `R/epiworldR-defunct.R`; Python: `epiworldpy/_defunct.py`;
  JS: throw); rewrite the affected tests (few, end-to-end), examples and the
  vignettes the checks build; a NEWS entry under the `1.0.0-0` development
  version. Names and messages follow the epiworldR port of the same phase.
- **Done when** the wrapper's full test suite and check pass in its
  container; then open the PR into the wrapper's `v1-layers`.

### 11.5 What each phase means for the wrappers

| Phase | Wrapper work |
|---|---|
| 1-2 (catch-up) | Population in the constructors (`n`); layer objects (`layer_network(name, graph_*())`, `add_layer()`, `get_layer()`); network edits and edge lists on the network layer; contact matrix on the mixing layer; `Bubbles` names its layer; `agents_smallworld()` & co. defunct. |
| 3a | `layer` column in the transmissions; `get_layer()` by id; `set_weight(layer, param)`; `add_virus_target(layer, virus)`. |
| 3b | Bind `layer::Mixing` for user-added layers: one constructor with the groups and exactly one of a contact matrix and contact probabilities (vector or one number, `1` for cliques); `set_contact_prob()` / `get_contact_prob()`; `members_of()`, `get_group()`. The queue is internal. Expected values of the mixing models' tests change (distinct contacts, D9). |
| 3c | Per-layer `enable_tracing()` / `is_tracing()` / `get_tracing()` and its logs (3c-1); `groupmates_of()` on within-group layers (3c-2); whatever the wrapper binds of the model-level tracer (e.g., the quarantine models' parameters) follows the core, and removed accessors become defunct. |
| Consolidation | The removed models (`ModelSIRMixing`, `ModelSEIRMixing`, the four `*CONN`, the two SEIR quarantine models) become defunct stubs naming the replacement (the plain model + `add_layer()`; `ModelSEIRQuarantine`); the parameter that sets a mixing layer's contact level; tests and the vignettes that use them (epiworldR: getting-started, advanced-modeling, mixing, run-multiple, implementation) rewritten. epiworldjs: registry entries. measles: unaffected. |
| 3d | Membership builders (replacing the entity distribution functions). Entities removed: R `src/entities.cpp`, the entity R functions and their tests (`test-entity_tools.R`, `test-distribute-to-set.R`); Python `entity.cpp`; JS registry entries -- all defunct stubs pointing to the layer-membership builders. Mixing layers take memberships instead of entities. measles package: its models move to layers, so its constructors, tests and vignettes are rewritten. |
| Registry (#312) | Per-model bindings replaced by `make_model()` / `list_models()`; epiworldjs drops `cpp/registry.hpp`; measles registers its models. |
| 4 | Exclusion / `isolate(agent, keep)`, layer-targeted tools, schedules, persistence. |
| 5 | Re-vendor only, unless a push/pull setting becomes public. |
| 6 | Expected values for the new combination rule; vignettes and migration guide; release in order (epiworld 1.0.0, then epiworldR, epiworldpy, epiworldjs, then measles after epiworldR is on CRAN). |

### 11.6 Status

Updated by every epiworld PR into `v1-layers` (last: 2026-10-28).

| Step | epiworld | epiworldR | epiworldpy | epiworldjs | measles |
|---|---|---|---|---|---|
| Phase 0 (benchmark) | #315 merged | -- | -- | -- | -- |
| Phases 1-2 | #316, #317 merged | #216 merged (catch-up through 3a) | #33 merged (catch-up through 3a) | #18 merged (catch-up through 3a) | #65 merged (catch-up through 3a) |
| Phase 3a (+ fixes) | #318, #319 merged | #216 | #33 | #18 | #65 |
| `downstream.yml` | #321 merged | | | | |
| Phase 3b | #323 merged (plan: #322; checked accessors: #324 merged) | #218 merged | #35 merged | #19 merged (re-vendor only: no layer objects in JS) | #66 merged (re-vendor only) |
| Phase 3c-1 (layer logs) | #325 merged (+ #329 merged: `ContactTracing::get_n_agents()`) | #220 merged | #36 merged | #20 merged (re-vendor only) | #67 (re-vendor only, with the consolidation) |
| `master` -> `v1-layers` | #328 merged (#327, the `ModelSEIR` recovery fix) | | | | |
| Model consolidation | #331 merged (plan: #326; wording: #332) | #222 merged | #37 merged | #21 merged | #67 (re-vendor only) |
| Phase 3c-2 (group history) | this PR | | | | |
| Phase 3d | not started | | | | |
| Registry (#312) | not started | | | | |
| Phases 4, 5, 6 | not started | | | | |

**Next** (as of the date above): the 3c-2 ports (`groupmates_of()` on the
mixing layer in epiworldR and epiworldpy; re-vendor in epiworldjs and
measles), then 3d.
