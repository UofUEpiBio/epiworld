# Natural history and agent status: separating what the disease does from where the agent is

**Status: draft for review (2026-11-03).** This is an experimental plan on the
`natural-history` branch, cut from `v1-layers`. Nothing here is settled. The
goal is to collect reviews (human and agent) and then try the core on this
branch to see how it goes before deciding whether, and when, it joins
`v1-layers` / 1.0.0. Reviewers: add comments to the "Open questions" section
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

## 3. Proposal: three layers of agent state

1. **Status, owned by the model.** For example free, quarantined, isolated,
   hospitalized or dead. It governs contacts (layer participation),
   movement and interventions. Each status records the day it was entered and
   what triggered it (traced contact, symptoms, test). Every agent has a
   status, infected or not.
2. **Infection episode, owned by the pathogen.** This is the natural-history
   state of one infection: latent, prodromal, infectious and so on. It exists
   only while the agent is infected. Each state carries attributes (infectious,
   symptomatic, severe), because the latent period (infection to infectious)
   and the incubation period (infection to symptoms) differ. In measles,
   prodromal agents are infectious before the rash.
3. **Immunity, per agent and pathogen.** Susceptibility, how protected the
   agent is, where the protection came from (vaccine, infection, maternal),
   and waning. It exists before infection and outlives it. Recovery, waning,
   vaccination, post-exposure prophylaxis (PEP) and, later, measles immune
   amnesia act on it.

**Immunity is staged.** For now it stays where it is: **tools** are
per-agent immune objects, and `post_recovery` hands one out. A removed virus
must **not** be kept around in a terminal "recovered" state as an immunity
record. That would separate "has a virus" from "is a carrier" in the queue,
`sample_virus` and both samplers. A dedicated immunity record is a later step
(Section 11, NH-5). This means single-pathogen models still use a "Recovered"
status, which keeps a little of the confounding.

