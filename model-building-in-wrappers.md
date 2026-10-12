# Building the canonical models from parts: a plan for lighter wrappers

Status: proposal, for discussion. No code changes in this PR.
Related: the contact-layers plan (UofUEpiBio/epiworld#302, `contact-layers.md`
on branch `contact-layers-plan`).

## 1. Question

Every canonical model (SIR, SEIR, SIRCONN, SIRMixing, SEIRMixingQuarantine,
...) is compiled into each wrapper (epiworldR, epiworldpy, epiworldjs) and
bound by hand. Could the wrappers instead build those models from the generic
model-building tools (states, parameters, update functions, viruses), so that
the wrappers get lighter and a new model does not need three sets of bindings?

This document records the answer, the evidence behind it, a review by a panel
of three independent reviewers (R/Python, C++, modeler), and a recommended
order of work.

## 2. Short answer

- **Binary size is not a reason.** Maintenance is. The canonical model classes
  are about 15% of the machine code of epiworldR, and the 23 MB library is
  almost all debug information (Section 3).
- **Mixing models cannot be rebuilt from the current builder.** They depend on
  a model subclass that owns per-run state (Section 4). The state is the
  missing primitive, and it is small.
- **Recipes must live once, in C++.** Three wrappers each rebuilding the
  recipes would copy the drift that already exists between them (Section 6).
- **Keep the named constructors** (`ModelSEIRMixing(...)`, ...). They become
  thin recipes; users and CRAN documentation are unaffected.
- **A first small PR** unlocks the goal: one mixing primitive, with the two
  mixing models ported onto it and results identical (Section 8).
- **Three bugs** turned up while reading the code. They are fixed in separate
  PRs (Section 7), so the work below starts from correct baselines.

## 3. What was measured

| | Value |
|---|---|
| `epiworldR.so` as built (Linux, with `-g`) | 23 MB |
| Same, `strip --strip-debug` | 1.5 MB |
| Machine code in the library (sum of `T/t/W/w` symbols) | ~790 KB |
| Of which, the 16 canonical model classes | ~115 KB (~15%) |
| `epiworldR/src/epimodels.cpp` | 536 lines, 16 `[[cpp11::register]]` constructors |
| `epiworldR/R/Model*.R` | 1,586 lines, mostly roxygen |
| `epiworldpy/epiworldpy/model.cpp` | 574 lines (all model exports) |
| `epiworldpy/epiworldpy/docstrings/epimodels.hpp` | 560 lines |
| `epiworldjs/cpp/registry.hpp` | 368 lines (already a model registry) |

Reading: removing the models from the binaries saves little. Stripping debug
symbols saves far more, if size matters (for example, for the WASM build).
The real saving is the per-model glue: about 1,500 lines across the three
wrappers, of which the documentation (roxygen, docstrings) stays.

## 4. Why the mixing models cannot be rebuilt from parts today

`ModelSIRMixing` and `ModelSEIRMixing` are subclasses of `Model` because the
mixing sampler needs a home:

- The susceptible update casts the model to the subclass to reach its
  `sampler::Mixing` and its `ContactMatrix`
  (`models/seirmixing.hpp`, `m_down->mixing.sample(...)`).
- A per-step hook calls `mixing.update(*m)`; it is the first global event, so
  it runs after the day's transitions and before user events
  (`models/seirmixing.hpp:289`, `models/sirmixing.hpp:263`).
- `reset()` and `clone_ptr()` are overridden so that each `run_multiple()`
  clone gets its own sampler state.
- `queuing_off()` is required (`models/sirmixing.hpp:281`).

The R builder (`Model()`, `add_state()`, `update_fun_susceptible()`,
`update_fun_rate()`, `src/model_builder.cpp`) produces a plain `Model<>` with
none of this. Writing the update function in R is not an option: it runs for
every agent every day, and `run_multiple()` runs in parallel threads where R
cannot be called.

## 5. The review panel

Three reviewers read the code independently, each briefed with the findings
above. Two were told to favor lean implementations.

### 5.1 Wrapper maintainer (R and Python)

