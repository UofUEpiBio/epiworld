# Natural history and agent status: separating what the disease does from where the agent is

**Status: draft, revised after the first review (2026-11-03).** This is an
experimental plan on the `natural-history` branch, cut from `v1-layers`.
Nothing here is settled. The core is being tried on this branch (NH-1,
Section 11) to see how it goes before deciding whether, and when, it joins
`v1-layers` / 1.0.0. Reviewers: add comments to the open questions
(Section 12) or open review threads on the PR.

## 1. Motivation

Today an agent has **one** state, an index into the model's list of states.
Viruses carry `state_init`, `state_post` and `state_removed`
(`include/epiworld/virus-bones.hpp:45-47`): the state an agent jumps to when
it acquires the virus, when the virus is removed, and when the agent is
removed. Update functions are registered per state on the model
(`Model::add_state`). Which states are infectious, or excluded from
transmission, is also a model property (`set_infectious_states`,
`set_excluded_states`).

That single state mixes two things:

- **The natural history of the infection**: latent, prodromal, infectious,
  rash, recovered. This is within-host, and it belongs to the pathogen.
- **The agent's status in the system**: free, quarantined, isolated,
  hospitalized, dead. This governs contacts, movement and interventions, and
  it belongs to the model.

The cost shows up in the code:

- **The measles models need 12 combined states.** `ModelMeaslesMixing`,
  `ModelMeaslesSchool` and `ModelMeaslesMixingRiskQuarantine` each register
  12 of the 20 combinations of natural history {S, Latent, Prodromal, Rash,
  Recovered} × status {free, quarantined, isolated, hospitalized}, with 11
  update functions. Most repeat the release-from-quarantine logic.
  `ModelSEIRQuarantine` has 10 states on the same pattern.
- **The natural-history layer is already rebuilt by hand.**
  `MeaslesStages::stage_of()` maps Latent and Quarantined Latent to one
  stage, Rash and Isolated to another, so that quarantine does not change the
  course of the disease.
- **A status layer already exists in side vectors**, under other names:
  `day_flagged`, `day_rash_onset` and `agent_quarantine_triggered`. Contact
  tracing depends on how the states are numbered (`get_state() > RASH`) and
  on a switch that maps each state to its quarantined twin.
- **Coupling rules are buried inside transitions, and some may not be
  intended:**
  - entering Hospitalized stops the rash clock;
  - a quarantined agent who develops rash is forced into Isolated;
  - quarantine skips agents holding any tool (`get_n_tools() != 0`) rather
    than agents who are immune;
  - competing risks are resolved by the order of `if/else` branches.
- **Adding one status multiplies the states.** A "tested, awaiting result"
  status, or a modified-measles branch after immune globulin, would add a
  copy of every compatible natural-history state.
- **Several viruses work only as strains of one disease.** All viruses share
  one state space, and an agent holds a single virus.

## 2. Precedent

The split is not new. The closest precedents:

- **OpenABM-Covid19.** The struct in `individual.h` has separate `status`
  (disease), `quarantined` and `hospital_state` fields, each with its own
  event chain, plus per-strain immunity arrays.
- **FRED.** Each agent is in one state of each "condition". A disease, testing
  and isolation are all conditions, and a change in one triggers a change in
  another (a positive test leads to isolation).
- **Starsim** (IDM). `People` holds only demographics and `alive`. Each disease
  module owns its state arrays. "Connectors" let one disease change another's
  `rel_sus`/`rel_trans`. A disease *requests* a death, and the death is
  carried out once for the agent, which closes every disease.
- **EMOD.** It keeps a list of `Infection` objects separate from a
  `Susceptibility` object (immune memory that survives clearance).
- **EpiHiper.** A disease state machine runs alongside intervention "traits".
- **The counter-example is Covasim v3.** It used one set of flags mixing
  disease and administrative state, with no consistency checks; IDM has since
  rebuilt Covasim on Starsim.
- **Theory.** Harel statecharts' orthogonal regions describe a state as a
  tuple without building the product state machine. Libkind et al. 2022
  describe stratification as a typed product of a disease model and an
  intervention model.

References are in Section 14.

## 3. Proposal: status and per-disease stages

