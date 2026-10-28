# State durations, simple version: one stage clock per agent

**Status:** alternative proposal, revision 1 (2026-10-27), written to be
compared with `state-durations.md` (revision 3) on the same branch (#330).
Section 9 compares the two, point by point. Nothing here is settled.

**Base:** every code reference is to `origin/v1-layers` at `1ab03e2`, as in
`state-durations.md`. Section 12 lists the claims to check and how.

**Goal:** let a state last a time that is not geometric (a lognormal latent
period, a gamma infectious period, a fixed quarantine) with:

- a **simple declarative API**: the distribution is given when the state is
  built, and the duration is drawn once, when the agent enters the state;
- a **flexible API** for the cases that need it: hand-written update
  functions, interventions and tools that read or change how long an agent
  stays;
- the **tools' enhancers and reductions** working the way they work today;
- **no change** to models that do not use it, by construction.

**Shape of the design.** Two fields on the agent (when its current stage
started and when it ends), one small `Duration` class, three update-function
factories, and one line in `events_run()`. There is no timer registry, no
decision cache, no chain resolver and no multi-clock machinery. A second
concurrent clock (quarantine) stays a model-owned vector, as the measles
models already have it.

---

## 1. What is needed, and what is not

### 1.1 The requirement

Every state is left today with a daily probability, so durations are
geometric. Section 1 of `state-durations.md` motivates the change (measles
incubation: a geometric latent period outlasts a 21-day quarantine 17% of the
time; the lognormal one 0.6%). That motivation is not repeated here.

What models actually need, from the models in the repository and the measles
work:

| Need | Where | Covered by |
|---|---|---|
| Non-geometric latent and prodromal periods | measles models, SEIR-type models | Tier 1 (3.5) |
| Non-geometric infectious period, with death or recovery at the end | SEIRD-type models | Tier 1, infection factory (3.5.3) |
| A stage that survives moving to a parallel state | Latent <-> Quarantined Latent, Prodromal <-> Quarantined Prodromal | linked states (3.4) |
| Two clocks at once (disease stage and quarantine) | measles quarantined states | hand-written, the stage clock plus the existing `day_flagged` (5.4) |
| Tools that speed recovery or prevent death | `recovery_enhancer`, `death_reduction` | 3.7 |
| An intervention that changes the time left | not in any model today | Tier 2 (3.6) |

### 1.2 Out of scope, on purpose

- **More than one core clock per agent.** A second clock is a model-owned
  vector, written by hand where it is used. Today only measles has one
  (quarantine, isolation).
- **Automatic resolution of states with two exit clocks.** States like
  Quarantined Latent are written by hand. They are few, and the hand-written
  rule is clearer than a resolver.
- **A generic per-parameter factor on tools** (`Tool::set_param_factor`).
  Enhancers and reductions keep their current role; a tool that needs to
  change a duration uses Tier 2.
- **Correlated stage durations** and drawing the whole course at infection.
- **A scheduled-event backend.** Agents in a disease stage are visited every
  day anyway.

---

## 2. Facts the design relies on

Beyond Section 2 of `state-durations.md`, which still holds:

- **Step order.** `Model::run()` calls `update_state()` (which ends with
  `events_run()`, `model-meat.hpp:1736`), then `run_globalevents()` (each
  global event followed by `events_run()`, line 2422), then `next()`
  (`model-meat.hpp:1435-1442`). An agent that enters a state on day `t` is
  first updated in it on day `t + 1`.
- **`state_last_changed` is not "day the state started".** `events_run()`
  sets it to `today()` after **every** event on the agent
  (`model-meat.hpp:429`), including `AddTool` and `RemoveTool`, which carry
  `new_state = -99`. Giving a vaccine to an exposed agent moves it. It also
  moves on Latent -> Quarantined Latent, where the disease stage must not
  restart. So the design adds its own field for the stage start (3.1).
- **`Event` carries `new_state` and `queue`** (`config.hpp:221`); no change to
  `Event` is needed.
- **`Agent::reset()`** sets `state = 0`, `state_prev = 0`,
  `state_last_changed = -1` (`agent-meat.hpp:440-456`). The agent copy
  constructor and both assignments copy `state_last_changed`
  (`agent-meat.hpp:16, 52, 84`); the new fields are copied in the same
  places.
- **Today's mixer formulas** (`agent-meat-state.hpp:158-163`,
  `model-meat.hpp:517-547`):
  - recovery: `1 - (1 - p_r)(1 - e)`, where `e = 1 - prod(1 - e_k)` over the
    tools that target the agent's virus;
  - death: `p_d (1 - r)`, where `r = 1 - prod(1 - r_k)`.