**Death is a status.** A pathogen requests it, and the model carries it out
once for the agent, closing every open episode and recording the cause
(Starsim's `request_death`).

## 4. A registry of virus definitions (the "controller virus")

The natural-history specification (labels, update functions, infectious
mask) has to live somewhere shared. That leads to a second change, which is
also worth doing on its own.

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

- **The model holds virus definitions in a registry** (a `std::vector` indexed
  by id). A definition holds everything that is shared: the name, the
  parameter references, the probability functions, the natural-history
  specification (states, update functions, attributes), the default
  status transitions (today's `state_init`/`post`/`removed`), and the
  distribution function.
- **Variants** are entries in the registry, each with a sequence and a parent
  definition. `DataBase::record_virus` already keys variants by sequence
  hash and tracks lineages, so the registry can replace that bookkeeping
  instead of duplicating it.
- **An agent holds a small record inline**, with no heap allocation:

  ```cpp
  struct Infection {
      int variant  = -1;   // registry index; -1 = not infected
      int nh_state = -1;   // natural-history state (index into the definition)
      int date     = -99;  // day of infection
      int nh_since = -99;  // day the current NH state was entered
  };                       // 16 bytes, the same as today's VirusPtr
  ```

  An agent's size stays the same (the `VirusPtr` it replaces is 16 bytes),
  but each infection no longer costs a 320-byte allocation plus copies.
- **Callbacks receive the definition and the record**:
  `(Agent*, const VirusDef&, Infection&, Model*)`. User code no longer gets a
  mutable full `Virus&` per infection.
- **Mutation** returns a new sequence. The model looks it up in the registry
  or adds it, then repoints the record's `variant`. Per-instance sequences go
  away.

### 4.3 Benefits

- **Cheaper copies, less to clear.** An infection or recovery is a write to
  16 bytes, with no allocation. Copying a model for `run_multiple` copies
  plain records.
- **One source of truth.** Changing a function on a definition changes it
  for every infected agent. Today, changing a virus after it has spread
  does not reach the copies agents hold; only parameters read through the
  model (`ParamRef`) do.
- **Natural fit for the NH split.** The natural-history specification lives
  on the definition, and the per-infection NH state lives in the record.
- **Simpler for users.** A virus is "defined once, referred to by id".
  `agent.get_virus()` returns a view built from the definition and the record.

### 4.4 Risks

- **User code that stores per-infection data on the `Virus` object.** Today
  someone can change one agent's copy (its sequence, a captured counter) and
  get per-infection behaviour. Per-infection data then needs an explicit
  home. Proposal: none in the first version. If a model needs it, it uses
  the agent's user data or a side vector, as the measles models already do.
- **The wrappers.** epiworldR, epiworldpy and epiworldjs return virus objects
  from agents. They would return a lightweight handle (definition id plus a
  copy of the record) with the same accessors.
- **Is the memory gain real?** This is a hypothesis until it is measured.
  NH-0 (Section 11) measures it on the `epiworld-benchmark` scenarios (peak
  memory, time per step, `run_multiple` copy time) before anything else
  builds on it.
- **Tools** follow the same clone-per-agent pattern. They are out of scope
  here, but the same registry design would apply later.

NH-0 changes no draws, so its acceptance test is **identical results** under
a fixed seed for every existing test and model.

## 5. How the layers interact

Neither layer writes the other's state. They talk through a small set of
mechanisms:

| Direction | Example | Mechanism |
|---|---|---|
| natural history → status | rash leads to isolation; severe disease leads to hospital; death | `model.on_nh_change(fun)`: the model reacts to NH transitions. The virus never knows the model's status codes. |
| status → natural history | treatment in hospital shortens the course or lowers mortality; PEP | A tool added on entering the status and removed on exit (`recovery_enhancer`, `death_reduction` already exist). |
| status → contacts | quarantine removes the agent from layers; leaky isolation | Phase 4 participation keyed by status: `layer.exclude_status({Q, I, H})`, with per-agent flags only for exceptions. This replaces `set_excluded_states`. |
| natural history → infectiousness | prodromal and rash are infectious | The `infectious` attribute of the NH state on the virus definition. This replaces `Model::set_infectious_states`. |
| immunity → susceptibility | vaccine, prior infection | Tools, as today (later the immunity record). |

Status rules may still read the NH state (release from isolation counts
from rash onset; tracing treats infected and uninfected contacts
differently). The state space is moved, not removed. The measure of success is
fewer and cleaner functions: for measles, from 11 update functions to about
4 status functions plus 3 natural-history functions on the virus.

## 6. Update order and competing risks

- **One snapshot per step.** Both layers read the state as of the start of
  the step. Both queue events, and the events apply at the end of the step,
  as today.
- **Order within the end of step:** natural-history events first, then the
  status rules that react to them (through `on_nh_change`) in the same step.
  This order is documented and tested.
- **Conflicts:** removal wins. Death or the removal of the virus ends the
  episode, and an NH event queued for a virus that was already removed is
  dropped.
- **Competing risks within a layer** are settled with one draw over the
  combined hazards: the probability of leaving is `1 - exp(-Σh)`, split in
  proportion to each hazard. Today they are settled by the order of `if/else`
  branches. This keeps a per-case probability (e.g. "hospitalization
  probability") separate from a daily rate, which today's measles parameter
  "Hospitalization rate" conflates. It is a per-model choice (the hazard vs.
  roulette results of 2026-10-20 apply).

## 7. Engine changes

- **Events** carry a target: status or natural history (plus the variant for
  NH). The same-day undo in `events_run` (which uses `state_prev`) works per
  target.
- **Agent:** `state` becomes the status (the same field, so the existing API
  keeps working), and `virus` becomes the inline `Infection` record.
- **The update loop** runs the status function for every agent whose status
  has one, and the NH function of the agent's infection, if any.
- **A set of agents whose status needs updates.** The queue only visits
  agents near infectious contacts, so a quarantined, uninfected agent far from
  any case would never be updated. The status layer needs its own active set
  (agents whose status has an update function). Today this is hidden because
  two of the three measles models call `queuing_off()` and the third runs on
  a mixing layer the queue does not see.
- **Push transmission's state index** counts infectious carriers (from the
  NH attribute) rather than carriers per state.
- **The Database:**
  - `today_total` / `hist_total` stay as status counts, so the saver, the
    model diagram and `get_hist_total` keep their meaning;
  - per virus definition: NH state counts per day and an NH transition
    matrix;
  - a joint table, status × NH state per virus per day (generalizing today's
    `today_virus[virus][state]`);
  - `EPI_DEBUG` checks that the three tables sum consistently (like the ones
    in `database-meat.hpp`).

## 8. API sketch

```cpp
// Natural history: owned by the virus definition, shared by all infections
NaturalHistory<> nh;
int L  = nh.add_state("Latent",    latent_update);
int P  = nh.add_state("Prodromal", prodromal_update, /*infectious=*/true);
int Rs = nh.add_state("Rash",      rash_update,      /*infectious=*/true,
                      /*symptomatic=*/true);

Virus<> measles("Measles");
measles.set_natural_history(nh, /*initial=*/L);   // no NH: behaves as today

// Status: owned by the model (today's add_state, renamed in docs only)
int FREE = model.add_state("Free",        free_update);
int QUAR = model.add_state("Quarantined", quarantined_update);
int ISOL = model.add_state("Isolated",    isolated_update);

// Coupling: the model reacts to the disease
model.on_nh_change([&](Agent<>* p, const VirusDef<>& v, int from, int to,
                       Model<>* m) {
    if (to == Rs && detected(p, m))
        p->change_status(m, ISOL);
});

// Inside an NH update function
p->change_nh(m, Rs);          // event with target = natural history
p->get_infection().nh_state;  // read
```

Names are placeholders for review (status vs. state, `change_nh` vs.
`advance`).

## 9. Outputs

- Per virus: true incidence (new episodes), detected incidence, symptom-onset
  incidence, prevalence by NH state, and NH transitions.
- Status: daily occupancy and person-days (quarantine-days, isolation-days,
  bed-days), entries by trigger, and quarantined agents who were never
  infected.
- The joint status × NH table per day. "Infectious but free" is the key
  leak for an intervention.
- Later, an episode line list written to disk: infection, infectious onset,
  symptom onset, detection, isolation and outcome dates. Generation and serial
  intervals and the share of transmission before isolation follow from it.

## 10. Compatibility and scope

**Additive by default.** A virus without a natural history behaves exactly as
today: status equals state, and `state_init`/`post`/`removed` act on status.
The SIR/SEIR models and the wrappers keep working.

**Only models that adopt it break.** Measles goes from 12 states to about 4
statuses plus 5 NH states, so its output labels change. A generated
"flattened" label for each pair seen (e.g. "Quarantined Latent") could keep
old plots readable, but it adds code: only if the wrappers need it.

**Out of scope for this plan:**

- **Co-infection.** An agent holds one infection today (`sample_virus`
  throws on a second). Several simultaneous infections need a small vector of
  records per agent, exposure draws per virus, and a rule for competing
  exposures (D1 in `contact-layers.md`). The registry makes that cheaper
  later (a vector of 16-byte records), but it is a separate project. Without
  it, this plan gives strains with different natural histories, not several
  diseases at once.
- **Cross-pathogen effects** (interference, cross-immunity, immune amnesia):
  a later model-level hook, in the style of Starsim connectors, once
  co-infection and the immunity record exist.
- **Tools on the registry.**

## 11. Phases on the `natural-history` branch

Each phase is a PR into `natural-history`, measured and reviewed before the
next. If the branch proves itself, it is merged into `v1-layers` in the
order agreed with the contact-layers plan.

- **NH-0: virus registry.** Definitions in the model, `Infection` records in
  agents, variants in the registry, the same public API through a view.
  Acceptance: identical results for every test under a fixed seed; memory and
  time measured on the `epiworld-benchmark` scenarios against `v1-layers`
  (CPU time, interleaved runs; see the benchmarking notes in Section 8 of
  `contact-layers.md`).
- **NH-1: natural history on the definition.** NH states with attributes,
  update functions, the event target, the status active set, the database
  tables, `on_nh_change`. The infectious mask moves from the model to the
  definition. Acceptance: every existing model unchanged (they have no NH),
  plus one new end-to-end test.
- **NH-2: pilot port of `ModelSEIRQuarantine`** (10 states, the smaller
  case). Acceptance: statistically equivalent results (attack rate,
  quarantine-days, R0) within Monte Carlo error. Count the update functions
  before and after.
- **NH-3: the measles models.** This depends on Phase 4 participation keyed
  by status. Ideally it is done as part of the measles consolidation, so the
  measles results change once rather than twice. Acceptance: the
  consolidation's R0 and transition-rate checks, and answers to the measles
  questions in Section 12.
- **NH-4: wrappers.** Accessors for NH histories, transitions and the joint
  table in epiworldR, epiworldpy and epiworldjs.
- **NH-5 (later): the immunity record**, then co-infection, then
  cross-pathogen hooks.

**Fit with `contact-layers.md`.** Phase 4 participation should be designed as
"status leads to exclusion" whichever way this goes. NH-0 and NH-1 are
additive and could land before or after 1.0.0. The measles relabelling
should land in 1.0.0 if it lands at all.

**Rough effort:** NH-0 and NH-1 together are about 1.5-2.5 thousand lines
with tests, in 2-3 PRs; the pilot and measles ports are moderate; the wrappers
need new accessors.

### Testing

Following `AGENTS.md`: few end-to-end test files, one `EPIWORLD_TEST_CASE`
each, using fixed (drawn-once) networks.

- **Equivalence:** with one virus and no natural history, every model's
  results stay identical (NH-0, NH-1).
- **Mean field:** an SEIR on full mixing written with NH states agrees with
  the ODE solution (final-size relation).
- **Separation:** quarantine and isolation do not change NH durations unless
  a tool says so. For example, with deterministic durations, the latent and
  prodromal periods of quarantined agents match those of free agents exactly.
- **Coupling:** with probability 1 of detection, every agent entering Rash
  is isolated the same day, and the joint table never shows "Rash × Free".
- **Bookkeeping:** status, NH and joint counts sum to the same totals every
  day; build the tests standalone with `-DEPI_DEBUG` as well (`make test`
  skips those checks).

## 12. Open questions for reviewers

1. **Names.** "Status" and "natural history", or keep "state" for status and
   call the NH layer "stage"? The measles code already says "stage".
2. **Is the registry worth it on its own?** NH-0's numbers decide. If the
   gain is small, NH-1 can keep a `shared_ptr` to a shared NH specification
   from today's clones instead.
3. **Per-infection user data.** None (use agent user data or side vectors),
   or a small typed slot in `Infection`?
4. **Where competing risks are settled.** A one-draw helper in the engine, or
   left to each model's update functions?
5. **Should a status function run every day for every agent who has one**
   (simple, slower), or only for agents whose status changed or who have a
   pending timer (faster, more bookkeeping)?
6. **Measles semantics the port forces into the open:**
   - Should hospitalization stop the rash clock?
   - Should quarantine plus rash always mean isolation?
   - Should the quarantine exemption depend on tools or on immune status?
   - Does PEP act on the infection (abort or modify the episode) or on
     immunity?
7. **The flattened joint label for backward compatibility:** needed by the
   wrappers or not?
8. **Timing.** Should this aim for 1.0.0 at all, or stay on its branch until
   after it?

## 13. Risks

| Risk | Mitigation |
|---|---|
| Bookkeeping errors in the same-day undo across two targets | `EPI_DEBUG` consistency checks; the bookkeeping test above |
| Quarantined agents silently never updated (queue) | The status active set; a test with queuing on and a quarantined agent far from any case |
| Performance regression in the push state index | Benchmarks per phase in `epiworld-benchmark` |
| User code relying on mutable per-infection `Virus` copies | Document in NH-0; search the wrappers and the measles package before merging |
| The complexity moves into status functions that branch on NH | Count functions before and after in NH-2; reject the design if the pilot is not simpler |
| Two breaking changes to measles (consolidation, then NH) | Do NH-3 inside the measles consolidation, or not at all before 1.0.0 |
| Scope creep into co-infection | Stated out of scope (Section 10) |

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
- Mina et al. 2015. Measles immune amnesia. *Science* 348:694.
  doi:10.1126/science.aaa3662

## 15. Status

| Phase | PR | State |
|---|---|---|
| Plan | this branch | draft, collecting reviews |
| NH-0 registry | | not started |
| NH-1 natural history | | not started |
| NH-2 SEIRQuarantine pilot | | not started |
| NH-3 measles | | waits on Phase 4 / the measles consolidation |
| NH-4 wrappers | | not started |
