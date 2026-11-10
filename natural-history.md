# Natural history and agent status

**Status: NH-1 to NH-4 merged, 2026-11-09** (Section 11); the decisions of
2026-11-09 (16-21) close the plan's open questions; several diseases per
model are the next phase, NH-5, with its own plan (decision 21). Supersedes PRs
#358 (plan), #359 (NH-1) and #360 (NH-2), which are closed without merging.
Their branches stay for reference.
This plan comes from a two-track review:
- one reviewer checked the earlier plan and its code;
- the other designed the API from scratch, without seeing that plan.

The maintainer then settled the differences (Section 3). Nothing in the
earlier branches is reused: implementation starts from `v1-layers`.

## 1. Motivation

Today an agent has one state, an index into the model's list of states
(`Model::add_state()`). A virus says which state an agent moves to when it
gains or loses the virus (`Virus::set_state()`). That one state mixes two
things:

- **the disease's natural history**: within-host progression (latent,
  prodromal, rash, recovered);
- **the agent's status**: free, quarantined, isolated or hospitalized. The
  status governs contacts and interventions.

Models that need both write the product by hand. The measles models register
12 combined states such as "Quarantined Latent", repeat the release rules in
11 update functions, and rebuild the disease stages with a helper
(`include/measles/stages.hpp`). `ModelSEIRQuarantine` has 10 states on the
same pattern. Each new status (say, "tested, awaiting result") would copy
every compatible disease state.

## 2. Precedent

- **OpenABM-Covid19**: separate disease `status`, `quarantined` and
  `hospital_state` fields, each with its own events. The severe course
  (hospitalised, critical) is part of disease progression.
- **FRED**: an agent is in one state of each "condition" (a disease, testing,
  isolation), and a change in one condition triggers changes in others.
- **Starsim**: disease modules own their state arrays. A disease *requests* a
  death, and the death is carried out once per agent.
- **Covasim**: severe and critical are disease stages with their own
  durations. Covasim v3 mixed disease and administrative flags; v4 is rebuilt
  on Starsim.
- **Theory**: the orthogonal regions of Harel statecharts describe a state as
  a tuple without building the product machine. Libkind et al. 2022 describe
  stratification as a typed product of a disease model and an intervention
  model.

## 3. Decisions (settled 2026-11-04)

1. **Two coordinates per agent.** The **stage** comes from the natural history
   (the disease). The **status** comes from the model. Internally the engine
   keeps one index for the pair, a *cell*; cells are never public.
2. **One way to declare a transition, on both axes.** Stages and statuses are
   both small graphs, built with the same vocabulary (Section 5.1). Code never
   moves an agent directly; it fires a declared transition.
3. **Each slot of a transition takes a number, a model parameter or a
   function.** This follows epiworldR's tool setters (`set_X`, `set_X_ptr`,
   `set_X_fun`). Functions may come from C++ factories (logit and similar) or
   from the user, in C++, R or Python. R functions are wrapped in a C++
   callable, as `globalevent_fun()` does today. A custom rule may only pick one
   of the node's declared transitions.
4. **Every parameter lives in the model.** Transitions, viruses, tools, agents
   and events refer to parameters by name; none holds a value. A number given
   directly becomes a model parameter. A variant changes a value by pointing a
   transition to another parameter.
5. **No fixed roles.** No start, end or death stages. A stage is a *carrier*
   (agents in it hold the virus) and may be *infectious* (implies carrier).
   "Susceptible" only means the stage has an infection transition out of it.
6. **Death is a stage** with contact weight 0 and no exits. The engine derives
   "removed" from those two facts; there is no death flag or death status.
7. **Within a day, the natural history goes first.** Stage changes apply
   first, then status rules, then reactions, which see the resulting status.
   Nothing is dropped or undone. Example: (Quarantined, Prodromal) →
   quarantine ends → (Free, Rash) → rash seen → (Isolated, Rash).
8. **Measles: rash is detected immediately in an agent who was quarantined at
   the start of the day**, even if the quarantine ended that day.
9. **Measles hospitalization is a Severe stage.** Rash competes daily with a
   move to Severe. Entering Severe stops the rash clock and admits the agent
   (status Hospitalized). The agent leaves Severe at rate 1/"Hospitalization
   period" and is discharged.
10. **Fractional contact weights**: on networks they scale the per-contact
    probability (no extra draw per tie); mixing layers thin contacts, as
    `SamplerMixing` does today. Traced network contacts are then the ties,
    which suits known contacts such as a household.
