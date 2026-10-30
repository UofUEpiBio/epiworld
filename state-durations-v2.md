# State durations, version 2: draw the stay when the agent enters

**Status:** proposal, revision 1 (2026-10-30). It replaces both
`state-durations.md` (timers and transitions, revision 3) and
`state-durations-simple.md` (stage clock, revision 1). Both stay in the
history of #330 as the record of what was considered. Nothing here is
settled.

**Base:** code references are to `origin/v1-layers` at `b699adb`.

## 1. The idea in one paragraph

Any state can have a **duration**: a function that returns a number of days.
When an agent **enters** the state, the core calls it once, with the agent as
it is at that moment (virus, tools, entity, parameters), and stores the exit
day on the agent. The state's update function checks whether that day has
come. Nothing changes the exit day afterwards. States can **share** a
duration (Latent and Quarantined Latent): moving between them keeps the exit
day, so quarantine does not pause or restart the disease. States without a
duration behave exactly as today.

## 2. Why this is enough

The earlier plans tried to support stays that change while the agent is in a
state: tools that rescale the remaining time, hazards that depend on the days
already spent, several clocks resolved by the core. Real models rarely need
that. What changes the course of a disease in practice is a **move to another
state**:

- **Quarantine and isolation** move the agent to a parallel state
  (Prodromal -> Quarantined Prodromal). The disease stage goes on; the shared
  duration covers it.
- **Post-exposure prophylaxis** moves the agent: on success, `rm_virus()` takes
  it out of the disease (`interventionmeaslespep-meat.hpp`); on failure, it
  goes to a quarantined state.
- **Policy clocks** (quarantine length, isolation length) are fixed deadlines
  that the measles models already keep in `day_flagged` and
  `day_rash_onset`. They stay as they are.

Anything an agent brings to the state is visible when the duration is drawn:
a tool received before infection (immunoglobulin, a vaccine), its age group,
its virus. That covers per-agent heterogeneity without a factor system. A
case that needs more (a stay that changes midway) is a hand-written update
function in C++, which epiworld already supports.

## 3. Design

### 3.1 On the agent: one int

```cpp
int state_end = -1;  ///< Day the agent leaves its current state; -1 = untimed
```

Copied by the copy constructor and both assignments, set to `-1` by
`Agent::reset()`. Accessors:

```cpp
int  Agent::get_state_end() const;
bool Agent::state_ended(const Model<TSeq> & m) const;  // state_end >= 0 && today >= state_end
```

`state_ended()` is a pure comparison: calling it twice gives the same answer
and draws nothing.

### 3.2 On the model: durations per state

```cpp
template<typename TSeq>
using DurationFun = std::function<epiworld_double(Agent<TSeq> &, Model<TSeq> &)>;

void Model<TSeq>::set_state_duration(std::vector<int> states, DurationFun<TSeq> d);
```

- The states in one call **share** the duration. Moving among them keeps
  `state_end`.
- A later call for a state replaces its duration (last call wins). Passing an
  empty function makes the states untimed again.
- State 0 cannot be timed: `Agent::reset()` puts agents there without an
  event, so nothing would draw. The call throws.
- Storage: a `std::vector<int>` of clock ids per state (`-1` = untimed) and a
  `std::vector<DurationFun>`. Both are copied with the model.

### 3.3 The one change in `events_run()`

After `p->state = new_state` (`model-meat.hpp:426`), so the event's virus or
tool is already on the agent:

```cpp
if (has_durations && new_state != -99 && new_state != state_old)
{
    int c_new = state_clock[new_state];
    if (c_new < 0)
        p->state_end = -1;                                  // untimed state
    else if (c_new != state_clock[state_old])
        p->state_end = today() + to_days(clocks[c_new](*p, *this));
    // else: same clock, keep state_end
}
```

with `to_days(x) = max(1, round(x))`. An agent entering on day `t` is first
updated on day `t + 1` and leaves on the update of day `t + d`, so it is
counted in the state for `d` days.

When no model state has a duration, `has_durations` is false and the block
costs one branch per event. Models that do not use durations make no new
draws: their results are identical by construction.

**Who is updated.** An agent in a timed state must be updated every day.
Agents with a virus always are. If an agent enters a timed state without a
virus while queuing is on, the draw throws, naming the state and saying to
turn queuing off (as `ModelMeaslesMixingRiskQuarantine` and
`ModelMeaslesSchool` already do).

**Initial states.** Seeds and initial states enter through events
(`dist_virus()`, the functions in `models/init-functions.hpp`, and the final
`events_run()` in `reset()`, `model-meat.hpp:1968-1983`), so they draw a full
duration on day 0, as newly infected agents. Starting an ongoing outbreak
with agents part-way through a stage is out of scope; a custom
`initial_states_fun` can do it.

