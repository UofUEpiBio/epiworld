# Parameter Lookup Benchmark

Times model parameter lookups by name, both on their own and inside models whose hot paths call them:

- **P**: a micro benchmark. It reports nanoseconds per `par()`, `get_param()` and `operator()` call on a model with 20 parameters. It looks up a name that fits in the short-string buffer (`"Recovery rate"`) and one that does not (`"Transmission rate"`). On versions that have them, it also times `par_at(ParamId)` and `ParamRef`, which read by position.
- **A**: an SEIRH model on a Watts–Strogatz network with mean degree 10. The virus reads `"Transmission rate"` by name for every susceptible–infected contact. `new_state_update_transition` reads its rates by name for every exposed and infected agent, every day.
- **M**: `ModelMeaslesMixing` with one group, 15 contacts a day, and quarantine and contact tracing on. Its update functions call `par()` about 20 times per affected agent per day.

For A and M, the program prints:

- the median (Q1, Q3) CPU milliseconds per `run()`;
- the median final size;
- a checksum of every replicate's daily counts and transmissions.

Equal checksums mean bit-identical runs. The file only uses the string-based parameter API, so it also compiles against older versions, and two versions can be timed side by side:

```sh
./main --sizes 100000 --reps 10 --days 60 --scenarios P,A,M
```

Other options are `--calls` (the number of calls per row in P). The defaults are 100,000 agents, five replicates and 60 days.