11. **One natural history per model in 1.0.0.** A second one is an error. The
    Superseded by decision 21: several diseases per model, one infection per
    agent, are NH-5, before Phase 4.
12. **Results**: bit-identical histories are not required. The statistical
    tests (R0 validation, rates, final sizes, stage durations) are the
    acceptance check.
13. **The virus carries its natural history** (settled 2026-11-05). A disease
    is built on the `Virus` itself (`flu.add_stage()`, `flu.rate()`, ...);
    `add_virus()` gives the model its stages. A strain copied from a virus, or
    made by mutation, keeps its natural history; its parameters can differ
    (`use_param()`). The viruses of a model share their stages (labels and
    flags) in 1.0.0, and an agent's transitions come from the virus it
    carries.
14. **One way to place things at the start of a run: `assign`** (settled
    2026-11-05). Viruses (and the stage each seeded agent starts in), tools,
    initial stages and the groups of mixing layers are all placed with the
    same objects (Section 5.7). `Seed`, the `distribute_*` functions,
    `set_distribution()`, the prevalence constructors of `Virus` and `Tool`,
    `initial_states()` and `grouping::` are removed.
15. **Histories keep status and stage apart** (settled 2026-11-05).
    `get_hist_total(date, status, stage, counts)` has one row per day,
    status and stage; models without statuses report the status "Free". The
    virus and tool histories gain the same two columns. There is no
    `get_hist_stage()` or `get_hist_status()`: a total by stage or by status
    is a group-by. The engine's index of the pair never leaves the engine:
    the database splits it in the one place that writes the rows, and no
    combined labels ("Quarantined Prodromal") are stored or built.

Settled 2026-11-09, after NH-4:

16. **`rule()` and the deadline calendar are deferred** (Section 13). No model
    needs either: every port was written with the declared kinds, and the
    engine derives each day's visits (Section 7.3) within the benchmark's 10%.
    Both can be added later without changing the API; the calendar joins the
    index-driven scheduler of issue #265.
17. **Transition matrices and agent states by margin** (built in NH-5, with
    the database's split by disease). The accessors that still return the
    engine's cells (`Model::get_agents_states()`,
    `DataBase::get_transition_probability()`,
    `DataBase::get_today_transition_matrix()`) are replaced by accessors that
    take a `Margin`: `pair` (status × stage, labeled by the pair, as stored),
    `stage` or `status` (the pair counts summed over the other axis, so a
    status change within a stage counts as staying in the stage matrix).
    Agents' states become `get_agents_stages()` and `get_agents_statuses()`.
    Transitions are recorded per strain.
18. **Who is exempt from measles quarantine** (open question 1): the holders
    of the tools a model lists by name, `{"MMR", "PEP MMR"}` by default
    (`set_quarantine_exempt_tools()`), whether or not the tool protects them.
    Documented MMR, routine or as post-exposure prophylaxis, is presumptive
    evidence of immunity; immunoglobulin is not (CDC). Holding any tool no
    longer exempts. Recovered agents are exempt by stage, as before.
    `ModelSEIRQuarantine` keeps "any tool": the rule is measles policy.
19. **PEP acts on the current episode** (open question 3): it may clear the
    infection ("PEP cleared"), and the tool it gives stays (MMR protects
    later; IG wanes). A milder course after IG is later (Section 13). PEP
    applies the day's events before its efficacy draw so that the tool it
    just gave counts; that stays.
20. **Rash onset stays the measles landmark** (open question 2): prodromal is
    not counted as symptom onset; the surveillance case definition starts at
    the rash. **Names** (open question 4): `NaturalHistory` and `Statuses`, as
    built.
21. **Several diseases per model, one infection per agent, in 1.0.0**
    (NH-5, before Phase 4; its plan is the next PR and is reviewed before any
    code). Settled so far:
    - Each agent has a stage in every disease and one status; at most one
      disease is in a carrier stage. A disease is a natural history; its
      strains share it. Co-infection is not a priority, but the rule that
      forbids it is one check.
    - One database, with its histories kept per disease. A long table
      returns every disease with a `disease` column; what is shaped by one
      disease's stages (a transition matrix, the agents' stages) needs the
      disease when the model has several.
    - A strain is made explicitly from a virus (`strain()`); a virus built
      from scratch is a new disease.
    - With several diseases, tools and stage references name their disease
      (an error otherwise), and infections are pulled, not pushed.
    - A removal in one disease moves the agent to a built-in "Removed
      (other disease)" stage of the others (the competing-risks state), which
      exists only when another disease can remove agents.

