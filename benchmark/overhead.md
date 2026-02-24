# Simulation Overhead Benchmark

This document reports timing overhead for epiworld models, broken down into
**setup time** (population initialization and distribution of viruses/tools)
and **simulation time** (main loop over days).

Timings were measured using the `get_elapsed_setup()` and `get_elapsed()`
getter functions introduced in epiworld. All benchmarks used a single run of
the model with a fixed random seed (123), compiled with `-O2` optimization.

## How to Interpret the Timings

| Term       | Description |
|------------|-------------|
| Setup (ms) | Time for `reset()`: restoring population backup, distributing viruses and tools, recording initial state |
| Sim (ms)   | Time for the main simulation loop over all days |
| Total (ms) | Sum of setup and simulation time |
| Setup %    | Fraction of total time spent in setup |

The setup cost is paid once per `run()` call. In `run_multiple()` it is
paid for every replicate and accumulates into the total reported by
`get_elapsed_setup()`.

---

## SEIR Mixing Model — 3 groups

Three equal-sized groups with a 3×3 contact matrix, 100 days, seed 123.

| Agents   | Setup (ms) | Sim (ms)   | Total (ms) | Setup % |
|----------|------------|------------|------------|---------|
|     1000 |       0.12 |       3.11 |       3.23 |    3.7% |
|     5000 |       0.36 |      15.79 |      16.15 |    2.2% |
|    10000 |       0.71 |      33.14 |      33.85 |    2.1% |
|    50000 |       4.46 |     166.31 |     170.77 |    2.6% |
|   100000 |       6.26 |     350.29 |     356.55 |    1.8% |

---

## SEIR Mixing Model — 100 groups

One hundred equal-sized groups with a 100×100 contact matrix (diagonal 0.9,
uniform off-diagonal), 100 days, seed 123.

| Agents   | Setup (ms) | Sim (ms)   | Total (ms) | Setup % |
|----------|------------|------------|------------|---------|
|     1000 |       0.10 |      28.75 |      28.85 |    0.3% |
|     5000 |       0.36 |     169.27 |     169.63 |    0.2% |
|    10000 |       0.59 |     383.49 |     384.08 |    0.2% |
|    50000 |       2.79 |    2343.29 |    2346.07 |    0.1% |
|   100000 |       5.25 |    4767.46 |    4772.72 |    0.1% |

**Effect of group count on mixing models:** Increasing from 3 to 100 groups
roughly **11×** increases simulation time (the per-step contact sampling must
loop over 100 groups), while setup time is nearly identical. Setup becomes
negligible (<0.3%) at high group counts.

---

## Measles Mixing with Quarantine Model — 3 groups

Three equal-sized groups with a 3×3 contact matrix, 120 days, seed 123.
This model tracks additional health states (prodromal, rash, quarantine) and
performs contact tracing, which increases both setup and simulation cost.

| Agents   | Setup (ms) | Sim (ms)   | Total (ms) | Setup % |
|----------|------------|------------|------------|---------|
|     1000 |       0.34 |       7.18 |       7.52 |    4.5% |
|     5000 |       1.49 |      41.96 |      43.44 |    3.4% |
|    10000 |       4.60 |      87.70 |      92.30 |    5.0% |
|    50000 |      26.97 |     752.95 |     779.92 |    3.5% |
|   100000 |      54.01 |    2292.20 |    2346.21 |    2.3% |

---

## Measles Mixing with Quarantine Model — 100 groups

One hundred equal-sized groups, same contact matrix structure as above,
120 days, seed 123.

| Agents   | Setup (ms) | Sim (ms)   | Total (ms) | Setup % |
|----------|------------|------------|------------|---------|
|     1000 |       0.44 |      14.01 |      14.45 |    3.1% |
|     5000 |       3.61 |      75.58 |      79.19 |    4.6% |
|    10000 |       4.65 |     146.30 |     150.95 |    3.1% |
|    50000 |      27.62 |     988.99 |    1016.61 |    2.7% |
|   100000 |      54.15 |    2268.05 |    2322.20 |    2.3% |

**Observation:** For the Measles model the simulation-time increase going from
3 → 100 groups is more moderate (~1.7–2.5×). The quarantine/contact-tracing
logic adds work that is independent of group structure, so the extra groups
matter less than in the simpler SEIR Mixing case.

---

## Network Models

### SIR — Small-World Network (k = 20, p = 0.01)

100 days, seed 123. Agents connected via Watts-Strogatz small-world graph.

