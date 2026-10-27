# State durations: timers and transitions for epiworld

**Status:** proposal, revision 3 (2026-10-27). Revisions 1 and 2 were
reviewed on #330; Section 13 maps every review comment to the change it
caused. Nothing
here is settled until the review is closed. It is a companion to
`contact-layers.md` (the 1.0.0 plan on `v1-layers`); Section 8 says how it
folds into that plan.

**Base:** every code reference is to `origin/v1-layers` at `1ab03e2`
("ContactTracing: get_n_agents() (#329)"). Section 11 lists the claims a
reviewer should check, with the commands to check them.

**Goal:** let any state of a model last a time that is not geometric (a
lognormal latent period, a fixed 21-day quarantine), with the dynamics handled
once by the core, for any number of states, while keeping today's
memoryless behavior, and its results, where a model wants it.

**Shape of the design.** Three things are kept apart (Section 4):

1. the **clock**: when a timer started, and when (or how) it ends;
2. the **decision**: which exit fires on a given step, made once per agent
   and step;
3. the **action**: what firing commits (a state change, removing the virus, a
   record), applied through the existing events.

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
| `include/measles/measlesmixing.hpp` (and the other two measles models) | Quarantine release | `day_flagged[id]` stored when quarantined; each step `Quarantine period <= today - day_flagged`, reading the parameter **live** |
| same | Isolation release | anchored on `day_rash_onset[id]`, set when Rash starts, read in Isolated and Isolated Recovered |
| `include/measles/measlesmixingriskquarantine.hpp` (`m_get_risk_period`, line 253) | Quarantine length by risk level | reads the agent's **current** risk level and the matching parameter every step |
| `include/measles/immunoglobulin-meat.hpp` (`_set_immunity`) | IG protection | draws a normal half-life on first use, stores `_removal_time` |

Each is a timer, written differently, with its own state vectors and reset
logic. A general mechanism removes the release arithmetic. `day_rash_onset`
itself stays: contact tracing and the measles quarantine processes read it as
history (`measlesmixing.hpp:617-626`, `measlesmixingriskquarantine.hpp:826`,
`measlesschool.hpp:177,188`), which is not what a timer is for (4.9).

---

## 2. How states work today (facts the design relies on)

- **State changes are events, applied at the end of the step.**
  `Agent::change_state()`, `set_virus()`, `rm_virus()` queue an event;
  `Model::events_run()` (`model-meat.hpp:349`) applies them, sets
  `p->state = new_state`, records `p->state_last_changed = today()`, and
  applies the event's queue change. `events_run()` can run several times in a
  step: global events call it (e.g., `InterventionMeaslesPEP::operator()`), so
  an agent can change state more than once in a day; the transition matrix
  handles that with `state_prev`.
- **`rm_virus()` is not a state change only.** `_event_rm_virus()`
  (`agent-events-meat.hpp:90`) runs the virus's post-recovery hook, clears the
  infection, updates the virus counts, and the event carries the virus's queue
  change. `default_update_exposed()` leaves Infected through `rm_virus()`, and
  the measles hospital exit also calls `record_hospitalization()`.
- **Reset bypasses events for the baseline state.** `Agent::reset()`
  (`agent-meat.hpp:440`) sets `state = 0` directly. `Model::reset()` then
  calls `dist_entities()`, `dist_virus()`, `dist_tools()`
  (`model-meat.hpp:1952-1954`), each followed by `events_run()`. So seeded
  infections are applied **before** baseline tools (vaccines) exist, and an
  agent that stays in state 0 never gets an event.
- **The queue decides who is updated.** With queuing on (the default) and no
  queue-blind layer, `update_state()` visits only agents with `queue[i] > 0`
  (`model-meat.hpp:1712-1715`); the push path filters the same way.
  `change_state()` makes no queue change by default. Infected agents are
  queued through their virus; an uninfected agent in a timed state (a
  quarantined susceptible on an isolated vertex) is not. The measles models
  turn queuing off.
- **Each infection has its own `Virus` copy**: `_event_add_virus`
  (`agent-events-meat.hpp:5`) stores the event's copy and sets its date.
- **Rates live on the virus and on tools.** `Virus` has `incubation`,
  `probability_of_recovery`, `probability_of_death` (`virus-bones.hpp:55-58`);
  tools have `recovery_enhancer` and `death_reduction`, combined by the
  model's mixers, which skip tools that do not target the agent's virus
  lineage (`Tool::target_mask`). `default_update_exposed`
  (`agent-meat-state.hpp:148`): death `= p_d (1 - reduction)`, recovery
  `= 1 - (1 - p_r)(1 - enhancer)`, then `roulette(2)`.
- **Three draw rules are in use for a state's exits**, and they are different
  laws, not interchangeable implementations:
  1. `runif() < p` (ModelSEIR's exposed state; measles latent, prodromal,
     hospital);
  2. `roulette()` (`misc.hpp:128`): inputs as independent competing events,
     odds `p / (1 - p)`, `P(none) = 1 / (1 + sum of odds)`; for inputs 0.5 and
     0.5, none / A / B = 1/3 each (`default_update_exposed`,
     `new_state_update_transition`);
  3. `sample_from_probs()` (`model-rand-meat.hpp:269`): mutually exclusive
     probabilities, `P(k) = p_k`; for 0.5 and 0.5, none / A / B = 0 / 0.5 / 0.5
     (measles Rash and Isolated).

  Rules 1 and 2 agree in law for a single exit but use the uniform differently
  (`r < p` vs `r >= 1 - p`).
- **`new_state_update_transition` is used by examples only**
  (`examples/15-seird-transitions`, `20-transmission-benchmark`,
  `21-parameter-lookup-benchmark`); no built-in model calls it.
- **Parameters are read through `ParamRef`** (`param-ref.hpp:53`): a name
  resolved to a position once per parameter layout; values are never cached;
  the cache is atomic, so a `ParamRef` is safe to share across copies and
  threads.

---

## 3. How we got here (alternatives already discussed)

Recorded so a reviewer does not re-propose them without new arguments.

1. *A clock on the virus, one per disease stage* (first draft). It cannot time
   a quarantine of a susceptible agent, and a quarantined latent agent needs
   two clocks at once. Replaced by timers on the agent, with an
   infection scope for disease timers (4.4), which keeps the one thing the
   virus clock got right: a new infection is a new clock.
2. *Geometric vs "other" distributions.* The real split is the mechanism:
   drawn at entry, or checked every step (hazard). A hazard that depends on the
   days already spent describes any discrete distribution and still responds
   to tools and global events at once. A fixed deadline that reads its
   parameter live is such a hazard (4.1).
