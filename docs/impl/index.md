# Implementation Documentation

This module contains information about the implementation of epiworld, and how
you can extend it with your own modules and code.

## Architecture and Design

- [Library Architecture](library-architecture.md) — modular structure of the `Agent`, `Entity`, and `Model` classes, the "bones and meat" pattern, and file organization.
- [Performance Optimization](performance-optimization.md) — profiling, memory layout, template inlining, and OpenMP parallelization.
- [Queueing System](queueing-system.md) — the `Queue` class that selectively activates agents for efficient per-step processing.

## Extending Epiworld

- [Extending the Library](extending-the-library.md) — custom agent behaviors, network rewiring, global events, and data collection strategies.
- [Extending the Library with Models](extending-the-library-with-models.md) — creating new epidemiological models via inheritance from `Model`.

## Models and Simulation

- [Core Simulation Models](core-simulation-models.md) — pre-built model templates (SIS, SIR, SEIR, mixing, quarantine, and more).
- [Mixing and Entity Distribution](mixing-and-entity-distribution.md) — contact matrices and entity distribution functions for structured group interactions.
- [Sampling Contacts](sampling-contacts.md) — binomial contact sampling optimization in mixing models.
- [Quarantine, Isolation, and Contact Tracing](quarantine-isolation-and-contact-tracing.md) — quarantine and isolation mechanisms and contact tracing modules.
- [Virus and Tool Distribution Functions](virus-and-tool-distribution.md) — distributing viruses and tools to agents.

## Data and Analysis

- [Data Collection and Analysis](data-collection-and-analysis.md) — the `DataBase` class, recording during simulations, exporting data, and basic analysis.
- [Generation Interval and Reproductive Number Calculation](generation-interval-and-reproductive-number.md) — computing generation intervals and reproductive numbers.

## Simulation Execution

- [Reproducibility and `run_multiple`](reproducibility-and-run-multiple.md) — single and multiple simulation runs, seeding, and parallel execution.
- [Random Number Generation and Statistical Functions](random-number-generation.md) — RNG utilities, statistical distributions, and the LFMCMC module.
- [Events and Multi-Event Handling](events-and-multi-event-handling.md) — the event dispatch system and LIFO scheduling.