## 4. Concepts

| Concept | Owner | Responsibility |
|---|---|---|
| Natural history | virus | Stages and the transitions between them, built on the `Virus` (Section 5.1). |
| Stage | natural history | A node: `carrier`, `infectious`, a contact weight. |
| `Statuses` | model | Status nodes, their contact weight per layer, and the transitions between them. The default is one status, "Free". |
| Transition | either axis | A named, counted edge from a set of source nodes to a target (or a branch of targets), of one kind (Section 5.1). |
| Reaction | model | "When transition T fires, or stage X is entered, fire transition U" (or call a function). It links the two axes. |
| Virus | model | A disease: its natural history, name, id and parent, mutation, lineage, and parameter redirections (`use_param()`). |
| `Assign` | model | Who gets a virus, a tool, an initial stage or a group at the start of each run (Section 5.7). |
| Tool | model | Effects on susceptibility, transmission, recovery and death, read from model parameters. No states, no queue. |
| Model | -- | The population, layers, parameters, viruses (and their stages), statuses, reactions and global events. Runs the step. |

## 5. The API

### 5.1 Transitions

```cpp
// A value slot: a number, a parameter name, or a function.
//   Value<TSeq>(0.1)                 a number (becomes a model parameter)
//   Value<TSeq>("Recovery rate")     a model parameter, by name
//   Value<TSeq>::inverse("Days")     1 / parameter
//   Value<TSeq>(fun)                 double(const Agent&, const Model&)
//
// A duration slot (whole days >= 1, drawn on entry):
//   Duration::fixed("p"), Duration::lognormal("mean", "sd"),
//   Duration::gamma("mean", "sd"), Duration::geometric("mean"),
//   or a function int(const Agent&, Model&).
//   .since(Anchor::stage_entry(s)) counts from another day than the entry.

template<typename TSeq> class Chart {   // the base of NaturalHistory and Statuses
public:
    Transition& rate(Nodes from, int to, Value<TSeq> daily_probability);
    Transition& after(int from, int to, Duration<TSeq> d);     // <= 1 per node
    Transition& external(Nodes from, int to, std::string name); // fired by code
    Transition& rule(int from, std::vector<int> transitions, RuleFun<TSeq> f);
    // rule(): deferred (decision 16)
};

// Every transition can be refined:
//   .name("...")                         default: "Latent -> Prodromal"
//   .to({{A, Value(p)}, {B, Value(1-p)}}) a branch of targets
//   .effect(Effect::recovery | Effect::death)  which tool effect applies
//   .when(Nodes)                         a condition on the other axis

template<typename TSeq> class NaturalHistory : public Chart<TSeq> {
public:
    explicit NaturalHistory(std::string name);
    int add_stage(std::string name, unsigned flags = 0);  // stage::carrier, stage::infectious
    void set_contact(int stage, Value<TSeq> weight);       // default 1
    Transition& infection(Nodes from, int to, Value<TSeq> per_contact);
    //   .sampler(fun): a custom infection sampler (C++ only); default: the layers
};

template<typename TSeq> class Statuses : public Chart<TSeq> {
public:
    int add_status(std::string name);
    void set_contact(int status, Value<TSeq> weight, std::string layer = ""); // "" = all
};

// Virus<TSeq> carries the NaturalHistory builder: add_stage(), set_contact(),
// infection(), rate(), after(), external() (the methods above)

// On Model
void add_virus(Virus<TSeq> v, Assign<TSeq> a);  // its stages become the model's
void add_tool(Tool<TSeq> t, Assign<TSeq> a);
void assign_virus(size_t id, Assign<TSeq> a);   // re-place a built-in model's virus
void assign_tool(size_t id, Assign<TSeq> a);
void set_initial(Assign<TSeq> a);               // labels: stages without the virus
void set_statuses(Statuses<TSeq> s);
void react(Trigger on, int transition);        // Trigger::fired(t), Trigger::entered(stage)
void react(Trigger on, ReactFun<TSeq> f);      //   .was(statuses): start-of-day status
bool fire(int transition, Agent<TSeq>& a);     // no-op unless a is in a source
void seed(Agent<TSeq>& a, const Virus<TSeq>& v, int stage); // imports during a run

// On Virus
void use_param(std::string param, std::string replacement); // a variant's values

// On Agent (read only)
int get_stage() const;  int get_status() const;
const Infection<TSeq>& infection() const;   // Section 6.6
int status_since() const;  int status_until() const;
```

### 5.2 SIR (and its family)

