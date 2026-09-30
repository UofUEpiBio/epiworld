# Post-Sampling Benchmark

Times the contact-sampling step of the network and mixing models with no post-sampling callback installed. It is the before/after check for the post-sampling hook work (issue #276): adding a hook that is not in use must not slow these models down, and the refactor of the mixing models onto a shared sampler must not change their results.

Scenarios:

- **L**: `ModelSEIR` on a Watts–Strogatz network (mean degree 10), pulling on every day.
- **U**: the same model, pushing on every day.
- **S**: `ModelSIRMixing` with 10 equal groups (8 contacts a day within a group, 0.5 with each other group).
- **E**: `ModelSEIRMixing`, same groups.
- **Q**: `ModelSEIRMixingQuarantine`, same groups. It records contacts for tracing on its own, so this is the cost of today's inline recording.
- **M**: `ModelMeaslesMixing` with one group, 15 contacts a day, and quarantine and contact tracing on.

For each cell, the program prints:

- the median (Q1, Q3) CPU milliseconds per 40-day `run()`;
- the median final size;
- a checksum of every replicate's daily counts and transmissions.

Equal checksums mean bit-identical runs. The file only uses the long-standing public API, so it compiles against any version:

```sh
./main --sizes 20000,100000 --reps 10 --days 40 --scenarios L,U,S,E,Q,M
```

The defaults are small (20,000 agents, five replicates) so that the example runs quickly.

## Comparing two builds

Wall-clock time is unreliable on a loaded machine, so the program reports CPU time and `compare.sh` runs two binaries interleaved and compares medians:

```sh
# On master (the baseline), and then on the branch under test
c++ -std=c++17 -O2 -DNDEBUG -Depiworld_double=double main.cpp -o base
c++ -std=c++17 -O2 -DNDEBUG -Depiworld_double=double main.cpp -o new
./compare.sh ./base ./new 9 -- --sizes 20000,100000
```

The script prints the median of the per-round medians, the ratio new / base, and whether the checksums agree. A ratio above 1.05 (a median slowdown above 5%) is flagged and makes the script exit with status 1. Use at least 15 rounds for a verdict. Two copies of the same binary differed by up to 9% in `M` over five rounds on a loaded laptop, and by 1–2% over 15. A change that must be bit-identical (a refactor, or an unused hook) should also show `same` in the checksum column.
