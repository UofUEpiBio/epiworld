# State durations: notes on a clock held by the virus

**Status:** discussion notes (2026-10-30), not a plan. They record a third
direction that came up after `state-durations-v2.md`, with the decisions
taken so far and the cases where it breaks. Nothing is scheduled; the measles
models get their non-geometric stages first, inside the measles models (see
the measles PR), and this note is the starting point when the general
mechanism is picked up again.

## 1. The idea

The duration of a disease stage belongs to the disease, not to the state:
two diseases in one model share the states (Exposed, Infected) but not their
periods. So the clock lives on the **agent's copy of the virus**:

```cpp
measles.set_duration({LATENT, QUARANTINED_LATENT}, {
  exit(PRODROMAL, 1.0, lognormal("Latent period", "Latent period sd"))
});
measles.set_duration({RASH, ISOLATED}, {
  exit(HOSPITALIZED, "Hospitalization prob", lognormal("Time to hospitalization", "... sd")),
  exit(RECOVERED,    rest(),                 lognormal("Rash period", "Rash period sd"))
});
```

When the agent enters a group of states (coming from outside it), its virus
picks the exit, draws that exit's duration and stores the exit, the exit day
and a speed. The state's update function checks `today() >= exit_day`.
Moving between states of one group (Latent <-> Quarantined Latent) keeps the
clock.

## 2. Why it fits the code

- **Each agent already has its own virus copy.** `Agent::set_virus()` clones
  it (`virus.clone_ptr()`, `agent-meat.hpp`) and `_event_add_virus()` sets its
  date. A clock stored there has infection scope for free: a new infection is
  a new copy, `rm_virus()` removes the clock, uninfected agents pay nothing.
- **Precedent:** `ToolImmunoglobulin` draws its removal day when the agent
  receives it, stores it on the agent's copy and compares it with `today()`.
- **Viruses already know model states** (`set_state(init, post, removed)`), so
  a state map on the virus is no new kind of coupling.
- **Who owns which clock:** disease clocks the virus, intervention clocks the
  tool (waning immunity after recovery is a tool, as immunoglobulin is),
  policy clocks the model (`day_flagged`). This answers the two reasons
  `state-durations.md` Section 3 gave for dropping a virus clock (no clock for
  a quarantined susceptible; two clocks for a quarantined latent agent).
- **No stranded agents for disease clocks:** agents with a virus are always
  updated, and only they have one.

## 3. Decisions so far

1. **Geometric is drawn at entry too.** One mechanism for every law; no
   daily-draw path kept for identity. Every result changes once.
2. **Only one way of doing things.** Several exits: the exit is chosen at
   entry, each with its own duration (Starsim's "separate transitions with
   `p=`"). One duration with the exit chosen at the end is the special case of
   equal durations.
3. **Durations are linked explicitly to model states** (`set_duration(states,
   ...)`), instead of a special `set_incubation()`. Undecided whether to keep
   `set_incubation()` as a compatibility shortcut.

## 4. Modifiers

- **At entry:** the law sees the agent's tools when it draws.
- **Mid-stay:** a speed on the virus copy, rescaled on tool add/remove events
  (Starsim 4's `rescale_exits()`: `new_exit = now + left * before / now`). No
  daily cost. A recovery enhancer `e` maps to speed `1 / (1 - e)`.
- **Scope:** tools already target viruses (target mask, lineage), so a measles
  modifier does not touch flu.

## 5. Three scenarios where it works

1. **Two diseases on one SEIRD layout.** Flu: Exposed lognormal mean 2 days;
   Infected exits death (0.1%, gamma time to death) or recovery (gamma 5
   days). Measles: its own durations on the same states. A measles-only
   vaccine changes only measles clocks.
2. **Measles with tracing and PEP.** Infected day 0, latent drawn 11 days;
   traced day 3 -> Quarantined Latent keeps day 11; released day 24 or still
   quarantined at day 11, the update function maps the exit ("onset") to
   Prodromal or Quarantined Prodromal. PEP success: `rm_virus()` drops the
   clock; failure: a move to a quarantined state keeps it.
3. **A mid-stay modifier.** Immunoglobulin on day 3 of an 11-day latent period
   with speed 0.5: the 8 days left become 16 (why quarantine is extended for
   immunoglobulin recipients). A flu antiviral at speed 1.5 shortens the
   infectious period. One rescale per tool event, only for the targeted virus.

## 6. Where it breaks

1. **Clocks with no virus.** Waning immunity (Recovered -> Susceptible after
   180 days), a second vaccine dose at 28 days, the fixed quarantine of a
   susceptible contact, isolation from rash onset. They need tools and the
   model as owners, so "one way" holds only if tools get the same
   `set_duration` machinery; policy clocks stay a hand-written second way.
2. **An exit chosen at entry cannot react.** Assigned "hospitalized on day 3"
   at Rash entry, given on day 2 a treatment that halves hospitalization: the
   speed changes when, never which. Today's daily `death_reduction` would lose
   function. Fix: re-pick the exit on tool events (more machinery, and what
   happens to the time spent?), or document that outcome modifiers act at
   entry only.
3. **Daily events that are not disease exits.** Measles detection in Rash
   ("Days undetected") moves the agent to Isolated, inside the Rash group, so
   it cannot be an exit chosen at entry; it stays a daily draw next to the
   timer. With the exit-to-state mapping of scenario 2, the quarantined
   measles states stay hand-written; the declarative part covers the simple
   states.

Smaller: a virus without a duration for a timed state leaves the agent stuck
(no geometric fallback), so the model needs a check at run start; a mid-stay
mutation keeps the old plan unless documented otherwise; every existing
expected value changes (acceptable on 1.0.0).

## 7. Open

- Outcome modifiers (`death_reduction`) at entry only, or a re-pick on tool
  events?
- Tools with the same duration machinery, so "one way" covers intervention
  clocks?
- `set_incubation()` kept as a shortcut, or removed?
