# epiworld 0.16.0

## Faster network transmission (#264)

* Network models can now sample transmission by **pushing** infection odds from
  the agents carrying a virus to their susceptible neighbors, instead of every
  susceptible agent **pulling** from all of its neighbors. Both give the same
  distribution of who gets infected and by whom, because `roulette()` depends
  only on the sum of the odds $p/(1-p)$ of an agent's infectious contacts. The
  cost of a push follows the carriers' ties, so it is much cheaper while an
  outbreak is small. See the new docs page "Push and Pull Transmission".

* `Model::set_transmission_mode()` takes `"auto"` (the default), `"push"`, or
  `"pull"`. With `"auto"`, the model compares the degree sums of the carriers
  and the susceptibles at every step, and pushes when the former is at most
  `kappa` (default 0.25; `set_transmission_kappa()`) times the latter. The
  choice ignores the queue, so queuing on and off still give identical runs.
  It applies to states whose update function is `default_update_susceptible`
  or `sampler::make_update_susceptible()`, which covers `ModelSIR`, `ModelSIS`,
  `ModelSISD`, `ModelSIRD`, `ModelSEIR`, `ModelSEIRD`, and epiworldR's
  `update_fun_susceptible()`. Every other update function keeps pulling.

* **Results for a given seed change** in these models, because the random
  number stream is consumed differently; their distribution does not. Use
  `model.set_transmission_mode("pull")` to reproduce runs from 0.15 and
  earlier.

* `Model::update_state()` no longer scans the whole population when queuing
  is on. The queue keeps an ordered set of queued agents (a bitset), which is
  walked in ascending id order, so pull runs are identical to before.

* New `Model::get_agents_in_state(state)`: the ids of the agents in a state,
  kept up to date as the model runs (no scan of the population).

* `sampler::make_update_susceptible()` now returns a
  `sampler::UpdateSusceptible<TSeq>` function object (still usable wherever a
  `std::function` is expected). Its excluded-state mask is no longer shared
  between copies of a model, which could race under `run_multiple()` with
  threads.

## Bug fixes

* `roulette()` now treats a probability of exactly 1 as a certain event. It
  tested `p > 1 - 1e-100`, which is `p > 1` in floating point. So `p == 1`
  divided 0 by 0, and the *last* entry won regardless of which one was
  certain. This picked the wrong infector (or virus) among several contacts,
  and the wrong transition in `default_update_exposed()` and
  `new_state_update_transition()` (e.g., recovery when the probability of
  death was 1). Runs where some probability is exactly 1 and is not the last
  one therefore change.

* `Queue::reset()` now always clears the counts. They could survive into the
  next run when they had been changed through `Queue::OnlySelf`, which also
  bypassed the count of queued agents.

* `Model::events_run()` no longer reads an event after running its handler,
  which could schedule more events and reallocate the list.

* The `Agent` copy constructor copies `state_prev` and `state_last_changed`,
  as the copy assignment does. Copies of a model made after a run (as
  `run_multiple()` makes for its threads) failed the `EPI_DEBUG` check in
  `Model::clone_ptr()`.

## Breaking changes

* `Queue::operator[]` returns the count by value. Writing through it would
  have bypassed the ordered set of queued agents; use `Queue::operator+=` and
  `operator-=` (or `Model::add_edge()`/`rm_edge()`) instead.

## New example

* `examples/20-transmission-benchmark` times the network transmission step in
  each mode on four network models, including a measles-like outbreak in a
  population of a million. It compiles against older releases too, for
  side-by-side comparisons.