Revised on 2026-11-03 after the first review (PR #358). Section 12 lists
what changed and why.

1. **Status, owned by the model.** For example free, quarantined, isolated or
   hospitalized. It governs contacts (layer participation), movement and
   interventions. This is today's `Agent::state`, so the model's states *are*
   the statuses, and every agent has one, infected or not.
2. **Stage, owned by a natural history.** A natural history (`NaturalHistory`)
   describes one disease: its stages (susceptible, latent, prodromal, rash,
   recovered), how agents move through them, and which stage an infection
   starts in and ends in. **Every agent has a stage in each natural history of
   the model, infected or not** (as in FRED's conditions and Starsim's disease
   modules). Being susceptible and being recovered are stages, so "no virus"
   is never ambiguous. In a single-disease model, immunity after recovery is
   the recovered stage, and waning is a transition out of it. A virus names
   the natural history it follows. Getting the virus moves the agent to the
   infection stage, and losing it moves the agent to the recovery stage.
3. **A separate immunity record comes later.** Immunity across pathogens
   (cross-protection, measles immune amnesia) and protection levels per source
   (vaccine, infection, maternal) need a record per agent and pathogen
   (EMOD's `Susceptibility`). Until then, vaccines stay as tools.

**Status is one value at a time.** Hospitalized, isolated, quarantined and
free are exclusive, with a documented priority (hospitalized > isolated >
quarantined > free) and each model's restoration rule (for example, measles
discharges to free). Overlapping obligations (isolated *and* admitted; a
quarantine order that outlasts a hospital stay) need independent obligations
with their own clocks and a derived contact status, as OpenABM keeps
`quarantined` and `hospital_state` apart. That is a known limitation of the
pilot, listed in Section 12. The tests cover what the pilot does: admission
during isolation, discharge, repeated quarantine, and recovery while
quarantined.

**Death is a status** (in models that have it). A disease requests it, and
the model carries it out once for the agent (Starsim's `request_death`). None
of the pilots has death.

### 3.1 What a natural history stores per agent

The review's main point was that a stage and a date are not enough. Sampled
durations, the day of each landmark, and later severity or individual
infectiousness must live somewhere with a defined lifecycle, or each model
reinvents it in side vectors (the measles models' `day_rash_onset`,
`MeaslesStages`). A natural history keeps, per agent:

| Field | Meaning | Lifecycle |
|---|---|---|
| `stage` | Current stage | Set to the first stage at reset |
| `episode` | Infections so far (the episode id) | +1 when an infection starts |
| `entered[s]` | Day the agent last entered stage `s` in this episode | Cleared when an infection starts; written on each entry |
| `until` | Day the current stage's sampled duration ends (-1: none) | Drawn on entry when the stage has a duration |

Everything is in plain per-agent arrays owned by the natural history, which
the model owns. So the arrays are reset with the model, copied with it
(`run_multiple`), and cleared when an infection starts, by the engine, the
same way for every model. Infections in the transmission log already record
the source, the target and the date. With `episode`, a line list can link each
infection to the infector's episode (Section 9).

**Stage durations are part of the natural history.** A stage can declare a
duration (a function drawing the number of days) and a next stage. The
duration is drawn when the agent enters the stage, and the agent moves on, at
its first update on or after `entered + duration`, to the next stage. A stage
with an update function decides for itself (it can read `due()`). Sampled
once per entry, a duration survives status changes: quarantine and isolation
no longer need to keep the day a stage ends (the reason `MeaslesStages`
exists).

Per-episode fields beyond these (severity, individual infectiousness) are the
next step. They would follow the same lifecycle (a named field cleared when an
infection starts).

## 4. A registry of virus definitions (the "controller virus")

The second idea: the model keeps the shared definition of each virus, and
agents keep a small record pointing to it, instead of a full copy per
infection. It is independent of the stage split, so it is gated separately
(NH-0 in Section 11) after the stage split is tried. The stage split does not
need it: natural histories live in the model, and a virus refers to one by
index.

### 4.1 Today: a full copy of the virus per infection

Each infection clones the whole `Virus` object (`agent-meat.hpp:149`,
`virus.clone_ptr()`) into a new `shared_ptr`. Copying an agent clones its
virus again (`agent-meat.hpp:56, 85`). That happens every time a model is
copied for `run_multiple`. A `Virus<int>` is **320 bytes** (measured with
`sizeof` on clang/macOS; an `Agent<int>` is 56). Each clone also allocates a
`shared_ptr` control block, copies the `std::string` name, and copies seven
`std::function`s (`mutation`, `post_recovery_fun`,
`probability_of_infecting`/`recovery`/`death`, `incubation`, `dist`). Each
of those may allocate when it captures state. Almost all of that is identical
across every copy. The only fields that are really per infection are the
agent pointer, the date and, after mutation, the sequence, the id and the
lineage.

### 4.2 Proposal: definitions in the model, a small record in the agent

- **The model holds virus definitions in a registry** indexed by id. A
  definition holds everything that is shared: the name, the parameter
  references, the probability functions, the natural history it follows, the
  default status changes, and the distribution function.
- **Variants** are entries in the registry, each with a sequence and a parent.
  `DataBase::record_virus` already keys variants by sequence hash and tracks
  lineages, so the registry replaces that bookkeeping instead of duplicating
  it. Entries live in stable storage (a `std::deque` or chunks), so a
  mutation that adds a variant does not invalidate a callback's reference to
  a definition (a `std::vector` append would).
- **An agent holds a small record inline** (variant id, infection date), with
  no heap allocation. The stage and the per-episode data are not in it: they
  are in the natural history's arrays (Section 3.1).
- **Callbacks receive the definition and the agent.** User code no longer
  gets a mutable `Virus&` per infection, so per-infection behaviour that
  relied on editing one agent's copy needs the per-episode fields instead.
- **Mutation** returns a new sequence. The model looks it up in the registry
  or adds it, then repoints the record's variant.

### 4.3 Gate

The registry changes many signatures: `Agent::get_virus()` returns
`VirusPtr&`, callbacks take `Virus&`/`VirusPtr&` (`config.hpp`), and
`clone_ptr()` is virtual. Breaking them is acceptable for 1.0.0, but only
for a measured gain. NH-0 compares the registry against the cheaper
alternative: keep the clones, with the definition's shared parts behind one
`shared_ptr`. Scenarios: low and high prevalence, many variants (mutation),
histories on and off. Measures: peak memory, time per step, and
`run_multiple` copy time. **Go/no-go:** the registry goes ahead only if it
beats the alternative by a clear margin (say 20% in peak memory or step
time) on at least one realistic scenario. Otherwise, the alternative is
kept.

## 5. How the layers interact

Neither layer writes the other's state behind its back. They talk through a
small set of mechanisms:

| Direction | Example | Mechanism |
|---|---|---|
| stage → status | rash onset leads to isolation for a quarantined agent | `model.set_stage_hook(fun)`: called when a stage change is applied (below). The natural history never knows the model's status codes. |
| status → stage | hospital care; PEP | The status update reads the stage and acts (removes the virus at discharge, for example). Tools (`recovery_enhancer`, `death_reduction`) for continuous effects. |
| status → contacts | quarantine removes the agent from contacts | Today: the model's sampler classifies agents by status and stage. Later: Phase 4 participation keyed by status. |
| stage → infectiousness | prodromal and rash are infectious | The model's sampler (measles). Later: an `infectious` attribute on the stage, read by the generic samplers. |

**Hook timing.** The stage hook runs inside `events_run()`, right after a
stage change is applied, and it sees the committed stage. It may schedule
more events, which `events_run()` applies in the same pass. This is what
the current `events_run()` already supports for handlers that append
events.

Status rules still read the stage (release from isolation counts from rash
onset). The state space is moved, not removed. The measure of success is in
Section 11's report: fewer states, fewer and shorter functions, less
branching, no duplicated rules and no hand-written timer bookkeeping.

## 6. Update order, conflicts and competing risks

- **One update per agent per step.** The engine calls the stage update of
  each natural history, then the agent's status update function: the disease
  first, then the model. All of them only schedule events, which apply at the
  end of the step in the order they were scheduled. So a stage change is
  applied before the status changes of the same step, the stage hook sees the
  status the agent had at the start of the step, and the events the hook
  schedules come last (they win). Found in the measles port: with the
  opposite order, a quarantine release scheduled on the day of rash onset was
  applied before the rash, the hook missed the quarantined agent, and the
  model needed a special case.
- **Stage events carry an identity.** A stage change records the stage it
  leaves and the episode. When it is applied, an event whose agent is no
  longer in that stage, or in that episode, is **stale and dropped**. So the
  first applied change wins, and a change queued before a recovery and a
  reinfection on the same day can never apply to the new episode.
- **Conflict table:**

| Same step | Result |
|---|---|
| Two stage changes from the same stage | The first applied wins; the other is stale |
| Stage change, then virus removal (recovery) | Both apply, in order (the removal moves the agent to the recovery stage) |
| Virus removal, then a stage change queued from the infected stage | The stage change is stale |
| Two status changes | The last wins (unchanged from today), and the transition counts are undone and redone |
| Status change and stage change | Independent; both apply |
| Hook-scheduled events | Applied in the same `events_run()` pass, after the event that triggered them |

- **Competing risks stay as they are in the port.** The measles port keeps
  the current order of checks, so its results can be compared with the
  current model. A helper that settles competing hazards in one draw is
  opt-in and separate. It needs stated units and a time step, and it does not
  turn a per-case probability into a daily rate. Policy changes (such as
  whether hospitalization pauses the rash clock) are separate commits with
  their own acceptance tests (Section 11).

## 7. Engine changes

Revised in NH-2 (2026-11-04). NH-1 kept stages in separate per-agent arrays,
with their own events, update loop and database tables. Porting every model
showed that the engine's existing machinery (push and pull transmission, the
agents-by-state index, the queue, the state transition bookkeeping with
same-day undo) is all keyed on the agent's single state, and duplicating it
for stages was the expensive part. So NH-2 keeps one state per agent and
**builds the states from the statuses and the stages**:

- **States are pairs, built by the engine.** `add_state()` adds a status.
  With natural histories, the model's states are every combination of a
  status and a stage (status + nstatuses × stage), rebuilt whenever a status
  or a natural history is added. Models never write these pairs:
  `Agent::change_status()` and `Agent::change_stage()` change one part and
  keep the other, and `get_status()`/`get_stage()` read them back. A state's
  update runs the stage's update function, then the status's; a single
  function is used as is, so the samplers still recognize
  `default_update_susceptible` and push transmission works unchanged.
- **With one status, the states are the stages**, with the same codes and
  labels. So the SIR family on natural histories gives bit-identical
  histories to the old models, and their tests and outputs are unchanged.
- **`NaturalHistory`** keeps the stages (label, update function, duration
  and next stage, infectious flag), the infection, recovery and death stages,
  and the per-agent lifecycle arrays of Section 3.1 (episode, entry days, end
  of the current duration). The stage itself is part of the state.
- **Events.** A change of status (`ChangeStatus`) or of stage
  (`ChangeStage`, with the stage it leaves and the episode, dropped if
  stale) is turned into a change of state when it is applied, from the
  agent's state at that moment. Gaining or losing a virus that follows a
  natural history moves the stage (to the infection, recovery or death
  stage) and keeps the status. After each event, the engine updates the
  lifecycle arrays of the stages entered and calls the stage hook.
- **Masks.** Infectiousness comes from the stages' flags combined with
  `set_infectious_states()`; with natural histories, `set_excluded_states()`
  lists statuses. Excluded agents are not infected by
  `default_update_susceptible`.
- **The queue.** Models with several statuses with update functions, or with
  stage durations, update every agent each step (the queue pitfall);
  single-status models keep the queue.
- **The Database records states, which are the joint counts**, so nothing
  new is recorded. `get_hist_status()`, `get_hist_stage()`,
  `get_hist_joint()`, their transition matrices and probabilities, and
  `get_today_status()`/`get_today_stage()` add the state histories up. This
  replaced NH-1's per-record scan.
- **Initial states.** `create_init_function_stages()` moves shares of the
  initial cases and of the others to given stages (the old initializers
  moved agents to SEIR state codes, wrong in the quarantine models).

## 8. API sketch

```cpp
// The natural history: stages, durations, and the infection/recovery stages
NaturalHistory<> nh("Measles");
int S  = nh.add_stage("Susceptible");
int L  = nh.add_stage("Latent");
int P  = nh.add_stage("Prodromal");
int Rs = nh.add_stage("Rash", rash_update);   // a stage can have an update
int R  = nh.add_stage("Recovered");
nh.set_duration(L, lognormal_days("Incubation period"), P); // then P
nh.set_duration(P, lognormal_days("Prodromal period"), Rs);
nh.set_duration(Rs, lognormal_days("Rash period"));         // rash_update decides
nh.set_infection(L, R);                                     // starts in L, ends in R

int k = model.add_natural_history(nh);
virus.set_natural_history(k);

// Statuses: the model's states
model.add_state("Free",         update_free);
model.add_state("Quarantined",  update_quarantined);
model.add_state("Isolated",     update_isolated);
model.add_state("Hospitalized", update_hospitalized);

// Coupling: the model reacts to stage changes
model.set_stage_hook([](Agent<>* p, size_t k, int from, int to, Model<>* m) {
    if ((to == Rs) && (p->get_state() == QUARANTINED))
        p->change_state(*m, ISOLATED);
});

// In update functions
p->get_stage(*m);                          // current stage (history 0)
m->get_natural_history(0).entered(*p, Rs); // day of rash onset
m->get_natural_history(0).due(*p, *m);     // the stage's duration is over
p->change_stage(*m, R);                    // event, target = stage
```

## 9. Outputs

- **Per natural history:** stage counts per day, the stage transition matrix,
  and the joint status × stage table. Transmission counts per virus as today.
- **Status:** the model's state history and transition matrix keep their
  meaning (status occupancy; quarantine-days are the sum of the daily
  quarantined count).
- **"Infectious but free"** (the key leak for an intervention) is a cell of
  the joint table.
- **Later, an episode line list** written to disk: the episode id, the
  infection date, each stage's entry day (`entered`), detection, status
  changes and outcome, plus the infector and the infector's episode (from the
  transmission log). Episodes with no symptomatic stage (asymptomatic or
  aborted) have no symptom-onset date. Generation and serial intervals and
  the share of transmission before isolation follow from it.