Verdict: support, with changes.

- Of the per-model surface, only about 500 lines of C++ in epiworldR and about
  150 in epiworldpy disappear; roxygen, argument checks, S3 classes, and the
  per-model tests stay. Users and CRAN need `?ModelSEIRMixing`.
- Drift is already real. Python's mixing constructors name arguments `vname`
  and `avg_incubation_days` (`epiworldpy/model.cpp:487-491`); R uses `name`
  and `incubation_days`. epiworldjs built its own table of models. Per-wrapper
  recipes would make it worse.
- The R `add_virus` S3 methods (`R/virus.R`) encode each model's states in R,
  and the mixing classes had none (fixed, Section 7).
- `get_contact_matrix()` / `set_contact_matrix()` in R do a
  `dynamic_cast<ContactMatrix*>` on the model (`epiworldR/src/model.cpp:379`).
  When the model stops inheriting from `ContactMatrix`, they throw.
- Do not remove or rename exported `Model*()` functions; reverse dependencies
  (epiworldRShiny) call them. Keep the S3 class vectors identical.
- Leanest path: composition in the core first; then one registry entry point
  that each wrapper binds once, with the named functions as short shims.
  Reject a declarative spec read by the core (a parser, a schema, and a second
  language for 16 fixed models).

### 5.2 C++ developer

Verdict: support the goal; Phase 1 of the contact-layers plan is larger than
the goal needs.

- **Reuse `GlobalEvent` instead of a new `ContactLayer` hierarchy for the
  first step.** It already has a virtual `reset(Model*)` run once per run and
  replicate, a per-step `operator()`, and a `clone_ptr()` that `Model`
  deep-copies (`globalevent-bones.hpp`; `model-meat.hpp:653-655` and
  `2398-2399`). `Bubbles` already keeps per-run state this way. A
  `MixingEvent : GlobalEvent` holding the `ContactMatrix` and `sampler::Mixing`,
  with a named susceptible-update functor, makes `ModelSIRMixing` a plain
  `Model`.
- **Do not fold mixing into the default susceptible update.** Push mode treats
  the default updater as pushable and skips it
  (`model-meat-transmission.hpp:53-62`), which would silently drop mixing
  transmission. Queueing and the timing of `mixing.update()` (after the day's
  transitions, before user events) would also change results. Use a distinct
  named functor and switch queueing off automatically.
- Bit-for-bit results are achievable if the draw order in `Mixing::sample`
  (one `rbinom` per non-empty group, a `runif` per draw, then `roulette`), the
  early return for agents without an entity, and the multiplication order of
  the transmission probability are kept.
- `ModelSIRCONN` is not "mixing with one group" in random-number terms (one
  model-level `rbinom`, then `runif_index` over its own infected list), so it
  needs its own sampler or an accepted, documented change in results.
- Few non-generic pieces remain: Mixing, Connected (CONN family), a logit
  exposure sampler (SIRLogit), and DiffNet exposure. SIRLogit and DiffNet need
  only immutable configuration captured by value; Mixing, CONN, SURV and the
  quarantine model need per-run state, which the event mechanism provides.
- Over-engineering flags: moving ties into `layer::Network` (Phase 2 of the
  contact-layers plan, about 100 references) does not serve this goal; weights
  and regrouping in Phase 1 are new features, not a refactor; `sample_reverse`,
  `supports_queue`, and weights should not exist until a phase uses them.

### 5.3 Computational epidemiologist (modeler)

Verdict: support, with changes.

- Keep the named one-liners for teaching, quick checks, and benchmarking
  against published results. Composition is what outbreak work needs
  (households, schools, community; isolation keeping the household layer),
  and nothing today can express it.
- The builder is not ready to carry that load: states are 0-based integers.
  The vignette error in Section 7 is an example of what that invites.
- Canonical recipes should live in one place, and the same definition should
  feed the Mermaid diagram (the hand-written `models/*.mmd` can go stale), a
  methods table, and be stored with each run.
