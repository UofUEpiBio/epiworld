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

The setup cost is paid once per `run()` call. In `run_multiple()` it is
paid for every replicate and accumulates into the total reported by
`get_elapsed_setup()`.

---

## SEIR Mixing Model

Three equal-sized groups with a 3×3 contact matrix, 100 days, seed 123.

| Agents   | Setup (ms) | Sim (ms)   | Total (ms) | Setup % |
|----------|------------|------------|------------|---------|
|     1000 |       0.11 |       3.57 |       3.68 |    3.0% |
|     5000 |       0.37 |      16.25 |      16.61 |    2.2% |
|    10000 |       0.65 |      33.22 |      33.87 |    1.9% |
|    50000 |       4.49 |     164.56 |     169.06 |    2.7% |
|   100000 |       6.93 |     340.60 |     347.53 |    2.0% |

**Observations:**
- Setup time grows roughly linearly with the number of agents.
- Setup overhead is consistently around **2–3%** of total runtime for this
  model, so the main loop dominates.

---

## Measles Mixing with Quarantine Model

Three equal-sized groups with a 3×3 contact matrix, 120 days, seed 123.
This model tracks additional health states (prodromal, rash, quarantine) and
performs contact tracing, which increases both setup and simulation cost.

| Agents   | Setup (ms) | Sim (ms)   | Total (ms) | Setup % |
|----------|------------|------------|------------|---------|
|     1000 |       0.33 |       6.99 |       7.32 |    4.5% |
|     5000 |       1.74 |      39.23 |      40.97 |    4.2% |
|    10000 |       4.64 |      87.03 |      91.66 |    5.1% |
|    50000 |      25.03 |     736.72 |     761.75 |    3.3% |
|   100000 |      50.53 |    2278.02 |    2328.55 |    2.2% |

**Observations:**
- Setup cost for the Measles model is higher than for SEIR Mixing because
  the model distributes more state information (vaccination status,
  quarantine risk levels) during initialization.
- At smaller populations the setup fraction is slightly higher (~4–5%),
  but it remains well below 10% across all tested sizes.
- The simulation loop dominates total runtime at all scales.

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
