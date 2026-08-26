# Agent Instructions for epiworld

This file contains conventions and rules for AI agents working on this repository.

## Testing Conventions

- Each test file in `tests/` must contain **exactly one** `EPIWORLD_TEST_CASE` macro.
- If you need multiple test cases for the same feature, create separate files using a letter suffix (e.g., `22a-testname.cpp`, `22b-testname.cpp`).
- Always use Catch2 expectations (`REQUIRE`, `REQUIRE_THAT`, `CHECK`, etc.) for test validation. Do not throw exceptions for test validation.
- For fully deterministic tests, use `model.run()`. For stochastic tests that need multiple runs to verify statistical properties, use `model.run_multiple()`.
- Prefer **end-to-end tests over unit tests**. Build a model, run it, and assert on what the public API exposes -- histories, transition matrices, edge lists, agent states. Reaching into internals couples the suite to implementation details and makes refactors expensive. When a change is internal (a data-structure rewrite, say), the test is that existing model-level results stay identical.

## General Rules

- The `./epiworld.hpp` file is a single-header amalgamation. Do not analyze it or suggest changes to it.
- Do not use `./epiworld.hpp` in your suggestions; include from `include/epiworld/` instead.