- **Measles latent and prodromal exits are a single `runif() < 1 / mean`**
  (`measlesmixing.hpp:342, 363, 503, 530`); the quarantined versions combine
  that draw with `day_flagged` (`measlesmixing.hpp:490-545`). Quarantined
  Prodromal moves to Isolated, not Rash, when the rash starts.
- **PEP moves agents between parallel states** in a global event: on failure
  `agent.change_state(*model, next_state_failure)`
  (`interventionmeaslespep-meat.hpp:429`), for example Latent -> Quarantined
  Latent. Contact tracing does the same inside the rash update
  (`measlesmixing.hpp:655-669`).
- **Model RNG:** `rlognormal(m, s)` and `rgamma(alpha, beta)` wrap
  `std::lognormal_distribution` (log-scale mean and sd) and
  `std::gamma_distribution` (shape and **scale**)
  (`model-rand-meat.hpp:150-186`).

---

## 3. Design

### 3.1 The stage clock: two ints on the agent

```cpp
int stage_start = -1;  ///< Day the current stage started
int stage_end   = -1;  ///< Day the agent leaves it; -1 = not drawn
```

A **stage** is the current state, or a group of linked states (3.4). Both
fields are set by the core and read by update functions:

| When | `stage_start` | `stage_end` |
|---|---|---|
| `Agent::reset()` | `-1` | `-1` |
| `events_run()`, the agent changes state, old and new states not linked | `today()` | `-1` |
| `events_run()`, the agent changes state, old and new states linked | kept | kept |
| `events_run()`, event without a state change (`new_state == -99`, or the same state) | kept | kept |
| first update in a timed stage (3.3) | read | drawn, if `-1` |
| Tier 2, `set_stage_end()` (3.6) | kept | set |

Cost: 8 bytes per agent and, in `events_run()`, one comparison and two stores
per state change. Nothing else in the core reads them.

Why not reuse `state_last_changed`: it moves on tool events and on linked
moves (Section 2). Changing what it means would change the transition-matrix
bookkeeping that reads it (`model-meat.hpp:386-396`).

### 3.2 `Duration`

```cpp
template<typename TSeq>
class Duration {
public:
    static Duration geometric(Mean mean);               // legacy daily draw
    static Duration fixed(Mean mean);
    static Duration lognormal(Mean mean, Mean sd);
    static Duration gamma(Mean mean, Mean sd);

    int  end(Agent<TSeq> * p, Model<TSeq> * m) const;   // draws once, see 3.3
    bool done(Agent<TSeq> * p, Model<TSeq> * m) const;  // leaves today?
};
```

- **`Mean`** is a parameter name (resolved through `ParamRef`), a number, or a
  function `(Agent *, Model *) -> double`. The function form covers the
  virus's incubation (`p->get_virus()->get_incubation(m)`), which the measles
  and SEIR models use today, and any agent-specific value (age, risk group).
  A helper `duration_mean::virus_incubation()` returns that function.
- **Parameterized by mean and sd**, the way epidemiologists report periods.
  The conversions:
  - lognormal: `sigma2 = log(1 + sd^2 / mean^2)`,
    `mu = log(mean) - sigma2 / 2`, draw `rlognormal(mu, sqrt(sigma2))`;
  - gamma: `shape = mean^2 / sd^2`, `scale = sd^2 / mean`, draw
    `rgamma(shape, scale)`.
