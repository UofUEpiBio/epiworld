# State durations: timers and transitions for epiworld

**Status:** proposal for review (2026-10-27). Nothing here is settled until the
review of this PR. It is a companion to `contact-layers.md` (the 1.0.0 plan on
`v1-layers`); Section 8 says how it folds into that plan.

**Base:** every code reference is to `origin/v1-layers` at `1ab03e2`
("ContactTracing: get_n_agents() (#329)"). Section 11 lists the claims a
reviewer should check, with the commands to check them.

**Goal:** let any state of a model last a time that is not geometric (a
lognormal latent period, a fixed 21-day quarantine), with the dynamics handled
once by the core, for any number of states, while keeping today's
memoryless behavior, and its results, where a model wants it.

---

## 1. Motivation

### 1.1 Every state today is left with a daily probability

A state ends when a daily draw succeeds (`runif() < 1 / mean days`, or a
roulette over several exits). The time spent in it is then geometric: the most
likely duration is one day and the standard deviation is close to the mean.
Real stages do not look like that. The measles incubation period is lognormal,
with a median of 12.5 days and a dispersion of 1.23 (Lessler et al. 2009).

It matters most where a policy works with a fixed window. With a mean of 12.5
days, a geometric latent period outlasts a 21-day quarantine 17% of the time;
the lognormal one does so 0.6% of the time:

```r
(1 - 1/12.5)^21                              # 0.174, geometric
1 - plnorm(21, log(12.5), log(1.23))         # 0.006, lognormal
```

It also shapes the generation interval, the PEP windows, and how rash onset
lines up with infectiousness.

### 1.2 Fixed durations already exist, written by hand each time

| Where | What | How |
|---|---|---|
| `include/measles/measlesmixing.hpp` (and the other two measles models) | Quarantine release | `day_flagged[id]` stored when quarantined; each step `Quarantine period <= today - day_flagged` |
| same | Isolation release | anchored on `day_rash_onset[id]`, set when Rash starts, read in Isolated and Isolated Recovered |
| `include/measles/immunoglobulin-meat.hpp` (`_set_immunity`) | IG protection | draws a normal half-life on first use, stores `_removal_time` |
| `include/measles/measlesmixingriskquarantine.hpp` | Quarantine length by risk level | `m_get_risk_period(agent)` |

Each is a timer, written differently, with its own state vectors and its own
reset logic. A general mechanism removes them.

---

## 2. How states work today (facts the design relies on)

- **State changes are events, applied at the end of the step.**
  `Agent::change_state()`, `set_virus()`, `rm_virus()` queue an event;
  `Model::events_run()` (`include/epiworld/model-meat.hpp`, from line 349)
  applies them, sets `p->state = new_state`, and records
  `p->state_last_changed = today()`. `events_run()` can run several times in a
  step: global events call it (e.g., `InterventionMeaslesPEP::operator()`), so
  an agent can change state more than once in a day; the transition matrix
  already handles that with `state_prev`.
- **Seeding goes through events too** (`events_run()` is called after the
  virus and tool distribution at reset), so a timer hook in `events_run()` sees
  seeded agents.
- **Agents with a virus are visited every step** (the queue adds them; the
  measles models run without the queue), so an update function that checks a
  timer runs every day the timer can fire.
- **Each infection has its own `Virus` copy**: `_event_add_virus`
  (`include/epiworld/agent-events-meat.hpp:5`) stores the event's copy and
  sets its date.
- **Rates live on the virus and on tools.** `Virus` has `incubation`,
  `probability_of_recovery`, `probability_of_death` (`virus-bones.hpp:55-58`)
  with setters by parameter name, number or function; tools have
  `recovery_enhancer` and `death_reduction`. `default_update_exposed`
  (`agent-meat-state.hpp:148`) combines them:
  death `= p_d (1 - reduction)`, recovery `= 1 - (1 - p_r)(1 - enhancer)`,
  then `roulette(2)`.
- **A generic transition factory exists:** `new_state_update_transition(params,
  targets)` (`agent-meat-state.hpp`, before line 148) builds an update function
  from parameter names (`ParamRef`) and target states, with a roulette.