## 10. Compatibility and scope

**This is a 1.0.0 change, and compatibility is not a goal.** Models that
adopt natural histories change their states (measles goes from 12 states to
4 statuses plus 5 stages), their outputs, and the tests that read them. The
wrappers follow in NH-4. What still holds: a model with no natural history
behaves exactly as today, so the SIR/SEIR family and its tests are untouched
by NH-1.

**Out of scope for this plan:**

- **Co-infection.** An agent holds one virus (`sample_virus` throws on a
  second). The stage arrays already allow one stage per disease per agent,
  but simultaneous infections need a small vector of viruses per agent,
  exposure draws per virus, and a rule for competing exposures (D1 in
  `contact-layers.md`). It is a separate project.
- **Cross-pathogen effects** (interference, cross-immunity, immune amnesia):
  a later model-level hook, in the style of Starsim connectors, once the
  immunity record exists.
- **Tools on the registry.**

## 11. Phases on the `natural-history` branch

Each phase is a PR into `natural-history`. Measles goes first because it is
where the combined states hurt most.

- **NH-1: natural histories in the engine, and the measles mixing pilot.**
  - `NaturalHistory`, stage events, the stage hook, the update loop, and the
    database tables (Sections 3, 6, 7).
  - `ModelMeaslesMixing` ported to 4 statuses × 5 stages. The port keeps the
    current behaviour (order of checks, hospitalization pausing the rash
    clock), so results are statistically equivalent rather than identical
    (the order of random draws changes).
  - **A complexity report**: states, update functions, lines, cyclomatic
    complexity, duplicated rules and timer bookkeeping, before and after
    (`complexity-report.md`).
  - **Acceptance:**
    - every test of the models without natural histories passes unchanged;
    - the measles mixing tests, rewritten on stages and statuses, pass with
      the same theoretical targets (R0, stage durations, hospitalization
      probability, detection);
    - paired deterministic scenarios (fixed durations, probabilities of 0 or
      1) give the same days of rash onset, isolation, release and recovery
      as the current model;
    - durations survive status changes, stale events are dropped, the hook
      fires in the same step, and the three database tables add up.