- **Discretization** (same rule as `state-durations.md` 4.5): a draw `x`
  becomes `d = max(1, ceil(x))` days, and the agent leaves on
  `stage_start + d`. This is today's rule for fixed periods
  (`Quarantine period <= today - day_flagged`): a 0-day period ends at the
  first update. The realized mean is larger than the input mean (about half a
  day for smooth distributions); the documentation says so, and tests check
  the discrete distribution.
- **Validation:** a negative mean means "never ends" (today's disabled
  sentinel for quarantine). NaN, `sd < 0`, or a geometric mean below 1 throws,
  naming the parameter and the agent. Lognormal or gamma with `sd = 0` is
  fixed.
- **Immutable.** A `Duration` holds no per-agent or per-run state, so copying
  it with the update function (model copies, `run_multiple` threads) is safe.
  `ParamRef` is already safe to share (`param-ref.hpp:53`).

**`done()` is the only call most code needs:**

```cpp
bool done(p, m):
    if family is geometric and p->stage_end < 0:
        return m->runif() < 1.0 / mean(p, m);   // exactly today's draw
    return m->today() >= end(p, m);
```

For the geometric family, `done()` uses one uniform, compared with `<`,
exactly as the measles and SEIR update functions do today. Replacing
`m->runif() < 1.0/(v->get_incubation(m))` with `incubation.done(p, m)` keeps
results **draw-for-draw identical**. A geometric stage that was given an
explicit end through Tier 2 obeys that end.

### 3.3 When the end is drawn

`end(p, m)` returns `p->stage_end` if it is set; otherwise it draws a
duration, sets `stage_end = stage_start + d`, and returns it. The first
update in the stage calls it, so the draw happens on day `stage_start + 1`.

Drawing on the first update instead of inside `events_run()`:

- **gives the same law.** The end is anchored on `stage_start`, not on the day
  of the draw.
- **needs no registry** of which state has which duration: the `Duration`
  lives in the update function that uses it.
- **handles seeds and reset** with no extra pass. Seeded agents enter their
  state through events during reset (`dist_virus()`), so `stage_start` is
  set, and the first update draws, after baseline tools exist.
- **wastes no draws** on same-day state changes (for example, an agent
  infected and moved by PEP on the same day).

The only order effect is on the random stream: the draw is part of the
agent's update, in id order, as every other draw in `update_state()` is.

**Invariant:** an agent in a timed stage is updated every day. Infected
agents are, through their virus's queue registration, or because queuing is
off (`ModelMeaslesMixingRiskQuarantine`, `ModelMeaslesSchool`). An agent
without a virus in a timed state under queuing is not; see 3.8.

### 3.4 Linked states

```cpp
model.link_states({LATENT, QUARANTINED_LATENT});
model.link_states({PRODROMAL, QUARANTINED_PRODROMAL});
```

Linked states form one stage. `events_run()` keeps `stage_start` and
`stage_end` when the agent moves between them:

```cpp
// in events_run(), after the state is set
if (new_state != -99 && new_state != state_old &&
    !states_linked(state_old, new_state)) {
    p->stage_start = today();
    p->stage_end   = -1;
}
```

`states_linked` reads a `std::vector<int>` of group ids (one entry per state,
`-1` for no group), filled by `link_states()`. A state belongs to at most one
group; linking it twice throws.

Why a declaration on the model and not a flag on `change_state()`: the moves
between parallel states happen in many places (the quarantined update
functions, contact tracing inside the rash update, PEP in a global event, in
three measles models). With a flag, each call site must remember it; with
groups, the model says once that Latent and Quarantined Latent are the same
disease stage. This is the one idea kept from `state-durations.md` (its
keep/start/drop rule), reduced to a group id.

### 3.5 Tier 1: the stay is given when the state is built

Three factories return an `UpdateFun<TSeq>`, like
`new_state_update_transition()` does today (`agent-meat-state.hpp:84`).

#### 3.5.1 One exit

```cpp
model.add_state("Exposed", new_state_update_duration<TSeq>(
    Duration<TSeq>::lognormal("Latent period", "Latent period sd"),
    INFECTED
));
```

