# Natural History and Statuses

A disease's **natural history** is a small graph: its nodes are **stages** (susceptible, latent, infectious, recovered, ...) and its edges are declared **transitions**. A model's **statuses** (free, quarantined, isolated, ...) are a second graph with the same vocabulary. Every agent is in one stage of each disease and in one status. This page explains how each kind of transition is evaluated, how they compete, and how to choose among them.

```cpp
Virus flu("Flu");
const int S = flu.add_stage("Susceptible");
const int E = flu.add_stage("Exposed", stage::carrier);
const int I = flu.add_stage("Infected", stage::infectious);
const int H = flu.add_stage("Hospitalized", stage::carrier);
const int R = flu.add_stage("Recovered");

flu.infection(S, E, "Transmission probability");          // by contact
flu.after(E, I, Duration::lognormal(3.0, 1.0));           // a drawn duration
flu.rate(I, R, "Recovery rate").effect(Effect::recovery); // a daily probability
flu.rate(I, H, "Hospitalization rate");                   // competes with recovery
flu.after(H, R, Duration::gamma(7.0, 2.0));
flu.external({E, I}, R, "Antiviral cure");                // fired by code

model.add_virus(flu, assign::share(0.01));
```

## Stages

`add_stage(label, flags)` returns the stage's id. The first stage is where agents start (unless `Model::set_initial()` places them elsewhere).

- `stage::carrier`: the agent holds the virus. `stage::infectious`: it also transmits (it implies carrier). Entering a stage that is not a carrier releases the virus.
- `set_contact(stage, weight)`: the stage's contact weight (default 1). A weight of 0 takes its agents out of every layer: they are not met and do not transmit. A stage with weight 0 and no exits is a death.

No stage has a fixed role: a stage is susceptible because an infection leaves it.

## The four kinds of transitions

| Kind | Declared with | When it happens | Evaluated |
|---|---|---|---|
| infection | `infection(from, to, per_contact)` | a contact with an infectious agent transmits | each day, from the day's contacts |
| rate | `rate(from, to, daily)` | with a daily probability | each day the agent is in `from` |
| after | `after(from, to, duration)` | when a duration, drawn on entering `from`, ends | once, when the agent enters `from` |
| external | `external(from, to, name)` | when code fires it (`Model::fire()`) | when fired |

`from` can be one stage or several (`{E, I}`) for `infection()`, `rate()` and `external()`; an `after()` leaves one stage, since its duration is drawn on entering it.

### Rates: a probability, every day

A rate is read every day the agent spends in its stage, so it can change during the run:

- a model parameter that a global event changes (e.g., a treatment campaign that raises "Recovery rate" on day 30);
- a function of the agent and the model (`Value([](Agent & a, Model & m) { ... })`), e.g., a rate by age;
- a tool's effect: `.effect(Effect::recovery)` turns the rate `p` into `1 - (1 - p) * (1 - recovery enhancer)`, and `.effect(Effect::death)` into `p * (1 - death reduction)`, for agents holding a tool with that effect.

With a constant rate `p`, the time in the stage is geometric with mean `1 / p`.

### Durations: a deadline, drawn on entry

An `after()` draws a number of days when the agent enters its stage, and the agent leaves on the day it ends. The draw is not repeated, so a parameter that changes later does not affect agents already in the stage.

| Duration | Days |
|---|---|
| `Duration::fixed(d)` | `d` |
| `Duration::geometric(m)` | geometric with mean `m` (the same distribution as a rate `1 / m`) |
| `Duration::lognormal(m, sd)` | lognormal with mean `m` and standard deviation `sd`, in days (not on the log scale) |
| `Duration::gamma(m, sd)` | gamma with mean `m` and standard deviation `sd` (shape `m² / sd²`, scale `sd² / m`) |
| `Duration(fun)` | `fun(agent, model)` |

