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

## Parallel `run_multiple()` Overhead

These tables compare **sequential (1 thread)** vs **parallel (2 threads)** execution
of `run_multiple()` with **10 replicates**, seed 123.

**Column definitions for this section:**

| Column       | Description |
|--------------|-------------|
| Thr          | Number of OpenMP threads |
| Setup (ms)   | Accumulated setup time across all runs on **thread 0** |
| Sim (ms)     | Accumulated simulation time across all runs on **thread 0** |
| Overhead (ms)| `Wall − Setup − Sim`: thread-creation, model cloning, and thread imbalance |
| Wall (ms)    | Total wall-clock time for the entire `run_multiple()` call |
| Overhead %   | `Overhead / Wall` |

> **Note on multi-thread accounting**: `Setup` and `Sim` only reflect work done
> by thread 0 (≈ half the replicates with 2 threads). Thread 1's work runs in
> parallel and is included in `Wall` but not in `Setup`/`Sim`. Therefore, for
> 2 threads: `Wall ≈ max(thread-0 work, thread-1 work) + true overhead`, and
> the `Overhead` column captures both thread-management cost and any runtime
> imbalance between threads.

---

### SEIR Mixing (3 groups) — 10 replicates

| Agents   | Thr | Setup (ms) | Sim (ms)   | Overhead (ms) | Wall (ms)  | Overhead% |
|----------|-----|------------|------------|---------------|------------|-----------|
|     1000 |   1 |       0.70 |      31.16 |          0.12 |      31.97 |      0.4% |
|     5000 |   1 |       2.83 |     159.55 |          0.41 |     162.79 |      0.3% |
|    10000 |   1 |       5.91 |     323.81 |          0.71 |     330.44 |      0.2% |
|    50000 |   1 |      37.03 |    1651.93 |          2.13 |    1691.09 |      0.1% |
|   100000 |   1 |      88.38 |    3808.34 |          6.14 |    3902.86 |      0.2% |
|          |     |            |            |               |            |           |
|     1000 |   2 |       0.47 |      21.96 |          1.17 |      23.60 |      5.0% |
|     5000 |   2 |       1.41 |      81.53 |          3.14 |      86.08 |      3.6% |
|    10000 |   2 |       2.95 |     167.86 |          5.72 |     176.53 |      3.2% |
|    50000 |   2 |      21.62 |     953.75 |          8.41 |     983.78 |      0.9% |
|   100000 |   2 |      50.66 |    2323.32 |         15.29 |    2389.26 |      0.6% |

Speedup (wall-clock): ~1.38–1.63× with 2 threads.

---

### Measles Mixing with Quarantine (3 groups) — 10 replicates

| Agents   | Thr | Setup (ms) | Sim (ms)   | Overhead (ms) | Wall (ms)  | Overhead% |
|----------|-----|------------|------------|---------------|------------|-----------|
|     1000 |   1 |       2.92 |      66.25 |          0.04 |      69.21 |      0.1% |
|     5000 |   1 |      16.35 |     387.19 |          0.09 |     403.63 |      0.0% |
|    10000 |   1 |      34.93 |     855.58 |          0.13 |     890.64 |      0.0% |
|    50000 |   1 |     169.35 |    7360.42 |          0.55 |    7530.32 |      0.0% |
|   100000 |   1 |     346.64 |   22418.96 |          2.09 |   22767.69 |      0.0% |
|          |     |            |            |               |            |           |
|     1000 |   2 |       1.73 |      38.21 |          0.14 |      40.09 |      0.3% |
|     5000 |   2 |       8.70 |     191.79 |          6.85 |     207.35 |      3.3% |
|    10000 |   2 |      18.96 |     436.45 |          2.86 |     458.27 |      0.6% |
|    50000 |   2 |      98.06 |    3710.51 |         47.40 |    3855.96 |      1.2% |
|   100000 |   2 |     190.63 |   11515.47 |          9.32 |   11715.42 |      0.1% |

Speedup (wall-clock): ~1.7–1.9× with 2 threads.

---

### SIR Small-World Network — 10 replicates

