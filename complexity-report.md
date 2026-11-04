# Complexity report: the measles mixing model with statuses and stages

NH-1 of `natural-history.md`. This report compares `ModelMeaslesMixing`
before the change (the `natural-history` branch, 12 combined states) and
after it (4 statuses × 5 stages of a natural history). It answers one
question: did the model become simpler, and what did that cost in the
engine?

**How it was measured.** Cyclomatic complexity (CCN) and non-comment lines
of code (NLOC) per function come from [lizard](https://github.com/terryyin/lizard)
1.25.0, run on `include/measles/measlesmixing.hpp` at both commits and on
the parts of `include/measles/stages.hpp` the old model used. The other
counts are greps over the same files (commands at the end). CCN counts the
decision points in a function plus one, so it measures how many paths there
are to test.

## 1. Summary

| | Before | After | Change |
|---|---:|---:|---:|
| States the user sees | 12 combined states | 4 statuses + 5 stages | 9 labels, no products |
| Update functions | 11 | 4 statuses + 1 stage | −55% |
| Disease and policy logic: functions | 15 | 8 | −47% |
| Disease and policy logic: NLOC | 281 | 129 | **−54%** |
| Disease and policy logic: total CCN | 71 | 42 | **−41%** |
| Most complex function (CCN) | 17 (`_quarantine_process`) | 14 (same) | −3 |
| `change_state()` calls | 17 | 8 | −53% |
| Copies of the release-from-quarantine/isolation rule | 6 | 2 | −67% |
| State-to-twin mappings (`stage_of()`, the `switch` in tracing) | 2 (10 cases) | 0 | gone |
| Per-agent vectors the model keeps for timers | 5 | 0 | gone |
| Lines in the model file | 824 | 716 | −13% |

"Disease and policy logic" is every update function, the quarantine
process, the stage hook, and, before, the three `MeaslesStages` functions the
model relied on (`stage_of()`, `stage_over()`, `reset_stages()`). It leaves
out the constructor, `reset()`, the getters and `initial_states()`, which
are mostly declarations (reported separately below).

## 2. Where the reduction comes from

**The product of states is gone.** Before, each combination of a disease
stage and a status that the model allowed was a state with its own update
function: Quarantined Susceptible, Quarantined Latent, Quarantined
Prodromal, Quarantined Recovered, Isolated, Isolated Recovered. Each of those
functions repeated the release rule (`days_since` against the period) and
then decided where the agent goes in both dimensions at once. After, there
is one function per status, and the release rule is written once for
quarantine and once for isolation:

| Before (12 states, 11 functions) | After |
|---|---|
| `_update_susceptible`, `_update_rash` | `_update_free` (infection; detection and admission with rash) |
| `_update_quarantine_suscep`, `_quarantine_latent`, `_quarantine_prodromal`, `_quarantine_recovered` | `_update_quarantined` (release) |
| `_update_isolated`, `_update_isolated_recovered` | `_update_isolated` (admission with rash; release) |
| `_update_hospitalized` | `_update_hospitalized` (discharge) |
| `_update_latent`, `_update_prodromal` | none: the stages declare a duration and a next stage |
| `MeaslesStages::stage_over()`, `stage_of()` | none: the natural history draws durations on entry |
| (rash onset in quarantine, inside `_update_quarantine_prodromal`) | `_on_stage_change` (the stage hook, 2 lines) |

**Timers moved to the engine.** The old model kept five per-agent vectors to
remember when things happened (`day_rash_onset`, `day_latent`, and
`MeaslesStages`'s `stage_end`, `stage_kind`, `stage_infection`), plus the
logic to tell a new stage from a status change within the same stage. The
natural history now keeps the entry day of every stage and the end of the
current stage's duration, with one lifecycle for every model (reset,
copied, cleared at a new infection). `day_flagged` stays: it is a
quarantine date, which belongs to the model.

**Contact tracing no longer depends on how states are numbered.** Before,
eligibility was `get_state() > RASH` (it relied on the order of the 12
codes), and a `switch` moved each state to its quarantined twin. After, it
reads "free and not recovered" and changes only the status.

## 3. What did not get simpler

- **`_update_free` is the largest function (CCN 11).** It holds infection
  (for susceptible agents) and detection plus admission (for agents with
  rash). Before, the same logic was split over `_update_susceptible` (5) and
  `_update_rash` (7), so the total is about the same; it is one function
  because both are rules for free agents. Status functions branch on the
  stage, as Section 5 of the plan expected: the state space was moved, not
  removed.
- **`_quarantine_process` stays the most complex function** (17 → 14). Most
  of it is reading the contact log, which is unrelated to states.
- **The constructor grew from 100 to 118 NLOC**: the natural history is
  declared there (stages, durations, infection and recovery stages), and the
  sampler gets a 7-line role function instead of four state codes.
- **`initial_states()` grew (CCN 1 → 9).** The old one reused the SEIR
  initializer, which moved agents to state codes 2 and 3: in this model
  those are Prodromal and Rash, so "recovered" agents started with rash. The
  new one sets stages and validates its arguments. This is a bug fix, not
  part of the comparison.

## 4. What it cost in the engine

| Engine part | Lines added (with documentation) |
|---|---:|
| `NaturalHistory` (new, `naturalhistory-*.hpp`) | 309 (150 NLOC in 14 functions, mean CCN 2.2) |
| Database: stage counts, transitions, joint table, accessors | 234 |
| Model: registration, stage events, hook, update loop, copies | 216 |
| Agent, Virus, Event, config | 94 |
| **Total in `include/epiworld/`** | **853 (571 without comments and blank lines)** |

The model shed about 150 lines of logic for about 570 lines of engine code.
That trade only pays off if other models use the engine too. The next
candidates are the two other measles models (each with 12 or more combined
states and its own use of `MeaslesStages`) and `ModelSEIRQuarantine` (10
states). The engine is generic, so each port costs only its own model code;
whether the ports shrink in the same proportion is NH-2's measurement.

The engine also took over things no model did before: stage histories, the
joint status × stage table, episode counts and the entry day of every
stage, which the epidemiological review asked for (Section 9 of the plan).

## 5. Behaviour

The port keeps the old model's rules, including the ones the review
questioned (hospitalization pauses the rash clock; quarantine skips any
agent holding a tool; a quarantined agent with rash is always isolated).
Each is now one visible line rather than a property of the state graph,
listed in Section 12 of the plan as a separate policy decision (NH-1b).

Two differences, both from the representation:

- The order of random draws changed (durations are drawn when a stage is
  entered rather than at the first update), so runs are statistically
  equivalent, not identical.
- When an agent with rash is admitted on the day the rash would end, both
  happen: the agent is recovered while in hospital. The old model kept the
  agent in Rash until discharge. Discharge times, and the counts of
  admissions, do not change.

Tests (end to end):

- `tests/18a`, `18b`, `18e` keep the same theoretical targets, now read
  from stages and statuses: R0, the latent and prodromal rates, detection,
  admission per day with rash (free and isolated), discharge, and the share
  of cases hospitalized.
- `tests/51b` replays the deterministic scenarios of the 12-state model
  (fixed durations; probabilities of 0 or 1) and gets the same days for rash
  onset, quarantine, isolation, admission, discharge and release.
- `tests/51a` tests the engine: durations, episodes and reinfection, stale
  events, the hook's timing, copying, and that the stage counts, the joint
  table and the statuses add up every day.

## 6. Speed

The same scenario on both versions: 200,000 agents in 4 groups, 150 days,
half vaccinated, about 79,000 infections. Six seeds, run interleaved, CPU
time of `run()`, in the development container on a loaded Mac (so the
numbers compare the two versions with each other, not with other machines):

| | Old (12 states) | New (statuses × stages) |
|---|---:|---:|
| CPU time, mean of 6 runs | 3.04 s | 3.41 s (**+12%**) |
| Range | 2.76–3.50 s | 3.19–3.61 s |
| Infections, mean of 6 runs | 78,781 | 78,907 |

The final sizes agree, as they should for a statistically equivalent
model. The new version is slower, and the cost is not yet attributed. Three
per-agent, per-day costs are new and are the likely sources:

1. the stage update of every agent (a lookup even when the stage has no
   function or duration, as for the susceptible);
2. the sampler's role function (a `std::function` call per agent per day,
   where the old sampler compared state codes);
3. the database scan at each record (stage counts, transitions and joint
   table).

Each has a cheap fix if it matters: skip stages with nothing to do, inline
the role in the measles sampler, and record the joint table only when asked.
Profiling them is the first task of the follow-up, before NH-2.

## 7. Commands

```sh
git show natural-history:include/measles/measlesmixing.hpp > old.hpp
git show natural-history:include/measles/stages.hpp > old_stages.hpp
lizard -l cpp old.hpp old_stages.hpp include/measles/measlesmixing.hpp
grep -c "change_state(" old.hpp include/measles/measlesmixing.hpp
grep -c "days_since =" old.hpp include/measles/measlesmixing.hpp
git diff natural-history --numstat -- include/epiworld
```