3. *A virtual `Virus::recover(model)` per disease.* Kept the idea that the
   disease defines its natural history, but it must cover every state, it must
   separate deciding from acting (the destination depends on the model), and
   it should be configured with objects, so R, Python and JS users need no
   callbacks into the interpreter every step.
4. *Drawing the whole course at infection* (Covasim 3.1.6 `People.infect()`).
   Same law for independent stages; it stores stages that never happen when
   the course branches, and it fixes every later stage before interventions
   act. Only correlated stage durations need it; not requested.
5. *An event calendar* (a priority queue of scheduled transitions). Not needed:
   agents with active timers are kept in the update set by the timers
   themselves (4.6), and checking an end day is cheaper than today's daily
   draw. Revision 1 argued this from "agents with a virus are visited every
   step", which is false for uninfected timed agents under the queue; the
   queue contribution in 4.6 is what makes the argument hold.

---

## 4. Design

### 4.1 Timers

A **timer** is a named, immutable definition of something an agent waits for
while in a state. It has one or more **outcomes** (which exit it leads to) and
one of two kinds.

- **Drawn** (at the start): when the timer starts, it picks the outcome with
  its probability, then that outcome's duration, and fires on the drawn day.
  Cheap, and the end is known in advance. This is a semi-Markov state: an
  embedded choice of exit plus a duration per exit (Covasim 3.1.6 and EMOD
  sample periods this way).
- **Hazard** (checked every step): each step, one draw decides whether it
  fires and through which outcome. A constant probability gives today's
  geometric duration. The probability may depend on the agent and on the
  days already spent (`days_in`), and it is read every step, so tools and
  global events act at once.

```cpp
// Drawn, one outcome
auto latent = timer::drawn(
  "latent", duration::lognormal("Latent period", "Latent period sd")
);

// Drawn, two outcomes: the outcome first ("" takes the remaining
// probability), then its own duration
auto rash = timer::drawn("rash", {
  {"Hospitalization prob", duration::lognormal("Days to hospital", "Days to hospital sd")},
  {"",                     duration::lognormal("Rash period", "Rash period sd")}
});

// Hazard, today's behavior
auto infectious = timer::hazard(
  "infectious", {"Death rate", "Recovery rate"}, hazard_rule::competing
);

// Hazard depending on the days in the state (non-geometric, still live)
auto waning = timer::hazard("waning",
  [](Agent<> & a, Model<> & m, int days_in) { return weibull_h(days_in, m); }
);

// Deadline: true once days_in reaches a value read every step; no random
// draw, and never spent: a predicate re-checked on every visit to a state
// where it has an exit (4.4). A negative value never fires (today's
// "disabled" sentinel).
auto quarantine = timer::deadline("quarantine", "Quarantine period");
auto risk_quarantine = timer::deadline("quarantine",
  [](Agent<> & a, Model<> & m) { return risk_period(a, m); }  // current risk level
);
```

`weibull_h` and `risk_period` stand for user functions.

**Probabilities and durations are per agent.** Every outcome probability,
hazard and duration is either a parameter name (read for the agent, 4.7) or a
C++ function of `(agent, model)` (plus `days_in` for hazards). So age-, risk-
or strain-dependent branch probabilities do not need artificial states or
tools. The wrappers expose the parameter forms only until callbacks across the
language boundary are supported (Section 8); a tabulated form (a value per
group of an agent attribute) is the candidate built-in for them (Q13).

**Hazard rules.** A hazard timer with several outcomes states its law,
because the draw rules in use are different laws (Section 2):

| Rule | Law | Today's users |
|---|---|---|
| `hazard_rule::bernoulli` | one outcome, fires if `runif() < p` | SEIR exposed, measles latent, prodromal, hospital |
| `hazard_rule::competing` | `roulette()`: independent competing inputs | `default_update_exposed` |
| `hazard_rule::exclusive` | `sample_from_probs()`: mutually exclusive probabilities | measles Rash, Isolated |

The default is `bernoulli` for one outcome and `competing` for several. Each
rule consumes the random stream exactly as the code it names, so ports can be
draw-for-draw identical (7.2).

**Durations** (`duration::`): `fixed(mean)`, `geometric(mean)`,
`lognormal(mean, sd)`, `gamma(mean, sd)`, and a function of
`(agent, model)`. `mean` and `sd` describe the **continuous** distribution
(lognormal: `sigma^2 = log(1 + sd^2/mean^2)`, `mu = log(mean) - sigma^2/2`;
gamma: `shape = mean^2/sd^2`, `scale = sd^2/mean`); the realized number of
days is its discretization (4.5).

### 4.2 Transitions and actions

A model declares, per state, which timers run there, where each outcome
leads, and what firing does:

```cpp
m.add_transition(state, timer, {target_0, target_1, ...}, actions);
m.add_timer(state, timer);   // runs in the state, no exit from it ("carried")
```

- **Targets** are state names, or `Virus::POST` / `Virus::REMOVED` (the
  agent's virus's own post-recovery or removal state, as
  `default_update_exposed` uses today).
- **Actions**, per outcome: `action::move` (the default: `change_state`),
  `action::rm_virus` (`rm_virus(target)`: post-recovery hook, virus counts,
  queue change), and an optional hook `on_fire(agent, model, outcome)` that
  runs when the outcome is committed (e.g., `record_hospitalization()`).
- `add_transition` implies `add_timer`. A **carried** timer keeps running
  through a state it cannot be left by. Measles isolation needs this: it is
  anchored on rash onset, so it starts in Rash and releases from Isolated and
  Isolated Recovered.
- The same timer can be used by several states; that is how states **share**
  it (Latent and Quarantined Latent share `latent`), each mapping the shared
  timer's outcomes to its own targets and actions. Every state using a timer
  must map all its outcomes (checked when the model starts a run).

A state with transitions gets a generated update function. A state with a
hand-written update function can still use timers through the decision API
(4.3) and leave the bookkeeping to the core.

### 4.3 The decision: sampled once per slot and day, resolved on every query

Two things are kept apart: **sampling** (the random part, per slot) and
**resolution** (where the agent goes, which depends on its current state).
Only sampling is remembered; resolution is recomputed on every query, so it
never uses an obsolete state, infection or timer.

**Which timers are evaluated.** Only the agent's active slots whose timer has
an **exit in the current state**. A carried timer (no exit here) is not
evaluated: a hazard only ages, a drawn timer or deadline keeps its end (4.4).

**Sampling** (per slot). Each evaluated slot yields fired / not fired, and an
outcome:

- a **hazard** slot draws at most **once per day**, under its rule, and
  records `{day, fired, outcome}` in the slot. Any later query that day reads
  the record: a second query never gives it a second chance
  (`1 - (1 - p)^2`), whoever asks (the generated update, `timer_fired()` in a
  hand-written one, a global event), and whatever state the agent is in by
  then, as long as the slot survives;
- a **drawn** slot fires when `days_in` reaches its end (outcome drawn at
  materialization); a **deadline** fires when `days_in` reaches its value
  read now. No random draw.

**Resolution** (every query). From the current state and the sampled results
of its evaluated slots, the chain below gives one destination and the
composed actions. Nothing is cached, so:

| Same-day event between two queries | Effect |
|---|---|
| state change (Latent -> Quarantined Latent) | the next query uses the new state's exits and its new slots (a new `quarantine` deadline); the shared `latent` hazard keeps today's record |
| new infection (`rm_virus` then `add_virus`) | infection-scoped slots are new, so they have no record: the new infection's hazards draw for themselves; nothing of the old infection is reused |
| `restart_timer` | a new slot, no record |
| `cancel_timer` | the slot is gone |
| `set_timer_end` | the drawn slot's end changes; deterministic, recomputed |

**Firing is consumed by committing.** A fired hazard record or an expired
drawn timer becomes `spent` when a transition through it is committed (4.4).
A firing that no one commits by the end of the step is discarded: the hazard
had its chance that day.

Hazard timers in one state are independent sources; within one hazard timer,
outcomes compete under its rule.

**Several timers fire on the same step: chain them.** Apply the first fired
transition (in declaration order); in the destination, if another fired timer
has an exit there, apply it too; repeat until no fired timer has an exit in
the current state. Only the final state is committed, with one event, and the
actions along the chain are composed (if any step removes the virus, the
event is a `rm_virus` into the final state; `on_fire` hooks run in chain
order). This reproduces both measles boundary cases from today's code:

| Start | Fired on the same step | Chain | Today's code |
|---|---|---|---|
| Quarantined Latent | `latent`, `quarantine` | -> Quarantined Prodromal -> Prodromal | Prodromal (`_update_quarantine_latent`) |
| Isolated | `rash` (recover), `isolation` | -> Isolated Recovered -> Recovered | Recovered (`_update_isolated`) |

For these two the order of the chain does not matter (quarantine first: ->
Latent -> Prodromal; isolation first: -> Rash -> Recovered). Each fired timer
is used at most once in a chain. A model whose chains depend on the order gets
declaration order, and the check below flags it.

**Derived moves are shown, not hidden.** A chained move (Quarantined Latent ->
Prodromal) appears in no `add_transition` row, so the core lists it. When a
run starts, the model computes, for every state and every set of its timers
that can fire together, the final state the chain reaches. `Model::print()`
shows these derived moves under the declared ones, and
`Model::get_transitions()` returns both (declared and derived, with the timers
involved), so the bindings and the tests can read them:

```
Transitions
  Quarantined Latent --[latent]--------------> Quarantined Prodromal
  Quarantined Latent --[quarantine]----------> Latent
  Quarantined Latent --[latent + quarantine]-> Prodromal          (derived)
  Isolated           --[rash: recover]-------> Isolated Recovered
  Isolated           --[isolation]-----------> Rash
  Isolated           --[rash: recover + isolation]-> Recovered    (derived)
```

**Order check.** When a run starts, for every state and every set of its
timers (with exits there) that can fire together, the model resolves the
chain in **every order** of that set, not only the declared order and its
reverse: with three timers, two orders can agree while a third differs
(review of #330: S -> X -> Y -> Z by `a,b,c` and `c,b,a`, but S -> X -> W by
`a,c,b`). Two things are compared, and flagged separately in the listing:

- the **final state** (`order-dependent target`);
- the **composed actions**: whether the virus is removed, and the sequence of
  `on_fire` hooks (`order-dependent actions`; equal targets can still run
  different hooks, or the same hooks in another order).

Cost: a state with `k` timers with exits has `sum over s of C(k, s) s!`
orders, `s` from 1 to `k` (4 for `k = 2`, 15 for 3, 64 for 4, 325 for 5). The check runs up to
`k = 5`; a state with more is listed as `order unchecked`. Measles has at most
two per state.

**Order of drawn timers vs hazards.** Hazards are evaluated on every step the
timer is active, including the step a drawn timer of the same state ends, so a
state lasting `d` days offers `d` daily chances to each hazard. If both fire,
the chain decides.

**Committing.** The resolved transition is applied through the existing
events (`change_state` / `rm_virus`), so counts, queue changes, post-recovery
hooks and the transition matrix behave as today.

### 4.4 The clock: lifecycle of a timer slot

Per agent, a slot per active timer: `{timer, outcome, start day, end day,
status}`, where status is `pending`, `active` or `spent`.

**Scope.** A timer is `state`-scoped (the default: quarantine, isolation) or
`infection`-scoped (disease timers: latent, prodromal, rash). An
infection-scoped slot belongs to the current infection: any `add_virus` or
`rm_virus` event drops it, so a new infection always starts new disease
timers, even on the same day and even if the state does not change.

**At a state change** (in `events_run()`, where `p->state` is set):

| Old state has it | New state has it | Result |
|---|---|---|
| yes | yes | keeps its slot (dates, outcome, status) |
| no | yes | a new slot, `pending`, start day = today |
| yes | no | dropped (`state`-scoped: revivable today, below) |

