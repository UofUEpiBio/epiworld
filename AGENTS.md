# Agent Instructions for epiworld

This file contains conventions and rules for AI agents working on this repository.

## Contact layers (epiworld 1.0.0)

On the `v1-layers` branch, and in the `v1-layers` branches of epiworldR,
epiworldpy, epiworldjs and the measles package, the work follows
`contact-layers.md` (in this repository's `v1-layers`). To resume it, read
its **Section 11 (Runbook)**: it says how to find where things stand from the
open pull requests and what to do next. Every epiworld PR into `v1-layers`
updates the runbook's status table.

The natural history (stages, statuses) follows `natural-history.md`, and
several diseases per model (NH-5) follow `several-diseases.md`.

## Build Environment

The repository ships a container image definition in `.devcontainer/` (`Containerfile`: Ubuntu 24.04, g++, make, gdb, doxygen, Perl, mkdocs). **Prefer building, testing and benchmarking inside that container whenever `docker` or `podman` is available** (check with `command -v podman docker`), so results do not depend on the host toolchain. Fall back to the host toolchain only when neither is installed.

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
- When an API change removes or replaces a feature, quickly check which tests only covered that feature and can be deleted rather than ported. List the deleted tests, and why, in the PR.

## General Rules

- The `./epiworld.hpp` file is a single-header amalgamation, generated from `include/epiworld/` by `script/amalgamate.pl`. Never edit it by hand, and do not analyze it or suggest changes to it: edit the sources in `include/epiworld/`.
- **Regenerate and commit `./epiworld.hpp` whenever a change touches anything under `include/epiworld/`** (including a version bump in `include/epiworld/epiworld.hpp`). Run `make build/epiworld.hpp` (in the container, see "Build Environment"), which rebuilds the amalgam and copies it to `./epiworld.hpp`, and commit the result with the source change. Check the diff contains only your changes.
- Do not use `./epiworld.hpp` in your suggestions; include from `include/epiworld/` instead.
- The model diagrams (`include/epiworld/models/*.mmd`, `include/measles/*.mmd`) are drawn from the models' declarations: when a change touches a model's stages, statuses or transitions, run `make diagrams` (in the container) and commit the result. Never edit them by hand.
- **Make the API explicit; do not guess.** A function should not infer what the user meant from a name, a value or a type that can mean two things (e.g., a name that could be a stage or a status): give each meaning its own function or argument, or reject the ambiguity with an error that says how to resolve it. Prefer named parameters, named transitions and explicit placements over defaults inferred from context.
- Versions are bumped once per release cycle, not once per PR. After a release (tagged `epiworld-vX.Y.Z`), the first change that affects simulation results or the public API bumps the version in `include/epiworld/epiworld.hpp` (patch for fixes and behavior changes, minor for new features); that opens the development version for the next release. Later changes keep adding to it without bumping again, unless they call for a bigger bump than the one already made (e.g., a new feature after a patch bump). The `please-bump` CI check compares against the last release, so it reports a missing bump only while the version still equals the released one.
