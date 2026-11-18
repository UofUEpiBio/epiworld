Changes to agents during a day (an agent moving to another stage or status, acquiring or losing a virus or a tool) are not applied when they are decided: they are queued as **events**, and `Model::events_run()` applies them. This way, every agent is updated against the same state of the model, and the changes of a day take effect together.

## When events are applied

Each step of `Model::run()`:

1. `update_state()` decides the day's changes and applies them: first the natural history, then the statuses, then the reactions to what changed (`Model::react()`).
2. `run_globalevents()` runs the global events; the changes they make (e.g., with `Model::fire()` or `agent.add_tool()`) are applied after them.
3. `next()` updates the layers (e.g., their rewiring), records the day in the database, and advances the date.
4. `mutate_virus()` lets viruses with a mutation function become new variants.

## What an agent's stage proposes

Each day, an agent's stage proposes at most one transition, tried in this order:

1. An infection from the day's contacts (agents in a stage that can be infected, not out of the layers).
2. The stage's rates, which compete: one draw picks one of them, with their probabilities, or none.
3. The stage's duration, if it has ended (and no rate was drawn).

Its status proposes at most one transition the same way (its rates, then its duration), judged on the stage at the start of the day.

## Several events for one agent

Events are applied in the order they were queued. An event from a declared transition applies only if the agent is still in one of the transition's sources: if an earlier event of the same pass moved the agent, a later event from its old stage (or status) is skipped. So two transitions that compete for an agent on the same day do not both apply, and the database counts the one that did. A stage transition and a status transition change different things, so both can apply on the same day.

## See Also

- [Extending the Library with Models](extending-the-library-with-models.md) — the run loop and the declarations.
- [Quarantine, Isolation, and Contact Tracing](quarantine-isolation-and-contact-tracing.md) — statuses and reactions.
- [Queueing System](queueing-system.md) — which agents are visited each step.