- For new multi-layer models, default to the hazard rule
  `P = 1 - prod(1 - p)`. The current roulette gives
  `P(infection) = S / (1 + S)` for `S` the sum of odds. With 10 contacts at
  `p = 0.1` that is 0.526 against 0.651, and the gap grows as layers stack,
  which would bias household secondary attack rates downward. Legacy models
  keep the roulette for reproducibility.
- Reconsider "no per-layer beta" and put layer scaling (school closure) in the
  first layer release; modelers set relative transmissibility by setting.
- Priorities: affiliation layers, isolation and quarantine via weights, contact
  tracing on every layer type, and a layer id on each transmission (secondary
  attack rate by setting) are must-haves. Weekday schedules are valuable.
  Transit persistence is nice to have. Missing from the plan: age-specific
  susceptibility and infectiousness, age-structured layers from contact
  surveys, how vaccination and post-exposure prophylaxis tools interact with
  layers (the measles models are the real use case and a good acceptance
  test), calibration over layer parameters, and synthetic-population helpers.

### 5.4 Where the panel agrees and disagrees

Agreed: drop binary size as a motivation; keep the named constructors; recipes
live once; keep `SEIRMixingQuarantine` compiled until a recipe reproduces it;
add a same-seed equivalence test before deleting any class.

| Topic | Panel | Position taken here |
|---|---|---|
| Home for per-run state | C++: `GlobalEvent`; plan: `ContactLayer` | `GlobalEvent`-based `MixingEvent` first; `ContactLayer` when several layers must combine in one draw |
| Layer-aware default susceptible update | C++: breaks push, queue, timing | Distinct named functor |
| Recipes | Modeler: declarative spec. Programmers: registry in C++ | C++ registry with rich parameter metadata (Section 8, step 4); revisit a spec only if needed |
| Phase 1 weights, regrouping; Phase 2 | C++: defer | Defer |

An earlier suggestion in this discussion, a layer-aware default susceptible
update, was wrong for the push and queue reasons above.

## 6. Relation to the contact-layers plan

The two efforts share their first step: giving a plain `Model` owned, cloneable
per-run sampler state. The plan builds that as `ContactLayer`; the C++ reviewer
shows `GlobalEvent` already provides it. The suggested changes to the plan:

1. **Phase 1:** deliver the mixing sampler as an event-owned primitive plus a
   named functor (`MixingEvent`, `UpdateSusceptibleMixing`) and make the two
   mixing models plain models; leave weights and regrouping for Phase 4. Keep
   the plan's draw-for-draw guarantee.
2. **Phase 2:** defer the move of ties into a network layer until multi-layer
   models are being built.
3. **Phase 3:** introduce `ContactLayer` here, when several sources of contacts
   have to combine in one draw, on top of what Phase 1 built.
4. **Phase 6 (bindings):** bind primitives (`add_layer()`, a mixing-layer
   constructor taking a matrix and group assignment) plus the registry of
   Section 8; do not bind models one by one.
5. **Open decisions to add:** where canonical recipes live (Section 8); the
   combination rule default for new layer models (hazard) versus legacy models
   (roulette); per-layer transmissibility and layer scaling in the first layer
   release; whether the exposed state transmits in each model (Section 7).
6. **Accessors:** `get_contact_matrix()` / `set_contact_matrix()` must move to
   the new owner in the same release in every wrapper.

## 7. Bugs found along the way