```cpp
Virus<> flu("Flu");
int S = flu.add_stage("Susceptible");
int I = flu.add_stage("Infected", stage::infectious);
int R = flu.add_stage("Recovered");
flu.infection(S, I, "Transmission rate");
flu.rate(I, R, "Recovery rate").effect(Effect::recovery);

model.add_param(0.3, "Transmission rate");
model.add_param(0.1, "Recovery rate");
model.add_virus(flu, assign::share(0.01));        // into the infection's target, I
model.set_initial(assign::shares({{R, 0.2}}));    // 20% of the others immune
```

- **SIS**: `rate(I, S, ...)` instead of a Recovered stage.
- **Waning**: add `rate(R, S, "Waning rate")`.
- **SIRD**: add a stage `D` with `set_contact(D, 0)` and
  `rate(I, D, "Death rate").effect(Effect::death)`. The two rates out of `I`
  compete.
- **SEIR**: add a carrier stage `E`, `infection(S, E, ...)` and
  `after(E, I, Duration::geometric("Incubation days"))`.

### 5.3 Surveillance (a branch)

```cpp
nh.after(L, SYM, Duration::gamma("Latent period", "Latent period sd"))
  .to({{SYM, "Prob symptomatic"}, {ASYM, Value::complement("Prob symptomatic")}});
nh.after(SYM,  R, Duration::gamma("Infectious period", "Infectious period sd"));
nh.after(ASYM, R, Duration::gamma("Infectious period", "Infectious period sd"));
nh.rate(SYM, D, "Death rate").effect(Effect::death);   // competes with the deadline
```

### 5.4 Measles mixing

```cpp
Virus<> measles("Measles");                      // the virus and its natural history
S  = measles.add_stage("Susceptible");
L  = measles.add_stage("Latent",    stage::carrier);
P  = measles.add_stage("Prodromal", stage::infectious);
RS = measles.add_stage("Rash",      stage::infectious);
SV = measles.add_stage("Severe",    stage::carrier);
R  = measles.add_stage("Recovered");
measles.set_contact(RS, Value::complement("Rash reduction contact rate"));
measles.infection(S, L, "Transmission rate");
measles.after(L, P,  Duration::lognormal("Incubation period", "Incubation period sd"));
measles.after(P, RS, Duration::lognormal("Prodromal period", "Prodromal period sd"));
measles.rate (RS, SV, "Hospitalization rate");       // competes with the rash deadline
measles.after(RS, R, Duration::lognormal("Rash period", "Rash period sd"));
auto out_severe = measles.rate(SV, R, Value::inverse("Hospitalization period"));
auto pep_clear  = measles.external(L, R, "PEP clearance");

Statuses<> s;
FREE = s.add_status("Free");
Q    = s.add_status("Quarantined");  s.set_contact(Q,   0.0);
ISO  = s.add_status("Isolated");     s.set_contact(ISO, 0.0);
H    = s.add_status("Hospitalized"); s.set_contact(H,   0.0);
auto detect = s.rate(FREE, ISO, detection_rate).when({RS});   // a function: 1/"Days undetected", 0 if no isolation
s.after(ISO, FREE, Duration::fixed("Isolation period").since(Anchor::stage_entry(RS)));
auto traced      = s.external(FREE, Q,   "Traced");
auto traced_rash = s.external(FREE, ISO, "Traced with rash");
s.after(Q, FREE, Duration::fixed("Quarantine period"));
auto isolate   = s.external({FREE, Q}, ISO, "Rash in quarantine");
auto admit     = s.external({FREE, Q, ISO}, H, "Admitted");
auto discharge = s.external(H, FREE, "Discharged");

model.add_virus(measles, assign::share(0.001));
model.add_tool(mmr, assign::share(0.9));         // or assign::group_shares("school", ...)
model.set_statuses(std::move(s));
model.react(Trigger::entered(RS).was({Q}), isolate);   // detected at once (decision 8)
model.react(Trigger::entered(SV), admit);
model.react(Trigger::fired(out_severe), discharge);
model.react(Trigger::fired(detect), mark_for_tracing); // a function
model.add_globalevent(quarantine_process);
// quarantine_process: for each willing contact c of a flagged agent,
//   model.fire(c.get_stage() == RS ? traced_rash : traced, c);
```

- The 12 combined states become 6 stages × 4 statuses.
- "Quarantined Latent" is the cell (Latent, Quarantined), and the Latent
  timer runs through quarantine untouched.
