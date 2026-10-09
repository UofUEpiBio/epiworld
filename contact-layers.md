# Contact layers: combining networks and mixing in one model

**Status:** plan, not started. Written 2026-10-08 from a design discussion.
**Goal:** let a single model draw transmission from several contact sources at
once -- e.g., mixing within households, a workplace network, and mixing in
public spaces -- each with its own structure and its own participation rules.

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
| Participation is all-or-nothing by state, per model (e.g., "available states"); the measles sampler hard-codes one reduced-contact state | `sampler-mixing.hpp`, `measles/samplermixing.hpp` |
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

1. **One generic contact layer type; no built-in network.** A layer is
   network-type or mixing-type. A mixing model has no network layer, a network
   model has no mixing layer, and a model may combine several of either (or have
   none, meaning no transmission).
2. **Each layer owns a sampler** that answers "whom did agent `i` contact this
   step?". A network layer returns the agent's ties; a mixing layer samples
   from the population and returns the same kind of list -- as if it were a
   random graph (a stochastic block model) redrawn every step, of which only the
   edges touching infectious agents are ever realized.
3. **Layers own their groupings.** A mixing layer takes a group id per agent; it
   does not read `Agent::entities[0]`. Helpers can build the grouping from
   entities.
4. **Repeated contacts are repeated exposures.** The same pair appearing twice
   (twice in a layer, or once in each of two layers) counts twice. That is
   intended: they see each other more often.
5. **No per-virus, per-layer beta.** Viruses keep their transmission
   probability; a layer's intensity comes from its contact rate, repetitions,
   and participation weights.
6. **Per-agent, per-layer participation is a weight in [0, 1]** (not on/off),
   set by policy. Isolation with the flu: weight 0 in work, school, and public
   layers; weight 1 in the household layer. Exclusion is enforced inside each
   layer's sampler, in both directions (an agent with weight 0 is neither drawn
   nor draws).
7. **`get_neighbors()` applies to network layers only**, per layer, through a
   non-allocating view. No `std::vector<std::pair<...>>` in the C++ core; the
   bindings can expose a named list.
8. **Recurrence is a first-class property of a layer.** Between a fixed network
   (same individuals every day) and mixing (fresh contacts every step) sits the
   *affiliation* layer: agents belong to venues (a household, a classroom, the
   7:40 train on route 12), possibly several per layer (the morning and the
   evening trip), and contact co-members drawn fresh each step. Recurrence comes
   from stable membership, controlled by a **persistence** probability `q`
   (Section 5.2). Households and classrooms are the case `q = 1` with one
   membership per agent.