Each day: `if (d.done(p, m)) p->change_state(*m, INFECTED);`

#### 3.5.2 Several exits, chosen when the stage ends

```cpp
model.add_state("Rash", new_state_update_duration<TSeq>(
    Duration<TSeq>::gamma("Rash period", "Rash period sd"),
    {HOSPITALIZED, RECOVERED},
    {"Hospitalization prob", ""}          // "" = the rest
));
```

When `done()` is true, one exit is drawn with **mutually exclusive**
probabilities (`sample_from_probs()`, rule 3 in `state-durations.md` 2),
read at that moment. Exactly one entry may be `""` and takes `1 - sum`; a
negative remainder throws. The exits share one duration: "how long" and
"where to" are separate questions, which is the usual semi-Markov
formulation and what the data give (a period, and a proportion
hospitalized).

#### 3.5.3 An infection stage: recovery or death through the virus

```cpp
model.add_state("Infected", new_state_update_infection_duration<TSeq>(
    Duration<TSeq>::gamma("Infectious period", "Infectious period sd"),
    "Case fatality"                        // per episode, optional
));
```

The exits are those of `default_update_exposed()`: death removes the virus
into the virus's removed state, recovery into its post-recovery state
(`rm_virus()`, so the post-recovery hook and the counts work as now). Each
day:

```text
e = recovery enhancer (mixer, today's formula)
if done(p, m):                      # the drawn period ends today
    r = death reduction (mixer)
    die with probability  CFR * (1 - r), else recover
else if e > 0 and runif() < e:      # a tool cut the infection short
    recover
```

Section 3.7 explains why this is the right reading of the enhancers. If the
agent has no virus, the factory throws, as `default_update_exposed()` does.

#### 3.5.4 Built-in models

The built-in models keep their update functions and get a setter for each
stage, so the R, Python and JS wrappers need no callbacks:

```cpp
ModelSEIR<> m("flu", 10000, 0.01, 0.5, 4.0, 1.0/7.0);
m.add_param(1.5, "Incubation sd");
m.set_duration("Exposed", Duration<>::lognormal(
    duration_mean::virus_incubation(), "Incubation sd"
));
```

`set_duration(state, d)` replaces the stage's `Duration` member (between runs
only). The default member is the geometric `Duration` with today's mean, and
the update functions call `done()`, so the defaults are draw-for-draw
identical (3.2). Which states accept a duration is listed per model; asking
for another state throws. In the wrappers this becomes
`set_duration(model, "Exposed", "lognormal", mean = ..., sd = ...)`.

### 3.6 Tier 2: reading and changing the time in the stage

```cpp
int  Agent::get_stage_start() const;
int  Agent::get_stage_end() const;               // -1 if not drawn yet
void Agent::set_stage_end(int day);              // -1 = draw again
```

That is all the flexible tier is: the same two fields, open to hand-written
update functions, global events and tool hooks. Examples:

- **Hand-written two-clock state** (measles Quarantined Latent):

  ```cpp
  bool released = EPI_PAR(m, "Quarantine period") <= today - day_flagged[id];
  if (model->incubation.done(p, m))
      p->change_state(*m, released ? PRODROMAL : QUARANTINED_PRODROMAL);
  else if (released)
      p->change_state(*m, LATENT);       // linked: the latent stage goes on
  ```

- **An intervention that shortens or lengthens the stage** (immunoglobulin
  lengthening the incubation, an antiviral shortening the infectious period),
  as an accelerated-failure-time effect on the time left:

  ```cpp
  int end  = model->incubation.end(&p, m);       // draws if needed
  int left = end - m->today();
  p.set_stage_end(m->today() + std::max(1, (int) std::ceil(left * factor)));
  ```

- **Days in the stage** for any rule that depends on them:
  `m->today() - p->get_stage_start()`.

`set_stage_end()` writes the field directly, not through an event, because
nothing else depends on it within the step. It is meant for the agent being
updated, or for a global event, which runs when no update loop is iterating.
Calling it on another agent from inside an update function makes the result
depend on the update order; the documentation says so.