### 3.4 Duration factories

```cpp
namespace epiworld::duration {
    DurationFun<TSeq> fixed(ParamRef mean);
    DurationFun<TSeq> geometric(ParamRef mean);           // same law as today
    DurationFun<TSeq> lognormal(ParamRef mean, ParamRef sd);
    DurationFun<TSeq> gamma(ParamRef mean, ParamRef sd);
}
```

- Arguments are model parameters, read at the draw, so `set_param()` between
  runs takes effect. The factories capture only parameter references, so
  copies and `run_multiple()` threads are safe.
- `mean` and `sd` are on the **natural scale, in days**, as periods are
  reported. Conversions:
  - lognormal: `s2 = log(1 + sd^2/mean^2)`, `mu = log(mean) - s2/2`,
    `rlognormal(mu, sqrt(s2))`;
  - gamma: `shape = mean^2/sd^2`, `scale = sd^2/mean`, `rgamma(shape, scale)`;
  - geometric: `1 + rgeom(1/mean)`, the law of today's daily
    `runif() < 1/mean` (same distribution, different random stream).
- `sd = 0` gives `fixed`. A geometric mean below 1, a negative sd or NaN
  throws, naming the parameter.
- Anything else is a lambda, for example a longer incubation for agents who
  received immunoglobulin, or a mean by age group.

### 3.5 Update functions

The update function still decides **where** the agent goes; the duration
decides **when**.

- **New factory** for user-built models:
  `new_state_update_timed<TSeq>(int target)`: each day,
  `if (p->state_ended(*m)) p->change_state(*m, target);`.
- **Built-in update functions** honor a duration if the user sets one, and
  keep today's draw otherwise:

  ```cpp
  bool Agent::leaves_state(Model<TSeq> & m, epiworld_double daily_prob);
  // timed:   state_ended(m), no draw
  // untimed: m.runif() < daily_prob, exactly today's draw
  ```

  This replaces each `m->runif() < 1.0/...` exit in `seir.hpp`, `seird.hpp`
  and `seirquarantine.hpp`.
- **`default_update_exposed()`** (the infected state of SIR, SIRD, SEIR,
  SEIRD, SIS, SISD): untimed, unchanged (roulette over death and recovery).
  Timed: recovery happens when the stay ends; death stays a daily risk,
  `runif() < p_d (1 - r)`, checked first, so the death reduction works as
  today and `p_d` keeps its meaning. The recovery enhancer is not applied to
  a timed state; a duration function can read the agent's tools at entry if
  a model wants that.

## 4. Measles

The three measles models switch to drawn durations by default, lognormal for
every disease stage. Policy clocks stay as they are.

| Clock (states sharing it) | Parameters | Exit at the end |
|---|---|---|
| Latent, Quarantined Latent | `Incubation period`, `Incubation period sd` | Prodromal (Quarantined Prodromal if still in quarantine) |
| Prodromal, Quarantined Prodromal | `Prodromal period`, `Prodromal period sd` | Rash (Isolated if quarantined), rash onset recorded |
| Rash, Isolated | `Rash period`, `Rash period sd` | Recovered (Isolated Recovered if detected or still isolated) |
| Hospitalized | `Hospitalization period`, `Hospitalization period sd` | Recovered |

Unchanged: quarantine and isolation deadlines (`day_flagged`,
`day_rash_onset`), detection (`Days undetected`, a daily probability),
contact tracing, PEP, and the Quarantined Susceptible / Quarantined
Recovered / Isolated Recovered states.

What changes in the update functions, using `ModelMeaslesMixing` as the
example:

- `runif() < 1.0/incubation` (lines 334, 495) and
  `runif() < 1.0/Prodromal period` (355, 522) become `p->state_ended(*m)`.
  The quarantined versions keep their logic: if the stage ended, go to the
  next disease state, quarantined or not according to the release check;
  else, if released, go back to the parallel state (same clock, so the end
  day is kept).
- **Rash and Isolated** (385-452): today one exclusive draw picks recovery
  (`1/Rash period`) or hospitalization (`Hospitalization rate`) each day.
  With the rash clock: hospitalization stays a daily risk, drawn first; else
  the agent recovers when the rash period ends. `Hospitalization rate` keeps
  its meaning, but the share hospitalized changes, because the rash stay is no
  longer geometric (Q3).
- **Hospitalized** (578): `p->state_ended(*m)`.

New parameters are added with defaults in the constructors, without new
constructor arguments, and changed with `set_param()`; `sd = 0` gives fixed
periods. Users who want the old geometric laws call
`set_state_duration(..., duration::geometric(...))`. Default sd values are
Q1.