- **NH-1b: policy corrections, if wanted**, as separate commits with their
  own tests: hospitalization does not pause the rash clock; the quarantine
  exemption depends on immunity rather than on holding any tool; Prodromal
  counts as symptomatic for symptom-onset outputs.
- **NH-2: every model on natural histories** (done, 2026-11-04). The
  engine builds the states from statuses × stages (Section 7). SIR, SIS,
  SIRD, SISD, SEIR, SEIRD, SIRLogit and DiffNet have one status, so their
  states are the stages, unchanged. SEIRQuarantine, the three measles models
  and Surveillance have several statuses. The PEP intervention keeps taking
  states (`ModelMeaslesSchool::state(status, stage)`).
- **NH-0: the virus registry**, gated by the benchmark in Section 4.3.
- **NH-3: wrappers.** Accessors for stage histories, transitions and the
  joint table in epiworldR, epiworldpy and epiworldjs.
- **NH-4 (later):** the immunity record, then co-infection, then
  cross-pathogen hooks.

**Fit with `contact-layers.md`.** Phase 4 participation should be designed as
"status leads to exclusion" whichever way this goes. The measles relabelling
should land in 1.0.0 if it lands at all.

### Testing

Following `AGENTS.md`: few end-to-end test files, one `EPIWORLD_TEST_CASE`
each, using fixed (drawn-once) networks.