- **Three different draw rules are in use for a state's exits:**
  1. `runif() < p` (ModelSEIR's exposed state, measles latent and prodromal);
  2. `roulette()` (`misc.hpp:128`): independent competing events, odds
     `p / (1 - p)`, `P(none) = 1 / (1 + sum of odds)`;
  3. `sample_from_probs()` (`model-rand-meat.hpp:269`): mutually exclusive
     probabilities, `P(k) = p_k` (measles Rash and Isolated).

  Rules 1 and 2 agree in distribution for a single exit but consume the
  uniform differently (`r < p` vs `r >= 1 - p`); rules 2 and 3 differ in
  distribution when there are several exits. This matters for identity
  (Section 7.2).
- **Parameters are read through `ParamRef`** (`param-ref.hpp:53`): a name
  resolved to a position once per parameter layout; values are never cached.

---

## 3. How we got here (alternatives already discussed)

Recorded so a reviewer does not re-propose them without new arguments.

1. *A clock on the virus, one per disease stage* (first draft). It ties timers
   to infections, so it cannot time a quarantine of a susceptible agent, and
   a quarantined latent agent needs two clocks at once (latent period and
   quarantine). Dropped in favor of timers on the agent.
2. *Geometric vs "other" distributions.* The real split is the mechanism:
   checked every step (hazard) or drawn at entry. A hazard that depends on the
   days already spent in the state describes any discrete distribution and
   still responds to tools and global events at once.
3. *A virtual `Virus::recover(model)` implemented by each disease.* Kept the
   idea that the disease defines its natural history, but: it must cover every
   state, not only recovery (measles has Latent, Prodromal, Rash, Hospitalized);
   it must be a question ("does it end today?"), not an action, because the
   destination depends on the model (Recovered vs Isolated Recovered); and it
   should be configured with objects rather than subclassing, so R, Python and
   JS users set a lognormal without callbacks into the interpreter every step.
4. *Drawing the whole course at infection.* Same distribution for independent
   stages, but it stores stages that never happen when the course branches.
   Only correlated stage durations need it; not requested.
5. *An event calendar* (a priority queue of scheduled transitions). Agents in
   timed states are visited every step anyway; a comparison with an end day is
   cheaper than today's daily draw.

---

## 4. Design

### 4.1 Timers

A **timer** is something an agent waits for while in a state. It has one or
more **outcomes** (which exit it leads to) and one of two kinds:

- **Drawn** (at entry): when the timer starts, it draws the outcome with its
  probability, then that outcome's duration. It fires on the drawn day. Cheap,
  and the end is known in advance. This is a semi-Markov state: an embedded
  choice of exit plus a duration per exit.
- **Hazard** (checked every step): each step, a draw decides whether it fires
  and through which outcome. A constant probability gives today's geometric
  duration. The probability may depend on the days already spent
  (`days_in`), so any discrete distribution is possible, and tools and global
  events act at once.

```cpp
// Drawn, one outcome
auto latent     = timer::drawn(
  duration::lognormal("Latent period", "Latent period sd")
);
auto quarantine = timer::drawn(duration::fixed("Quarantine period"));

// Drawn, two outcomes: the outcome first ("" takes the remaining probability),
// then its own duration
auto rash = timer::drawn({
  {"Hospitalization prob", duration::lognormal("Days to hospital", "Days to hospital sd")},
  {"",                     duration::lognormal("Rash period", "Rash period sd")}
});

// Hazard: today's behavior
auto infectious = timer::hazard({"Death rate", "Recovery rate"});

// Hazard that depends on the days in the state (not geometric, still live)
auto waning = timer::hazard(
  [](Agent<> & a, Model<> & m, int days_in) { return weibull_h(days_in, m); }
);
```

`weibull_h` stands for any user function returning the day's probability.

**Durations** (`duration::`) are distributions over model parameters
(`contact-layers.md` point 17: scalars are model parameters):
`fixed(mean)`, `geometric(mean)`, `lognormal(mean, sd)`, `gamma(mean, sd)`,
and a user function `(agent, model) -> days`. All are set by mean and sd in
days: papers report those, and switching family keeps a calibrated mean.
Lognormal from mean `m` and sd `s`: `sigma^2 = log(1 + s^2/m^2)`,
`mu = log(m) - sigma^2/2`; gamma: `shape = m^2/s^2`, `scale = s^2/m`.

**Rounding:** a drawn duration is rounded to whole days, at least 1, so a
state lasts at least one step, as today (open item Q8).

### 4.2 Transitions

A model declares, per state, which timers run there and where each outcome
leads:

```cpp
m.add_transition(from_state, timer, {target_for_outcome_0, ...});
m.add_timer(state, timer);   // runs in the state, no exit from it
```

`add_transition` implies `add_timer`. A **carried** timer (`add_timer` only)
keeps running through a state that cannot be left by it. Measles isolation
needs this: it is anchored on rash onset, so it starts in Rash, which is not
left when it ends, and it releases from Isolated and Isolated Recovered.

The same timer object can be used by several states; that is how states
**share** a timer (Latent and Quarantined Latent share `latent`), and each state
maps the shared timer's outcomes to its own targets (Rash's "recover" goes to
Recovered, Isolated's to Isolated Recovered).

A state that has transitions gets a generated update function (as
`new_state_update_transition` does today). A state with a hand-written update
function can still use timers (Section 4.6).

### 4.3 Dynamics

**At a state change** (in `events_run()`, where `p->state = new_state` is
set), for each timer:

| Old state has it | New state has it | Result |
|---|---|---|
| yes | yes | keeps running (start day, end day and outcome unchanged) |
| no | yes | starts: drawn picks outcome and end day; hazard records the start day |
| yes | no | dropped |

Same-day churn: an agent can change state more than once in a step (Section 2).
A dropped timer is only marked `dropped_on = today`; if a later change on the
same day enters a state that has it again, it is revived unchanged. Marked
slots are discarded on the next step. Without this, A -> B -> A in one day
would redraw A's timers.

Agent reset (`Agent::reset()`) clears all timers.

**Each step**, the generated update function of a state, for an agent in it:

1. evaluates its hazard timers (one draw for all their outcomes);
2. if none fired, fires the first drawn timer whose end day has come, in the
   order the state's transitions were declared;
3. moves the agent to the target of the fired outcome.

Hazards go first so that a state lasting `d` days offers `d` daily chances to
every hazard (open item Q2). Timers carried without an exit are never fired;
they only keep their dates.

**Timing.** An agent entering a state at the end of step `t` is first
updated at step `t + 1`. A drawn duration `d` fires at the update of step
`t + d` (the agent spent `d` steps in the state). A hazard sees
`days_in = today - start`, so 1 at its first check. This must reproduce the
measles release rule `Quarantine period <= today - day_flagged` exactly, which
the implementation checks against the current models (Section 7.2).

### 4.4 Storage

Per agent, a small list of slots: `{timer id, outcome, start day, end day,
dropped_on}` (about 16 bytes). Most agents have none (susceptible,
recovered); an infected, quarantined agent has two or three. Proposed: a
`std::vector` per agent reserved on first use (24 bytes when empty); measure
against a fixed inline array before choosing (open item Q6). Slots are copied
with the agent, so `run_multiple()` clones need nothing more.

### 4.5 Durations and probabilities per agent: tools scale parameters

A timer reads its parameters **for an agent**. A tool can scale a parameter
for the agents holding it:

```cpp
ToolVaccine<> mmr("MMR");
mmr.set_param_factor("Prodromal period",     "MMR prodromal factor");
mmr.set_param_factor("Hospitalization prob", "MMR hospitalization factor");
```

The factor is itself a parameter name (point 17). The value a timer sees is
the model's value times the product of the factors of the agent's tools that
target that parameter.

- A **drawn** timer reads it **once, at entry**: vaccination later in the
  state does not change the time already drawn. That is the trade-off of a
  drawn timer; a model that needs an immediate effect uses a hazard timer, or
  the intervention re-times the agent (Section 4.7).
- A **hazard** timer reads it **every step**.
- This covers vaccination status known before the run (the tool is there at
  entry) and vaccination during the run (PEP, campaigns), which precomputing
  at the start of the simulation would miss.
- Agent attributes that are not tools (age, risk level) go through a user
  duration function of `(agent, model)`.

Cost: a parameter read for an agent scans that agent's tools (few). Measure.

### 4.6 Hand-written update functions

Models whose states do more than move (the measles Rash state draws detection
and triggers contact tracing) keep their update functions and ask the core:

```cpp
int k = p->timer_fired(*m, rash);   // -1, or the outcome that fires today
```

and read or set a timer's dates (`get_timer_end()`, `set_timer_end()`), for
example to replace `day_rash_onset` reads with the rash timer's start day.
The keep/start/drop rule still runs in `events_run()`, so these models need
no bookkeeping of their own.

### 4.7 Interventions

| What the intervention does | Effect on timers | Example |
|---|---|---|
| Ends the infection (moves to Recovered) | timers of the old state that Recovered lacks are dropped | effective PEP |
| Moves to a parallel state | shared timers keep running; new ones start | Latent -> Quarantined Latent keeps `latent`, starts `quarantine` |
| Develops the disease during quarantine | the quarantine timer is dropped by the rule, unless the new state carries it | Quarantined Prodromal -> Isolated |
| Changes a rate | hazard timers see it at once; drawn timers do not | a global event changing "Recovery rate" |
| Changes the time left | code sets the end day, or redraws | not needed today |

PEP, step by step, in the measles mixing model with timers: an agent in Latent
is offered PEP. If it works, `rm_virus(RECOVERED)`: Recovered has no timers,
so `latent` is dropped. If not, the agent goes to Quarantined Latent:
`latent` keeps its drawn end day (quarantine does not pause the disease) and
`quarantine` starts. Today, a quarantined latent agent instead becomes
prodromal by a fresh daily draw.

**Not now:** a tool that speeds up or slows down a drawn timer (IG lengthens
the measles incubation; antivirals shorten the infectious period). With an end
day stored, it means rescaling the time left when the tool is added or
removed. Deferred until a model needs it (Q10).

### 4.8 What it replaces

- `new_state_update_transition()` becomes `add_transition()` with hazard
  timers.
- The measles `day_flagged`, `day_rash_onset` (as a timer anchor) and the
  release arithmetic in about ten update functions.
- Possibly `ToolImmunoglobulin::_removal_time` (a timer on a tool, not a state:
  out of scope for now, Q11).
- Virus-level rates stay for now (Q1): the built-in hazard for infected states
  reads `virus->get_prob_recovery()` and `get_prob_death()` with the tools'
  enhancer and reduction, exactly as `default_update_exposed` does, so
  multi-strain models keep working.

---

## 5. Worked examples

### 5.1 ModelSEIR, defaults (results unchanged)

```cpp
// in the constructor
auto latent     = timer::hazard(virus_incubation());     // runif() < 1 / incubation
auto infectious = timer::hazard(virus_death_recovery()); // as default_update_exposed
add_transition("Exposed",  latent,     {"Infected"});
add_transition("Infected", infectious, {"Deceased", "Recovered"});   // SEIRD
```

`virus_incubation()` and `virus_death_recovery()` are the built-in hazards
over the virus's rates (Section 4.8). They must consume the random stream as
the current functions do (Section 7.2).

### 5.2 ModelSEIR with a lognormal latent period

```cpp
ModelSEIR<> m("measles-like", 10000, ...);
m.add_param(2.7, "Latent period sd");
m.set_timer("Exposed", timer::drawn(
  duration::lognormal("Latent period", "Latent period sd")
));
```

`set_timer(state, timer)` replaces the timer of a state that has one
(name to settle in review).

### 5.3 A simple model with a fixed quarantine

```cpp
auto quarantine = timer::drawn(duration::fixed("Quarantine period"));
m.add_transition("Quarantined", quarantine, {"Susceptible"});
```

Every quarantined agent returns to Susceptible exactly `Quarantine period`
days after entering.

### 5.4 Measles (mixing model), whole table

Timers: `latent`, `prodromal` (drawn or hazard), `rash` (two outcomes:
hospitalize, recover), `hospital`, `quarantine` (fixed), `isolation` (fixed,
anchored on rash onset).

| State | Timers (exit -> target) | Carried | Hand-written part |
|---|---|---|---|
| Latent | latent -> Prodromal | | |
| Quarantined Latent | latent -> Quarantined Prodromal; quarantine -> Latent | | |
| Prodromal | prodromal -> Rash | | |
| Quarantined Prodromal | prodromal -> Isolated; quarantine -> Prodromal | | |
| Rash | rash: hospitalize -> Hospitalized, recover -> Recovered | isolation | detection draw, quarantine trigger, Isolated if detected; recovery goes to Isolated Recovered if detected |
| Isolated | rash: hospitalize -> Hospitalized, recover -> Isolated Recovered; isolation -> Rash | | |
| Isolated Recovered | isolation -> Recovered | | |
| Quarantined Susceptible | quarantine -> Susceptible | | |
| Quarantined Recovered | quarantine -> Recovered | | |
| Hospitalized | hospital -> Recovered | | |

What changes in behavior:

- With drawn `latent` and `prodromal`, quarantine no longer redraws them
  (Section 4.7).
- Quarantined Latent whose `latent` and `quarantine` end on the same day: today
  the agent goes straight to Prodromal; with "first declared wins" it goes to
  Quarantined Prodromal, then to Prodromal the next day (its quarantine timer
  is still expired). Q3.
- Rash and Isolated today use `sample_from_probs` (exclusive probabilities);
  the generic hazard uses one rule (Q4).

---

## 6. API summary (proposed names, to settle in review)

| Item | Kind |
|---|---|
| `timer::drawn(duration)`, `timer::drawn({{prob_param, duration}, ...})` | timer factory |
| `timer::hazard({params...})`, `timer::hazard(fun)` | timer factory |
| `duration::fixed`, `geometric`, `lognormal`, `gamma`, user function | duration factory |
| `Model::add_transition(state, timer, targets)`, `Model::add_timer(state, timer)`, `Model::set_timer(state, timer)` | model |
| `Agent::timer_fired(model, timer)`, `get_timer_start()`, `get_timer_end()`, `set_timer_end()` | agent |
| `Tool::set_param_factor(param, factor_param)` | tool |

States are named by their string names in the public API (resolved to ids at
`run()`), so the bindings need no state ids.

---

## 7. Implementation plan

### 7.1 PRs

| PR | Content | Results |
|---|---|---|
| **T1. Core** | `include/epiworld/timer-bones.hpp` / `timer-meat.hpp` (timers, durations), agent slots, the keep/start/drop rule in `events_run()`, `add_transition` / `add_timer` / `set_timer`, the generated update function, `timer_fired` and the date accessors | No model changes: every result identical |
| **T2a. Default models** | SIR, SEIR, SIRD, SEIRD, SIS, SISD, `ModelSEIRQuarantine` (after the consolidation) on `add_transition`; `new_state_update_transition` removed | Identical where the draw rule is reproduced (7.2) |
| **T2b. Measles** | the three measles models on timers; `day_flagged`, `day_rash_onset` and the release arithmetic removed (after 3d) | Distributional acceptance (7.2) |
| **T3. Tool factors** | `Tool::set_param_factor`; parameter reads per agent | Identical without factors |

### 7.2 Identity and acceptance

- **T1:** the identity harness (the scratchpad `idnet.cpp` pattern in
  `contact-layers.md`, Section 11): every model draw-for-draw identical.
- **T2a:** single-exit hazards use `runif() < p` and multi-exit ones
  `roulette()`, as the functions they replace; then draw-for-draw identical.
  Any model that differs is reported with the cause.
- **T2b:** the measles models are not draw-for-draw identical (`sample_from_probs`
  vs the generic rule; Q3 and Q4; quarantine no longer redrawing the latent
  period when drawn timers are chosen). Acceptance as in the consolidation
  plan: 1,000 runs per model, standardized mean difference of final size and
  peak prevalence at most 0.1, sd ratio within 0.9-1.1, **with geometric
  (hazard) defaults**. Fixed timers (quarantine, isolation) must release on
  exactly the same day as today.

### 7.3 Tests (end-to-end, three files, one `EPIWORLD_TEST_CASE` each)

1. **Fixed and drawn durations.** `ModelSEIR`, no transmission, every seed in
   Exposed on day 0, Exposed fixed at 5 days: nobody Infected before day 5,
   everyone on day 5 (`run()`). Lognormal and gamma: over `run_multiple()`,
   the time in Exposed (from the daily transitions) has the parameters' mean
   and sd within tolerance.
2. **Shared and dropped timers.** `ModelSEIRQuarantine` with a fixed latent
   period: quarantined exposed agents become infected on the same day as the
   others; an agent recovered by an intervention never progresses; a fixed
   quarantine releases exactly `Quarantine period` days after entry; an agent
   that becomes infected in quarantine leaves on its disease timer, not on the
   quarantine one.
3. **Branching and tools.** A two-outcome drawn timer sends the expected share
   to each outcome (`run_multiple()`); a tool with a parameter factor on a
   duration shortens it for holders only.

### 7.4 Benchmarks

CPU time, interleaved, in the devcontainer (`AGENTS.md`): SEIR at 1e5 and 1e6
agents, hazard defaults vs today (T1 overhead: the hook in `events_run()`),
and drawn vs hazard latent periods.

---

## 8. Folding into `v1-layers`

The feature does not depend on layers, but it touches the update functions
that the model consolidation and the measles port (3d) rewrite. Doing it after
them rewrites each update function once and keeps 3d's acceptance (measles
draw for draw) a check of layers alone.