The measles test suite's expected numbers will change; this is a new model
on `v1-layers` (1.0.0), not an identity refactor.

## 5. API summary

| Name | Purpose |
|---|---|
| `Model::set_state_duration(states, d)` | states sharing one drawn duration |
| `duration::fixed / geometric / lognormal / gamma` | factories by mean (and sd), from parameters |
| `Agent::get_state_end()`, `Agent::state_ended(m)` | read the exit day |
| `Agent::leaves_state(m, daily_prob)` | built-in update functions: timed or today's draw |
| `new_state_update_timed(target)` | one-exit update function for user-built models |

Wrappers (epiworldR first): `set_state_duration(model, states, family, mean,
sd)`, and the measles functions gain the sd arguments.

## 6. Left out, on purpose

- Changing the exit day after entry (tool factors, `set_stage_end()`,
  accelerated-failure-time effects). A hand-written update function can keep
  its own deadline if a model needs it.
- Hazards that depend on the days spent in the state.
- Several core clocks per agent and automatic resolution of two exits on the
  same day. Policy clocks stay model-owned and the quarantined update
  functions stay hand-written.
- Outcome-specific durations (a different time to hospitalization and to
  recovery). The exit is decided by the update function; competing risks
  stay daily.
- A multi-exit timed factory, a scheduled-event backend, prevalent seeds with
  elapsed stage ages. Each can be added later without changing the above.

## 7. How the review points of #330 are met

| Point | Answer |
|---|---|
| Draw at entry or lazily | At entry, in `events_run()`, one contract |
| Baseline and initial states | All go through events; state 0 cannot be timed |
| Repeated queries draw again | `state_ended()` draws nothing; geometric is drawn at entry too |
| Agents never updated | Throws at the draw (no virus, queuing on) |
| Same-state reinfection | Keeps the exit day (no state change); documented |
| Clone and thread independence | Factories hold parameter references only; a test runs `run_multiple()` with threads |
| CFR conversion error | Gone: no per-episode death probability; `p_d` stays daily |
| Shared vs outcome-specific durations | Stated as a restriction (Section 6) |
| Incubation vs latent | The measles defaults name the period they model (Q1) |

## 8. Implementation

PRs into `v1-layers`:

1. **D1, core.** `state_end`, `set_state_duration()`, the `events_run()`
   block, factories, `leaves_state()`, `new_state_update_timed()`, the
   `default_update_exposed()` timed branch. Built-in models untouched in
   behavior. Regenerate `./epiworld.hpp`.
2. **D2, measles.** The three measles models on durations, new parameters,
   updated expected values in the measles tests.
3. **D3, wrappers**, following the runbook in `contact-layers.md`.

**Tests** (end-to-end, three files):

1. `XXa-durations-fixed.cpp` (`run()`, deterministic): SEIR with
   transmission 0, every agent seeded, a fixed latent period `L`; all E -> I
   moves happen on day `L` (transition history). A global event moves every
   agent Latent -> Quarantined Latent on day 2 in a custom model sharing the
   clock; they still leave on day `L`.
2. `XXb-durations-laws.cpp` (`run_multiple()`, with threads): lognormal and
   gamma latent periods; the share still exposed on day `t` matches
   `P(D > t)` of the discretized law within binomial error.
3. `XXc-durations-measles.cpp` (`run()`): `ModelMeaslesMixing` with sd = 0,
   transmission 0, seeded agents: each stage change happens on its fixed day.

Plus the existing suite, unchanged, as the check that untimed models are
identical.

**Benchmark** (devcontainer, CPU time, interleaved): SIR and SEIR at 1M agents
before and after D1 (expect no change, 4 MB more memory); SEIR with a
lognormal latent period vs geometric (expect one draw at entry instead of one
per day).

## 9. Open questions (recommendation first)

1. **Default sd for measles.** Start from Lessler et al. (2009): incubation
   (exposure to rash) lognormal with median 12.5 days and dispersion 1.23,
   about sd 2.7. The model's `Incubation period` ends at prodromal onset, not
   rash, so its sd must be derived, not copied. Prodromal, rash and
   hospitalization sd need sources; to agree with the measles team.
2. **Discretization:** `max(1, round(x))` (recommended: the realized mean
   matches the input) vs `max(1, ceil(x))` (half a day longer on average).
3. **Measles hospitalization:** keep `Hospitalization rate` as a daily risk
   (recommended, smallest change) vs a per-case probability decided at the end
   of the rash. Either way the share hospitalized changes and the defaults
   need recalibrating.
4. **Recovery enhancer on timed infected states:** not applied (recommended)
   vs dividing the drawn duration by `1 - e` at entry.