Durations are rounded to whole days, with a minimum of one; a standard deviation of 0 gives a fixed duration. Each number can be a value, a model parameter, or a function (see [Values](#values)).

`.since(stage)` counts the duration from the day the agent entered another stage, in the same infection, instead of from the day it entered `from`. For example, the measles models end isolation a fixed number of days after the rash onset, however late the agent was isolated: `Duration::fixed("Isolation period").since(RASH)`. If the agent never entered that stage in this infection, the duration counts from the entry into `from`.

### External transitions: fired by code

`external()` declares a transition that only code takes, through `Model::fire(transition, agent)`, from a global event, a reaction, or an intervention (e.g., post-exposure prophylaxis clearing an infection). `fire()` can also apply a rate or an `after()` transition (every kind but infections, which are seeded). It returns false, and does nothing, if the agent is not in one of the transition's sources; a transition is identified by its id, `model.transition_id(name)`.

## Within a day

Each day, for each agent:

1. **Infection.** An agent without an infection draws at most one, from the day's contacts (see [Combining Exposures](combining-exposures.md)). If it is infected, its stage in that disease does nothing else that day (with several diseases, its stages in the others still follow steps 2 and 3).
2. **Rates.** In each disease, the rates out of its stage compete: one draw picks one of them, or none (below).
3. **Duration.** If no rate fired and the stage's duration has ended, the agent takes the `after()` transition. A rate that fires first wins over the deadline; the deadline then no longer applies, since the agent left the stage.
4. **Status.** Then the status, with the same rules (rates, then the duration), judged on the stages at the start of the day.
5. **Reactions** (`Model::react()`) to what changed, in the same step.

The changes take effect at the end of the day, so an agent entering a stage on day `t` is first evaluated in it on day `t + 1`, and an `after()` of `d` days moves it on day `t + d`.

![How a run proceeds: at the start of each run the agents are reset, the layers drawn, the viruses, tools and initial stages placed and the global events set up; every day each agent proposes an infection, a stage change in each disease and a status change, in that order and from its state at the start of the day; the proposals are applied together and reactions fire, global events run, tomorrow's parameters and layer schedules apply, the layers are updated, the day is recorded and viruses may mutate.](../assets/img/epiworld-day.svg)

*A run, day by day (`Model::run()`): every agent decides from the same start-of-day state, and the proposals are applied together at the end of the step.*

### How competing rates are drawn

When several rates leave a stage, one draw (`roulette()`) picks at most one. With rates `p_1, ..., p_k`, it uses their odds, `o_i = p_i / (1 - p_i)`:

- nothing happens with probability `1 / (1 + o_1 + ... + o_k)`;
- rate `i` fires with probability `o_i / (1 + o_1 + ... + o_k)`.

With one rate, this is exactly `p`. With several, each fires **less often than its nominal value**: two rates of 0.5 each fire with probability 1/3, and nothing happens with probability 1/3. (It is the distribution of independent draws of each rate, given that at most one fires.) For small rates the difference is small: two rates of 0.1 fire with probability 0.091 each.

To get exact probabilities, use one rate for leaving the stage and a branch for where to go:

```cpp
// Leaves at 0.2 a day; 90% recover, 10% are hospitalized
flu.rate(I, R, "Leave rate").to({{R, 0.9}, {H, 0.1}});
```

`.to({{stage, probability}, ...})` draws the target when the transition is taken. Its probabilities can be values, parameters or functions; if they add up to less than 1, the last target takes the rest.

## Which to use

| Situation | Use |
|---|---|
| A period with a known distribution: incubation, a fixed quarantine, an infectious period with a set shape | `after()` |
| A hazard that changes during the run, by agent, or under a tool (treatment, detection) | `rate()` |
| Several possible outcomes with given probabilities | one `rate()` (or `after()`) with `.to()` |
| A change only an intervention or a policy makes | `external()` and `Model::fire()` |
| A memoryless period (geometric) | either: `after(..., geometric(m))` or `rate(..., 1 / m)`; the rate can then change during the run, the duration cannot |

Both kinds can leave the same stage. In the measles models, Rash has a daily hospitalization rate and a lognormal duration to recovery: an agent hospitalized before its rash ends goes to Severe, which stops the rash's clock.

## Values

Every number a transition uses is a `Value`:

- `Value(0.1)`: a number. When the virus is added to a model, it becomes a model parameter named after the virus and the transition, so it can be read and changed with `set_param()`.
- `Value("Recovery rate")`: a model parameter, by name.
- `Value::inverse("Infectious period")` and `Value::complement("p")`: `1 / parameter` and `1 - parameter`.
- `Value(fun)`: `fun(agent, model)`.

A parameter is read through the agent's virus, so a strain can point it to another parameter (`Virus::use_param()`), e.g., a strain that recovers faster.

Other options of a transition: `.name()` (default `"From -> To"`; the name `transition_id()`, `get_hist_transitions()` and reactions use) and `.sampler()` (an infection's own contact sampler, in C++).

## Statuses

`Statuses` declares a model's statuses with the same vocabulary, without infections or carriers, and `Model::set_statuses()` gives them to the model. The first status is where agents start; a model without statuses has one, "Free". See [Quarantine, Isolation, and Contact Tracing](quarantine-isolation-and-contact-tracing.md) for an example.

```cpp
Statuses st;
const int free_    = st.add_status("Free");
const int isolated = st.add_status("Isolated");
st.set_contact(isolated, 0.0);  // out of every layer

st.rate(free_, isolated, "Detection rate").name("Detected").when({I}).of("Flu");
st.after(isolated, free_, Duration::fixed(7).since(I).of("Flu")).name("Released");
model.set_statuses(st);
```

- **`.when({stages})`** restricts a status transition to agents in those stages at the start of the day. For an `after()`, the deadline is taken only while the agent is in one of those stages: an agent whose isolation ends while it is not in them stays isolated until it is.
- **`.since(stage)`** counts a status's duration from a stage's entry, as above.
- **`.of(disease)`** names the disease of the stages in `.when()` and `.since()`. It is required when the model has several diseases; with one, it can be left out.
- A status with contact weight 0 takes its agents out of every layer, for every disease; `ContactLayer::exclude_status()` takes them out of one layer only.
- An agent removed by a disease (dead) takes no status transitions.

The status transitions are counted once, in `get_hist_transitions()`; with several diseases, they come after the first disease's transitions, with the disease `""`, when every disease is asked for.

## Counting the transitions

`DataBase::get_hist_transitions()` returns how many agents took each declared transition, each day. `get_hist_virus_transitions()` splits the stage transitions by strain: infections under the strain caught, and the transitions out of a carrier stage under the strain carried. Over a disease's strains, they add up to `get_hist_transitions()`, except for an `external()` transition that also leaves stages without the virus: its firings from those stages have no strain. Seeding and imports are not transitions.

## See Also

- [Extending the Library with Models](extending-the-library-with-models.md) — building a model on a natural history, and several diseases.
- [Quarantine, Isolation, and Contact Tracing](quarantine-isolation-and-contact-tracing.md) — statuses, reactions and contact logs.
- [Combining Exposures](combining-exposures.md) — how the infection draw combines several contacts.
- [Data Collection and Analysis](data-collection-and-analysis.md) — the histories.