- **Same-day churn.** A `state`-scoped slot dropped by a state change is kept
  as revivable until the end of the day: if a later change on the same day
  enters a state that has it, it comes back unchanged (A -> B -> A in one step
  keeps A's end day). Infection-scoped slots are never revived across an
  infection change.
- **Explicit restart.** `p->restart_timer(m, "quarantine")` starts a slot anew
  (a second quarantine order extends the quarantine even if the state does
  not change); `p->cancel_timer(m, name)` drops it.
- **Materialization.** A `pending` slot draws its outcome and end day (drawn
  timers) or just becomes `active` (hazards, deadlines) at the end of the
  `events_run()` call that created it, after every event of that call. So a
  tool added in the same batch (PEP adds its tool, then moves the agent) is
  seen by the draw.
- **Reset.** `Agent::reset()` clears the slots. `Model::reset()` runs, in
  order (`model-meat.hpp:1952-1967`): `dist_entities()`, `dist_virus()`,
  `dist_tools()`, `initial_states_fun()`, each global event's `reset()`, and
  a final `events_run()`. Materialization is suppressed for all of it: every
  slot created during reset stays `pending`, keeping its start day 0. One
  **initialization pass runs after that final `events_run()`**, when the
  states and tools are final. It visits every agent in id order and
  (a) creates the slots of its current state, including state 0, for agents
  that got no event (an initially immune agent with waning, a model whose
  initial state is quarantine), (b) drops pending slots that the agent's
  final state does not use (a seed moved from Exposed to Infected by
  `initial_states_fun()` loses `latent` without drawing it), and
  (c) materializes the rest. Seeded infections therefore see baseline
  vaccines, including tools installed by a global event's `reset()`. The
  pass consumes random numbers only for drawn timers; models with only
  hazards and deadlines (every model today) draw nothing new, so today's
  random stream is unchanged.
- **Spent** (drawn and hazard timers only). A drawn or hazard timer whose
  firing was committed is `spent`: it is kept while the agent stays in states
  that use it, never fires again and is never restarted implicitly (only
  `restart_timer`), and is dropped when the agent leaves those states. So a
  drawn timer that fires into a state sharing it does not fire again.
- **Deadlines are never spent.** A deadline is a predicate on its anchor
  (start day) and a value read each step, re-checked on every evaluation in a
  state where it has an exit. It keeps its anchor while the agent is in
  states that use it (as an exit or carried). Measles needs this: Isolated ->
  Rash on release keeps `isolation` (carried in Rash) with its rash-onset
  anchor; if Rash detects the agent again (`_update_rash()` draws detection
  every step) and it goes back to Isolated or Isolated Recovered, the
  deadline is already past and releases it at the next update, as today.
  Re-anchoring on the new detection would change today's behavior, and is
  what `restart_timer` is for.
- **Carried and expired before its exit.** Only timers with an exit in the
  current state are evaluated (4.3), so for a carried timer:
  - a **hazard** only ages: no draws while carried; it is sampled from the
    first step in a state with its exit, with `days_in` counted from its
    start. No firing is latched while carried;
  - a **drawn** timer keeps its end; if the end passed while carried, it
    fires at the first evaluation in a state with its exit;
  - a **deadline** likewise fires at the first evaluation in a state with its
    exit once its value is reached (isolation shorter than the time to
    detection: Rash -> Isolated, then released at the next update, as
    `_update_isolated` does today with `days_since >= period`).
- **Seeds start at age 0.** A seeded infection starts its disease timers at
  day 0, with no time already elapsed. With hazards (today) this changes
  nothing; with drawn timers it means every seed is newly infected. Starting
  prevalent cases part-way (an elapsed age, or a remaining duration drawn
  conditionally) is deferred (Q14).

### 4.5 Discretization and domain

- A drawn duration `x` (continuous) becomes `d = max(1, ceil(x))` days: the
  slot ends `d` steps after it started. This is today's elapsed-time rule
  (`Quarantine period <= today - day_flagged` releases a 1.4-day quarantine
  after 2 updates and a 0-day one at the first update), so fixed periods
  release on the same day as now.
- The realized mean is therefore larger than `mean` (about +0.5 day for a
  smooth distribution, more when much of the mass is below 1). `mean` and
  `sd` are the continuous input; the documentation says so, and the tests
  check the discrete distribution (7.3), not the input moments.
- A negative value never fires (today's disabled sentinel for quarantine and
  isolation). NaN or infinite values, `sd < 0`, a geometric mean below 1, a
  probability outside [0, 1], and a residual `""` probability below 0 are
  errors, raised when the model starts a run where they can be checked and
  otherwise at the draw, naming the timer, the parameter and the agent. Gamma
  or lognormal with `sd = 0` is `fixed`. An end day past `INT_MAX` never fires.

### 4.6 Who is updated: timer-owned queue membership

An agent with at least one `active` slot whose state can be left by a timer
must be visited every step, whatever the queue says. The slot bookkeeping
adds a queue contribution of its own: `OnlySelf` +1 when the agent goes from
no such slot to at least one, -1 when it goes back, balanced independently of
the infection and contact contributions. With queuing off (measles) nothing
changes. Visits stay in ascending id order, so the random stream of models
without timers is unchanged.

### 4.7 Parameters per agent; tools scale them

A timer reads its parameters **for an agent**. A tool can scale a parameter
for the agents holding it:

```cpp
ToolVaccine<> mmr("MMR");
mmr.set_param_factor("Prodromal period",     "MMR prodromal factor");
mmr.set_param_factor("Hospitalization prob", "MMR hospitalization factor");
```

- **Which tools count.** A tool counts if it scales that parameter and, for an
  infection-scoped timer, targets the agent's virus lineage (the same check
  as the recovery and death mixers). For a timer with no virus, every tool
  that scales the parameter counts.
- **Stacking.** Factors multiply. The result is validated (4.5): a scaled
  probability above 1, or a residual `""` below 0, is an error, not a silent
  clamp.
- **Durations.** A factor on a duration's mean scales the **drawn** value
  (`x * factor`), which scales the mean and sd together and keeps the
  coefficient of variation. It does not change the sd parameter alone, and it
  is not a rescaling of a duration already drawn (that is Q10).
- **When it is read.** A drawn timer reads factors once, at materialization
  (4.4); a hazard or deadline reads them every step. Baseline tools are seen
  by seeded timers because materialization during reset waits for the end of
  reset (after `dist_tools()`, `initial_states_fun()` and the global events'
  `reset()`).

### 4.8 Timer definitions, model copies, and threads

- Definitions are immutable and named; the model keeps them by name as
  `std::shared_ptr<const Timer>`. Model copies (`run_multiple()`, clones)
  share the definitions, which is safe because they never change; slots are
  copied with the agents.
- `m.set_timer("latent", new_definition)` replaces the definition **by name**,
  so every state sharing the timer sees it (one logical clock). It replaces
  the **law** (kind, rule, outcome probabilities, durations), never the
  **scope**: the scope belongs to the name and comes from the model's
  declaration. A new definition that leaves the scope unset inherits it; one
  that states a different scope is an error (a different scope is a
  different timer, declared with its own name and transitions). So swapping
  in a lognormal latent period keeps infection scope, with its reinfection
  and lineage-filtering semantics. The new definition must have the same
  number of outcomes. It is allowed between runs only (throws during a run,
  like `add_layer()`); active slots never migrate.
- Parameters are read through `ParamRef` (thread-safe). C++ callbacks must be
  thread-safe for `run_multiple()` with threads; the documentation says so.
- T1 tests: a clone runs independently of its original after `set_timer()` on
  the original; serial and parallel `run_multiple()` give the same results.

