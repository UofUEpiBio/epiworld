# Agent Instructions for epiworld

This file contains conventions and rules for AI agents working on this repository.

## Testing Conventions

- Each test file in `tests/` must contain **exactly one** `EPIWORLD_TEST_CASE` macro.
- If you need multiple test cases for the same feature, create separate files using a letter suffix (e.g., `22a-testname.cpp`, `22b-testname.cpp`).
- Always use Catch2 expectations (`REQUIRE`, `REQUIRE_THAT`, `CHECK`, etc.) for test validation. Do not throw exceptions for test validation.
- For fully deterministic tests, use `model.run()`. For stochastic tests that need multiple runs to verify statistical properties, use `model.run_multiple()`.

## General Rules

- The `./epiworld.hpp` file is a single-header amalgamation. Do not analyze it or suggest changes to it.
- Do not use `./epiworld.hpp` in your suggestions; include from `include/epiworld/` instead.