- `stages.hpp`, `day_rash_onset` and `day_flagged` go away.
- `ModelMeaslesSchool` is the same model on a one-group layer.
- `ModelMeaslesMixingRiskQuarantine` gives `after(Q, FREE, ...)` a duration
  function that reads the agent's risk level.

### 5.5 SEIR with quarantine

This is the measles pattern without the prodromal stage:
- stages S, E, I, Severe and R;
- the same four statuses;
- detection while Infected, tracing, and isolation of a quarantined agent at
  onset;
- Severe and Hospitalized, as in measles.

### 5.6 The R wrapper

```r
flu <- virus("Flu")
add_stage(flu, "Susceptible")
add_stage(flu, "Infected", infectious = TRUE)
add_stage(flu, "Recovered")
add_infection(flu, "Susceptible", "Infected", rate = "Transmission rate")
add_rate(flu, "Infected", "Recovered", rate = "Recovery rate", effect = "recovery")
m <- Model(1000)
add_param(m, "Transmission rate", 0.3)
add_param(m, "Recovery rate", 0.1)
add_virus(m, flu, assign_share(0.01))
set_initial(m, assign_shares(Recovered = 0.2))
```

- A slot takes a number, a parameter name, or a function object. The
  function object is built by a C++ factory (`rate_fun_logit(vars, coefs, m)`,
  as `tool_fun_logit()` is today) or wraps an R function. The cost of an R
  function is one R call per evaluation, and it needs `nthreads = 1` in
  `run_multiple()`. Document both, as for `globalevent_fun()`.
- Nodes and transitions are named by their labels.
- Outputs: `get_hist_total()` (date, status, stage, counts) and
  `get_hist_transitions()` (decision 15).
- Python follows the same shape in snake_case. JS (embind) takes the
  declarative forms and the factories.

### 5.7 Placement: `assign`

Who gets what at the start of each run, for viruses, tools, initial stages
and the groups of mixing layers, is said with one vocabulary
(`include/epiworld/assign.hpp`). An `Assign` gives each eligible agent a label,
or none, drawn with the model's random numbers at each reset:

| Form | Labels |
|---|---|
| `assign::share(p)`, `assign::count(n)` | A random share (or number) of the eligible agents |
| `assign::shares({{l, p}, ...})`, `assign::counts({{l, n}, ...})` | Several labels, each its share (or number), drawn in order from those left |
| `assign::group_shares(layer, {p_g})`, `assign::group_counts(layer, {n_g})` | A share (or number) of each group of a mixing layer |
| `assign::agents(ids)` | The given agents |
| `assign::labels(v)` | `v[i]` for agent `i`; fixed (the same every run) |
| `assign::by(fun)` | `fun(agent, model)`; an R or Python function is cheap here (once per run) |