| Agents   | Setup (ms) | Sim (ms)   | Total (ms) | Setup % |
|----------|------------|------------|------------|---------|
|     1000 |       0.01 |       0.77 |       0.79 |    1.9% |
|     5000 |       0.06 |       4.26 |       4.32 |    1.4% |
|    10000 |       0.12 |       8.34 |       8.46 |    1.4% |
|    50000 |       0.84 |      50.50 |      51.34 |    1.6% |
|   100000 |       1.74 |     126.41 |     128.15 |    1.4% |

### SEIR — Small-World Network (k = 20, p = 0.01)

100 days, seed 123.

| Agents   | Setup (ms) | Sim (ms)   | Total (ms) | Setup % |
|----------|------------|------------|------------|---------|
|     1000 |       0.01 |       0.88 |       0.90 |    1.4% |
|     5000 |       0.05 |       4.53 |       4.58 |    1.1% |
|    10000 |       0.11 |       9.12 |       9.23 |    1.2% |
|    50000 |       0.93 |      58.68 |      59.61 |    1.6% |
|   100000 |       1.70 |     144.99 |     146.69 |    1.2% |

### SIR — Connected Population (random contacts, no explicit network)

100 days, seed 123. Each agent makes a fixed number of random contacts
each day (contact rate = 10).

| Agents   | Setup (ms) | Sim (ms)   | Total (ms) | Setup % |
|----------|------------|------------|------------|---------|
|     1000 |       0.02 |       1.04 |       1.06 |    1.8% |
|     5000 |       0.06 |       5.14 |       5.20 |    1.1% |
|    10000 |       0.12 |      12.61 |      12.73 |    1.0% |
|    50000 |       0.72 |      62.08 |      62.80 |    1.2% |
|   100000 |       1.17 |     138.41 |     139.58 |    0.8% |

### SEIR — Connected Population (random contacts, no explicit network)

100 days, seed 123.

| Agents   | Setup (ms) | Sim (ms)   | Total (ms) | Setup % |
|----------|------------|------------|------------|---------|
|     1000 |       0.01 |       1.04 |       1.06 |    1.4% |
|     5000 |       0.06 |       5.72 |       5.78 |    1.0% |
|    10000 |       0.14 |      13.01 |      13.15 |    1.1% |
|    50000 |       0.65 |      64.28 |      64.93 |    1.0% |
|   100000 |       1.20 |     137.31 |     138.51 |    0.9% |

**Network model observations:**
- Setup overhead is the smallest of all model families (~1–2%), because
  there is no per-agent entity membership to distribute during `reset()`.
- Simulation times are the fastest per agent as well: the fixed-degree
  network keeps the contact lookup O(k) rather than O(groups).
- Connected and small-world models have very similar runtimes.

---

## Summary

| Model family                       | Setup % range | Sim driver |
|------------------------------------|---------------|------------|
| SEIR Mixing (3 groups)             | 1.8–3.7%      | group contacts |
| SEIR Mixing (100 groups)           | 0.1–0.3%      | group contacts × 100 |
| Measles Quarantine (3 groups)      | 2.3–5.0%      | state transitions + contact tracing |
| Measles Quarantine (100 groups)    | 2.3–4.6%      | state transitions + contact tracing |
| SIR / SEIR small-world network     | 1.2–1.9%      | edge traversal |
| SIR / SEIR connected population    | 0.8–1.8%      | random contacts |

**Key takeaways:**
- **Setup is never a bottleneck** — across all model types and population
  sizes, setup time stays below 5% of total runtime.
- **Group count matters far more for mixing models than agent count**: going
  from 3 → 100 groups in SEIR Mixing roughly multiplies simulation time by
  11×, while doubling the agent count roughly doubles it.
- **Network models are the fastest** per agent because contact resolution is
  O(k) (fixed degree) rather than O(n_groups × contact_rate).

---

## Accessing Timing Information

After calling `run()` or `run_multiple()`, detailed timing can be retrieved
programmatically:

```cpp
epiworld_double last_setup_us, total_setup_us;
std::string abbr;

// Setup time (reset + initialization)
model.get_elapsed_setup("microseconds", &last_setup_us, &total_setup_us, &abbr, false);

// Simulation time (main loop)
epiworld_double last_run_us, total_run_us;
model.get_elapsed("microseconds", &last_run_us, &total_run_us, &abbr, false);
```

The `print()` method also displays both timings automatically:

```
Last run setup t    : 0.65ms
Last run elapsed t  : 33.22ms
Last run speed      : 30.10 million agents x day / second
```

For multiple runs:

```
Last run setup t    : 0.60ms
Total setup t       : 6.10ms (10 runs)
Last run elapsed t  : 32.50ms
Total elapsed t     : 325.00ms (10 runs)
```