| Agents   | Thr | Setup (ms) | Sim (ms)   | Overhead (ms) | Wall (ms)  | Overhead% |
|----------|-----|------------|------------|---------------|------------|-----------|
|     1000 |   1 |       1.18 |       8.73 |          0.29 |      10.20 |      2.8% |
|     5000 |   1 |       6.06 |      45.62 |          0.54 |      52.23 |      1.0% |
|    10000 |   1 |      13.30 |      93.06 |          1.39 |     107.75 |      1.3% |
|    50000 |   1 |     128.92 |     671.66 |          9.14 |     809.73 |      1.1% |
|   100000 |   1 |     282.17 |    1627.55 |         20.13 |    1929.85 |      1.0% |
|          |     |            |            |               |            |           |
|     1000 |   2 |       0.72 |       4.43 |          1.06 |       6.20 |     17.1% |
|     5000 |   2 |       3.12 |      23.27 |          2.56 |      28.94 |      8.8% |
|    10000 |   2 |       8.88 |      50.66 |          6.28 |      65.83 |      9.5% |
|    50000 |   2 |      69.49 |     388.07 |         33.99 |     491.55 |      6.9% |
|   100000 |   2 |     141.41 |     889.25 |         97.19 |    1127.84 |      8.6% |

Speedup (wall-clock): ~1.4–1.7× with 2 threads. Overhead % is higher here
because network models complete fast and thread-management cost becomes
relatively larger.

---

### SEIR Connected Population — 10 replicates

| Agents   | Thr | Setup (ms) | Sim (ms)   | Overhead (ms) | Wall (ms)  | Overhead% |
|----------|-----|------------|------------|---------------|------------|-----------|
|     1000 |   1 |       0.29 |      10.82 |          0.05 |      11.16 |      0.4% |
|     5000 |   1 |       1.62 |      59.86 |          0.15 |      61.64 |      0.2% |
|    10000 |   1 |       3.16 |     128.48 |          0.28 |     131.93 |      0.2% |
|    50000 |   1 |      23.12 |     675.06 |          1.49 |     699.66 |      0.2% |
|   100000 |   1 |      62.67 |    1544.55 |          5.04 |    1612.26 |      0.3% |
|          |     |            |            |               |            |           |
|     1000 |   2 |       0.21 |       8.22 |          0.26 |       8.68 |      2.9% |
|     5000 |   2 |       0.93 |      32.76 |          0.93 |      34.62 |      2.7% |
|    10000 |   2 |       1.36 |      66.26 |          0.80 |      68.42 |      1.2% |
|    50000 |   2 |       9.95 |     363.44 |          5.29 |     378.68 |      1.4% |
|   100000 |   2 |      30.14 |     897.29 |         45.36 |     972.80 |      4.7% |

Speedup (wall-clock): ~1.6–1.7× with 2 threads.

---

**Parallel overhead takeaways:**
- **Sequential overhead is negligible** (<0.4% wall-clock) — `run_multiple()`
  infrastructure (seed generation, backup, progress bar) adds under 1 ms.
- **Parallel overhead with 2 threads is 1–20 ms** depending on model and
  population size. For slow models (Measles, SEIR Mixing at large *n*) it is
  <1% of wall-clock time. For fast models (SIR network at small *n*) it can
  reach 10–17% because the absolute simulation time is only a few milliseconds.
- **Parallel speedup is 1.4–1.9×** with 2 threads (theoretical max = 2×).
  The gap from ideal is due to thread-management overhead, model-cloning cost,
  and occasional thread imbalance.



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

// Total wall-clock time for the entire run_multiple() call (0 if only run() was used)
epiworld_double wall_us;
model.get_elapsed_run_multiple("microseconds", &wall_us, &abbr, false);
```

The `print()` method also displays all timings automatically:

```
Last run setup t    : 0.65ms
Last run elapsed t  : 33.22ms
Last run speed      : 30.10 million agents x day / second
```

For multiple runs (including parallel):

```
Last run setup t    : 0.60ms
Total setup t       : 6.10ms (10 runs)
Last run elapsed t  : 32.50ms
Total elapsed t     : 325.00ms (10 runs)
Average run speed   : 30.77 million agents x day / second
run_multiple wall t : 332.00ms (10 runs)
```