9. **Layers can change over time.** Layers follow schedules (transit and work on
   weekdays only; a remote-work day sets that agent's weight to 0), and
   affiliation memberships can change between steps. All such changes take
   effect at the end of a step, like state changes.

---

## 4. Open decisions

### D1. Combination rule: roulette (odds) or hazard

Today `roulette()` (`include/epiworld/misc.hpp`) treats exposures as independent
Bernoullis **conditioned on at most one success**:

    P(no infection) = 1 / (1 + R),  R = sum_j r_j,  r_j = p_j / (1 - p_j)
    P(infected by j) = r_j / (1 + R)

The alternative keeps a single infector but drops the conditioning (competing
risks):

    lambda_j = -log(1 - p_j)
    P(no infection) = exp(-sum_j lambda_j) = prod_j (1 - p_j)
    P(infected by j) = (lambda_j / sum lambda) * (1 - exp(-sum lambda))

Both are additive (sum of odds or sum of hazards), so composing layers, the
reservoir sampling of the infector, and push all work the same way with
either. They agree for one infectious contact and diverge as contacts pile up,
which is exactly what stacking layers does:

| Infectious contacts | Roulette | Hazard / independent |
|---|---|---|
| 1 x p=0.5 | 0.500 | 0.500 |
| 2 x p=0.5 | 0.667 | 0.750 |
| 3 x p=0.3 (household) | 0.563 | 0.657 |
| 10 x p=0.1 | 0.526 | 0.651 |

Cost, per (susceptible, infectious contact) pair, microbenchmark (CPU time,
4M values, 20 interleaved rounds, host clang -O2): odds 0.75 ns, hazard
(`log1p`) 4.68 ns, product 2.13 ns. Plus one `exp` per agent with any
infectious contact. These pairs already pay for tool reduction calls,
`get_prob_infecting()`, a uniform draw, and usually a cache miss, so the
difference is probably a few percent at most -- **to be confirmed on real
models** (see Section 8).

Recommendation: make the rule a model option (`set_transmission_rule(...)`),
keep roulette as the default until the benchmark and a decision on changing
results, and use hazard for new multi-layer models if the overhead is small.
Changing the default is a separate, versioned decision.

### D2. How a participation weight enters the probability

- With **roulette**, apply the weight as thinning of the contact:
  `p_ij = w_i * w_j * p`. Exact (contact and transmission are independent),
  no extra random draws. Requires `w <= 1`.
- With **hazard**, weights could instead scale the hazard,
  `lambda_ij = w_i * w_j * lambda(p)`, which also allows `w > 1` (e.g., more
  intense household contact during a lockdown). Note `w = 0.5` then gives
  `1 - (1 - p)^0.5`, not `0.5 p` ("half the exposure time" rather than "half
  the contacts").

Recommendation: thinning (`w * p`) everywhere, `w` in [0, 1]; intensity above
1 is expressed with repeated contacts. Revisit if D1 lands on hazard.

### D3. Legacy neighbor API with several network layers

`Agent::get_neighbors()`, `neighbors_view()`, `get_n_neighbors()`,
`Model::add_edge()`, `rm_edge()`, `has_edge()`, `agents_from_edgelist()`,
`agents_from_adjlist()`, `agents_smallworld()`, `rewire()`, `write_edgelist()`:
recommendation is that they act on the **default network layer**, named
`"network"`, created on first use. New overloads take a layer id. With several
network layers, code that wants all of them loops over layers explicitly.

### D4. When weight changes take effect

Recommendation: like state changes, at the end of the step (`events_run()`), so
results do not depend on the order in which agents are updated.

### D5. Contact tracing and full contact lists

A mixing layer only cheaply produces *infectious* contacts. Contact tracing
needs all contacts. Options: a `full` flag on `sample()` (slower, used only
when tracing or post-sampling is on), or tracing only on network layers.
Recommendation: the flag; the post-sampling callback today already only sees
"eligible" contacts, so document what each layer reports.

### D6. Layer-level scale

A scalar per layer (school closure = 0; lockdown = 0.2 on work) is the same as
scaling every agent's weight, but O(1) to change. Cheap to add; recommended for
Phase 4 unless it turns out to duplicate global events.

### D7. Directed networks

Today `directed` is model-wide and forces pull. Recommendation: per network
layer; a model pulls if any layer is directed.

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
crowding studies (e.g., peak-hour interventions) set it directly.

### D10. Where schedules live

Either in the layer (`update()` reads a calendar: active days, per-agent
schedules) or in global events that set layer scales and weights. Layers are
the natural owner of recurring, routine schedules; global events of one-off
interventions (closing schools). Recommendation: both, with that split.

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
`Bubbles` with `BubbleTies::Complete` (cliques added and withdrawn) could later
become an `Affiliation` layer instead of edits to the network.

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
the members of each of its venues who are present (weight > 0), drawn fresh:
a binomial number of draws from the venue's infectious members with probability
`k / (sum of weights of present members - own weight)`, as the `Mixing`
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
weekdays), and an agent's weight can follow its own schedule (a remote-work day
is weight 0 that day). Both are evaluated in the layer's `update()`; changes
take effect at the end of the step like any other (D4). With data instead of a
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
    virtual void reset(Model<TSeq> & m) = 0;    // run start: size buffers
    virtual void update(Model<TSeq> & m) = 0;   // once per step, before transmission
    virtual std::unique_ptr<ContactLayer<TSeq>> clone() const = 0; // run_multiple

    // Pull: append the contacts of `i` this step that carry a virus (all
    // contacts when `full`), repeats allowed. Returns how many were appended.
    virtual size_t sample(
        size_t i, Model<TSeq> & m, std::vector<size_t> & out, bool full = false
    ) = 0;

    // Optional: push (whom did carrier `j` contact?), for Phase 5
    virtual bool can_push() const { return false; }
    virtual size_t sample_reverse(size_t j, Model<TSeq> & m, std::vector<size_t> & out);

    // Optional: queue support (agents carrier `j` can reach)
    virtual bool supports_queue() const { return false; }

    // Participation (Section 5.5)
    void set_weight(size_t agent, float w);   // applied at end of step
    float get_weight(size_t agent) const;
};
```

Virtual dispatch happens once per (agent, layer) per step, not per contact.

The model holds `std::vector<std::unique_ptr<ContactLayer<TSeq>>>`, with
`add_layer()`, `get_layer(name|id)`, `get_layer_id(name)` (resolve names once,
use ids in hot loops). `Model`'s copy constructor and `clone_ptr()` must
deep-copy layers (`run_multiple()` clones models), and `reset()` must restore
each layer's initial state (as the network backup and
`contact_matrix_backup` do today).

### 5.4 Combining layers in one draw

`default_update_susceptible` (and `sampler::UpdateSusceptible`) become
layer-aware: for a susceptible `i`, every layer appends contacts; each contact
`j` with a virus contributes `p_ij = w_i^L * w_j^L * (1 - sus_red_i) * p_v *
(1 - trans_red_j)` to the per-agent accumulator; one draw decides infection and
infector (rule from D1). This is the same accumulator the push path already
uses (`PushTarget`: summed odds + weighted reservoir), so pulling and pushing
layers can be mixed in one step: reservoir samples merge exactly.

Keep the arrays-then-`roulette()` code path for models whose only layer is a
single network layer, so their random stream does not change.

### 5.5 Participation weights

- Storage: `std::vector<float>` per layer (4 bytes x agents x layers; 16 MB for
  1M agents and 4 layers) plus an `all_ones` flag that skips the lookup.
- Network layer: one load and a multiply per *infectious* contact; non-carriers
  are skipped before it.
- Mixing/Affiliation layer: agents with `w = 0` are removed from the pools and from
  the denominators; the denominator becomes the **sum of weights** of available
  agents per group (equal to today's count when all weights are 1, exactly, in
  floating point). A drawn contact `j` contributes `w_j * p`, so a full
  participant keeps contact rate `c` and an agent at `w = 0.2` receives 20% of
  the exposure.
- With default weights of 1.0, `x * 1.0 == x` exactly, so refactor phases keep
  identical results.
- Policy helpers: `isolate(agent, keep = {"household"})`, layer scale (D6).
- Migration target: the measles sampler's "Rash reduction contact rate" (a
  reduced-infectious state sampled at reduced rate) is a participation weight.

### 5.6 Neighbors per network layer

- `Agent::neighbors_view(model, layer_id)` returns the existing non-allocating
  `NeighborsView`, now per layer. Legacy `neighbors_view(model)` uses the
  default `"network"` layer (D3).
- Ties move from `Agent` (`neighbors`, `neighbor_pos`, `n_neighbors`) into
  `layer::Network`, keeping per-agent tie lists so edits (`add_edge`,
  `rm_edge`, `swap_neighbors`/`rewire_degseq`) and **neighbor order** (which
  fixes the random stream) are preserved.
- Bindings: a named list of per-layer neighbor vectors; edgelist I/O gets a
  layer column.

### 5.7 Queue

- The queue counts over the union of queue-capable layers (`Network`,
  `Affiliation`). A carrier queues its ties in every network layer and the
  members of its venues in every `Affiliation` layer; a membership change is
  notified to the queue like an edge edit.
- Any `Mixing` layer turns queuing off, as mixing models do now.
- Weights do not affect the queue (a queued agent with weight 0 just gets
  `p = 0`); conservative and simple.

### 5.8 Push and pull

- Phase 1-4: push only when the model has a single undirected network layer
  (today's behavior). Everything else pulls.
- Phase 5: `sample_reverse()` for `Mixing` (for infectious `j` in group `g`,
  contacts in group `h` ~ Binomial(available in `h`, c(h,g) / available in
  `g`), which gives the same pair probabilities as pulling) and `Affiliation`; push
  when every layer can push; the auto cost model sums per-layer costs.

### 5.9 Outputs

- `record_transmission(i, j, virus, date)` gains a layer id
  (`get_transmissions()` gets a `layer` column; -1 for seeded/non-contact).
- The post-sampling callback reports the layer of each contact.
- `write_edgelist()` / `get_edgelist()` per layer.

### 5.10 Backward compatibility

- Network models built with today's API get one `"network"` layer implicitly;
  results identical.
- Mixing models create one `layer::Mixing`; `set_contact_matrix()` forwards to
  it; results identical if the same draws happen in the same order (the
  sampler is moved, not rewritten).
- `diffnet`, `sirlogit`, `surveillance`, `Bubbles`, `randgraph.hpp` keep using
  the default network layer.

---

## 6. Phases (one PR each)

Each phase ships on its own and keeps the suite green. Phases 1-2 are pure
refactors: the test is that existing model-level results stay identical.

### Phase 0 -- benchmark the combination rule (independent; any time)

- Implement the hazard rule behind a flag on a scratch branch.
- Run SEIR network (pull and push) and SEIR mixing at 1e5-1e6 agents; CPU time,
  interleaved, in the devcontainer.
- Output: overhead numbers for D1; decide default.

### Phase 1 -- layer interface + `layer::Mixing`

- Add `ContactLayer`, the model's layer list, `add_layer()`/`get_layer()`.
- Move `sampler::Mixing` into `layer::Mixing` with its own grouping (no
  `get_entity(0)`), weights stored (all 1) and sum-of-weights denominators.
- Port `ModelSIRMixing`, `ModelSEIRMixing`, `ModelSEIRMixingQuarantine`.
- Files: new `contactlayer-bones.hpp`, `layer-mixing.hpp`;
  `sampler-mixing.hpp`, `contactmatrix-*.hpp`, `models/*mixing*.hpp`,
  `model-bones.hpp`, `model-meat.hpp`.
- Tests: existing `05*-mixing`, `06*`, `08-mixing-entities`, `17*` unchanged
  and passing; one new test: an agent's group in the layer differs from its
  first entity.
- Version: minor (new API, no result changes).

### Phase 2 -- `layer::Network`

- Move ties out of `Agent` into `layer::Network`; legacy API maps to the
  `"network"` layer; per-layer `NeighborsView`.
- Update the queue, `state_index_degree` / carrier-degree sums (push cost
  model), the push loop, `add_edge`/`rm_edge`/`rewire`, directed handling,
  `Bubbles`, `diffnet`, `sirlogit`, `surveillance`, `seirnetworkquarantine`,
  `randgraph.hpp`, database edgelist output.
- Tests: the whole suite unchanged (notably `15`, `20*`, `33*`, `34*`, `35*`,
  `36a`). Benchmark: no regression in pull or push (CPU time, interleaved).
- Version: minor.
- Risk: largest phase (about 100 references to `Agent::neighbors`, 47 in
  `agent-meat.hpp`). Neighbor order must be preserved exactly.

### Phase 3 -- several layers in one model

- Layer-aware susceptible update and accumulator (Section 5.4);
  `layer::Affiliation` with fixed memberships (`q = 1`, several memberships per
  agent allowed); layer id in transmission records and post-sampling;
  per-layer edgelist and membership I/O.
- An example model: households (`Affiliation`) + workplace (`Network`) + public
  spaces (`Mixing`).
- Tests (end-to-end, about three files):
  - a network split into two layers matches the single-layer network in
    distribution (`run_multiple`, attack rate and transmission counts);
  - an `Affiliation` layer with `q = 1` matches the equivalent complete-graph-per-household
    network in distribution;
  - a combined model attributes transmissions to the expected layers (e.g.,
    with one layer's rate at 0, no transmission is recorded on it).
- Version: minor.

### Phase 4 -- participation weights, policies, schedules, and persistence

- `set_weight()` applied at end of step; `isolate(agent, keep = ...)`; layer
  scale (D6); global event to scale/close a layer.
- Schedules (D10): layers active on given steps (weekdays), per-agent weight
  schedules.
- Persistence (D8, D9): venue pools, usual venue with probability `q`, per-venue
  contact rate `k`, membership changes notified to the queue.
- Port the measles reduced-contact state to weights.
- An example model: the Phase 3 model plus a transit layer (morning and evening
  trips, route pools, `q < 1`, weekdays only).
- Tests (end-to-end):
  - deterministic -- with `p = 1`, an isolated infectious agent infects only
    household members; a closed layer records no transmission;
  - schedules and persistence -- no transit-layer transmission on inactive
    steps; with `q = 1` every transit transmission is between members of the
    same usual venue, with `q = 0` transmissions also cross venues in a pool;
  - a weight of 0.5 halves exposure in a mixing layer (statistical).
- Version: minor; patch-level result changes for measles models only if the
  port is not draw-for-draw identical.

### Phase 5 -- combination rule option and push across layers

- `set_transmission_rule()` (D1) if Phase 0 supports it.
- `sample_reverse()` for `Mixing` and `Affiliation`; multi-layer push; per-layer
  auto cost model (revisit `EPI_TRANSMISSION_AGENT_COST` and kappa).
- Version: minor (option); a default change is its own decision.

### Phase 6 -- bindings and docs

- epiworldR, epiworldpy, epiworldjs: `add_layer()`, named-list neighbors,
  layer column in transmissions.
- Docs: new `docs/impl/contact-layers.md`; update
  `mixing-and-entity-distribution.md`, `queueing-system.md`,
  `transmission-sampling.md`.

---

## 7. Testing conventions (from AGENTS.md)

- One `EPIWORLD_TEST_CASE` per file; letter suffixes for related cases.
- End-to-end: build a model, run it, assert on histories, transition matrices,
  transmissions, edgelists. Refactor phases are tested by unchanged results.
- `model.run()` for deterministic tests, `run_multiple()` for statistical ones.
- Run in the devcontainer; also build with `-DEPI_DEBUG`, since `make test`
  skips those checks.

## 8. Benchmarks

- Phase 0: rule overhead on real models.
- Phase 2: no regression for pull/push on network models.
- Phase 3: overhead of a single-layer model through the multi-layer path vs the
  legacy path (to decide whether the legacy path must stay).
- Phase 4: cost per step of an affiliation layer as `q` falls (membership
  churn and queue notifications), at transit-like sizes (venues of 50-200,
  two memberships per commuter).
- Phase 5: push vs pull for mixing layers early in an outbreak.
- Use CPU time and interleave variants; wall-clock on the macOS host is
  unreliable.

## 9. Risks

- **Random stream changes.** Any change of draw order changes results. Phases
  1-2 must be draw-for-draw identical; later phases bump versions.
- **Cloning.** Layers behind `unique_ptr` must be deep-copied in `Model`'s copy
  constructor/assignment and `clone_ptr()` of every model, or `run_multiple()`
  shares state across threads.
- **Hidden single-network assumptions** in models and global events (Phase 2).
- **Binding churn** across three language packages (Phase 6).
- **Membership churn.** With low `q`, many memberships change every step; the
  queue notifications and venue member lists must stay O(changes), not
  O(population).

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
