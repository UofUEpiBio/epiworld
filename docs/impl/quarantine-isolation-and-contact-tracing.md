Quarantine, isolation and hospitalization are **statuses**: where an agent is, apart from the stage of its disease. An agent has one stage and one status, so a quarantined agent can be latent, or susceptible, or infectious, and its disease progresses the same way whatever its status.

## Statuses

`Statuses` declares them with the vocabulary of a natural history (`rate()`, `after()`, `external()`; no infections), and `Model::set_statuses()` gives them to a model. The first status is where agents start; a model without statuses has one, "Free".

```cpp
Statuses st;
int free        = st.add_status("Free");
int quarantined = st.add_status("Quarantined");
int isolated    = st.add_status("Isolated");

// Quarantined and isolated agents are out of every layer
st.set_contact(quarantined, 0.0);
st.set_contact(isolated, 0.0);

// Quarantine is fired by code (contact tracing, below) and lasts 14 days
int quarantine = st.external(free, quarantined, "Quarantined");
st.after(quarantined, free, Duration::fixed(14)).name("Quarantine ends");

// Infectious agents are detected at a daily rate and isolated for 7 days
// from their onset
st.rate(free, isolated, "Detection rate").name("Detected").when({INFECTED});
st.after(isolated, free, Duration::fixed(7).since(INFECTED)).name("Isolation ends");

model.set_statuses(st);
```

- **Contact weight.** A status (or a stage) with contact weight 0 takes its agents out of every layer: they are not met and do not transmit. In a mixing layer, their group's contacts go to the others.
- **Out of some layers only.** `layer->exclude_status("Quarantined")` takes the agents in that status out of that layer only, in both directions: a school layer can exclude the quarantined while the household layer keeps them. A mixing layer counts only the agents present, so its contact rate stays constant (within groups, the probability per pair does), and the days an agent was out are not days shared in `groupmates_of()`. The excluded statuses are set between runs; the status history already counts who was out.
- **Closing a layer.** `layer->close()` and `layer->reopen()`, e.g. from a global event, take the whole layer out (a school closure): from the next day on it samples no contacts, so it logs none and transmits nothing, and the closed days are not days shared in `groupmates_of()`. A closure made during a run lasts that run; one made between runs, every run. A layer weight of 0 is different: the contacts still happen (and are logged) but transmit nothing.
- **Order.** Global events run at the end of the day, in the order they were added, and each one's changes apply before the next runs.
- **`.when({stages})`** restricts a status transition to agents in those stages.
- **`.since(stage)`** counts a duration from the day the agent entered that stage, in the same infection, instead of the day it entered the status.

Within a day, stage changes apply first, then status changes, then reactions.

## Reactions

`Model::react(trigger, transition)` fires a transition on an agent, in the same step, when something happens to it; `Model::react(trigger, fun)` calls a function instead.

- `Trigger::entered(stage)`: the agent enters the stage.
- `Trigger::fired(transition)`: the transition is applied to the agent.
- `.was({statuses})`: only if the agent's status at the start of the day was one of these.

For example, the measles models isolate a quarantined agent at its rash onset (a reaction to entering Rash, `.was({QUARANTINED})`), and a reaction function can quarantine an agent's contacts when it is detected.

## Contact tracing

A layer keeps a log of its contacts once `ContactLayer::enable_tracing(max_contacts)` is called: for each agent, up to `max_contacts` of its contacts, with their dates. `get_tracing().get_contacts(agent)` returns them, so a reaction can fire the quarantine transition on the contacts of a detected agent made within a tracing window, in every layer whose log is on. `ModelSEIRQuarantine` and the measles models do this: they turn tracing on for each of their layers at reset.

## The included models

- `ModelSEIRQuarantine`: stages Susceptible, Exposed, Infected, Severe, Recovered; statuses Free, Quarantined, Isolated, Hospitalized.
- `ModelMeaslesMixing`, `ModelMeaslesMixingRiskQuarantine`, `ModelMeaslesSchool`: measles stages (Susceptible, Latent, Prodromal, Rash, Severe, Recovered) with the same statuses; post-exposure prophylaxis (`InterventionMeaslesPEP`) fires the transitions out of quarantine.

With several diseases, these models' detection, tracing and declared rules follow their own disease (the first); quarantine and isolation still take the agent out of every layer, so they stop the other diseases' contacts too. `ModelSEIRQuarantine` exempts from quarantine a contact holding a tool for its disease; the measles models exempt by tool name (`set_quarantine_exempt_tools()`).

Each model's diagram, drawn from its declarations (`model.draw_natural_history()`), shows both its stages and its statuses.

## See Also

- [Natural History and Statuses](natural-history-and-statuses.md) — rates, durations, `.when()`, `.since()` and `.of()` in detail.
- [Core Simulation Models](core-simulation-models.md) — `SEIRQuarantine` and its policy parameters.
- [Extending the Library with Models](extending-the-library-with-models.md) — declaring a natural history.
- [Events and Multi-Event Handling](events-and-multi-event-handling.md) — how the day's changes are applied.
