- The `./epiworld.hpp` is a single header that contains all the necessary includes for the Epiworld library. Do not analyze it or suggest changes to it.
- Don't use the `./epiworld.hpp` file in your suggestions.

## Testing Guidelines

- Each test file in `tests/` should contain only ONE `EPIWORLD_TEST_CASE` macro. If you need multiple test cases, create separate files (e.g., `22a-testname.cpp`, `22b-testname.cpp`).
- Ensure that each created test file can be compiled individually as well as part of the main test suite.
- Always use Catch2 expectations (`REQUIRE`, `REQUIRE_THAT`, `CHECK`, etc.) when creating tests. Do not throw exceptions for test validation.
- If a test is fully deterministic (same seed produces same results), use `run()` method. Otherwise, use `run_multiple()` for stochastic tests that need multiple runs to verify statistical properties.
