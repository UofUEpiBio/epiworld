# Agent Instructions for epiworld

This file contains conventions and rules for AI agents working on this repository.

## Build Environment

The repository ships a container image definition in `.devcontainer/` (`Containerfile`: Ubuntu 24.04, g++, make, gdb, lcov, doxygen, Perl, mkdocs). **Prefer building, testing and benchmarking inside that container whenever `docker` or `podman` is available** (check with `command -v podman docker`), so results do not depend on the host toolchain. Fall back to the host toolchain only when neither is installed.

```sh
# Build the image once (from the repository root; the context must be the root)
podman build -f .devcontainer/Containerfile -t epiworld-dev .    # or: docker build ...

# Run a command in it, with the checkout mounted at the same path
podman run --rm -v "$PWD":"$PWD" -w "$PWD" epiworld-dev make test WITH_OPENMP=0
```

- In a git worktree, also mount the main checkout's `.git` directory at its own path (the worktree's `.git` file points to it), or run git on the host.
- Podman on macOS needs a running machine (`podman machine start`).
- Tools the image lacks (e.g., `valgrind` for cachegrind counts) can be installed inside a throwaway container with `apt-get`; if one is needed regularly, add it to the `Containerfile` instead.
- Wall-clock timings inside a container on macOS run in a VM: compare variants interleaved within the same environment, never against host timings.

## Testing Conventions

- Each test file in `tests/` must contain **exactly one** `EPIWORLD_TEST_CASE` macro.
- If you need multiple test cases for the same feature, create separate files using a letter suffix (e.g., `22a-testname.cpp`, `22b-testname.cpp`).
- Always use Catch2 expectations (`REQUIRE`, `REQUIRE_THAT`, `CHECK`, etc.) for test validation. Do not throw exceptions for test validation.
- For fully deterministic tests, use `model.run()`. For stochastic tests that need multiple runs to verify statistical properties, use `model.run_multiple()`.
- Prefer **end-to-end tests over unit tests**. Build a model, run it, and assert on what the public API exposes -- histories, transition matrices, edge lists, agent states. Reaching into internals couples the suite to implementation details and makes refactors expensive. When a change is internal (a data-structure rewrite, say), the test is that existing model-level results stay identical.

## General Rules

- The `./epiworld.hpp` file is a single-header amalgamation. Do not analyze it or suggest changes to it.
- Do not use `./epiworld.hpp` in your suggestions; include from `include/epiworld/` instead.