### 3.7 Enhancers and reductions

Today, in `default_update_exposed()`:

```text
P(recover today) = 1 - (1 - p_r)(1 - e)
P(die today)     = p_d (1 - r)
```

**The recovery enhancer is already an independent second chance to leave.**
`1 - (1 - p_r)(1 - e)` is the probability that at least one of two
independent daily trials succeeds: natural recovery, with `p_r`, or recovery
caused by the tool, with `e`. With a drawn duration the natural trial is
replaced by the deadline and the tool's trial is kept unchanged:

```text
leave today  <=>  today >= stage_end   or   runif() < e
```

- With a geometric duration, this is today's law exactly (the same
  probability each day), though not the same random draws.
- A tool received in the middle of the stage acts from that day; one removed
  stops acting. Nothing is snapshotted at entry.
- With no tool, `e = 0` and no uniform is drawn, so the stream matches a
  model without enhancers.
- It reads as a competing risk ("the tool clears the infection with
  probability `e` per day"). The drawn period then becomes an upper bound on
  the stage.

**The death reduction thins the death outcome.** `p_d (1 - r)` keeps its form,
applied to the per-episode death probability at the end of the stage:
`P(death) = CFR (1 - r)`, read when the stage ends, so a tool received
during the stage counts.

**One change of meaning, stated plainly.** In the geometric model `p_d` is a
**daily** probability, competing with recovery every day, so the share who
die is `p_d / (p_d + p_r)` (more or less, under `roulette()`'s odds rule). In
a duration stage the death probability is **per episode** (a case fatality
ratio). The factory therefore takes its own parameter (`"Case fatality"`)
instead of reusing the virus's `prob_death`, so the two meanings never mix.
Someone moving a geometric SEIRD to durations sets
`CFR = p_d / (p_d + p_r)` to keep the share who die.

**Which enhancers apply where.**

| Stage | Recovery enhancer | Death reduction |
|---|---|---|
| infection stage (3.5.3), exits through `rm_virus` | early recovery, as above | thins death at the end |
| other stages (latent, prodromal, quarantine) | not applied | not applied |

Today, `get_incubation()` and the measles latent and prodromal updates read
no tool either, so leaving them out changes nothing. A tool that changes a
non-infection stage uses Tier 2.

**Not chosen:** rescaling the drawn duration by the enhancer (a shorter
infectious period instead of an early-exit chance). It is a reasonable model,
but it is not what the current formula means, and it needs to know when the
tool arrived. It is one call in Tier 2 for anyone who wants it.

### 3.8 Who is updated

Agents with a virus are updated every day (queue or no queue), so infection
stages are covered. A timed state for agents **without** a virus (for
example, a fixed quarantine of a susceptible) needs one of:

- queuing off (`model.queuing_off()`), as
  `ModelMeaslesMixingRiskQuarantine` and `ModelMeaslesSchool` do; or
- entering the state with `change_state(m, s, Queue<>::OnlySelf)` and leaving
  it with `-Queue<>::OnlySelf`, which keeps the agent in the queue for the
  stay.

In debug builds (`EPI_DEBUG`), `Duration::end()` checks the invariant: if the
first update happens later than `stage_start + 1`, it throws, because the
agent may have missed its end day. Automatic queue membership for timed
states (`state-durations.md` 4.6) can be added later if a model needs it
(Q5).

### 3.9 Copies, threads, and runs

- `stage_start` and `stage_end` are agent fields: they are copied with the
  population and reset by `Agent::reset()`, so `run_multiple()` and model
  copies need nothing new.
- `Duration` objects are immutable and live inside update functions or model
  members, which are copied with the model.
- The group vector of `link_states()` is a model member, copied with the
  model.

---

## 4. What changes in the code

| File | Change |
|---|---|
| `include/epiworld/agent-bones.hpp`, `agent-meat.hpp` | `stage_start`, `stage_end`; copy, assign, reset; three accessors |
| `include/epiworld/model-bones.hpp`, `model-meat.hpp` | `link_states()`, group vector; four lines in `events_run()` |
| `include/epiworld/duration-bones.hpp`, `duration-meat.hpp` (new) | `Duration`, `duration_mean::virus_incubation()` |
| `include/epiworld/agent-meat-state.hpp` | the three factories |
| `include/epiworld/models/seir.hpp`, `seird.hpp` | `Duration` members, `set_duration()`, update functions call `done()` |
| `include/measles/*.hpp` | `Duration` members for incubation and prodromal; links; `done()` in the update functions |

Nothing changes in the queue, the events, the tools, the viruses, or reset.

---

## 5. Worked examples

### 5.1 SEIR, defaults (results unchanged)

`update_exposed_seir` becomes:

```cpp
if (model->latent.done(p, m))     // geometric, mean = virus incubation
    p->change_state(*m, ModelSEIR<TSeq>::INFECTED);
```

`done()` draws `runif() < 1 / incubation`, as now. The Infected state keeps
`default_update_exposed()`.

### 5.2 SEIR with a lognormal latent period

```cpp
ModelSEIR<> m("measles-like", 10000, 0.001, 0.9, 12.5, 1.0/8.0);
m.add_param(2.7, "Incubation sd");
m.set_duration("Exposed", Duration<>::lognormal(
    duration_mean::virus_incubation(), "Incubation sd"
));
```

### 5.3 SEIRD with a gamma infectious period and an antiviral

```cpp
m.add_param(7.0, "Infectious period");
m.add_param(2.0, "Infectious period sd");
m.add_param(0.01, "Case fatality");
m.set_duration("Infected", Duration<>::gamma("Infectious period", "Infectious period sd"));

Tool<> antiviral("Antiviral");
antiviral.set_recovery_enhancer(0.2);   // a 20% chance per day of clearing early
antiviral.set_death_reduction(0.5);     // halves the case fatality
```

`set_duration("Infected", ...)` switches the state to the infection factory
(3.5.3), which reads `"Case fatality"`; the model documents the parameter.

### 5.4 Measles (mixing model)

| State | Today | With stage clocks |
|---|---|---|
| Latent | `runif() < 1 / incubation` | `incubation.done()`; linked with Quarantined Latent |
| Quarantined Latent | incubation draw + `day_flagged` | `incubation.done()` + `day_flagged` (3.6), hand-written |
| Prodromal | `runif() < 1 / Prodromal period` | `prodromal.done()`, then record `day_rash_onset`; linked with Quarantined Prodromal |
| Quarantined Prodromal | prodromal draw + `day_flagged` | `prodromal.done()` + `day_flagged`, hand-written; rash goes to Isolated |
| Rash, Isolated, Hospitalized, Isolated Recovered | daily draws, `day_rash_onset` | unchanged in the first round |
| Quarantined Susceptible, Quarantined Recovered | `day_flagged` | unchanged |

Contact tracing and PEP move agents Latent -> Quarantined Latent through
`change_state()`; the link keeps the drawn end, so quarantine does not pause
or restart the disease. When PEP works, `rm_virus()` moves the agent to a
state outside the group, and the stage ends.

The defaults are geometric, so the measles test suite must pass unchanged
(the identity check of PR D2). `set_duration("Latent", lognormal(...))` and
`set_duration("Prodromal", ...)` turn on the new laws.

### 5.5 A fixed quarantine of a susceptible, in a custom model

```cpp
model.queuing_off();                     // or enter with Queue<>::OnlySelf, 3.8
model.add_state("Quarantined", new_state_update_duration<>(
    Duration<>::fixed("Quarantine period"), SUSCEPTIBLE
));
```

---

## 6. API summary

| Name | Kind | Purpose |
|---|---|---|
| `Duration<TSeq>::geometric / fixed / lognormal / gamma` | class | a stage length, by mean (and sd) |
| `Duration::done(p, m)` | method | does the agent leave today? |
| `Duration::end(p, m)` | method | the end day, drawn once |
| `duration_mean::virus_incubation()` | helper | mean from the agent's virus |
| `new_state_update_duration(d, target)` | factory | one exit |
| `new_state_update_duration(d, targets, probs)` | factory | exits chosen at the end |
| `new_state_update_infection_duration(d, cfr)` | factory | recovery or death through the virus, with enhancers |
| `Model::link_states(states)` | method | states that share one stage |
| `Agent::get_stage_start / get_stage_end / set_stage_end` | methods | Tier 2 |
| `ModelX::set_duration(state, d)` | method | built-in models, wrappers |

---

## 7. Implementation plan

### 7.1 PRs (into `v1-layers`)

1. **D1, core.** Agent fields, `link_states()`, `Duration`, the three
   factories, enhancer handling. No built-in model changes, so no result
   changes. Regenerate `./epiworld.hpp`. `v1-layers` is already at 1.0.0, so
   no version bump.
2. **D2, built-in models.** SEIR, SEIRD and the three measles models: the
   update functions call `done()` on `Duration` members with geometric
   defaults; `set_duration()`; measles links. The existing suite must pass
   unchanged (the identity check).
3. **D3, wrappers.** `set_duration()` in epiworldR (and the other wrappers),
   following the wrappers-after-each-phase runbook in `contact-layers.md`.

D1 and D2 together are small enough for one PR if review prefers.

### 7.2 Identity

- Models that do not use durations: untouched code paths, plus two ints per
  agent and a comparison per state change. The whole existing suite checks
  it.
- Built-in models with geometric defaults: `done()` makes exactly the draw it
  replaces (3.2). The existing model tests check it, draw for draw.
- No other identity claim is made: a non-geometric law is a new model.

### 7.3 Tests (end-to-end, three files)

All three build a model, run it, and check what the public API exposes. They
use a population with no contacts (transmission 0) and every agent seeded, so
each check is a cohort followed through time.

1. **`tests/XXa-durations-fixed.cpp`** (deterministic, `run()`): SEIR with a
   fixed latent period `L` and SEIRD with a fixed infectious period. All
   E -> I transitions happen on day `L`, read from the transition matrix; with
   a recovery enhancer of 1 and `L > 1`, everyone recovers on day 1. Linked
   states: a custom model with Latent <-> Quarantined Latent, moved at day 2, still
   leaves on day `L`.
2. **`tests/XXb-durations-laws.cpp`** (stochastic, `run_multiple()`):
   lognormal and gamma latent periods. The number still exposed on day `t`,
   averaged over replicates, matches `N * P(D > t)` for the discretized law,
   within a tolerance from the binomial sd.
3. **`tests/XXc-durations-enhancers.cpp`** (stochastic, `run_multiple()`):
   infection factory with a fixed period `L`, enhancer `e` and death
   reduction `r` on a tool given to all agents. Still infected on day `t < L`
   is `N (1 - e)^t`; deaths are `N (1 - e)^(L-1) CFR (1 - r)` (the agents who
   reach the end of the period); everyone else recovers.

Plus, in D2, the existing measles and SEIR tests as the identity check, and
one measles run with a lognormal incubation and quarantine checking that no
agent leaves Quarantined Latent for Prodromal before its drawn day.

### 7.4 Benchmarks

On `v1-layers`, CPU time, interleaved runs, in the devcontainer:

- **Unused:** SIR and SEIR at 1M agents before and after D1. Expect no
  measurable change; report peak memory (8 MB more at 1M agents).
- **Per timed agent-day:** SEIR exposed state, legacy `runif()` vs
  `done()` with geometric vs lognormal. Expect lognormal to cost one draw at
  entry and a comparison per day, cheaper than a uniform per day for long
  stages.

---

## 8. Open questions (recommendation first)

1. **Where the built-in models keep their durations:** `Duration` members
   with `set_duration(state, d)` (recommended), vs a model-level registry by
   state name used by every model. The registry would let `add_state()` take a
   duration, but it is a second way to define a state's update.
2. **`link_states` vs a `keep` flag on `change_state()`:** groups
   (recommended, 3.4).
3. **Discretization:** `max(1, ceil(x))` (recommended, same as fixed periods
   today) vs rounding to nearest with a minimum of 1 (closer mean, but a fixed
   1.4-day period would end after 1 day instead of 2).
4. **Exits chosen at the end vs competing exits with their own durations:**
   at the end (recommended). Competing durations (`min` of two draws) are a
   Tier 2 hand-written update if ever needed.
5. **Automatic queue membership for timed states without a virus:** not now
   (3.8); add it if a model with queuing needs it.
6. **The geometric mean below 1** (a daily probability above 1): an error
   (recommended), as in `state-durations.md` 4.5.
7. **Name of the fields:** `stage_start` / `stage_end` vs `state_entered` /
   `state_ends`. "Stage" fits linked states.

---

## 9. Comparison with `state-durations.md` (revision 3)

| | revision 3 (timers and transitions) | this proposal (stage clock) |
|---|---|---|
| Concepts | timer (drawn, hazard, deadline), slot, scope, decision, action, hazard rule, transitions, order check | `Duration`, stage, linked states |
| Clocks per agent | any number | one, plus model-owned vectors |
| Per-agent storage | a vector of slots (24 bytes empty) or a pool index | 8 bytes |
| Core touch points | `events_run()`, queue, reset (init pass), infection scope, tools, transition resolution | `events_run()` (4 lines), `Agent` fields |
| Where the duration is given | `add_transition(state, timer, targets)` | the state's update function (factory), or `set_duration()` |
| Multi-exit states | outcomes on a timer, three hazard rules | one duration, exit drawn at the end |
| Two exit clocks in one state | resolved by the core, with a per-day decision cache | written by hand |
| Hazards depending on days in state | built in | Tier 2 (`today - stage_start`) |
| Recovery enhancer | a factor through `set_param_factor` (read at entry for drawn timers) | an independent daily early exit, today's law |
| Death reduction | a factor on the outcome probability | thins the per-episode death probability |
| Tools changing a duration | `set_param_factor` | Tier 2, `set_stage_end()` |
| Identity of defaults | each legacy rule reproduced by a hazard rule | the update functions keep their draw (`done()`) |
| Risk | many interacting rules; long review | fewer features; two-clock states stay hand-written |
| PRs | T1, T2a, 3d, T2b, T3 | D1, D2, D3 |

**What this proposal gives up:** a second core clock, automatic handling of
states with two exit clocks, generic per-parameter tool factors, and
hazard-based durations as a first-class family. Each can be added later on
top of the stage clock if a model needs it, and `state-durations.md` stays on
record as the design for that case.

---

## 10. Folding into `contact-layers.md`

If accepted, add a short phase after the current one: "D1 + D2: stage
durations (`state-durations-simple.md`)", with D3 following the wrappers
runbook. No other phase depends on it.

## 11. Risks

- **Forgetting a link.** A parallel state not linked restarts the stage. The
  measles tests in D2 cover the three models; the documentation of
  `change_state()` points to `link_states()`.
- **Timed states without a virus under queuing** (3.8): caught in debug
  builds.
- **The CFR change of meaning** (3.7): a separate parameter name, and a note
  in the model documentation.

## 12. Review checklist

Run from a checkout of `origin/v1-layers` (`1ab03e2`):

```sh
# step order: update_state, global events, next
grep -n 'update_state();\|run_globalevents();\|this->next();' include/epiworld/model-meat.hpp
# state_last_changed moves on every event, including tool events
sed -n 425,432p include/epiworld/model-meat.hpp
# enhancer and reduction formulas
sed -n 155,165p include/epiworld/agent-meat-state.hpp
# measles latent / prodromal draws and the quarantined versions
grep -n 'runif() < 1.0/' include/measles/measlesmixing.hpp
sed -n 490,545p include/measles/measlesmixing.hpp
# PEP moves agents between parallel states
sed -n 415,430p include/measles/interventionmeaslespep-meat.hpp
# RNG parameterizations
sed -n 150,186p include/epiworld/model-rand-meat.hpp
```