### 4.9 Live and snapshotted values

| Value | Read | Why |
|---|---|---|
| Deadline (quarantine, isolation, risk-level quarantine) | every step | today's semantics: a policy change from 21 to 7 days affects agents already in quarantine; a risk reclassification changes the period |
| Hazard probabilities | every step | today's semantics |
| Drawn outcome and duration | once, at materialization | that is what drawn means; an intervention that must change it calls `restart_timer` or sets the end day (`set_timer_end`) |

`day_rash_onset` is history, read by contact tracing and quarantine
processes; it stays a model vector, written where it is today. Slots are
disposable and are not history.

### 4.10 Interventions

| What the intervention does | Effect on timers | Example |
|---|---|---|
| Ends the infection (`rm_virus`) | infection-scoped slots dropped; state-scoped ones follow the state rule | effective PEP |
| Moves to a parallel state | shared slots keep running; new ones start | Latent -> Quarantined Latent keeps `latent`, starts `quarantine` |
| Develops the disease during quarantine | the quarantine slot follows the state rule | Quarantined Prodromal -> Isolated drops it |
| Orders quarantine again | `restart_timer("quarantine")` | repeated orders |
| Changes a policy parameter | deadlines and hazards see it at once | 21 -> 7 days |
| Changes the time left | `set_timer_end`, or `restart_timer` | not needed today |

PEP, step by step, in the measles mixing model with timers: an agent in
Latent is offered PEP. If it works, `rm_virus(RECOVERED)` drops `latent`
(infection-scoped). If not, the agent goes to Quarantined Latent: `latent`
keeps its end day (quarantine does not pause the disease) and `quarantine`
starts. With hazard defaults this is today's behavior; with a drawn `latent`,
the agent becomes prodromal on its drawn day instead of by a fresh daily
draw.

**Not now:** a tool that speeds up or slows down a drawn timer already
running (IG lengthens the measles incubation; antivirals shorten the
infectious period): rescaling the time left when the tool is added or
removed (Q10).

### 4.11 What it replaces

- `new_state_update_transition()`: becomes `add_transition()` with hazard
  timers (examples only; their streams change where they use one outcome,
  since `bernoulli` replaces `roulette(1)`).
- The measles `day_flagged` and the release arithmetic in about ten update
  functions (not `day_rash_onset`, 4.9).
- Virus-level rates stay (Q1): built-in hazards read
  `virus->get_prob_recovery()`, `get_prob_death()`, `get_incubation()` with the
  tools' enhancer and reduction, exactly as today, so multi-strain models keep
  working.
- `ToolImmunoglobulin::_removal_time` is a timer on a tool, not on a state:
  out of scope (Q11).

---

## 5. Worked examples

### 5.1 ModelSEIR, defaults (results unchanged)

```cpp
// in the constructor
auto latent = timer::hazard("latent", virus_incubation());      // bernoulli, 1 / incubation
auto infectious = timer::hazard(
  "infectious", virus_death_recovery(), hazard_rule::competing  // as default_update_exposed
);
latent.scope(timer_scope::infection);
infectious.scope(timer_scope::infection);
add_transition("Exposed",  latent,     {"Infected"});
add_transition("Infected", infectious, {Virus::REMOVED, Virus::POST},
               {action::rm_virus, action::rm_virus});
```

`virus_incubation()` and `virus_death_recovery()` are built-in hazards over
the virus's rates and the tools' mixers (4.11).

### 5.2 ModelSEIR with a lognormal latent period

```cpp
ModelSEIR<> m("measles-like", 10000, ...);
m.add_param(2.7, "Latent period sd");
m.set_timer("latent", timer::drawn(
  "latent", duration::lognormal("Latent period", "Latent period sd")
));  // between runs; same outcome count; inherits infection scope (4.8)
```

### 5.3 A simple model with a fixed quarantine

```cpp
auto quarantine = timer::deadline("quarantine", "Quarantine period");
m.add_transition("Quarantined", quarantine, {"Susceptible"});
```

Every quarantined agent returns to Susceptible exactly `Quarantine period`
days after entering, with queuing on and no infectious neighbors (4.6).

### 5.4 Measles (mixing model), whole table

Timers: `latent`, `prodromal`, `hospital` (infection-scoped; hazards with
`bernoulli` by default), `rash` (infection-scoped; `exclusive` hazard over
recovery and hospitalization by default, as today), `quarantine` (deadline),
`isolation` (deadline, state-scoped, carried from rash onset).

| State | Timers (outcome -> target, action) | Carried | Hand-written part |
|---|---|---|---|
| Latent | latent -> Prodromal | | |
| Quarantined Latent | latent -> Quarantined Prodromal; quarantine -> Latent | | |
| Prodromal | prodromal -> Rash | | `day_rash_onset` written by an `on_fire` hook |
| Quarantined Prodromal | prodromal -> Isolated; quarantine -> Prodromal | | same hook |
| Rash | rash: hospitalize -> Hospitalized (`on_fire`: `record_hospitalization`), recover -> Recovered (`rm_virus`) | isolation | detection draw and quarantine trigger; if detected, recover goes to Isolated Recovered and "no exit" goes to Isolated |
| Isolated | rash: hospitalize -> Hospitalized, recover -> Isolated Recovered (`rm_virus`); isolation -> Rash | | |
| Isolated Recovered | isolation -> Recovered | | |
| Quarantined Susceptible | quarantine -> Susceptible | | |
| Quarantined Recovered | quarantine -> Recovered | | |
| Hospitalized | hospital -> Recovered (`rm_virus`) | | |

Rash keeps a hand-written update function: the detection draw happens before
the rash decision today, and the destination depends on it. It reads the rash
decision through `timer_fired()` and commits it itself. The two same-day
boundary cases resolve as today through the chain (4.3).

Quarantined Latent has two declared exits, but three possible moves; the
third comes from the chain. Every case on one step, against today's
`_update_quarantine_latent`:

| `latent` ends | `quarantine` ends | Proposed | Today |
|---|---|---|---|
| no | no | stays | stays |
| no | yes | -> Latent (declared) | `else if (unquarantine)` -> LATENT |
| yes | no | -> Quarantined Prodromal (declared) | -> QUARANTINED_PRODROMAL |
| yes | yes | -> Quarantined Prodromal -> **Prodromal** (derived, one recorded change) | `unquarantine ? PRODROMAL : ...` -> PRODROMAL |

Isolated likewise, against today's `_update_isolated`:

| `rash` (recover) ends | `isolation` ends | Proposed | Today |
|---|---|---|---|
| no | no | stays | stays |
| no | yes | -> Rash (declared) | `else if (unisolate)` -> RASH |
| yes | no | -> Isolated Recovered (declared) | -> ISOLATED_RECOVERED |
| yes | yes | -> Isolated Recovered -> **Recovered** (derived) | `unisolate ? RECOVERED : ...` -> RECOVERED |

(Hospitalization from Isolated is a declared exit of `rash` and chains with
nothing: Hospitalized has no `isolation` exit.)

With the defaults above, no difference from today is intended: T2b must
reach draw-for-draw identity for each measles model, or report the exact
cause (7.2). Drawn latent and prodromal periods, and a hospitalization
probability per case (Q5), are then options a user sets, not changes to the
defaults.

---

## 6. API summary (proposed names, to settle in review)

| Item | Kind |
|---|---|
| `timer::drawn(name, duration)`, `timer::drawn(name, {{prob, duration}, ...})` | timer |
| `timer::hazard(name, probs or fun, rule)`, `timer::deadline(name, param or fun)` | timer |
| `.scope(timer_scope::state / infection)` | timer |
| `hazard_rule::bernoulli / competing / exclusive` | timer |
| `duration::fixed`, `geometric`, `lognormal`, `gamma`, function | duration |
| `Model::add_transition(state, timer, targets, actions)`, `add_timer(state, timer)`, `set_timer(name, timer)` | model |
| `Model::get_transitions()` (declared and derived moves); `Model::print()` lists them | model |
| `action::move`, `action::rm_virus`, `on_fire` hook; targets `Virus::POST`, `Virus::REMOVED` | transition |
| `Agent::timer_fired(model, name)`, `restart_timer`, `cancel_timer`, `get_timer_start`, `get_timer_end`, `set_timer_end` | agent |
| `Tool::set_param_factor(param, factor_param)` | tool |

States and timers are named by strings in the public API (resolved when a run
starts), so the bindings need no ids.

---

## 7. Implementation plan

### 7.1 PRs

| PR | Content | Results |
|---|---|---|
| **T1. Core** | `include/epiworld/timer-bones.hpp` / `timer-meat.hpp`: definitions, rules, durations, validation; agent slots and lifecycle (4.4) in `events_run()` and `reset()`; the initialization pass; timer-owned queue membership; decision cache and chain resolution; actions through existing events; `add_transition`, `add_timer`, `set_timer`; the agent API | No model uses it: every result identical |
| **T2a. Default models** | SIR, SEIR, SIRD, SEIRD, SIS, SISD, `ModelSEIRQuarantine` (after the consolidation) on `add_transition`; `new_state_update_transition` removed, examples ported | Draw-for-draw identical (7.2) |
| **T2b. Measles** | the three measles models on timers; `day_flagged` and the release arithmetic removed (after 3d) | Draw-for-draw identical with defaults, or the cause reported (7.2) |
| **T3. Tool factors** | `Tool::set_param_factor`, parameter reads per agent with lineage targets | Identical without factors |

### 7.2 Identity

- **T1:** the identity harness (`contact-layers.md`, Section 11): every model
  draw-for-draw identical.
- **T2a and T2b:** each state keeps its legacy rule (`bernoulli`,
  `competing`, `exclusive`), and the decision consumes the stream in the
  same order as the update function it replaces; so both are expected to be
  draw-for-draw identical, including exact quarantine and isolation release
  days. A model that is not must report the first diverging draw and its
  cause; the user decides. Aggregate statistics are not enough here: final
  size and peak can agree while quarantine person-days differ.
- A **new random-number scheme** (e.g., common random numbers per agent and
  timer, as in Starsim) is not part of any of these PRs.

### 7.3 Tests

End-to-end, one `EPIWORLD_TEST_CASE` per file. Deterministic cases assert
exact histories: agent states by day, virus presence and counts, tools, the
transition matrix, release days, hospitalization records.

**T1, lifecycle (deterministic, `run()`):**
- a fixed deadline releases exactly `period` days after entry, with queuing
  on and no infectious neighbors; a 1.4-day deadline releases after 2 updates;
  a negative one never;
- a state-0 timer (all agents baseline, no virus) fires on its day, again
  after `reset()` in a second run;
- reset: an `initial_states_fun()` that moves an Exposed seed to Infected
  leaves no `latent` slot and draws nothing for it; a seed whose baseline
  tool is installed by a global event's `reset()` materializes with that
  tool's factor (T3 for the factor itself);
- same-day A -> B -> A keeps A's end day; a same-day recovery and reinfection
  gets new infection timers; `restart_timer` extends a quarantine without a
  state change;
- repeated `timer_fired()` queries on one step return the same result, and a
  hazard draws once per slot and day; between two queries on the same day,
  a state change makes the second query use the new state's exits and slots,
  and a recovery plus reinfection makes it use the new infection's slots
  (no result of the old infection reused);
- a drawn timer firing into a state that shares it does not fire again
  (spent); a deadline is never spent;
- carried timers: a carried drawn timer or deadline that expired fires on
  entering its exit state; a carried hazard makes no draws until it has an
  exit (the stream shows no draws in the carrying state);
- the two chain cases of 4.3; the three-timer case of 4.3 (`a,b,c` and
  `c,b,a` reach Z, `a,c,b` reaches W) is flagged `order-dependent target`;
  equal targets with different `on_fire` hook sequences are flagged
  `order-dependent actions`; `get_transitions()` lists the two derived moves
  of 5.4 and a state with six timers is listed `order unchecked`;
- `set_timer` with a definition that leaves the scope unset keeps infection
  scope (a same-day reinfection gets a new slot), and one that states another
  scope is an error;
- an `rm_virus` action clears the virus, runs the post-recovery hook, updates
  virus counts; SIS reinfection afterwards works;
- validation errors for each case of 4.5.

**T1, distributions (`run_multiple()`, fixed seeds):** for drawn lognormal,
gamma and fixed durations, the realized days (from the transitions,
censored at the end of the run) match `max(1, ceil(x))` of the input
distribution by a Kolmogorov-Smirnov-type comparison of the survival curve;
branch frequencies of a two-outcome drawn timer match the probabilities; the
joint laws of `competing` and `exclusive` hazards match their formulas
(e.g., 0.5 and 0.5: 1/3 each vs 0 / 0.5 / 0.5); clone independence and
serial vs parallel equality (4.8).

