## Overview

In a network model, a susceptible agent can acquire a virus from any neighbor that carries one. `epiworld` can sample that outcome in two ways:

- **Pull**: each susceptible agent looks at its neighbors, collects the transmission probabilities of those carrying a virus, and draws *at most one* infector (`sample_virus_single()` and `roulette()`). This is how `epiworld` has always worked.
- **Push**: each agent carrying a virus adds its *infection odds* to its susceptible neighbors; afterwards, every susceptible agent that received something makes a single draw.

Both produce **exactly the same distribution** of who gets infected and by whom. They differ in cost. Pulling visits every tie of every (queued) susceptible agent, while pushing visits only the ties of agents carrying a virus. While an outbreak is small, pushing is much cheaper; near the peak of a large outbreak in a dense network, pulling can be.

By default (`"auto"`), the model picks the cheaper of the two at every step. This page shows why the two are equivalent, how the choice is made, and what it means for reproducibility.

```cpp
model.set_transmission_mode("auto"); // default: cheaper of the two, each step
model.set_transmission_mode("push");
model.set_transmission_mode("pull"); // the random streams of epiworld <= 0.15
```

## Pulling: at most one infector

Take a susceptible agent $j$ and let $K_j$ be its neighbors carrying a virus. As described in [Sampling Contacts](sampling-contacts.md#probability-of-infection), the probability that $k \in K_j$ transmits to $j$ is

$$
p_{kj} = \left(1 - \text{susceptibility reduction}_j\right) \times p_{v_k} \times \left(1 - \text{transmission reduction}_k\right),
$$

where $v_k$ is $k$'s virus and the reductions come from each agent's tools. `epiworld` assumes that $j$ acquires the disease from at most one neighbor per step. Conditioning on "none or exactly one" gives

$$
P(\text{none}) = \frac{\prod_{k}\left(1 - p_{kj}\right)}{Z_j},
\qquad
P(k) = \frac{p_{kj}\prod_{l \neq k}\left(1 - p_{lj}\right)}{Z_j},
$$

$$
Z_j = \prod_{k}\left(1 - p_{kj}\right) + \sum_{k} p_{kj} \prod_{l \neq k}\left(1 - p_{lj}\right).
$$

`roulette()` computes these terms and makes one uniform draw to pick "none" or one $k$.

## The odds form

Divide the numerators and $Z_j$ by $\prod_{k}(1 - p_{kj})$. With the **odds** $r_{kj} = p_{kj} / (1 - p_{kj})$ and their sum $R_j = \sum_{k \in K_j} r_{kj}$:

$$
\boxed{
P(\text{none}) = \frac{1}{1 + R_j},
\qquad
P(k) = \frac{r_{kj}}{1 + R_j}.
}
$$

So the outcome for $j$ depends on its neighbors only through the **sum of their odds**, and the infector is chosen with probability proportional to its odds. A sum can be accumulated from either end of a tie, which is what makes pushing possible.

## Pushing

For each agent $k$ carrying a virus, and each eligible susceptible neighbor $j$ (see [Scope](#scope)):

1. Add $r_{kj}$ to $R_j$ (the first time $j$ is touched, record it in a list).
2. Keep a **candidate infector** for $j$. The first contribution becomes the candidate; each later contribution replaces it with probability $r_{kj} / R_j$, where $R_j$ already includes $r_{kj}$.

Then, for each touched $j$:

3. With probability $1/(1 + R_j)$ nothing happens; otherwise $j$ acquires the candidate's virus.

**The candidate is drawn with probability proportional to the odds.** This is a sequential weighted reservoir sample. Suppose the contributions arrive as $r_1, \dots, r_m$ with partial sums $R^{(i)} = r_1 + \dots + r_i$, and assume that after $m - 1$ contributions $P(\text{cand} = k) = r_k / R^{(m-1)}$ (true for $m = 1$). Contribution $m$ becomes the candidate with probability $r_m / R^{(m)}$. Otherwise, with probability $R^{(m-1)} / R^{(m)}$, the current candidate stays, so for $k < m$

$$
P(\text{cand} = k) = \frac{r_k}{R^{(m-1)}} \cdot \frac{R^{(m-1)}}{R^{(m)}} = \frac{r_k}{R^{(m)}}.
$$

**Pushing reproduces the pull.** The last draw is independent of the reservoir, so

$$
P(\text{infected by } k) = \frac{R_j}{1 + R_j} \cdot \frac{r_{kj}}{R_j} = \frac{r_{kj}}{1 + R_j},
\qquad
P(\text{none}) = \frac{1}{1 + R_j},
$$

which is the odds form above. The order in which carriers are visited does not matter.

**Certain transmissions.** If $p_{kj} = 1$, the odds are infinite. In the limit, infection is certain and the infector is drawn uniformly among the contacts with $p_{kj} = 1$ (the others have finite odds and vanishing weight). Pushing handles this case separately: it counts those contacts, keeps a uniform reservoir among them, and skips the final draw. `roulette()` does the same.

!!! note "Before 0.16"
    `roulette()` tested `p > 1 - 1e-100`, which is `p > 1` in floating point. A probability of exactly 1 then gave $0/0$ and the *last* entry won, whether or not it was the certain one. This also affected state transitions (e.g., a death probability of 1 yielded recovery). Since 0.16, `p >= 1` is certain.

## Equivalence for the whole model

Each step of an `epiworld` simulation is synchronous. Every update function reads the model as it was at the start of the step and only *schedules* changes, and the changes are applied together at the end (`events_run()`). Given the state $X_t$ of the model, each agent's outcome is drawn with its own random numbers, independently of the other agents':

- Pulling uses one uniform per susceptible agent with an infectious neighbor.
- Pushing uses the reservoir draws and one uniform per touched agent, and none of these are shared between agents.

Both give every susceptible agent the same outcome distribution (previous section), independently across agents, and leave every other state's update function unchanged. So the one-step transition kernel $P(X_{t+1} \mid X_t)$ is **identical** under both.

Under `"auto"`, the mode for step $t$ is a deterministic function of $X_t$ (the rule below). A chain that picks, at each step, one of two identical kernels based on its current state still has that same kernel. So the epidemic has the same law under `"push"`, `"pull"`, and `"auto"`.

What does change is the **stream of random numbers**. The draws happen in a different order, and pushing uses a different number of them. So a given seed produces a different, equally likely, epidemic.

## Choosing between them

A pull visits every tie of every susceptible agent it updates. A push visits every tie of every agent that can transmit. With queuing on, only queued susceptibles pull, but they include every neighbor of anyone carrying a virus, including latent or hospitalized agents that may not transmit.

The model keeps, per state, the sum of the members' degrees and the sum of the degrees of those carrying a virus, updated as agents change state. `"auto"` pushes at step $t$ when

$$
\underbrace{\sum_{s \,\in\, \text{source states}} \; \sum_{k \in s,\; k \text{ carries a virus}} \deg(k)}_{\text{push cost}}
\;\le\;
\kappa \times
\underbrace{\sum_{s \,\in\, \text{susceptible states}} \; \sum_{j \in s} \deg(j)}_{\text{pull cost (without queuing)}}
$$

and pulls otherwise. This takes $O(\text{number of states})$ time per step.

The rule deliberately ignores the queue. The decision, and therefore the random stream, is the same with queuing on or off, so queuing remains a pure optimization: a run with it on and a run with it off give identical results in every mode.

The queue does make pulling cheaper than the right-hand side suggests, because a queued pull skips the susceptibles with no infectious neighbor. That is why $\kappa$ defaults to 0.25 rather than 1 (`set_transmission_kappa()`). With cachegrind on the benchmark below, 0.25 keeps the full gain where pushing wins (the epiworld-benchmark model pushes at every step) without pushing through the peak of large outbreaks, where the queued pull is cheaper.

## Scope

Pushing applies to states whose update function is `default_update_susceptible` or `sampler::make_update_susceptible(exclude)` (which is what epiworldR's `update_fun_susceptible()` uses). This covers the network models `ModelSIR`, `ModelSIS`, `ModelSISD`, `ModelSIRD`, `ModelSEIR`, and `ModelSEIRD`, and custom models built on these functions. A push mirrors the pull exactly:

- **Sources.** Every neighbor carrying a virus is a source, as in the pull (so, e.g., exposed agents in `ModelSEIR` transmit), except those in the states listed in `exclude`.
- **Probabilities.** $p_{kj}$ is computed with the same expression, the same tool mixers, and the same virus functions, for exactly the same (target, source) pairs.
- **Queue.** With queuing on, a susceptible agent is only updated if it is queued, as when pulling.
- **Errors.** An agent in a susceptible state that carries a virus raises the same error as the pull.

Everything else keeps pulling: states with custom update functions (e.g., `ModelSIRLogit`, `ModelSURV`, `ModelSEIRNetworkQuarantine`, `ModelDiffNet`) and the connected, mixing, and measles models, which sample contacts differently. The model's `directed` flag, if set, also forces pulling, and so does building without RTTI (recognizing the functions uses `std::function::target()`). Pushing relies on every tie being stored at both ends, which is how `epiworld` builds networks.

Two numerical differences are worth knowing:

- Pulling fills fixed-size scratch arrays, so it fails for agents with more than 1024 neighbors carrying a virus. Pushing has no such limit.
- `roulette()` multiplies the $(1 - p)$ terms in `epiworld_double` (a `float` by default). With many high-probability contacts (e.g., 40 contacts with $p = 0.9$), the product underflows to 0 and the pull falls back to the last contact. Pushing sums the odds in `double` and keeps the exact probabilities.

## Reproducibility

Because the random stream differs, **results for a given seed changed in epiworld 0.16**, while their distribution did not. To reproduce a run from epiworld 0.15 or earlier, set

```cpp
model.set_transmission_mode("pull");
```

This gives identical runs, except where some probability is exactly 1 (see the note above). In pull mode, the model visits agents in the same order as before: the queue now keeps an ordered set of queued agents instead of scanning the population (see [Queueing System](queueing-system.md)).

## Benchmark

`examples/20-transmission-benchmark` times the three modes on:

- **(A)** the SEIRH model of the [epiworld-benchmark](https://github.com/UofUEpiBio/epiworld-benchmark) study: a Watts–Strogatz network with mean degree 10, $R_0 = 2$, 100 initial cases and 100 days;
- **(B)** `ModelSEIR` on the same network;
- **(C)** a dense ($\bar k = 50$), high-prevalence SIR.

Median CPU milliseconds per 100-day run (50 replicates, Apple M3 Pro, Apple clang 16, `-O3`), epiworld 0.15.1 against 0.16.0:

| Scenario | Agents | 0.15.1 | 0.16 auto | 0.16 push | 0.16 pull |
|:--|--:|--:|--:|--:|--:|
| A: epiworld-benchmark SEIRH | 10,000 | 11.6 | **6.8** | 6.9 | 12.5 |
| | 100,000 | 24.0 | **11.2** | 11.1 | 22.0 |
| B: `ModelSEIR`, ~95% attack | 10,000 | 16.9 | 17.3 | 17.6 | 17.3 |
| | 100,000 | 191 | 204 | 225 | 201 |
| C: dense SIR ($\bar k = 50$) | 10,000 | 18.6 | **17.7** | 18.8 | 18.5 |
| | 100,000 | 308 | **283** | 384 | 325 |

`"pull"` runs are bit-identical to 0.15.1 (same checksums).

- **A.** Agents keep the virus after recovery and never leave the queue, so pulling scans tens of thousands of susceptibles a day while only a few hundred agents can transmit. Pushing halves the run time, and `"auto"` pushes at every step.
- **B.** Recovered agents leave the queue, so the queued pull already visits little more than the susceptibles next to an infectious agent. Pushing brings little, and the index and queue bookkeeping costs a few percent over 0.15.1.
- **C.** `"auto"` switches between the two and beats both.

Through epiworldR, on the epiworld-benchmark study itself (its `runners/epiworld.R`, 100 replicates, `simulate_seconds`), with epiworldR built on epiworld 0.15.1 and on 0.16.0:

| Agents | 0.15.1 | 0.16.0 | Median attack rate (0.15.1 / 0.16.0) |
|--:|--:|--:|:--|
| 10,000 | 0.013 s | 0.009 s | 0.382 / 0.381 |
| 100,000 | 0.029 s | 0.016 s | 0.060 / 0.059 |

Timings on a laptop are noisy, so the same runs were also counted exactly with cachegrind (Linux container, GCC 13, `-O3`). The cells show instructions and last-level data-cache misses per run, with the change in misses relative to 0.15.1 in parentheses. Misses are what dominate the run time at these sizes:

| Scenario | Agents | 0.15.1 | 0.16 auto | 0.16 push | 0.16 pull |
|:--|--:|--:|--:|--:|--:|
| A | 10,000 | 182M / 1.91M | 120M / 0.50M (-74%) | 120M / 0.50M (-74%) | 176M / 1.94M (+2%) |
| A | 100,000 | 337M / 4.25M | 203M / 1.60M (-62%) | 203M / 1.60M (-62%) | 264M / 3.97M (-7%) |
| B | 10,000 | 304M / 1.98M | 295M / 2.06M (+4%) | 294M / 2.43M (+23%) | 302M / 2.06M (+4%) |
| B | 100,000 | 2974M / 20.59M | 2900M / 23.59M (+15%) | 2897M / 29.99M (+46%) | 2951M / 21.26M (+3%) |
| C | 10,000 | 349M / 1.67M | 335M / 1.58M (-5%) | 356M / 2.12M (+27%) | 344M / 1.73M (+4%) |
| C | 100,000 | 3505M / 18.52M | 3353M / 17.41M (-6%) | 3580M / 27.33M (+48%) | 3453M / 18.78M (+1%) |


## See Also

- [Sampling Contacts](sampling-contacts.md) — the probability of infection and how tools adjust it.
- [Queueing System](queueing-system.md) — which agents are updated at each step.
- [Reproducibility and `run_multiple`](reproducibility-and-run-multiple.md) — seeds and multiple runs.
- [Performance Optimization](performance-optimization.md) — benchmarking the library.
