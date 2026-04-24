# Poisson vs. Binomial Sampling

This example compares epiworld's `rbinom` (Binomial) and `rpoiss` (Poisson)
random number generators to verify the Poisson approximation to the Binomial
distribution for large *n* and small *p*.

## What it shows

- Empirical means and variances of both distributions
- Side-by-side probability mass function values for selected *k*
- Performance comparison (draws per second) between the two generators
- The maximum absolute difference between the empirical PMFs

This is useful for understanding when the fast Poisson approximation built into
epiworld is accurate and how much speed it provides.

## Source

See [main.cpp](main.cpp) for the full source code.