**T2b, measles boundaries (deterministic):** a quarantined latent agent whose
period and quarantine end the same day goes to Prodromal; an isolated agent
recovering on its isolation deadline goes to Recovered; a mid-quarantine
change of "Quarantine period" and a risk reclassification change the release
day; PEP effective and ineffective paths; release -> detection again ->
re-entry into Isolated (and into Isolated Recovered) -> released at the next
update with the original rash-onset anchor, as today.

**T3, factors:** baseline-vaccinated seeds draw with the factor; vaccination
after entry does not change a drawn duration but changes a hazard; a factor
for one lineage leaves another lineage's agents alone; stacked factors
multiply; an invalid scaled probability is an error; after
`set_timer("latent", lognormal)`, a factor restricted to another lineage
still leaves this lineage's latent period alone (scope kept, 4.8).

### 7.4 Benchmarks

CPU time, interleaved, in the devcontainer (`AGENTS.md`): SEIR at 1e5 and 1e6
agents, before and after T1 (the hook in `events_run()`, the initialization
pass, the decision cache), and drawn vs hazard latent periods.

---

## 8. Folding into `v1-layers`

The feature does not depend on layers, but it touches the update functions
that the model consolidation and the measles port (3d) rewrite. Doing it after
them rewrites each update function once and keeps 3d's acceptance (measles
draw for draw) a check of layers alone.

**Order.** Model consolidation -> **T1** (can run in parallel with 3c-2) ->
**T2a** -> 3d -> **T2b** -> **T3** (with Phase 4, which also gives tools
layer targets: both kinds of targets should be designed together). T1 breaks
no API; it could go to `master` instead (Q9).

**Wrappers.** Each PR is ported like a phase (`contact-layers.md`,
Section 6): epiworldR, then epiworldpy, epiworldjs, measles; lag at most one.

- R: `timer_drawn()`, `timer_hazard()`, `timer_deadline()`,
  `duration_lognormal()` and the others return external pointers;
  `add_transition(model, state, timer, targets, actions)`, `set_timer()`.
- Callbacks across the language boundary (`contact-layers.md`, Section 9) are
  not exposed until needed; parameter-based and tabulated forms cover the
  examples above.
- `new_state_update_transition` bindings become defunct stubs naming
  `add_transition` (T2a).

**Edits to `contact-layers.md` once this plan is accepted** (not made in this
PR):

1. Section 4: a short **D12. State durations** pointing to this file, with the
   settled answers to Section 9 below.
2. Section 6: entries T1, T2a, T2b, T3 at the places given above.
3. Section 9 (Risks): per-agent memory; callbacks in the bindings.
4. Sections 11.5 and 11.6: one row per PR.
5. The consolidation's parameter table, if Q7 is accepted.

---

## 9. Open questions (recommendation first)

- **Q1. Virus-level rates.** Keep `Virus::set_prob_recovery`, `set_incubation`,
  `set_prob_death` as the sources of the built-in hazards (recommended).
- **Q2. Chain order.** Declaration order (recommended), or reject
  order-dependent chains when a run starts (costly to detect in general).
  Explicit joint rows (`add_transition(state, {timer_a, timer_b}, target)`)
  were considered and not chosen: every pair that can end together would need
  one, and a forgotten pair would fall back silently. The derived moves are
  printed instead (4.3).
