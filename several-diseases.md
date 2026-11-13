# Several diseases per model (NH-5)

**Status: plan, 2026-11-10, for review before any code.** It carries out
decision 21 of `natural-history.md`, whose vocabulary (stage, status,
transition, reaction, cell) it uses. It comes before Phase 4 of
`contact-layers.md` and is part of 1.0.0.

## 1. Goal

A model can hold several diseases, say measles and influenza, each with its
own natural history, running on the same population, layers and statuses.

- **In scope:** several diseases per model; **at most one infection per agent
  at a time**; outputs by disease; the accessors by margin (decision 17 of
  `natural-history.md`).
- **Not in scope:** co-infection (several infections in one agent). The design
  must not stand in its way: the rule that forbids it is one check (Section
  4.3).
- **A model with one disease behaves, costs and reads as it does today**,
  except for one new column, `disease`, in the output tables.

## 2. Decisions (settled 2026-11-09 and 2026-11-10)

1. **A disease is a natural history.** Its strains share it. A model has one
   or more diseases.
2. **Every agent has a stage in every disease, and one status.** An agent can
   be Recovered from influenza and Susceptible to measles. At most one of its
   diseases is in a carrier stage.
3. **One database.** Its histories are kept per disease; what does not depend
   on a disease (the registries of viruses and tools, the statuses) is kept
   once. There is one way to reach it, `Model::get_db()`.
4. **Leaving the disease out returns every disease**, never "the first one".
   A table that is one row per something gains a `disease` column. What is
   shaped by one disease's stages (a transition matrix, the agents' stages)
   needs the disease when the model has several, and is an error without it.
5. **A strain is made explicitly** (`Virus::strain(name)`); a virus built from
   scratch is a new disease. The structure of two viruses is never compared
   to guess which one was meant.
6. **With several diseases, what refers to a stage names its disease**, and a
   tool names the diseases it acts on. Leaving either out is an error at the
   start of the run that says how to fix it.
   **Tools target whole diseases** (settled 2026-11-10): a tool acts on every
   strain of the diseases it targets. This replaces the targets by virus
   lineage. Strains that need their own tools are modeled as another
   disease.
7. **With several diseases, infections are pulled, not pushed.** A model with
   one disease keeps both.
8. **A removal in one disease shows in the others** as a built-in stage,
   "Removed (other disease)": the competing-risks state of a disease-specific
   table. It exists only in the diseases of a model where another disease
   can remove agents.
9. **Code cannot give an agent a second infection.** `fire()` of a transition
   into a carrier stage returns `false` when the agent carries another
   disease.

## 3. The API

```cpp
// Two diseases, each built on its virus (as today)
Virus flu("Influenza");
flu.add_stage("Susceptible");
flu.add_stage("Infected", stage::infectious);
flu.add_stage("Recovered");
flu.infection(0, 1, "Flu transmission rate");
flu.rate(1, 2, "Flu recovery rate").effect(Effect::recovery);

Virus measles("Measles");
// ... its own stages and transitions

Model model(10000);
model.add_layer(layer::network("contacts", graph::smallworld(10, 0.1)));
model.add_virus(flu, assign::share(0.01));       // disease 0, "Influenza"
model.add_virus(measles, assign::count(5));      // disease 1, "Measles"

// A strain belongs to its virus's disease
Virus flu_b = flu.strain("Influenza B");
flu_b.use_param("Flu transmission rate", "Flu B transmission rate");
model.add_virus(flu_b, assign::count(10));       // disease 0 again

// Asking by disease
const int d = model.disease_id("Measles");
model.get_stage(agent, d);
model.get_stages(d);                              // its labels
```

| What | With one disease | With several |
|---|---|---|
| `Model::add_virus(v, a)` | The first virus defines the disease. | A virus from `strain()` (or a mutation) joins its disease; any other virus is a new disease. Two diseases with the same name: an error. |
| `Virus::strain(name)` | New: a copy that shares the natural history. It replaces "copy the virus and rename it". | Same. |
| `Model::disease_id(name)`, `get_n_diseases()`, `get_diseases()` | New. | Same. |
| `Model::get_stage(agent)`, `get_stages()`, `get_stage_entered(id, stage)`, `get_stage_until(id)`, `get_episode(id)`, `set_initial(a)` | Unchanged. | Take the disease (`get_stage(agent, d)`, `set_initial(a, d)`); an error without it. |
| `Trigger::entered(stage)`, `TransitionRef::when(stages)`, `Duration::since(stage)` (the three places where the statuses and the reactions name a stage) | Unchanged. | `.of(disease)`, by name; an error at the start of the run without it. |
| `Model::transition_id(name)` | Unchanged. | Takes the disease when two diseases have a transition of that name. Status transitions never need it. |
| `Model::fire(transition, agent)` | Unchanged. | `false` if it would give the agent a second infection (decision 9). |
| `Agent::get_virus()` | Unchanged: the agent's one infection, or none. | Same. |
| `Tool::add_target(disease)` / `set_targets(diseases)`, by name | Replaces the targets by lineage (`add_target(lineage_id)`, `add_target(virus)`). Optional: a tool without targets acts on the model's disease. | Required (decision 6): a measles vaccine must not protect against influenza unless it says so. |
| `Model::record_hospitalization(agent)` | Unchanged. | Recorded under the disease of the virus the agent carries. |