| Use | Label | Eligible |
|---|---|---|
| `add_virus(v, a)` | A carrier stage (default: the infection's target) | No virus, and a stage that can be infected |
| `add_tool(t, a)` | -- | Everyone |
| `set_initial(a)` | A stage without the virus | No virus, in the first stage |
| `Mixing::set_groups(a, n)` | A group | Everyone |

The order at each reset is fixed: the layers (so a placement can use their
groups), the viruses, the tools, the initial stages, then the global events'
set-up. A subset of agents to draw from (a school, a set of ids) is a group of
a mixing layer.

## 6. Semantics

### 6.1 The step (day t)

1. **Propose.** Each agent that needs a visit (Section 7.3) proposes at most
   one stage transition and at most one status transition. Both are judged
   against its stage and status at the start of the day.
   - Stage proposals, in order: infection by contact (pull, or push for the
     whole population at once), then the node's rates and deadline, then its
     rules.
   - Status proposals: the node's rates, deadline and rules, with `.when()`
     read against the start-of-day stage.
2. **Apply** (`events_run()`), in this order:
   1. stage changes;
   2. status changes;
   3. reactions, which see the resulting stage and status. A reaction's
      `.was()` reads the start-of-day status.
3. **Global events**, in order. Each applies its own `fire()` calls against
   the state left by the previous phase.
4. **Next**: layers update, the database records, and the day advances.
   Mutation runs.

`fire()` checks the transition's sources when it is applied. An agent that is
no longer in a source is skipped. That check replaces the earlier plan's
"stale event" rules, on both axes.

### 6.2 Competing transitions

- All rates out of a node are drawn together with `roulette()`: one or none
  fires.
- A node has at most one deadline (`after`). Rates are drawn every day, the
  deadline day included; the deadline fires only if no rate did.
- An infection transition, when the node has one, is drawn before the rates.
- A branch (`.to()`) picks its target with one more draw after the transition
  fires.

### 6.3 Seeding and initial conditions

- `add_virus(v, a)` seeds the virus (Section 5.7). The labels are carrier
  stages (`assign::shares({{E, 0.007}, {I, 0.003}})`); the forms without
  labels (`assign::share(0.01)`) use the infection's target. Agents are drawn
  among those whose stage can be infected.
- `set_initial(a)` places the agents left without the virus in stages (such
  as Recovered); NH-2 adds statuses the same way.
- Seeded agents enter on day 0 and draw their durations then. They count in
  day 0, and the transmission log records them with source −1.
- `Model::seed()` imports cases during a run through the same path.

### 6.4 Reinfection and episodes

- Entering a non-carrier stage releases the virus.
- An infection transition out of any non-carrier stage allows reinfection:
  SIS, waning, partial immunity. There can be several, each with its own
  target.
- Each infection starts a new episode, with a fresh `Infection` record.

### 6.5 Death

- A stage with contact weight 0 and no exits is a removal.
- The engine takes the agent out of every layer, stops visiting it, and drops
  its later status changes.
- Its stage history keeps "died of X".

### 6.6 What is stored per infection and per agent

- `Infection` holds:
  - the variant and the episode;
  - the infection day, the source and the layer;
  - the entry day of each stage in this episode;
  - the day the current stage's deadline falls.
- The record stays readable after clearance until the next infection.
  Release from isolation, timed from rash onset, depends on that.
- Status data: `status_since` and `status_until`.
- **Durations are drawn on entry and belong to the disease axis**, so status
  changes cannot touch them. An `external` transition into a timed stage
  draws its duration like any other entry.
- An anchored deadline that has already passed fires at the next update.

### 6.7 Tools and variants

- Susceptibility and transmission effects apply to every infection
  transition.
- Recovery and death effects apply to the transitions tagged with
  `.effect()`.
- All of them read model parameters.
- A variant redirects parameters with `use_param()`. Variants are
  identified by id and parent, not by sequence (V-1 in `contact-layers.md`).
- Tools cannot shorten an `after` duration in 1.0.0. That is a later
  addition (a per-agent factor read on entry).

## 7. The engine

### 7.1 Cells

- `Agent::state` is replaced by a stage and a status.
- The engine keeps `cell = stage * n_status + status` (24 cells in measles).
  The agents-by-state index, the carrier counts and the degree sums move from
  states to cells unchanged, at O(#cells) per step.
- With one status, the cells are the stages.

### 7.2 The cell table

Built once in `run()`. One row per cell:
- **pushable**: the stage has an infection transition, and the weight is
  above 0;
- **source**: the stage is infectious, and the weight is above 0;
- the contact weight per layer (stage weight × status weight);
- whether the cell needs a visit.

This replaces `set_infectious_states()`, `set_excluded_states()` and the RTTI
check on `default_update_susceptible` in `transmission_prepare()`. On mixing
layers the sampler draws contacts in proportion to the weight. On networks
the weight scales the per-contact probability.

### 7.3 Visits and the queue

The engine derives which agents need a visit; `set_queue` and the queue
arguments go away.
- **Rates and rules** (either axis): visited through the agents-by-cell index.
- **Infection**: by pull, or by push from the sources. Push applies only with
  the default sampler.
- **Deadlines** (deferred, decision 16: today each visited agent's deadline
  is checked, and agents with a deadline are always visited): in a
  calendar, one bucket per due day, holding
  `(agent, axis, epoch)`. Stale entries are skipped by epoch. A quarantined,
  unqueued susceptible is released without being visited, at O(1) per entry.

The same derivation serves pull and push, so neither path can miss an agent
the other would visit. That is the class of bug found in #360.

### 7.4 Events

- Each event carries a transition id.
- There are three short queues (stage, status, reactions), applied in the
  order of Section 6.1.
- The undo logic in the transition matrix goes away.

### 7.5 Database

- **Daily counts per cell**, written as rows with a status and a stage
  column (decision 15). Stage and status totals are group-bys.
- **Daily counts per declared transition.** These replace the state × state
  matrix, and the stage and status transition matrices are derived from
  them. For example, "Admitted" counts hospitalizations.
- Per-variant counts by stage.
- The transmission log is unchanged.
- Memory per day: `n_cell + n_transition` integers. In measles that is about
  40, against 156 for today's 12 states.

### 7.6 Memory per agent

- The stage, the status, two deadlines, `status_since`, and the `Infection`
  record: about 40 bytes plus 4 bytes per stage.
- Measles keeps similar vectors today, as model members.

## 8. Every model, mapped

| Model | Stages / statuses | Needs |
|---|---|---|
| SIR, SIS, SIRD, SISD, SEIR, SEIRD | 3-5 / 1 | rates, geometric deadline, `.effect()` |
| SIRLogit | 3 / 1 | a logit infection sampler and a logit recovery rate (C++ factories) |
| DiffNet | 2 / 1 | an exposure-threshold sampler; Adopter is a carrier with no exit |
| Surveillance | 6 / 1 | gamma deadlines, a branch, a rate with a deadline |
| SEIRQuarantine | 5 / 4 | Section 5.5 |
| Measles mixing, school, risk quarantine | 6 / 4 | Section 5.4 |
| `InterventionMeaslesPEP` | -- | Eligibility is a set of stages and statuses. When PEP works it fires declared transitions (`pep_clear`, a status release); efficacy is computed from the tool, with no nested `events_run()` |
| Immunoglobulin tool | -- | unchanged (a tool) |
| The Bubbles example (3d-3b) | 3-5 / 1 | layers only |

## 9. What is removed

- **Model:**
  - `add_state`, `set_state_function`, `state_of`, `get_states`,
    `print_state_codes`;
  - `set_infectious_states`, `set_excluded_states`, `initial_states()`;
  - `epimodels::create_init_function_*`;
  - `set_natural_history()` (the virus carries it).
- **Placement** (decision 14): `Seed`, `distribute_virus_*`,
  `distribute_tool_*`, `Virus::set_distribution()`,
  `Tool::set_distribution()`, the prevalence constructors
  `Virus(name, prevalence, as_proportion)` and `Tool(...)`, `grouping::random()`,
  and `Mixing::set_groups(vector, n)` / `set_groups(Grouping)` (now
  `set_groups(Assign, n)`). Done in NH-1.
- **Update helpers:** `UpdateFun`, `default_update_susceptible`,
  `default_update_exposed`, `new_state_update_transition`.
- **Virus:** `set_state/get_state/set_queue`, `set_prob_infecting/recovery/death`,
  `set_incubation(_fun)`, `set_post_recovery`, `set_post_immunity`. Their
  values become model parameters that the transitions name.
- **Tool:** `set_state/set_queue`. Effects read parameters.
- **Agent:**
  - `change_state`, `get_state(_prev)`;
  - the state and queue arguments of `set_virus`, `rm_virus`, `add_tool` and
    `rm_tool`;
  - the public `rm_virus`.
- **Measles:** `include/measles/stages.hpp` (`MeaslesStages`) and the state
  arguments of `SamplerMixing::configure`.

## 10. Measles policy

These are separate commits with their own tests:
- **The Severe stage** (decision 9). Done in NH-2.
- **The quarantine exemption.** Done after NH-4 (decision 18): by tool name,
  `{"MMR", "PEP MMR"}` by default; before, an agent holding any tool was
  exempt (`get_n_tools() != 0`).
- **Prodromal and symptom onset.** Settled with no change (decision 20).
- **What PEP acts on.** Settled with no change (decision 19). The CDC manual
  describes both: MMR within 72 hours can prevent illness, and IG within 6
  days can prevent or modify it. A modified course would be a branch to a
  milder stage (Section 13).

## 11. Phases

The work is on the branch `nh-1-engine`, which also carries this plan, and
merges into `v1-layers`. Between NH-1 and NH-2, the old state API still exists
for the multi-status models. NH-2 removes it, before any release. Each phase
updates the runbook table in `contact-layers.md` (Section 11.6) and reruns
the benchmark (Section 11.7 there).

Status (2026-11-09): NH-1 and NH-2 merged (#363), then V-1 (#364), V-2
(#365), NH-3 (#366, with the benchmark follow-ups #367, #368 and #369) and
NH-4 (epiworldR #231, epiworldpy #44, epiworldjs #30, measles #72). What
was left is settled by decisions 16-21:
- **NH-close** (one epiworld PR): the decisions, the quarantine exemption
  (decision 18), and the runbook in `contact-layers.md`.
- **NH-5** (decision 21): several diseases per model, the database kept per
  disease, and the accessors by margin (decision 17). Its plan is the next
  PR; then the engine, the database, and the wrapper ports.
- Deferred: `rule()` and the calendar (decision 16).

- **NH-1: the engine and the single-status models.**
  - Cells, the cell table, the natural history on the virus, the `Chart`
    vocabulary and value slots, the calendar, the derived queue, and the
    database (counts per cell and per transition).
  - `assign` for every placement (decision 14).
  - Ports SIR, SIS, SIRD, SISD, SEIR, SEIRD, SIRLogit, DiffNet and
    Surveillance, with their tests and examples.
- **NH-2: statuses, reactions and the old API removed.**
  - `Statuses`, `fire()`, reactions and contact weights.
  - Ports SEIRQuarantine, the three measles models and PEP.
  - Removes everything in Section 9 and rewrites the remaining tests and
    examples.
  - Includes the measles policy commits of Section 10.
- **Between NH-2 and NH-3: variants by id (V-1) and no `TSeq` template
  (V-2)**, as in `contact-layers.md` ("Variants by id, no sequence
  template"), so NH-3 documents, and NH-4 ports, the final API once.
- **NH-3: docs and runners.** `docs/impl`, the README, model diagrams drawn
  from the declarations, and the epiworld-benchmark runners.
- **NH-4: wrappers.** epiworldR, epiworldpy, epiworldjs and measles, as in
  `contact-layers.md` Section 11.4.

**Fit with `contact-layers.md`:**
- Phase 4's per-agent exclusion flags become status contact weights per
  layer.
- The measles consolidation's "Rash agents get a fraction r of their
  contacts" becomes the Rash stage weight.
- So NH-2 should land before Phase 4.

## 12. Testing

Following `AGENTS.md`: a few end-to-end files, one `EPIWORLD_TEST_CASE` each,
on fixed (drawn-once) networks. Also build with `-DEPI_DEBUG`, since
`make test` skips those checks.
- **Statistical**: the existing R0, rate and final-size tests still pass for
  every model.
- **Measles timing**:
  - With fixed durations, quarantined and free agents have the same latent
    and prodromal periods.
  - (Quarantined, Prodromal) whose quarantine ends on its rash-onset day ends
    the day (Isolated, Rash).
  - Severe stops the rash clock.
  - Isolation ends on the right day after rash onset.
- **Bookkeeping**:
  - Cell counts add up to the population every day.
  - Each stage's change from one day to the next equals its transitions in
    minus its transitions out.
  - Push, pull and automatic give the same distributions in a quarantine
    model (the #360 bug).
  - The calendar releases agents that are never visited.

## 13. Later, out of scope for 1.0.0

- **`rule()` and the deadline calendar** (decision 16; the calendar with
  issue #265).
- **A milder course after PEP** (decision 19): a branch from the exposed
  stages to a milder stage when IG does not clear the infection.
- **Co-infection** (not a priority): a stage per history per agent, a virus
  slot per history, and a rule for competing exposures. The API gains a
  history argument on `get_stage()` and `infection()`. Cells stay private, so
  their encoding can change.
- **An immunity record** per agent and pathogen (EMOD's `Susceptibility`),
  for cross-protection and immune amnesia.
- **Interventions that add statuses and reactions to a model**, so that
  `ModelSEIRQuarantine` could become `ModelSEIR` plus a quarantine
  intervention.
- **Tool factors on durations**, and hazards that depend on the days spent in
  a stage.
- **The virus registry** (a shared definition per variant instead of a clone
  per infection), as a separate issue, gated by a benchmark.

## 14. Open questions

Questions 1 to 4 of this plan were settled on 2026-11-09 (decisions 18, 20,
19 and 20). Several diseases per model (decision 21) get their own plan.

## 15. References

- Hinch et al. 2021. OpenABM-Covid19. *PLoS Comput Biol*.
  doi:10.1371/journal.pcbi.1009146
- Grefenstette et al. 2013. FRED. *BMC Public Health*.
  doi:10.1186/1471-2458-13-940
- Starsim: https://github.com/starsimhub/starsim
- Kerr et al. 2021. Covasim. *PLoS Comput Biol*.
  doi:10.1371/journal.pcbi.1009149
- Bershteyn et al. 2018. EMOD. *Pathog Dis*. doi:10.1093/femspd/fty059
- Harel 1987. Statecharts. *Sci Comput Program* 8:231.
  doi:10.1016/0167-6423(87)90035-9
- Libkind et al. 2022. An algebraic framework for structured epidemic
  modelling. *Phil Trans R Soc A* 380:20210309. doi:10.1098/rsta.2021.0309
- CDC. Manual for the Surveillance of Vaccine-Preventable Diseases, Chapter 7:
  Measles.
  https://www.cdc.gov/surv-manual/php/table-of-contents/chapter-7-measles.html
