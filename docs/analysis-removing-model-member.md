# Analysis: Removing the `model` Member Pointer from Core Classes

## Executive Summary

This analysis evaluates the feasibility and cost of removing the stored `Model<TSeq> *`
pointer from `Agent`, `DataBase`, `Queue`, and `UserData` classes. The conclusion is
that **the `model` pointer can be removed from `Agent`** — the highest-impact class —
with moderate effort, and there would be significant safety benefits. Removing it from
`DataBase`, `Queue`, and `UserData` is feasible but provides less benefit since those
classes have a clear ownership relationship with `Model`.

> **Note:** `Virus`, `Tool`, and `Entity` do **not** store a `model` pointer — they
> already follow the pattern of receiving `Model*` as a function parameter. Only `Virus`
> and `Tool` store an `Agent*` pointer.

---

## 1. Current State: Who Stores a `model` Pointer?

| Class | Has `model` member? | Declared In | Used In |
|-------|:------------------:|-------------|---------|
| **Agent** | ✅ | `agent-bones.hpp:83` | `agent-meat.hpp` (14 access sites) |
| **DataBase** | ✅ | `database-bones.hpp:42` | `database-meat.hpp` (60+ access sites) |
| **Queue** | ✅ | `queue-bones.hpp:25` | `queue-bones.hpp:109` (1 access site) |
| **UserData** | ✅ | `userdata-bones.hpp:22` | `userdata-meat.hpp` (3 access sites) |
| Virus | ❌ | — | Receives `Model*` as function param |
| Tool | ❌ | — | Receives `Model*` as function param |
| Entity | ❌ | — | Receives `Model*` as function param |

---

## 2. Why the `model` Pointer Is Problematic

### 2.1 Stale Pointer Bugs (the current PR's motivation)

When a `Model` is copied (e.g., for `run_multiple()`), all `Agent` objects in
`population` and `population_backup` are shallow-copied. Their `model` pointers still
reference the **source** model. This creates:

- **Dangling pointers** when the source model is destroyed before the copy
- **Data races** when parallel threads share the source's state through stale pointers
- **Container-overflow** ASAN errors (the bug that motivated this PR)

Every copy/move constructor and `reset()` path must manually fixup `model` pointers
for `Agent`, `DataBase`, `Queue`, and `UserData`. Currently:

```
model-meat.hpp — 17 separate lines that set `.model = this`
```

Each is a potential bug if omitted (as was the case with `population_backup`).

### 2.2 Maintenance Burden