**The included models name their own disease** in their statuses and
reactions (`.of("Measles")`), so a user can add a second disease to
`ModelMeaslesMixing` or `ModelSEIRQuarantine` without breaking their rules.

**Cross-disease effects need nothing new.** A rate given as a function can
read the agent's stage in another disease, and a reaction can give a tool:
"on entering Recovered of A, give tool T, which targets B" is partial
cross-immunity.

## 4. Semantics

### 4.1 The step

Unchanged in order (decision 7 of `natural-history.md`): stages, then the
status, then reactions. An agent is visited at most once a day; the visit
proposes one transition in **each** of its diseases (an agent infected with
A can lose its immunity to B the same day), then one for the status.

### 4.2 Infection: one draw across diseases

An agent without an infection can be infected by a contact's virus only if
its stage in **that virus's disease** has an infection transition, and that
disease's cell is not excluded.

- **The default sampler** gathers the exposures from every eligible contact,
  whatever its disease, and one draw picks at most one virus: the draw that
  already picks among strains. An agent recovered from A ignores its contacts
  with A and can be infected by those with B. Two infections are never
  proposed on the same day.
- **A custom sampler** (the measles models') returns only strains of its own
  disease; the engine checks it. When an agent can be infected by diseases on
  both paths, the diseases are tried in a random order that day, and the
  first infection ends the trial.
- **Pull only** (decision 7). An agent pushed to for A and visited for
  another reason would otherwise be exposed twice. Asking for push with
  several diseases is an error; `auto` chooses pull.

### 4.3 One infection per agent

It is the rule the engine already has: the agent has one virus slot, and the
infection step runs only when it is empty. It is enforced in three places,
and nowhere else:
- the infection step (the slot must be empty at the start of the day);
- `fire()` and seeding (decision 9; a placement skips agents that already
  carry a virus, as it skips the ineligible ones today);
- the events: an infection event finding the slot taken is skipped, like
  any stale event.

Lifting it later (co-infection) means a slot per disease and removing these
checks.

### 4.4 Removal

A stage with contact weight 0 and no exits removes the agent (decision 6 of
`natural-history.md`). With several diseases, the agent also moves, in every
other disease, to "Removed (other disease)" (contact weight 0, no exits, not
a carrier). No reaction fires for that move. Each disease's counts keep
adding up to the population, and no dead agent is counted as susceptible.

### 4.5 Contact weights and the queue

- An agent's contact weight on a layer is its status's weight times its
  stage's weight in each disease.
- The queue counts, per agent, the reasons to visit it, and each disease
  adds its own: a carrier queues its contacts, and a stage with a drawn exit
  queues its agent. A carrier of A also queues contacts that only B could
  infect; that visit finds no eligible exposure and costs a little.
- With the queue off (the measles models), agents are iterated once, not once
  per disease.

## 5. The engine

Each disease gets a **track**, which is today's machinery for one natural
history: the cell of each agent (its stage in that disease × its status),
the agents-by-cell index, the per-agent records (`nh_row`, `nh_rows`,
`nh_until`), and its part of the database. The status is stored once and is
part of every track's cell, so a status change moves the agent in each
track.

- `Model::nh` (about 130 references) becomes the list of tracks.
- Events carry their track.
- With one track, the code path is today's.

## 6. The database and the outputs

| Kept once | Kept per disease |
|---|---|
| The viruses (name, origin, parent, and now their disease) and the tools | Counts by status × stage, today and by day |
| Counts by status, and the status transitions | Transitions, by strain |
| Run settings, `n_founders`, user data | Virus and tool histories by status × stage (a tool's, only for the diseases it targets) |
| | Transmissions, reproductive numbers, generation times, active cases, hospitalizations |

- **Tables** (`get_hist_total()`, `get_hist_virus()`, `get_hist_tool()`,
  `get_hist_transitions()`, `get_hist_transition_matrix()`,
  `get_transmissions()`, the reproductive number, the generation time, the
  active cases, the hospitalizations, `get_today_*`): an optional disease
  argument, and a `disease` column (the name) in every case, with one disease
  too. The savers and `SaverMemory` write the same tables: one file per
  output, not per disease.
- **By margin** (decision 17 of `natural-history.md`):
  `get_transition_probability(Margin, disease, strain)` returns a
  `TransitionMatrix` (the labels of the status and of the stage, apart, and
  the values), for `Margin::pair`, `stage` or `status`, over every strain or
  one. `Model::get_agents_stages(disease)` and `get_agents_statuses()`
  replace `get_agents_states()`. The status margin and the agents' statuses
  need no disease.
- `get_today_total_stage(name)` needs the disease when two diseases have a
  stage of that name.

## 7. The wrappers

One database object, so a port is "an argument and a column":
- **Tool targets by disease** in epiworldR and epiworldpy (`add_target()` /
  `set_targets()` take disease names); the forms by lineage become defunct.
- **epiworldR:** the output functions and the `run_multiple()` savers gain
  `disease = NULL` and the `disease` column; plots facet by disease when
  there are several; `strain(virus, name)`; `get_diseases(model)`; the
  references to stages take `of =`; `get_transition_probability(model,
  margin = "pair")`; `get_agents_states()` becomes defunct.
- **epiworldpy:** the same, in its idiom.
- **epiworldjs, measles, epiworldRShiny:** one disease each; they take the
  new column and nothing else.

## 8. Phases

Each phase is one epiworld PR into `v1-layers`, followed by the benchmark
rerun and the wrapper ports, as in `contact-layers.md` (Section 11).

- **NH-5a: tracks, with one disease.** The engine's single natural history
  becomes a list of tracks of length one. No public change; a second disease
  is still an error. **Gate:** histories identical to the previous commit
  for every model, and the benchmark within its 10% rule.
  Done as: a `Model::Track` holds a disease's natural history and its
  per-agent records (deadlines, stage-entry days, episodes), its queue modes
  and its initial placement; the step proposes a stage transition in each
  track, then the status. The track's cell is still the agent's `state`, and
  the agents-by-cell index, the push masks and the database counts stay as
  they are: they serve the first disease, since push is for one disease
  only. Each further track's per-agent cell, counts and event tagging come
  with NH-5b, where a second disease tests them.
- **NH-5b: several diseases**, in two PRs:
  - **NH-5b-1: the outputs by disease, with one disease.** The disease
    argument and column on every function that returns rows, the `disease`
    column first in every output table and file but `tool_info`, the
    accessors by margin (`Margin`, `TransitionMatrix`,
    `get_agents_stages()`, `get_agents_statuses()`) and the model's
    diseases (`get_n_diseases()`, `get_diseases()`, `disease_id()`).
    Removed: `get_agents_states()` and `get_today_transition_matrix()`; the
    array form of `get_transmissions()` became private. Identical numbers.
    Transitions by strain are left for NH-5b-2 (they need new recording).
  - **NH-5b-2: several diseases in the engine.** `strain()` and the
    disease of a virus; a record per disease; the sampling across diseases;
    the rules of Sections 4.3 to 4.5; the explicit references (`.of()`,
    tool and layer targets by disease); transitions by strain. The
    benchmark gains a two-disease scenario.
- **NH-5c: the wrapper ports**, in the usual order.

## 9. Tests

A few end-to-end files, on fixed networks:
1. Two SIR diseases: agents recovered from A are infected by B; no agent
   ever carries two infections; each disease's counts add up to the
   population every day; the `disease` column splits the transmissions.
2. A model with one disease and the same model built as "the first of two,
   the second never seeded" give the same histories for the first disease.
3. A death in A is "Removed (other disease)" in B; a stage reference without
   its disease, a tool without targets and a forced push are rejected with
   their messages; `fire()` refuses a second infection.

## 10. Limits (to state in the docs)

- A long infection blocks the others: a chronic carrier never catches
  anything else. Fine for acute diseases.
- One status axis: two diseases whose policies both quarantine share the
  statuses, so they are designed together.
- Two diseases that read a parameter of the same name share it.

## 11. Questions settled in review (2026-11-10)

1. **Tool targets: by disease, as a whole** (decision 6). The targets by
   lineage go; strains are rarely used, and those that differ enough (the
   COVID-19 variants) can be another disease.
2. **The `disease` column is always present**, with one disease too:
   consistency first.
3. **Diseases with custom samplers are tried in a random order** each day.
4. **The name** is "Removed (other disease)".

5. **Layers' virus targets** (`add_virus_target(layer, virus)`) go by
   disease too, as the tools' (settled 2026-11-11, by default after no
   objection).