- **Q4. Default rule for several hazard outcomes**: `competing`
  (recommended, today's default) with `exclusive` and `bernoulli` selectable.
- **Q5. Measles hospitalization**: keep the daily hazard as the default
  (identity); offer a drawn `rash` timer with a hospitalization probability
  per case as an option.
- **Q6. Slot storage**: per-agent vector vs fixed inline array (measure).
- **Q7. Parameter names**: rename "Recovery rate" to "Infectious period" (mean
  days) in the consolidation table, so every state is set by its mean.
- **Q9. Where T1 lands**: `v1-layers` (recommended) or `master`.
- **Q10. Rescaling a drawn timer when a tool is added or removed mid-state**:
  deferred.
- **Q11. Timers on tools** (IG protection, waning vaccines): out of scope.
- **Q12. `set_param_factor` vs the existing tool effects**
  (`recovery_enhancer`, `death_reduction`): keep both in 1.0.0; factors for
  timer parameters only; revisit after T3.
- **Q13. Agent-dependent values in the wrappers**: a tabulated form (a value
  per group of an agent attribute) before general callbacks.
- **Q14. Prevalent seeds** (elapsed age or a conditional remaining duration):
  deferred; seeds start at age 0 (4.4).

(Q3 and Q8 of revision 1 are settled: the chain resolves simultaneous expiry
as today, and discretization is `max(1, ceil(x))`.)

---

## 10. Risks

- **Identity in T2a/T2b** depends on reproducing each legacy draw in order;
  the decision cache and chain must not add or reorder draws.
- **Lifecycle bugs** (missed initialization, queue imbalance, revive across
  infections) leak between replicates or freeze agents; the T1 deterministic
  tests target each.
- **Memory** at 1e6+ agents if slots are inline (Q6).
- **Callbacks in the bindings** (Section 8).
- **Semantics users may not expect:** a drawn timer ignores later parameter
  changes and later tools, and the realized mean of a drawn duration is above
  its input mean (4.5); both documented in the vignette that introduces timers.

---

## 11. Review checklist (for a human or an AI reviewer)

Check the facts in Section 2 against `origin/v1-layers` (`1ab03e2`):

```sh
git show 1ab03e2:include/epiworld/model-meat.hpp | sed -n 349,470p        # events_run
git show 1ab03e2:include/epiworld/model-meat.hpp | sed -n 1690,1740p      # update_state, queue filter
git show 1ab03e2:include/epiworld/model-meat.hpp | sed -n 1890,1960p      # reset order: dist_entities, dist_virus, dist_tools
git show 1ab03e2:include/epiworld/agent-meat.hpp | sed -n 440,456p        # Agent::reset sets state 0 directly
git show 1ab03e2:include/epiworld/agent-events-meat.hpp | sed -n 1,131p   # add/rm virus events
git show 1ab03e2:include/epiworld/agent-meat-state.hpp | sed -n 30,190p   # new_state_update_transition, default_update_exposed
git show 1ab03e2:include/epiworld/misc.hpp | sed -n 120,180p              # roulette
git show 1ab03e2:include/epiworld/model-rand-meat.hpp | sed -n 265,285p   # sample_from_probs
git show 1ab03e2:include/measles/measlesmixing.hpp | sed -n 330,660p      # measles updates, day_flagged, day_rash_onset readers
git show 1ab03e2:include/measles/measlesmixingriskquarantine.hpp | sed -n 250,275p  # live risk period
git grep -n 'new_state_update_transition' 1ab03e2 -- include tests examples
```

Questions worth a reviewer's time:

1. Does the lifecycle (4.4) give the right answer on every measles path in
   5.4, including PEP (4.10) and the carried isolation timer?
2. Is there a path into a timed state that the initialization pass and the
   `events_run()` hook both miss?
3. Can each legacy update function be expressed with its rule and the chain,
   consuming the same draws in the same order (7.2)? Which cannot?
4. Is anything here redundant with `contact-layers.md` (Phase 4 participation,
   layer-targeted tools), or could fewer building blocks do the same?
5. Is the queue contribution (4.6) balanced on every path (spent, revived,
   restarted, dropped by `rm_virus`)?

---

## 12. References

- Lessler J, Reich NG, Brookmeyer R, et al. (2009). Incubation periods of
  acute respiratory viral infections: a systematic review. *Lancet Infect
  Dis* 9(5):291-300. Measles: median 12.5 days, dispersion 1.23 (cited from
  memory; check before relying on it).
- Covasim **3.1.6**, `People.infect()` and `check_quar()`
  (<https://github.com/starsimhub/covasim/blob/v3.1.6/covasim/people.py>):
  branches and dates sampled at infection; quarantine tracked separately,
  repeated orders extend its end. Describes that version only.
- Starsim distribution parameters and time conversion
  (<https://docs.starsim.org/user_guide/basics_parameters.html>,
  <https://docs.starsim.org/api/distributions.html>) and common random numbers
  (<https://docs.starsim.org/user_guide/advanced_random.html>).
- EMOD `Infection.cpp` at `01f8b6f`
  (<https://github.com/EMOD-Hub/EMOD/blob/01f8b6f20522ede55da11c208e24ef5c5207bb38/Eradication/Infection.cpp>):
  sampled incubation and infectious periods with mortality checks during or
  at the end of infection.
- `contact-layers.md` on `v1-layers`: point 17, Sections 6, 9 and 11.

---

## 13. Review of revision 1 (#330) and what changed

| Comment | Severity | Change |
|---|---|---|
| Transition actions, not only targets (`rm_virus`, hooks, hospitalization records) | P1 | Actions per outcome, `Virus::POST`/`REMOVED` targets, `on_fire` hook, committed through existing events (4.2, 4.3); tests on virus counts, hooks, SIS reinfection (7.3) |
| Uninfected timed agents skipped by the queue | P1 | Timer-owned queue membership (4.6); §3 item 5 qualified; queued tests with no infectious neighbors (7.3) |
| Baseline state never gets an event | P1 | Initialization pass after `dist_tools()`, including state 0 (4.4); state-0 and second-run tests |
| Seeds drawn before baseline vaccines | P1 | Pending slots materialized by the initialization pass, after `dist_tools()`; no draws for hazard-only models, so today's stream is kept (4.4, 4.7); T3 tests |
| Joint hazard semantics; samplers are different laws | P2 | Three named rules with their laws and legacy stream use (4.1); decision evaluated jointly per agent and step (4.3); identity per legacy rule (7.2) |
| Simultaneous release and progression | P2 | Chain resolution, reproducing both measles cases (4.3); deterministic boundary tests (7.3) |
| Live quarantine policy and risk level | P2 | `timer::deadline` reads its value every step, with an agent function for risk (4.1, 4.9); tests of mid-quarantine changes |
| Resuming vs a new episode | P2 | Infection scope; revive only for state-scoped slots; `restart_timer`, `cancel_timer` (4.4) |
| `timer_fired` observes, samples or consumes | P2 | Cached decision per agent and step; spent status; expired carried timers (4.3, 4.4); tests |
| Discretization and domain | P2 | `max(1, ceil(x))` matching today's elapsed-time rule; mean/sd are the continuous input; sentinels and errors (4.5); survival-curve tests |
| Factors vs lineage targets; invalid scaled probabilities | P2 | Lineage check as in the mixers; errors, not clamps (4.7); T3 tests |
| Summary: agent-dependent branch probabilities, multi-outcome hazard callbacks | -- | Every probability and duration takes a parameter or an `(agent, model)` function (4.1); tabulated form for wrappers (Q13) |
| Summary: `set_timer` contract | -- | By name, same outcome count, between runs only, no slot migration (4.8) |
| Summary: identity across copies and threads | -- | Immutable shared definitions; clone and serial/parallel tests (4.8) |
| Summary: prevalent seeds | -- | Explicit assumption (age 0) and deferral (4.4, Q14) |
| Summary: tracing reads `day_rash_onset` | -- | It stays as history (1.2, 4.9) |
| Summary: factor on mean vs sd | -- | A factor scales the drawn value (CV kept), not the sd parameter, not an already-drawn duration (4.7) |
| Summary: acceptance beyond aggregates; no new RNG scheme | -- | Identity per legacy rule, exact histories, survival-curve tests (7.2, 7.3) |
| Summary: pin Covasim version | -- | 3.1.6 pinned; Starsim and EMOD added (12) |

### Second round (review of revision 2)

| Comment | Severity | Change |
|---|---|---|
| A spent isolation deadline cannot release after a second detection | P2 | Deadlines are predicates, never spent; they keep their anchor while the agent is in states that use them, so release -> detection -> re-entry -> release works with the original rash-onset anchor, as today (4.1, 4.4); deterministic test (7.3, T2b) |
| A cache keyed by agent and day can resolve an obsolete state or infection | P2 | Sampling (once per hazard slot and day, recorded in the slot) separated from resolution (recomputed on every query from the current state); table of same-day invalidations (4.3); tests with a state change and a reinfection between queries (7.3) |
| Materialize after all of reset, not after `dist_tools()` | P2 | Materialization suppressed through `initial_states_fun()`, the global events' `reset()` and the final `events_run()`; the pass runs after that, drops pending slots the final state does not use, keeps day-0 anchors (4.4, 4.7); tests (7.3) |
| `set_timer` example loses infection scope | P2 | Scope belongs to the name; `set_timer` replaces the law only, an unset scope is inherited, a different one is an error (4.8, 5.2); tests on reinfection and lineage factors after replacement (7.3) |
| Stochastic carried timers sampled before they have an exit | P2 | Only timers with an exit in the current state are evaluated; a carried hazard only ages; carried drawn timers and deadlines fire on the first evaluation where they have an exit (4.3, 4.4); tests (7.3) |
| Order-dependence check misses three-timer cases; targets vs actions | P2 | Every order of every fired set is resolved up to five timers per state (`order unchecked` above); final state and composed actions flagged separately (4.3); the reviewer's three-timer case is a test (7.3) |
