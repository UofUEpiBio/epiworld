# Contact layers: combining networks and mixing in one model

**Status:** Phases 0 and 1 done (#315, #316); Phase 2 next. Written
2026-10-08; revised 2026-10-19 after the design review (layers replace the
in-`Agent` network and entities; per-layer queues; layer weights and virus
targets; release as 1.0.0 on `v1-layers`), and again the same day (no
compatibility shims; model parameters are the single source of truth for
scalars). Revised 2026-10-21 before Phase 2 (models create no layers;
population in the constructor; networks come from generators; rewiring in the
layer).
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
   mixing-type (or affiliation-type, Section 5.1). There is **no network
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
    and fresh mixing sits the *affiliation* layer (Section 5.2): stable venue
    memberships with a persistence probability `q`.
13. **Layers can change over time**: schedules (weekdays), membership changes.
    All changes take effect at the end of a step (like state changes).
14. **Contact rate does not shrink when people leave a layer.** If classmates
    are quarantined, the remaining children keep contacting other children at
    the same rate. The denominator policy lives in one function so that this can
    be revisited (D2).
15. **Versioning: this is epiworld 1.0.0**, with breaking changes allowed
    (layer column in outputs, contact-matrix accessors move to the layer, entities removed).
    Work happens on an integration branch, `v1-layers` (Section 6).
16. **Combination rule.** Refactor phases keep today's roulette so existing
    results must stay identical; a dedicated step (Phase 6) switches the rule
    to independent exposures (D1) and updates expected results.
17. **Model parameters are the single source of truth for scalars.** A layer
    never stores its own copy of a number a user may calibrate or change: its
    weight, a default contact rate, a persistence `q`, a within-venue `k`, a
    reduced-contact factor are named model parameters, which the layer reads
    with a `ParamRef` (the same way `Virus::set_prob_infecting("name")` works).
    So calibration (LFMCMC), `set_param()` and global events reach layers with
    no layer-specific code, and parameters keep their usual semantics (they are
    not part of the layer's baseline or dirty flag). What is structure, not a
    scalar, stays in the layer: contact matrices, ties, groupings, memberships.
    A layer may still default a parameter it needs (adding it to the model when
    absent), the way models add theirs in their constructors. The line between
    the two: what a layer reads **every step** (weight, contact rates, `q`, `k`,
    rewiring rate) is a model parameter; what is used **once per draw** (the
    arguments of a graph generator) is structure, like a contact matrix. A
    generator that should follow a parameter reads it itself (it receives the
    model).
18. **No compatibility shims.** The 1.0.0 API lives where the data lives: the
    model does not forward layer methods (no `model.set_contact_matrix()`; it
    is `model.get_layer_as<layer::Mixing<>>("mixing").set_contact_matrix()`),
    and agents do not forward network methods (D3). The bindings follow the
    same rule and move with the core: each wrapper has its own `v1-layers`
    branch, updated by one companion PR per phase (Section 6). An API the core
    removes becomes a defunct stub in the wrappers (it errors and names the
    replacement), never a working alias.
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

Contact rate when agents leave (settled, point 14): the rate stays constant, so
a mixing layer divides by the number of agents *present*, not the original
count. For network layers the contacts with an excluded agent are simply lost
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

### D8. Persistence model for affiliation memberships

Section 5.2 proposes "usual venue with probability `q`, otherwise a random venue
in the same pool", with `q` per layer. Alternatives: `q` per agent (strict vs
irregular commuters), or a user-supplied assignment function
`venue(agent, step)` for data-driven schedules. Recommendation: per-layer `q`
plus an optional per-agent override, and the assignment function as the
general escape hatch.

### D9. Within-venue contact rate

A fixed `k` per layer, `k` as a function of venue size, or `k` per venue
(crowded peak trips vs empty off-peak ones). Recommendation: per-venue `k`
with a per-layer default; it costs one value per venue and lets capacity and
crowding studies set it directly.

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
- **Structural tracing.** "Notify the whole class" needs no log: `Affiliation`
  exposes `members_of(group)` and `groups_of(agent)`, and the measles "high
  risk" branch (shares an entity with the case) becomes that query. A network's
  current ties are not the contacts someone had last week, and
  `ModelSEIRNetworkQuarantine` traces from the log, so structural tracing is an
  option next to the log, not a replacement.
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
| `layer::Mixing` | contact matrix across a few large groups | group id per agent + `G x G` matrix | age groups in public spaces |
| `layer::Affiliation` | mixing **within** venues an agent belongs to; memberships may persist or change | venues per agent (CSR) + members per venue + within-venue contact rate | households, classrooms, transit trips |

`Affiliation` is a mixing-type layer with a block-diagonal matrix. It exists
because a dense matrix cannot hold 100k households, because it can express
recurring cohorts (Section 5.2), and because, unlike `Mixing`, it supports the
queue (an infectious agent reaches only the members of its venues).
`Bubbles` with `BubbleTies::Complete` (cliques added and withdrawn) becomes an
`Affiliation` layer instead of edits to the network (Phase 3).

Recurrence, from most to least, across the three kinds: `Network` (same
individuals), `Affiliation` with `q = 1` (same cohort), `Affiliation` with
`q < 1` or one large venue per route (same pool), `Mixing` (none).

### 5.2 Affiliation layers, recurrence, and schedules

**Venues and memberships.** An affiliation layer holds venues (a household; the
7:40 departure of route 12) and, per agent, a short list of venues, stored
compactly (CSR: offsets + venue ids; members per venue likewise). An agent may
hold several memberships in one layer (morning trip and evening trip), each a
separate exposure.

**Contacts within a venue.** Each step, an agent makes about `k` contacts among
the members of each of its venues who are present (not excluded), drawn fresh:
a binomial number of draws from the venue's infectious members with probability
`k / (number of present members - 1)`, as the `Mixing`
sampler does per group. Recurrence is at the cohort level: you meet the same
crowd, not necessarily the same seatmate. Dyad-level recurrence (same seatmate)
belongs in a `Network` layer.

`k` can be a single rate for the layer or a function of venue size (a
household of 4 vs a full train car); for households, `k = size - 1` makes the
venue a clique.

**Persistence.** Each agent has a *usual* venue per membership slot, and venues
are grouped into *pools* (a route, a station, a school). Each step, with
probability `q` the agent attends its usual venue; otherwise it attends another
venue drawn from the same pool:

- `q = 1`: fixed cohorts (households, classrooms, strict commuters);
- `0 < q < 1`: familiar strangers (mostly the same train, sometimes another);
- `q = 0`: random vehicle at your station each step (JUNE);
- one venue per pool: route-level mixing.

Only agents who switch venue cost anything per step (`O(riders x (1 - q))`
membership updates), plus `O(k)` per infectious member. A membership change
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
both directions; contact rates of the others stay constant (point 14). Partial
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

- Each queue-capable layer (`Network`, `Affiliation`) keeps its own queue:
  a carrier queues its ties (or the members of its venues) in that layer; a
  tie or membership change notifies that layer's queue (as `add_edge()` /
  `rm_edge()` do today, `Queue::notify_edge_added/removed`).