**Order.** Model consolidation -> **T1** (can run in parallel with 3c-2) ->
**T2a** -> 3d -> **T2b** -> **T3** (with Phase 4, which also gives tools new
targets: layer targets and parameter factors should be designed together).
T1 adds API without breaking any; it could go to `master` instead (Q9).

**Wrappers.** Each PR is ported like a phase (`contact-layers.md`, Section 6):
epiworldR, then epiworldpy, epiworldjs, measles; lag at most one PR.

- R: `timer_drawn()`, `timer_hazard()`, `duration_lognormal()` and the
  others return external pointers; `add_transition(model, state, timer,
  targets)`, `set_timer()`.
- User-defined durations and hazards written in R, Python or JS are callbacks
  across the language boundary (risk already listed in `contact-layers.md`,
  Section 9): not exposed until needed; the built-in ones cover the use cases
  above.
- `new_state_update_transition` bindings become defunct stubs naming
  `add_transition` (T2a).

**Edits to `contact-layers.md` once this plan is accepted** (not made in this
PR):

1. Section 4: a short **D12. State durations** pointing to this file, with the
   settled answers to Section 9 below.
2. Section 6: entries T1, T2a, T2b, T3 at the places given above.
3. Section 9 (Risks): the T2b change of measles results; per-agent memory.
4. Section 11.5 (what each phase means for the wrappers) and 11.6 (status):
   one row per PR.
