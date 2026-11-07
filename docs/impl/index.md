# Implementation Documentation

This module contains information about the implementation of epiworld, and how
you can extend it with your own modules and code.

## Architecture and Design

- [Library Architecture](library-architecture.md) — modular structure of the `Agent` and `Model` classes, the "bones and meat" pattern, and file organization.
- [Performance Optimization](performance-optimization.md) — profiling, memory layout, inlining, and OpenMP parallelization.
- [Queueing System](queueing-system.md) — the `Queue` class that selectively activates agents for efficient per-step processing.

## Extending Epiworld

- [Extending the Library](extending-the-library.md) — custom agent behaviors, network rewiring, global events, and data collection strategies.
- [Extending the Library with Models](extending-the-library-with-models.md) — declaring a natural history (stages, transitions, statuses, reactions) and building models on it.

## Models and Simulation

- [Core Simulation Models](core-simulation-models.md) — pre-built models (SIS, SIR, SEIR, quarantine, and more) and their diagrams.
- [Mixing by Group](mixing-and-entity-distribution.md) — contact matrices, groupings, and mixing within groups.
- [Sampling Contacts](sampling-contacts.md) — binomial contact sampling optimization in mixing models.
- [Combining Exposures](combining-exposures.md) — roulette versus independent exposures when an agent has several infectious contacts.
- [Push and Pull Transmission](transmission-sampling.md) — why pushing infection odds from infectious agents gives the same outcomes as susceptibles pulling from their neighbors, and how `epiworld` picks the cheaper of the two at each step.
- [Quarantine, Isolation, and Contact Tracing](quarantine-isolation-and-contact-tracing.md) — statuses, reactions and the layers' contact logs.
- [Virus and Tool Distribution Functions](virus-and-tool-distribution.md) — placing viruses, tools, initial stages and groups with `assign`.

## Data and Analysis

- [Data Collection and Analysis](data-collection-and-analysis.md) — the `DataBase` class, recording during simulations, exporting data, and basic analysis.
- [Generation Interval and Reproductive Number Calculation](generation-interval-and-reproductive-number.md) — computing generation intervals and reproductive numbers.

## Simulation Execution

- [Reproducibility and `run_multiple`](reproducibility-and-run-multiple.md) — single and multiple simulation runs, seeding, and parallel execution.
- [Random Number Generation and Statistical Functions](random-number-generation.md) — RNG utilities, statistical distributions, and the LFMCMC module.
- [Events and Multi-Event Handling](events-and-multi-event-handling.md) — how the day's changes are queued and applied.