| | Where | Status |
|---|---|---|
| Exposed agents transmit in the network `ModelSEIR` and `ModelSEIRD`, contrary to `docs/impl/core-simulation-models.md` ("not yet infectious"). `ModelSEIRCONN`, `ModelSEIRNetworkQuarantine`, and the mixing models count only Infected agents. With an incubation so long that nobody leaves Exposed, 10 initial cases turn into 1,000. | epiworld, `models/seir.hpp`, `models/seird.hpp` | PR open (UofUEpiBio/epiworld#313), with an end-to-end test (`tests/44-seir-exposed-not-infectious.cpp`) that fails without the fix |
| SEIRD builder vignette excludes Recovered and Deceased (`exclude = c(3L, 4L)`) instead of Exposed (index 1), so exposed agents transmit. | epiworldR, `vignettes/model-builder.qmd` | PR open (UofUEpiBio/epiworldR#210) |
| `add_virus()` fails on `ModelSIRMixing`, `ModelSEIRMixing`, `ModelSEIRMixingQuarantine` with `has no -init- state`: the mixing classes have no `add_virus` method. | epiworldR, `R/virus.R` | Same PR, with a test |

The first row changes simulation results of network SEIR and SEIRD models
(fewer early infections). The existing C++ tests that already work around it
by overriding the susceptible function (for example, `34e`) are unaffected.

## 8. Recommended order of work

Each step ships on its own and keeps the suite green; steps 3 and 4 are pure
refactors with results identical for a fixed seed.

1. **Correctness fixes** (Section 7). Pull requests open; they need to merge before the equivalence tests of step 3 are written, since step 3 compares against the models as they behave then.
2. **Move per-model virus states into C++.** Store, on the model, the states a
   virus enters, ends in, and is removed to. Then the eight `add_virus.*` S3
   methods in epiworldR (and their equivalents elsewhere) can be deleted, and
   the bug class behind the third row of Section 7 disappears.
3. **A mixing primitive.** `MixingEvent` plus a named susceptible-update
   functor; port `ModelSIRMixing` and `ModelSEIRMixing` to plain models built
   from them; queueing off automatically; the clone hazard handled (a functor
   that caches an event pointer is copied into each clone; re-resolve it per
   model). Test: a model-level equivalence test per ported model (same seed,
   identical histories and transition matrices, in `run()` and
   `run_multiple()`) before the old class is deleted. Same release: switch
   `get_contact_matrix()` / `set_contact_matrix()` in the wrappers off
   `dynamic_cast<ContactMatrix*>`.
4. **A registry in the core** (`make_model(id, params)`, `list_models()`;
   UofUEpiBio/epiworld#312). Wrappers bind these two once; the named
   functions become shims keeping their documentation.
5. **Further samplers**, as needed to build the rest: Connected (CONN family),
   logit exposure, DiffNet exposure. `SEIRMixingQuarantine` stays compiled
   until a recipe reproduces it exactly.
6. **Contact layers** (the plan above, revised as in Section 6), guided by the
   modeler's priorities, with the measles mixing-risk-quarantine model as an
   acceptance test.

Expected outcome: about 1,000 lines of per-model glue removed across the three
wrappers (an estimate, Section 3), the same user-facing API, and every wrapper
gets every model from one place.

## 9. Risks

- **Reproducibility.** A recipe must register states, parameters, and samplers
  in the same order as the legacy class, or same-seed results change silently.
  The equivalence tests in step 3 guard this.
- **Push and queue coupling.** Anything that changes how the susceptible update
  is recognized (`transmission_prepare()`) can silently disable mixing in push
  mode.
- **API and CRAN.** Never remove exported constructors; a rename needs a
  deprecation cycle; S3 class vectors stay identical.
- **Registry scope.** Parameters are heterogeneous (scalars, a contact matrix,
  entities that must be added before a mixing model runs). The metadata format
  has to stay small, or it becomes the declarative spec the reviewers advised
  against.

## 10. Verification status of the claims above

Checked directly while preparing this document: all numbers in Section 3; the
mechanism of Section 4; the `GlobalEvent` hooks and clone behavior; the push
eligibility rule; the R `dynamic_cast`; the Python `vname` argument; the three
bugs of Section 7 (each reproduced, and each fix confirmed to change the
behavior); and the roulette and hazard probabilities.

Reported by the reviewers and not re-checked: the ~500 and ~150 line removal
estimates; that `epiworldRShiny` calls the exported constructors; the
bit-for-bit conditions in Section 5.2 (they describe what must be preserved,
not a completed comparison); and the claim that `SIRCONN` differs in random
draws from a one-group mixing model.

Linked work: epiworld#313 (SEIR/SEIRD fix), epiworldR#210 (R fixes), epiworld#312 (registry issue), epiworld#302 (contact-layers plan).