5. The consolidation's parameter table, if Q7 is accepted.

---

## 9. Open questions (recommendation first)

- **Q1. Virus-level rates.** Keep `Virus::set_prob_recovery`, `set_incubation`,
  `set_prob_death` as the sources of the built-in hazards (recommended: multi-
  strain models and every wrapper use them), or move all durations to
  model-level timers.
- **Q2. Order within a step.** Hazards before drawn timers (recommended: a
  state of `d` days gives `d` chances to each hazard) or the reverse.
- **Q3. Measles simultaneous expiry** in Quarantined Latent: accept the
  one-day detour through Quarantined Prodromal, or keep a hand-written update
  for that state.
- **Q4. Rule for several hazard exits.** `roulette()` (independent competing
  events, today's default) or exclusive probabilities (`sample_from_probs`,
  today's measles). One rule for the generic function (recommended:
  `roulette()`; the measles change is small at their hospitalization rates,
  to be measured in T2b).
- **Q5. Measles hospitalization**: a branch probability of the rash timer
  (calibrates "x% of cases hospitalized" directly; recommended if drawn timers
  are used) or a daily hazard.
- **Q6. Slot storage**: per-agent vector vs fixed inline array (measure).
- **Q7. Parameter names**: rename "Recovery rate" to "Infectious period" (mean
  days) in the consolidation table, so every state is set by its mean.