- **Equivalence:** the models without natural histories give identical
  results.
- **Timing, not only totals:** compare the distributions of the latent,
  prodromal and rash periods, the days of detection, isolation and release,
  and hospital occupancy, not only R0 and attack rates. Stochastic checks
  use `run_multiple()` with tolerances stated before the run.
- **Separation:** with fixed durations, quarantined and free agents have the
  same latent and prodromal periods.
- **Lifecycle:** a duration drawn on entry survives a status change, is
  redrawn on reinfection, and is copied with the model.
- **Conflicts:** stale stage events are dropped; the hook's events apply in
  the same step.
- **Bookkeeping:** stage counts, the joint table and status counts add up to
  the population every day. Build the tests standalone with `-DEPI_DEBUG` as
  well (`make test` skips those checks).

## 12. Review log and open questions

### Changes after the first review (PR #358, 2026-11-03)

| Review point | Change |
|---|---|
| NH-0 compatibility boundary (P1) | Compatibility is not a goal for 1.0.0 (Section 10). The registry is gated by a benchmark against the cheaper alternative (Section 4.3). |
| Episode identity for stale events (P1) | Stage events carry the stage they leave and the episode; stale ones are dropped. Conflict table and hook timing (Section 6). |
| Per-episode attributes need a lifecycle (P1) | Per-agent arrays owned by the natural history, with stage durations sampled on entry and landmark days (Section 3.1). |
| Overlapping obligations (P1) | One status with a documented priority in the pilot; independent obligations listed as a limitation (Section 3). |
| Hazard helper vs. policy (P2) | The port keeps the current order of checks; policy changes are separate commits (Sections 6, 11). |
| Prodrome is symptomatic (P2) | Marked as an NH-1b correction; the rash-onset day stays its own landmark (`entered`). |
| Pilot gate on timing (P2) | Timing distributions and paired deterministic scenarios in the acceptance tests (Section 11). |
| Benchmark against the cheaper alternative (P2) | Section 4.3, with a go/no-go rule and stable storage for definitions. |
| Generation and serial intervals need linkage (P2) | Episode ids plus the transmission log (Section 9). |

