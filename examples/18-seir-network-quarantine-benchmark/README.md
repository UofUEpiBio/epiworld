## Example: `18-seir-network-quarantine-benchmark`

Benchmark comparing the wall-clock time of two SEIR quarantine models:
- **SEIRMixingQuarantine**: Uses a mixing matrix and entities for contacts.
- **SEIRNetworkQuarantine**: Uses a Stochastic Block Model (SBM) network.

Both models use equivalent mixing parameters so the average number of
contacts matches. The network model leverages the queueing system for
computational efficiency.

**Parameters:**
- 10 groups of 500 agents each (5,000 total)
- Mean contacts per agent: 20
- 100 days per run, 50 repetitions
- Within-block degree: 12, between-block degree: ~0.89
- Disease: Flu with transmission rate 0.5, incubation 3 days, recovery 1/5