- **Q8. Rounding** of drawn durations: nearest day with a minimum of 1
  (recommended) or ceiling.
- **Q9. Where T1 lands**: `v1-layers` (recommended, with the rest of 1.0.0) or
  `master` (it breaks nothing).
- **Q10. Re-timing drawn timers when a tool is added mid-state**: not now.
- **Q11. Timers on tools** (IG protection, waning vaccines): out of scope; the
  same duration objects can serve them later.
- **Q12. `Tool::set_param_factor` vs the existing tool effects**
  (`recovery_enhancer`, `death_reduction`). They overlap for the infected
  state: a factor multiplies a parameter, the enhancer combines probabilities
  (`1 - (1 - p)(1 - e)`), which is not a multiplier. Recommended: keep both in
  1.0.0 (the enhancer feeds the built-in hazards, Q1), factors for timer
  parameters only; revisit after T3.

---

## 10. Risks

- **Random stream changes** in T2b (measles), and in T2a if a draw rule is not
  reproduced exactly.
- **Same-day churn** (Section 4.3): a missing revive would redraw timers when
  global events change states mid-step; test it (A -> B -> A in one step keeps
  A's end day).
- **Memory** at 1e6+ agents if slots are inline (Q6).
- **Callbacks in the bindings** (Section 8).
- **Semantics users may not expect:** a drawn timer ignores later parameter
  changes and later tools; documented in the vignette that introduces timers.

---

## 11. Review checklist (for a human or an AI reviewer)

Check the facts in Section 2 against `origin/v1-layers` (`1ab03e2`):

```sh
git show 1ab03e2:include/epiworld/model-meat.hpp | sed -n 349,470p        # events_run, state_last_changed
git show 1ab03e2:include/epiworld/agent-events-meat.hpp | sed -n 1,25p    # per-infection Virus copy
git show 1ab03e2:include/epiworld/agent-meat-state.hpp | sed -n 30,190p   # new_state_update_transition, default_update_exposed
git show 1ab03e2:include/epiworld/misc.hpp | sed -n 120,180p              # roulette
git show 1ab03e2:include/epiworld/model-rand-meat.hpp | sed -n 265,285p   # sample_from_probs
git show 1ab03e2:include/measles/measlesmixing.hpp | sed -n 330,600p      # measles update functions, day_flagged, day_rash_onset
git show 1ab03e2:include/measles/immunoglobulin-meat.hpp | sed -n 70,100p # IG removal time
git grep -n -E 'set_prob_recovery|set_incubation' 1ab03e2 -- include/     # users of the virus rate setters
```

Questions worth a reviewer's time:

1. Does the keep/start/drop rule (4.3) give the right answer for every
   measles path in 5.4, including PEP (4.7) and the isolation anchor on rash
   onset (a carried timer)?
2. Is there a path into a timed state that bypasses `events_run()` (and so
   would start no timer)?
3. Can the generated update function reproduce each current model's draws
   (7.2)? Which cannot, and is the difference only in the stream or also in
   distribution?
4. Is anything here redundant with `contact-layers.md` (Phase 4 participation,
   layer-targeted tools), or could fewer building blocks do the same?
5. Do the timing rules (4.3) match the current release rule
   `Quarantine period <= today - day_flagged` to the day?

---

## 12. References

- Lessler J, Reich NG, Brookmeyer R, et al. (2009). Incubation periods of
  acute respiratory viral infections: a systematic review. *Lancet Infect
  Dis* 9(5):291-300. Measles: median 12.5 days, dispersion 1.23 (cited from
  memory; check before relying on it).
- Covasim's "prognoses" (outcome drawn at infection, then durations per
  outcome) is the model for the branching drawn timer (from memory).
- `contact-layers.md` on `v1-layers`: point 17 (model parameters as the single
  source of truth), Section 6 (phases, wrapper ports), Section 9 (risks),
  Section 11 (runbook, identity harness).