- A susceptible is sampled in a layer only if queued there (or if the layer has
  no queue, e.g. `Mixing`, until push exists, D4).
- Phase 2 keeps today's single queue; an agent's count covers its ties in every
  network layer (counts add across layers). Per-layer queues come in Phase 3.
- Excluded agents (flag) simply produce no contacts; the queue stays
  conservative.

### 5.8 Push and pull

- Phases 1-4: push only for a model whose only layer is one undirected
  network layer (today's behavior). Everything else pulls.
- Phase 5: `sample_reverse()` for `Mixing` (for infectious `j` in group `g`,
  contacts in group `h` ~ Binomial(available in `h`, c(h,g) / available in
  `g`), same pair probabilities as pulling) and `Affiliation`; each layer
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

**Wrappers move with the core.** epiworldR, epiworldpy, epiworldjs and the
measles R package each keep a `v1-layers` branch (version `1.0.0-0`; measles
its own next minor, depending on epiworldR's branch), so a binding problem
shows up while the C++ API can still change, not in Phase 6. Their `main`
branches keep tracking epiworld `master`.

- A first catch-up PR per wrapper brings it to Phases 1-2 (population in the
  model constructors, layer objects, network and contact-matrix methods on the
  layers).
- Every phase PR into `v1-layers` gets a companion PR with the **same branch
  name** in each wrapper it breaks. A CI job in epiworld (`downstream.yml`,
  non-blocking) builds each wrapper (the same-named branch if it exists, else
  its `v1-layers`) against the PR's headers and runs its tests, so a phase
  shows what it breaks before it merges.
- Merge order: the epiworld PR, then the companion PRs re-vendored from the
  merged commit (the wrapper records it), then measles after epiworldR.
  `master` -> `v1-layers` merges here are mirrored by `main` -> `v1-layers`
  merges in the wrappers.
- The wrappers mirror the core API and add none of their own. What the core
  removes becomes a defunct stub (R `.Defunct()`, collected in one file and
  one help page; Python and JS raise or throw), with one message table per
  wrapper that also feeds the migration notes. The stubs stay through 1.x.
- Every wrapper vendors epiworld and measles headers from the same epiworld
  commit.

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
- **3b. `layer::Affiliation` and per-layer queues.**
- **3c. Contact tracing into the layers** (point 9, D11).
- **3d. Entities removed, measles models and the example model on layers**
  (the acceptance tests of D11).

The list below is the whole of Phase 3.

- The accumulator (5.4) for every kind of layer (Phase 2 does networks);
  layer weights; virus targets per layer; per-layer queues (5.7); layer id in
  transmission records, contact tracing and post-sampling; `layer::Affiliation` with fixed memberships (`q = 1`, several
  memberships per agent, `members_of()` / `groups_of()` queries); per-layer
  edgelist and membership I/O.
- Contact tracing moves into the layers (point 9, D11): `enable_tracing()` on
  a layer, the model's tracer and its shims removed, the consumers reading each
  layer's log with per-layer parameters.
- Layer-membership builders replace the entity distribution functions; remove
  entities (D6), including the entity fallback of `layer::Mixing`, the
  entity-level tools and events, and `Bubbles` as an entity (it becomes an
  `Affiliation` layer). The measles models move to layers here (their
  reduced-contact state reads a model parameter, point 17).
- An example model: households (`Affiliation`) + workplace (`Network`) + public
  spaces (`Mixing`).
- Acceptance: `ModelMeaslesMixingRiskQuarantine` (school traced by membership,
  community by log) and the SEIR mixing and network quarantine models are
  rebuilt on layers and stay draw-for-draw identical, which also shows the
  per-layer tracing design holds. Then a model with tracing at school and in the
  community with different success rates, as the first test of per-layer
  policies.
- Tests (end-to-end, about three files):
  - an `Affiliation` layer with `q = 1` matches the equivalent
    complete-graph-per-household network in distribution;
  - a combined model attributes transmissions to the expected layers (a
    layer with weight 0, or one that excludes the virus, records none).

### Phase 4 -- participation, tools, schedules, persistence

- Per-agent exclusion flags (end-of-step), layer-targeted tools (D2, D7),
  `isolate(agent, keep = ...)`, global event to scale/close a layer.
- Schedules (D10), persistence and per-venue contact rate (D8, D9), membership
  changes notified to the layer queue.
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
    same usual venue, with `q = 0` transmissions also cross venues in a pool;
  - contact rate stays constant when part of a mixing group is excluded.

### Phase 5 -- push for mixing and affiliation layers

- `sample_reverse()` for `Mixing` and `Affiliation`; per-layer push/pull choice
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
- Wrappers: the companion PR updates expected values for the new rule; then
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
- Phase 4: cost per step of an affiliation layer as `q` falls (membership
  churn and queue notifications), at transit-like sizes (venues of 50-200,
  two memberships per commuter).
- Phase 5: push vs pull for mixing layers early in an outbreak.
- Use CPU time and interleave variants; wall-clock on the macOS host is
  unreliable.

## 9. Risks

- **Random stream changes.** Any change of draw order changes results. Phases
  1-2 must be draw-for-draw identical; the rule switch is isolated in Phase 6.
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
  companion PRs; the `layer` column and moved accessors break positional
  readers.
- **Callables across the language boundary.** Generators, rewirers and
  user-defined layers written in R, Python or JS (calls back into the
  interpreter from inside a run, and from `run_multiple()` threads). The
  wrappers expose only the built-in generators until a phase needs more.
- **Membership churn.** With low `q`, many memberships change every step; the
  queue notifications and venue member lists must stay O(changes), not
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
