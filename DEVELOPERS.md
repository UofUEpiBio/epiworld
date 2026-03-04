# Developer Guide

This guide provides information for developers working on the Epiworld project, with a focus on the build system and development workflow.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Build System Overview](#build-system-overview)
- [Building the Project](#building-the-project)
- [Running Tests](#running-tests)
- [Running Examples](#running-examples)
- [Build Configuration](#build-configuration)
- [Project Structure](#project-structure)
- [Advanced Usage](#advanced-usage)

## Prerequisites

The project requires:
- C++17 compatible compiler (g++, clang++, etc.)
- GNU Make
- Perl (for build scripts)
- Optional: OpenMP support
- Optional: Code coverage tools (lcov)
- Optional: Doxygen (for documentation)

## Build System Overview

Epiworld uses a custom GNU Make-based build system designed for ease of use and maintainability. The build system is:

- **Modular**: Build rules are organized in `share/mk/*.mk` files
- **Parallel-capable**: Supports parallel builds with `make -j`
- **Test-aware**: Automatically discovers and runs test cases
- **Coverage-ready**: Integrated code coverage support

### Key Components

- `GNUmakefile`: Main makefile entry point
- `share/mk/`: Shared makefile modules
  - `epw.util.mk`: Utility functions and macros
  - `epw.compile.mk`: Compilation rules
  - `epw.prog.mk`: Program building rules
  - `epw.test.mk`: Test suite integration
  - `epw.example.mk`: Example building and execution
  - `epw.artifact.mk`: Build artifact management
- `tests/Makefile`: Test suite configuration
- `examples/Makefile`: Example projects configuration

### API Design Philosophy

The build system follows a consistent pattern for exposing functionality:

- **File-level operations** become **file targets**: Operations tied to specific files (e.g., generating a README, building a binary) are exposed as make targets corresponding to those files.
  - Example: `make examples/00-hello-world/README.md`
  - Example: `make build/tests/tests`

- **Binary-level operations** get **dedicated targets**: Operations that affect an entire binary or package (e.g., running a complete test suite) get their own phony targets.
  - Example: `make test` (runs entire test suite)
  - Example: `make examples` (builds and runs all examples)

- **Subsection operations** use **flags**: Operations that target specific parts of a binary (e.g., running select test cases from a suite) are controlled via make variables.
  - Example: `make test TESTS="test-name-1;test-name-2"`
  - Example: `make test WITH_COVERAGE=1`

This design ensures consistency: files map to targets, binaries map to targets, and subdivisions of binaries use flags.

## Building the Project

### Quick Start

View available targets:
```bash
make
```

This displays all available build targets including examples, tests, and READMEs.

### Build Tests

Build a test suite (example using the default `tests` package):
```bash
make build/tests/tests
```

Or use the provided task:
```bash
VERBOSE=1 CXXFLAGS="-gdwarf-4" WITH_OPENMP=0 make ./build/tests/tests -j$(perl script/sys-get-nproc.pl)
```

The build output directory mirrors the source directory structure: source in `<package>/` produces builds in `build/<package>/`.

### Build Examples

Build and run all examples:
```bash
make examples
```

Build a specific example (paths mirror source structure):
```bash
make build/examples/00-hello-world/00-hello-world
```

Run a specific example:
```bash
make example-00-hello-world-run
```

## Running Tests

### Run All Tests

```bash
make test
```

This will:
1. Build the test suite
2. Run all test cases in parallel
3. Generate JUnit XML reports
4. Display a summary of results

### Run Specific Tests

To run specific test cases, use the `TESTS` variable with semicolon-separated test names:

```bash
make test TESTS="test-name-1;test-name-2"
```

### Test Reports

Test results are stored in `build/<package-name>/.test/` where `<package-name>` mirrors your source directory:
- `build/<package-name>/.test/report.xml`: JUnit XML format
- `build/<package-name>/.test/report.html`: HTML summary

For example, the default test suite in `tests/` produces reports in `build/tests/.test/`.

## Running Examples

### Generate Example READMEs

Regenerate README files for all examples:
```bash
make readmes
```

This runs each example and captures its output to create documentation.

### Run Individual Examples

Each example can be run directly:
```bash
make example-<example-name>-run
```

Example names include:
- `00-hello-world`
- `01-sir`, `01-sis`, `01-seir`
- `02-sir_multiple_runs`
- `03-simple-sir`
- `05-user-data`
- `06-sir-omp` (requires OpenMP)
- `07-surveillance`
- And more...

## Build Configuration

### Build Variables

The build system supports several configuration variables:

#### `BUILD_PROFILE` (default: `debug`)
Controls optimization and debug symbols:
- `debug`: `-g -O0 -DDEBUG` with extra safety checks
- `release`: `-O2 -g` with optimizations

Usage:
```bash
make test BUILD_PROFILE=release
```

#### `WITH_OPENMP` (default: `1`)
Enable/disable OpenMP support:
- `1`: Enable OpenMP (`-fopenmp`)
- `0`: Disable OpenMP

Usage:
```bash
make test WITH_OPENMP=0
```

#### `WITH_COVERAGE` (default: `0`)
Enable code coverage instrumentation:
- `0`: No coverage
- `1`: Enable coverage with `--coverage` flag

Usage:
```bash
make test WITH_COVERAGE=1
```

Coverage reports are generated in `build/<package-name>/.coverage/` (e.g., `build/tests/.coverage/` for the default test suite).

#### `VERBOSE` (default: unset)
Show full build commands:
```bash
make test VERBOSE=1
```

#### `TESTS` (default: empty)
Run specific test cases (semicolon-separated):
```bash
make test TESTS="SIR Model;SEIR Model"
```

### Combining Variables

Variables can be combined:
```bash
BUILD_PROFILE=release WITH_OPENMP=1 WITH_COVERAGE=1 VERBOSE=1 make test
```

## Project Structure

```
epiworld/
├── GNUmakefile              # Main build entry point
├── epiworld.hpp             # Single-header amalgamated library
├── include/                 # Header files
│   └── epiworld/           # Core library headers
├── tests/                   # Default test suite package
│   ├── Makefile            # Test configuration
│   ├── main.cpp            # Test runner main
│   └── *.cpp               # Individual test files
├── examples/                # Example programs package
│   ├── Makefile            # Examples configuration
│   └── */                  # Individual examples
├── share/mk/                # Build system modules
├── script/                  # Build scripts
│   ├── amalgamate.pl       # Header amalgamation
│   ├── test-gen-runner.pl  # Test runner generation
│   └── junit-*.pl          # Test reporting scripts
└── build/                   # Build output (mirrors source structure)
    ├── tests/              # Built artifacts from tests/
    └── examples/           # Built artifacts from examples/
```

**Note**: The build directory structure mirrors the source structure. Any package at `<path>/` produces build artifacts at `build/<path>/`.

## Advanced Usage

### Test System Details

The test system uses Catch2 and automatically:
1. Discovers test cases at build time
2. Generates individual make targets for each test
3. Runs tests in parallel
4. Aggregates results into unified reports

Each test file should contain ONE `EPIWORLD_TEST_CASE` macro. For multiple test cases, create separate files (e.g., `22a-testname.cpp`, `22b-testname.cpp`).

### Amalgamated Header

Generate the single-header `epiworld.hpp`:
```bash
make build/epiworld.hpp
```

This combines all headers from `include/epiworld/` into one file using `script/amalgamate.pl`.

### Code Coverage Workflow

1. Build with coverage enabled:
```bash
make test WITH_COVERAGE=1
```

2. Coverage data is collected in `build/<package-name>/.coverage/` (e.g., `build/tests/.coverage/` for the default test suite)
3. LCOV generates `coverage.info` file in that directory
4. Use with your favorite coverage visualization tool

### Clean Build

Remove all build artifacts:
```bash
make purge
```

Or use the alias:
```bash
make clean
```

### Debugging Build Issues

Enable verbose output to see full commands:
```bash
make test VERBOSE=1
```

View internal build databases:
```bash
make program-database  # List all programs
make test-database     # List all tests
```

### Testing Guidelines

When creating tests:
- Use Catch2 expectations (`REQUIRE`, `REQUIRE_THAT`, `CHECK`)
- Do NOT throw exceptions for test validation
- One `EPIWORLD_TEST_CASE` per file
- Use `run()` for deterministic tests
- Use `run_multiple()` for stochastic tests that need statistical validation

### Parallel Builds

Speed up compilation with parallel jobs:
```bash
make test -j$(nproc)
```

Or use the helper script:
```bash
make test -j$(perl script/sys-get-nproc.pl)
```

## Tips and Best Practices

1. **Always run tests**: Before submitting changes, run `make test`
2. **Use appropriate build profile**: Debug for development, release for benchmarks
3. **Check coverage**: Run with `WITH_COVERAGE=1` periodically
4. **Parallel builds**: Use `-j` for faster compilation
5. **Keep tests focused**: One test case per file
6. **Document examples**: Run `make readmes` after modifying examples
7. **Understand path mirroring**: Build artifacts in `build/<path>/` correspond to sources in `<path>/`

## Getting Help

- Check available targets: `make`
- View full documentation: See README.md
- Build system issues: Enable `VERBOSE=1`
- Test failures: Check `build/<package-name>/.test/report.html` (e.g., `build/tests/.test/report.html`)