A design change also came out of the review: every agent now has a stage
in each natural history, so susceptible and recovered are stages rather than
the absence of a virus (Section 3).

### Still open

1. **Names.** "Status" for the model's states and "stage" for the natural
   history (the review's suggestion, adopted in this draft). The code keeps
   `state` for the status.
2. **Measles policy:** should hospitalization pause the rash clock (the
   review says no)? Should the quarantine exemption depend on immunity rather
   than on any tool (the review says yes)? Should quarantine plus rash always
   mean isolation, whatever the isolation willingness? Does PEP act on the
   episode (MMR within 72 hours, IG within 6 days, which may prevent or
   modify illness) or on future protection? The CDC surveillance manual
   describes both.
3. **Independent obligations** for status: when, and is the exclusive status
   enough for the measles models?
4. **Should a natural history's stage update run every day for every agent**
   (simple, the pilot), or only for agents with a pending timer or an update
   function (faster, more bookkeeping)?
5. **Timing.** Should this aim for 1.0.0, or stay on its branch until after
   it?

## 13. Risks

| Risk | Mitigation |
|---|---|
| Quarantined agents silently never updated (queue) | Models with natural histories update every agent each step |
| Stale or double-applied stage events | Event identity (stage left, episode); a test |
| The database tables drift from the agents | Computed by a scan at each record, not event by event |
| The extra scan costs too much at 1M agents | Measured; incremental counts only if needed |
| The complexity moves into status functions that branch on the stage | The complexity report; reject the design if the pilot is not simpler |
| Two breaking changes to measles (consolidation, then stages) | Coordinate NH-2 with the measles consolidation |
| Scope creep into co-infection | Out of scope (Section 10) |
| The registry costs more API churn than it saves | The go/no-go benchmark (Section 4.3) |

## 14. References

- Hinch et al. 2021. OpenABM-Covid19. *PLoS Comput Biol*.
  doi:10.1371/journal.pcbi.1009146. Source: `src/individual.h` at
  https://github.com/BDI-pathogens/OpenABM-Covid19
- Grefenstette et al. 2013. FRED. *BMC Public Health*.
  doi:10.1186/1471-2458-13-940; conditions in later releases:
  https://www.phdl.pitt.edu/news/coming-soon-new-release-fred-11-august-2017;
  SIDD-NC example: https://tarheels.live/siddnc/overview
- Starsim: https://github.com/starsimhub/starsim,
  https://docs.starsim.org/tutorials/t4_diseases.html; STIsim connectors:
  https://docs.idmod.org/projects/stisim/en/latest/api/stisim.connectors.hiv_sti.html
- Kerr et al. 2021. Covasim. *PLoS Comput Biol*.
  doi:10.1371/journal.pcbi.1009149; v4 on Starsim:
  https://docs.covasim.org/migrate3to4.html
- Bershteyn et al. 2018. EMOD. *Pathog Dis*. doi:10.1093/femspd/fty059.
  Source: `Eradication/Individual.h` at https://github.com/EMOD-Hub/EMOD
- EpiHiper: https://epihiper.readthedocs.io
- Harel 1987. Statecharts. *Sci Comput Program* 8:231.
  doi:10.1016/0167-6423(87)90035-9
- Libkind, Baas, Halter, Patterson, Fairbanks 2022. An algebraic framework
  for structured epidemic modelling. *Phil Trans R Soc A* 380:20210309.
  doi:10.1098/rsta.2021.0309
- Shaw, Morgan, Perkins 2025. Co-circulating pathogens of humans: a
  systematic review of mechanistic transmission models. *Proc R Soc B*.
  doi:10.1098/rspb.2025.1453
- CDC. Manual for the Surveillance of Vaccine-Preventable Diseases,
  Chapter 7: Measles (PEP: MMR within 72 hours, IG within 6 days).
  https://www.cdc.gov/surv-manual/php/table-of-contents/chapter-7-measles.html
- CDC. Measles clinical overview (prodrome: fever, malaise, cough, coryza,
  conjunctivitis). https://www.cdc.gov/measles/hcp/clinical-overview/
- Mina et al. 2015. Measles immune amnesia. *Science* 348:694.
  doi:10.1126/science.aaa3662

## 15. Status

| Phase | PR | State |
|---|---|---|
| Plan | #358 | revised after the first review |
| NH-1 engine + measles mixing pilot | #359 | done; see `complexity-report.md` |
| NH-2 states built from statuses × stages; every model ported | (stacked on #359) | in review |
| NH-1b policy corrections | | not started |
| NH-0 registry | | waits on the benchmark |
| NH-3 wrappers | | not started |