Any new feature that copies or moves a model must remember to patch all `model`
pointers. This is a violation of the [Rule of Zero](https://en.cppreference.com/w/cpp/language/rule_of_three)
— the compiler-generated copy/move would work correctly if there were no raw pointers
to fix up.

### 2.3 Unclear Ownership

The `Agent::model` pointer creates a back-reference from the **owned** object
(`Agent`) to the **owner** (`Model`). This is a design smell — an agent should not
independently "know" about its model context when that context is always available via
the call chain.

---

## 3. Where `Agent::model` Is Actually Used

The `Agent::model` member is accessed (not via function parameter) in exactly **6 functional areas**:

### 3.1 `swap_neighbors()` — access `model->population` and `model->directed`
```cpp
// agent-meat.hpp:591-602
auto & pop = model->population;
...
if (!model->directed) { ... }
```
**Alternative:** Add `Model<TSeq> &` parameter.

### 3.2 `get_neighbors()` — access `model->population`
```cpp
// agent-meat.hpp:625
res[i] = &model->population[(*neighbors)[i]];
```
**Alternative:** Add `Model<TSeq> &` parameter.

### 3.3 `print()` — access `model->states_labels` and `model->get_agents_data_ncols()`
```cpp
// agent-meat.hpp:771, 783, 789
model->states_labels[state].c_str()
...
size_t nfeats = model->get_agents_data_ncols();
```
**Alternative:** Add `const Model<TSeq> &` parameter.

### 3.4 `operator()(j)` / `operator[](j)` — access `model->agents_data` and `model->size()`
```cpp
// agent-meat.hpp:825-852
return *(model->agents_data + j * model->size() + id);
```
**Alternative:** Add `const Model<TSeq> &` parameter; or better, provide a
`Model::get_agent_data(agent_id, col)` accessor so agents don't reach into model internals.

### 3.5 Copy/move constructors — propagate `model` pointer
```cpp
// agent-meat.hpp:17, 56, 100
model(p.model)
model = other_agent.model;
```
**Alternative:** These become unnecessary — the pointer is removed entirely.

### 3.6 `AgentsSample` — reads `model->population`, `model->runif()`, `model->size()`
```cpp
// agentssample-bones.hpp:236, 255, 262, 339-453
```
`AgentsSample` already receives `Model*` as a constructor parameter. It could use that
instead of `agent.model`.

---

## 4. Where `DataBase::model` Is Actually Used

`DataBase::model` is accessed in **60+ lines** in `database-meat.hpp`. Virtually every
method in `DataBase` uses `model->` to access:

- `model->nstates` (state count)
- `model->today()` (current date)
- `model->size()` (population size)
- `model->population` (agent vector)
- `model->states_labels` (state names)
- `model->get_ndays()` (number of days)

### Assessment

DataBase is always a **member** of Model (`Model::db`). It has exactly one owner, and
the ownership is clear. The `model` pointer is set once during construction and only
needs fixup during Model copy/move. Because DataBase is a private member of Model (not
copied independently), the risk is lower than for Agent.

**Option A — Keep it:** The pointer is safe as long as copy/move fix it up (currently
done). The cost of removing it is high (60+ sites).

**Option B — Replace with reference:** Change to `Model<TSeq> & model` (set in
constructor, never reassigned). This prevents null-pointer bugs but makes DataBase
non-copy-assignable (which is fine — it's owned by Model).

---

## 5. Where `Queue::model` and `UserData::model` Are Used

| Class | Access Sites | Notes |
|-------|:----------:|-------|
| `Queue` | 1 | Only `active.resize(model->size(), 0)` in `reset()` |
| `UserData` | 3 | Only `model->today()` for timestamping data |

Both are members of Model (or DataBase). Same assessment as DataBase — the pointer is
safe if copy/move handle it, but could be replaced with a reference for additional
safety.

---

## 6. Impact of Removing `Agent::model`

### 6.1 Functions That Need a New `Model&` / `Model*` Parameter

| Function | File:Line | Change |
|----------|-----------|--------|
| `swap_neighbors()` | `agent-meat.hpp:563` | Add `Model<TSeq> &` param |
| `get_neighbors()` | `agent-meat.hpp:621` | Add `Model<TSeq> &` param |
| `print()` | `agent-meat.hpp:761` | Add `const Model<TSeq> &` param |
| `operator()(j)` | `agent-meat.hpp:822` | Add `const Model<TSeq> &` param |
| `operator[](j)` | `agent-meat.hpp:833` | Add `const Model<TSeq> &` param |
| `operator()(j) const` | `agent-meat.hpp:839` | Add `const Model<TSeq> &` param |
| `operator[](j) const` | `agent-meat.hpp:850` | Add `const Model<TSeq> &` param |

**Total: 7 function signatures** need to change in `Agent`.

### 6.2 Call Sites That Need Updating

Every caller of the above 7 functions must be updated. The primary callers are:

- `model-meat.hpp` — rewire functions, setup functions
- `agentssample-bones.hpp` — sampling logic
- `randgraph.hpp` — graph generation (calls `swap_neighbors`)
- User-facing code that calls `agent.print()` or `agent[j]`
- Derived models that call `agent.get_neighbors()`

Estimated: **20–40 call site changes** across the codebase.

### 6.3 What Gets Simpler

- **Copy/move constructors and `operator=` for Agent:** All `model = ...` lines removed
  (3 locations)
- **Model copy/move constructors and `operator=`:** All `p.model = this` fixup loops
  removed (7+ locations across `model-meat.hpp`)
- **`population_backup` stale-pointer bugs:** Eliminated entirely — no model pointer to
  become stale
- **`reset()` model fixup:** The loop `for (auto & p : population) p.model = this` in
  `Model::reset()` becomes unnecessary

### 6.4 What Gets Harder

- **`operator()(j)` / `operator[](j)` API:** These provide convenient `agent(j)` syntax
  for accessing agent features. Adding a model parameter changes the ergonomics:
  ```cpp
  // Before:
  double val = agent(0);
  
  // After (option 1 — model parameter):
  double val = agent(0, model);
  
  // After (option 2 — model accessor):
  double val = model.get_agent_data(agent.get_id(), 0);
  ```

- **`get_neighbors()` / `swap_neighbors()`:** Need model passed from rewiring
  functions. This is already the pattern used by most agent methods (`add_tool`,
  `set_virus`, `add_entity`, `rm_virus`, etc.) — all take a `Model*` parameter.

---

## 7. Suggested Approach: Phased Removal

### Phase 1 — Remove `Agent::model` (HIGH PRIORITY)

This eliminates the most dangerous class of bugs (stale pointers in copied models) and
is moderate effort.

**Steps:**

1. Remove `Model<TSeq> * model;` from `agent-bones.hpp`
2. Add `Model<TSeq> &` (or `Model<TSeq> *`) parameter to the 7 functions listed above
3. Update all call sites (estimated 20–40 changes)
4. Remove all `p.model = this` fixup loops in `model-meat.hpp`
5. Remove `model = other_agent.model` from Agent copy/move constructors
6. For the `operator[]`/`operator()` feature-access pattern, consider adding a
   `Model::get_agent_data(id, col)` method as a cleaner alternative

**Breaking change for downstream:** Any code that calls `agent.print()`,
`agent.get_neighbors()`, `agent.swap_neighbors()`, or `agent(j)` / `agent[j]` would
need to pass the model. The R package (epiworldR) likely calls some of these.

### Phase 2 — Convert `DataBase::model` to Reference (MEDIUM PRIORITY)

Change from `Model<TSeq> * model` to `Model<TSeq> & model`. This prevents null pointer
bugs and makes the ownership relationship explicit. DataBase is never default-constructed
without a Model anyway.

### Phase 3 — Same for `Queue::model` and `UserData::model` (LOW PRIORITY)

Same conversion to reference. These are simple given the small number of access sites.

---

## 8. Alternative: Keep `model` But Add Safety Guards

If removing `Agent::model` is too disruptive, a lighter approach would be:

1. **Make the pointer private with a setter** — Add `void set_model(Model<TSeq> *)` and
   `Model<TSeq> & get_model() const` methods. The setter could add debug assertions.

2. **Add `#ifdef EPI_DEBUG` validation** — In debug builds, check that `model` is
   non-null and points to a valid model before every access.

3. **Centralize fixup** — Create a single `Model::rebind_all_pointers()` method that
   fixes up all `model` pointers in all subobjects. Call it from every copy/move path.

However, this approach addresses symptoms rather than the root cause. The structural fix
(removing the pointer) is safer long-term.

---

## 9. Summary Table

| Class | Recommendation | Effort | Risk Reduction | API Breaking? |
|-------|---------------|:------:|:--------------:|:-------------:|
| **Agent** | Remove `model` pointer | Medium (20-40 call sites) | **High** | Yes (7 functions) |
| **DataBase** | Convert to reference | Low (internal only) | Medium | No |
| **Queue** | Convert to reference | Trivial | Low | No |
| **UserData** | Convert to reference | Trivial | Low | No |
| Virus | Already clean | — | — | — |
| Tool | Already clean | — | — | — |
| Entity | Already clean | — | — | — |

### Additional Finding: Dead Code

`agentssample-bones.hpp:149` references `entity_.model`, but `Entity` does not have a
`model` member. This is likely dead code (the template is never instantiated with that
path) or a remnant from a previous design. It should be cleaned up as part of this work.

---

## 10. Conclusion

Removing `Agent::model` is the highest-value change. It eliminates an entire category of
subtle copy/move bugs (like the one this PR fixes), reduces maintenance burden, and
aligns Agent with the pattern already used by Virus, Tool, and Entity — where the model
is passed as a parameter to functions that need it. The cost is moderate: 7 function
signatures and 20–40 call sites, with no algorithmic changes required.
